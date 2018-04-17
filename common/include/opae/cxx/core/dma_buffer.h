// Copyright(c) 2017, Intel Corporation
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
#include <memory>
#include <cstdint>
#include <vector>
#include <initializer_list>
#include <chrono>
#include <thread>

#include <opae/buffer.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/except.h>

namespace opae {
namespace fpga {
namespace types {

/** Host/AFU shared memory blocks
 *
 * dma_buffer abstracts a memory block that may be shared
 * between the host cpu and an accelerator. The block may
 * be allocated by the dma_buffer class itself (see allocate),
 * or it may be allocated elsewhere and then attached to
 * a dma_buffer object via attach.
 */
class dma_buffer {
 public:
  typedef std::size_t size_t;
  typedef std::shared_ptr<dma_buffer> ptr_t;

  dma_buffer(const dma_buffer & ) = delete;
  dma_buffer & operator =(const dma_buffer & ) = delete;

  /** dma_buffer destructor.
   */
  virtual ~dma_buffer();

  /** dma_buffer factory method - allocate a dma_buffer.
   * @param[in] handle The handle used to allocate the buffer.
   * @param[in] len    The length in bytes of the requested buffer.
   * @return A valid dma_buffer smart pointer on success, or an
   * empty smart pointer on failure.
   */
  static dma_buffer::ptr_t allocate(handle::ptr_t handle, size_t len);

  /** Attach a pre-allocated buffer to a dma_buffer object.
   *
   * @param[in] handle The handle used to attach the buffer.
   * @param[in] base The base of the pre-allocated memory.
   * @param[in] len The size of the pre-allocated memory,
   * which must be a multiple of the page size.
   * @return A valid dma_buffer smart pointer on success, or an
   * empty smart pointer on failure.
   */
  static dma_buffer::ptr_t attach(handle::ptr_t handle, uint8_t *base,
                                  size_t len);

  /** Retrieve the virtual address of the buffer base.
   */
  volatile uint8_t *get() const { return virt_; }

  /** Retrieve the handle smart pointer associated with
   * this buffer.
   */
  handle::ptr_t owner() const { return handle_; }

  /** Retrieve the length of the buffer in bytes.
   */
  size_t size() const { return len_; }

  /** Retrieve the underlying buffer's workspace id.
   */
  uint64_t wsid() const { return wsid_; }

  /** Retrieve the address of the buffer suitable for
   * programming into the accelerator device.
   */
  uint64_t iova() const { return iova_; }


  /** Write c to each byte location in the buffer.
   */
  void fill(int c);

  /** Compare this dma_buffer (the first len bytes)
   * to that held in other, using memcmp().
   */
  int compare(ptr_t other, size_t len) const;

  /** Read a T-sized block of memory at the given location.
   * @param[in] offset The byte offset from the start of the buffer.
   * @return A T from buffer base + offset.
   */
  template <typename T>
  T read(size_t offset) const {
    if ((offset < len_) && (virt_ != nullptr)) {
      return *reinterpret_cast<T *>(virt_ + offset);
    } else if (offset >= len_) {
      throw except(OPAECXX_HERE);
    } else {
      throw except(OPAECXX_HERE);
    }
    return T();
  }

  /** Write a T-sized block of memory to the given location.
   * @param[in] value The value to write.
   * @param[in] offset The byte offset from the start of the buffer.
   */
  template <typename T>
  void write(const T &value, size_t offset) {
    if ((offset < len_) && (virt_ != nullptr)) {
      *reinterpret_cast<T *>(virt_ + offset) = value;
    } else if (offset >= len_) {
      throw except(OPAECXX_HERE);
    } else {
      throw except(OPAECXX_HERE);
    }
  }

 protected:
  dma_buffer(handle::ptr_t handle, size_t len, uint8_t *virt, uint64_t wsid,
             uint64_t iova);


  handle::ptr_t handle_;
  size_t len_;
  uint8_t *virt_;
  uint64_t wsid_;
  uint64_t iova_;
};


}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
