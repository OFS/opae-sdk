// Copyright(c) 2020-2023, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <opae/vfio.h>


static inline void assert_config_op(uint64_t offset,
                                    ssize_t expected,
                                    ssize_t actual,
                                    const char *op)
{
  if (actual != expected) {
    std::stringstream ss;
    ss << "error: [pci_config:" << op << " @0x" << std::hex << offset
       << " expected " << std::dec << expected
       << ", processed " << actual << "\n";
    throw std::length_error(ss.str().c_str());
  }
}

struct mmio_region {
  uint32_t index;
  uint8_t *ptr;
  size_t size;

  uint32_t read32(uint64_t offset)
  {
    return *reinterpret_cast<uint32_t *>(ptr + offset);
  }

  void write32(uint64_t offset, uint32_t value)
  {
    *reinterpret_cast<uint32_t *>(ptr + offset) = value;
  }

  uint64_t read64(uint64_t offset)
  {
    return *reinterpret_cast<uint64_t *>(ptr + offset);
  }

  void write64(uint64_t offset, uint64_t value)
  {
    *reinterpret_cast<uint64_t *>(ptr + offset) = value;
  }
};

struct system_buffer {
  size_t size;
  uint8_t *buf;
  uint64_t iova;
  opae_vfio *vfio;

  uint64_t get_uint64(uint64_t offset)
  {
    return *(reinterpret_cast<uint64_t *>(buf) + offset/sizeof(uint64_t));
  }

  void set_uint64(uint64_t offset, uint64_t value)
  {
    *(reinterpret_cast<uint64_t *>(buf) + offset/sizeof(uint64_t)) = value;
  }

  template<typename T>
  T get(uint64_t offset)
  {
    return *(reinterpret_cast<T *>(buf + offset));
  }

  template<typename T>
  void fill(T value)
  {
    T *ptr;
    for (ptr = reinterpret_cast<T *>(buf) ;
          ptr < reinterpret_cast<T *>(buf + size) ;
            ++ptr) {
      *ptr = value;
    }
  }

  size_t compare(system_buffer *other)
  {
    size_t i;
    for (i = 0 ; i < std::min(size, other->size) ; ++i) {
      if (*(buf + i) != *(other->buf + i)) {
        return i;
      }
    }
    return size;
  }
};

struct vfio_device {
  ~vfio_device() {
    close();
  }

  vfio_device(const vfio_device &) = delete;
  vfio_device &operator=(const vfio_device &) = delete;

  static vfio_device* open(const char *pci_addr)
  {
    opae_vfio *v = new opae_vfio();

    if (opae_vfio_open(v, pci_addr)) {
      delete v;
      return nullptr;
    }

    return new vfio_device(v);
  }

  void close()
  {
    if (v_) {
      opae_vfio_close(v_);
      delete v_;
      v_ = nullptr;
    }
  }

  int descriptor() const
  {
    return v_->cont_fd;
  }

  const char *address() const
  {
    return v_->cont_pciaddr;
  }

  template <typename T>
  T config_read(uint64_t offset) const
  {
    T value = 0;
    ssize_t bytes;
    bytes = pread(v_->device.device_fd,
                  &value,
                  sizeof(T),
                  v_->device.device_config_offset + offset);
    assert_config_op(offset, sizeof(T), bytes, "read");
    return value;
  }

  template <typename T>
  void config_write(uint64_t offset, T value)
  {
    ssize_t bytes;
    bytes = pwrite(v_->device.device_fd,
                   &value,
                   sizeof(T),
                   v_->device.device_config_offset + offset);
    assert_config_op(offset, sizeof(T), bytes, "write");
  }

  uint32_t num_regions() const
  {
    uint32_t count = 0;
    struct opae_vfio_device_region *r = v_->device.regions;
    while (r) {
      ++count;
      r = r->next;
    }
    return count;
  }

  std::vector<mmio_region> regions()
  {
    std::vector<mmio_region> vect;
    struct opae_vfio_device_region *r = v_->device.regions;
    while (r) {
      mmio_region region = { r->region_index, r->region_ptr, r->region_size };
      vect.push_back(region);
      r = r->next;
    }
    return vect;
  }

  system_buffer *buffer_allocate(size_t sz)
  {
    system_buffer *b = new struct system_buffer();

    b->size = sz;
    b->buf = nullptr;
    b->iova = 0;

    if (opae_vfio_buffer_allocate(v_, &b->size, &b->buf, &b->iova)) {
      delete b;
      b = nullptr;
      return b;
    }

    b->vfio = v_;

    return b;
  }

  #define VFIO_VF_TOKEN_LEN (16)
  int set_vf_token(const char *vf_token)
  {
    struct vfio_device_feature *df;
    int ret;

    df = (struct vfio_device_feature *)
         new uint8_t[sizeof(struct vfio_device_feature) + VFIO_VF_TOKEN_LEN];

    df->argsz = sizeof(df) + VFIO_VF_TOKEN_LEN;
    df->flags = VFIO_DEVICE_FEATURE_SET | VFIO_DEVICE_FEATURE_PCI_VF_TOKEN;
    memcpy(df->data, vf_token, VFIO_VF_TOKEN_LEN);

    ret = ioctl(v_->device.device_fd, VFIO_DEVICE_FEATURE, df);
    delete[] df;

    if (ret)
      std::cerr << "ioctl failed " << errno << std::endl;

    return ret;
  }
private:
  opae_vfio *v_;
  vfio_device(opae_vfio *v)
    : v_(v){}
};

