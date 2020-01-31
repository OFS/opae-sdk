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
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include <opae/cxx/core/pvalue.h>
#include <opae/properties.h>

namespace opae {
namespace fpga {
namespace types {

class token;
class handle;
/** Wraps an OPAE fpga_properties object.
 *
 * properties are information describing an
 * accelerator resource that is identified by
 * its token. The properties are used during
 * enumeration to narrow the search for an
 * accelerator resource, and after enumeration
 * to provide the configuration of that
 * resource.
 */
class properties {
 public:
  typedef std::shared_ptr<properties> ptr_t;

  /** An empty vector of properties.
   * Useful for enumerating based on a
   * "match all" criteria.
   */
  const static std::vector<properties::ptr_t> none;

  properties(const properties &p) = delete;

  properties &operator=(const properties &p) = delete;

  ~properties();

  /** Get the underlying fpga_properties object.
   */
  fpga_properties c_type() const { return props_; }

  /** Get the underlying fpga_properties object.
   */
  operator fpga_properties() const { return props_; }

  /** Create a new properties object.
   * @return A properties smart pointer.
   */
  static properties::ptr_t get();

  /** Create a new properties object from a guid.
   * @param guid_in A guid to set in the properties
   * @return A properties smart pointer with its guid initialized to guid_in
   */
  static properties::ptr_t get(fpga_guid guid_in);

  /** Create a new properties object from an fpga_objtype.
   * @param objtype An object type to set in the properties
   * @return A properties smart pointer with its object type set to objtype.
   */
  static properties::ptr_t get(fpga_objtype objtype);

  /** Retrieve the properties for a given token object.
   * @param[in] t A token identifying the accelerator resource.
   * @return A properties smart pointer for the given token.
   */
  static properties::ptr_t get(std::shared_ptr<token> t);

  /** Retrieve the properties for a given fpga_token.
   * @param[in] t An fpga_token identifying the accelerator resource.
   * @return A properties smart pointer for the given fpga_token.
   */
  static properties::ptr_t get(fpga_token t);

  /** Retrieve the properties for a given handle object.
   * @param[in] h A handle identifying the accelerator resource.
   * @return A properties smart pointer for the given handle.
   */
  static properties::ptr_t get(std::shared_ptr<handle> h);

 private:
  properties(bool alloc_props = true);
  fpga_properties props_;

 public:
  pvalue<fpga_objtype> type;
  pvalue<uint32_t> num_errors;
  pvalue<uint16_t> segment;
  pvalue<uint8_t> bus;
  pvalue<uint8_t> device;
  pvalue<uint8_t> function;
  pvalue<uint8_t> socket_id;
  pvalue<uint32_t> num_slots;
  pvalue<uint64_t> bbs_id;
  pvalue<fpga_version> bbs_version;
  pvalue<uint16_t> vendor_id;
  pvalue<uint16_t> device_id;
  pvalue<char *> model;
  pvalue<uint64_t> local_memory_size;
  pvalue<uint64_t> capabilities;
  pvalue<uint32_t> num_mmio;
  pvalue<uint32_t> num_interrupts;
  pvalue<fpga_accelerator_state> accelerator_state;
  pvalue<uint64_t> object_id;
  pvalue<fpga_token> parent;
  guid_t guid;
};

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
