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

#include <opae/cxx/core/except.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/dma_buffer.h>

namespace opae {
namespace fpga {
namespace resource {

/**
 * @brief       An accelerator represents an accelerator resource
 * identified by OPAE. It encapsulates a token, properties, and handle data
 * structures that are used to identify and define the resource.
 * Additionally, the accelerator class encapsulates operations that may be
 * performed with or on the resource.
 */
class accelerator {
 public:
  /**
   * @brief     Defines an alias representing a shared_ptr of an
   * accelerator
   */
  typedef std::shared_ptr<accelerator> ptr_t;

  /**
   * @typedef   std::vector<ptr_t> list_t
   *
   * @brief     Defines an alias representing a list of accelerators
   */
  typedef std::vector<ptr_t> list_t;

  /**
   * @fn        accelerator::accelerator() = delete;
   *
   * @brief     Default constructor is disabled
   */
  accelerator() = delete;

  /** @brief     Destructor */
  virtual ~accelerator();

  /**
   * @brief     Enumerates using the given filter
   * @param     filter  (Optional) A list of properties to use as a
   *            filter. The default value of filter is a list of one
   *            properties object set to type FPGA_ACCELERATOR.
   * @return    A list of accelerator shared_ptr objects.
   */
  static list_t enumerate(std::vector<opae::fpga::types::properties> filter =
                          {FPGA_ACCELERATOR});

  /**
   * @fn        void accelerator::open(int flags);
   *
   * @brief     Open an accelerator resource and stores properties and
   * structures internally.
   *
   * @throws    opae::fpga::types::except
   *
   * @param     flags   The flags.
   */
  void open(int flags);

  /**
   * @fn        opae::fpga::types::dma_buffer::ptr_t accelerator::allocate_buffer(size_t size);
   *
   * @brief     Allocates a shared buffer to use with the accelerator.
   *
   * @param     size    The size of the buffer (in bytes).
   *
   * @return    A shared pointer to a dma_buffer object.
   */
  opae::fpga::types::dma_buffer::ptr_t allocate_buffer(size_t size);

 private:
  opae::fpga::types::token::ptr_t token_;
  opae::fpga::types::properties::ptr_t props_;
  opae::fpga::types::handle::ptr_t handle_;
  accelerator(opae::fpga::types::token::ptr_t t);
};

}  // end of namespace resource
}  // end of namespace fpga
}  // end of namespace opae
