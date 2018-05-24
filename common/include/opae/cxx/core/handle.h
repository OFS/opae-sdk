// Copyright(c) 2018, Intel Corporation
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
#include <vector>

#include <opae/cxx/core/token.h>
#include <opae/enum.h>
#include <opae/types.h>

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

  handle(const handle &) = delete;
  handle &operator=(const handle &) = delete;

  ~handle();

  /** Retrieve the underlying OPAE handle.
   */
  fpga_handle c_type() const { return handle_; }

  /** Retrieve the underlying OPAE handle.
   */
  operator fpga_handle() const { return handle_; }

  /**
   * @brief Read 32 bits from a CSR belonging to a resource associated
   * with a handle.
   *
   * @param[in] offset The register offset
   * @param[in] csr_space The CSR space to read from. Default is 0.
   *
   * @return The 32-bit value read from the CSR
   */
  uint32_t read_csr32(uint64_t offset, uint32_t csr_space = 0) const;

  /**
   * @brief Write 32 bit to a CSR belonging to a resource associated
   * with a handle.
   *
   * @param[in] offset The register offset.
   * @param[in] value The 32-bit value to write to the register.
   * @param[in] csr_space The CSR space to read from. Default is 0.
   *
   */
  void write_csr32(uint64_t offset, uint32_t value, uint32_t csr_space = 0);

  /**
   * @brief Read 64 bits from a CSR belonging to a resource associated
   * with a handle.
   *
   * @param[in] offset The register offset
   * @param[in] csr_space The CSR space to read from. Default is 0.
   *
   * @return The 64-bit value read from the CSR
   */
  uint64_t read_csr64(uint64_t offset, uint32_t csr_space = 0) const;

  /**
   * @brief Write 64 bit to a CSR belonging to a resource associated
   * with a handle.
   *
   * @param[in] offset The register offset.
   * @param[in] value The 64-bit value to write to the register.
   * @param[in] csr_space The CSR space to read from. Default is 0.
   *
   */
  void write_csr64(uint64_t offset, uint64_t value, uint32_t csr_space = 0);

  /** Retrieve a pointer to the MMIO region.
   * @param[in] offset The byte offset to add to MMIO base.
   * @param[in] csr_space The desired CSR space. Default is 0.
   * @return MMIO base + offset
   */
  uint8_t *mmio_ptr(uint64_t offset, uint32_t csr_space = 0) const;

  /** Open an accelerator resource, given a raw fpga_token
   *
   * @param[in] token A token describing the accelerator
   * resource to be allocated.
   *
   * @param[in] flags The flags parameter to fpgaOpen().
   *
   * @return pointer to the mmio base + offset for the given
   * csr space
   *
   */
  static handle::ptr_t open(fpga_token token, int flags);

  /** Open an accelerator resource, given a token object
   *
   * @param[in] token A token object describing the
   * accelerator resource to be allocated.
   *
   * @param[in] flags The flags parameter to fpgaOpen().
   *
   * @return shared ptr to a handle object
   */
  static handle::ptr_t open(token::ptr_t token, int flags);

  /** Reset the accelerator identified by this handle
   */
  virtual void reset();

  /** Close an accelerator resource (if opened)
   *
   * @return fpga_result indication the result of closing the
   * handle or FPGA_EXCEPTION if handle is not opened
   *
   * @note This is available for explicitly closing a handle.
   * The destructor for handle will call close.
   */
  fpga_result close();

 private:
  handle(fpga_handle h);

  fpga_handle handle_;
  fpga_token token_;
};

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
