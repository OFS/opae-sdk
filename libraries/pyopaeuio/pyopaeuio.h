// Copyright(c) 2021-2022, Intel Corporation
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

#ifndef PYOPAE_UIO_H
#define PYOPAE_UIO_H

#include <opae/uio.h>
#include <string.h>

#include <iostream>
#include <stdexcept>

// opae uio python binding class
class pyopae_uio {
 public:
  pyopae_uio() : num_regions(0), uio_mmap_ptr_(nullptr) {
    memset(&uio_, 0, sizeof(uio_));
  }
  virtual ~pyopae_uio() {}

  int open(const std::string &uio_str);
  void close();
  uint8_t read8(uint32_t region_index, uint32_t offset);
  uint16_t read16(uint32_t region_index, uint32_t offset);
  uint32_t read32(uint32_t region_index, uint32_t offset);
  uint64_t read64(uint32_t region_index, uint32_t offset);
  uint64_t write8(uint32_t region_index, uint32_t offset, uint8_t value);
  uint64_t write16(uint32_t region_index, uint32_t offset, uint16_t value);
  uint64_t write32(uint32_t region_index, uint32_t offset, uint32_t value);
  uint64_t write64(uint32_t region_index, uint32_t offset, uint64_t value);
  size_t get_size(uint32_t region_index);
  uint32_t num_regions;

 private:
  uint8_t *get_region(uint32_t region_index, uint32_t offset);
  uint8_t *uio_mmap_ptr_;
  struct opae_uio uio_;
};
#endif  // PYOPAE_UIO_H
