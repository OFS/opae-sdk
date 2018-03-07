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
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/log.h>

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

  handle(const handle & ) = delete;
  handle & operator =(const handle & ) = delete;

  ~handle();

  /** Retrieve the underlying OPAE handle.
   */
  fpga_handle get() const { return handle_; }

  /** Retrieve the underlying OPAE handle.
   */
  operator fpga_handle() const { return handle_; }

  /**
   * @brief Read CSR from resource associated with the handle
   *
   * @tparam T The type of value to return.
   *
   * @note Only uint32_t or uint64_t are allowed.
   *
   * @param offset The register offset
   * @param csr_space The CSR space to read from. Default is 0.
   *
   * @return The value of type T read from the CSR
   */
  template<typename T>
  T read_csr(uint64_t offset, uint32_t csr_space = 0) const {
    (void)offset;
    (void)csr_space;
    throw except(OPAECXX_HERE);
  }

  /**
   * @brief Write value to CSR from resource associated with the handle
   *
   * @tparam T The type of value to write.
   *
   * @note Only uint32_t or uint64_t are allowed.
   *
   * @param offset The register offset.
   * @param value The value to write to the register.
   * @param csr_space The CSR space to read from. Default is 0.
   *
   */
  template<typename T>
  void write_csr(uint64_t offset, T value, uint32_t csr_space = 0) {
    (void)offset;
    (void)value;
    (void)csr_space;
    throw except(OPAECXX_HERE);
  }

  /** Retrieve a pointer to the MMIO region.
   * @param[in] offset The byte offset to add to MMIO base.
   * @return MMIO base + offset
   */
  uint8_t *mmio_ptr(uint64_t offset) const { return mmio_base_ + offset; }

  /** Open an accelerator resource, given a raw fpga_token
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

  /** Open an accelerator resource, given a token object
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

  /** Reset the accelerator identified by this handle
   */
  virtual void reset();

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
