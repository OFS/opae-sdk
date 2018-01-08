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
#include <vector>
#include <memory>

#include <opae/types.h>
#include <opae/enum.h>
#include <opae/cxx/token.h>
#include <opae/cxx/log.h>

namespace opae {
namespace fpga {
namespace types {

/** An allocated accelerator resource
 *
 * Represents an accelerator resource that has
 * been allocated by OPAE. Depending on the type
 * of resource, its register space may be
 * read/written using a handle object.
 */
class handle {
 public:
  typedef std::shared_ptr<handle> ptr_t;

  ~handle();

  /** Retrieve the underlying OPAE handle.
   */
  fpga_handle get() const { return handle_; }

  /** Retrieve the underlying OPAE handle.
   */
  operator fpga_handle() const { return handle_; }

  /** Write 32-bit value to MMIO.
   * @param[in] offset The byte offset from MMIO base to write.
   * @param[in] value  The value to be written.
   * @return Whether the write was successful.
   */
  virtual bool write(uint64_t offset, uint32_t value);

  /** Write 64-bit value to MMIO.
   * @param[in] offset The byte offset from MMIO base to write.
   * @param[in] value  The value to be written.
   * @return Whether the write was successful.
   */
  virtual bool write(uint64_t offset, uint64_t value);

  /** Read 32-bit value from MMIO.
   * @param[in]  offset The byte offset from MMIO base to read.
   * @param[out] value  Receives the value read.
   * @return Whether the read was successful.
   */
  virtual bool read(uint64_t offset, uint32_t &value) const;

  /** Read 64-bit value from MMIO.
   * @param[in]  offset The byte offset from MMIO base to read.
   * @param[out] value  Receives the value read.
   * @return Whether the read was successful.
   */
  virtual bool read(uint64_t offset, uint64_t &value) const;

  /** Retrieve a pointer to the MMIO region.
   * @param[in] offset The byte offset to add to MMIO base.
   * @return MMIO base + offset
   */
  uint8_t *mmio_ptr(uint64_t offset) const { return mmio_base_ + offset; }

  /** Allocate an accelerator, given a raw fpga_token
   *
   * @param[in] token A token describing the accelerator
   * resource to be allocated.
   *
   * @param[in] flags The flags parameter to fpgaOpen().
   *
   * @param[in] mmio_region The zero-based index of
   * the desired MMIO region. See fpgaMapMMIO().
   */
  static handle::ptr_t open(fpga_token token, int flags,
                            uint32_t mmio_region = 0);

  /** Allocate an accelerator, given a token object
   *
   * @param[in] token A token object describing the
   * accelerator resource to be allocated.
   *
   * @param[in] flags The flags parameter to fpgaOpen().
   *
   * @param[in] mmio_region The zero-based index of
   * the desired MMIO region. See fpgaMapMMIO().
   */
  static handle::ptr_t open(token::ptr_t token, int flags,
                            uint32_t mmio_region = 0);

 protected:
  fpga_result close();

 private:
  handle(fpga_handle h, uint32_t mmio_region, uint8_t *mmio_base);

  fpga_handle handle_;
  uint32_t mmio_region_;
  uint8_t *mmio_base_;
  opae::fpga::internal::logger log_;
};

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
