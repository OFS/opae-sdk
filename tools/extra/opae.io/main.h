// Copyright(c) 2020, Intel Corporation
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

#include <opae/vfio.h>

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
