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
#include <opae/cxx/core/except.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/token.h>
#include <opae/utils.h>

namespace opae {
namespace fpga {
namespace types {

const std::vector<properties::ptr_t> properties::none = {};

properties::properties(bool alloc_props)
    : props_(nullptr),
      type(&props_, fpgaPropertiesGetObjectType, fpgaPropertiesSetObjectType),
      num_errors(&props_, fpgaPropertiesGetNumErrors,
                 fpgaPropertiesSetNumErrors),
      segment(&props_, fpgaPropertiesGetSegment, fpgaPropertiesSetSegment),
      bus(&props_, fpgaPropertiesGetBus, fpgaPropertiesSetBus),
      device(&props_, fpgaPropertiesGetDevice, fpgaPropertiesSetDevice),
      function(&props_, fpgaPropertiesGetFunction, fpgaPropertiesSetFunction),
      socket_id(&props_, fpgaPropertiesGetSocketID, fpgaPropertiesSetSocketID),
      num_slots(&props_, fpgaPropertiesGetNumSlots, fpgaPropertiesSetNumSlots),
      bbs_id(&props_, fpgaPropertiesGetBBSID, fpgaPropertiesSetBBSID),
      bbs_version(&props_, fpgaPropertiesGetBBSVersion,
                  fpgaPropertiesSetBBSVersion),
      vendor_id(&props_, fpgaPropertiesGetVendorID, fpgaPropertiesSetVendorID),
      device_id(&props_, fpgaPropertiesGetDeviceID, fpgaPropertiesSetDeviceID),
      model(&props_, fpgaPropertiesGetModel, fpgaPropertiesSetModel),
      local_memory_size(&props_, fpgaPropertiesGetLocalMemorySize,
                        fpgaPropertiesSetLocalMemorySize),
      capabilities(&props_, fpgaPropertiesGetCapabilities,
                   fpgaPropertiesSetCapabilities),
      num_mmio(&props_, fpgaPropertiesGetNumMMIO, fpgaPropertiesSetNumMMIO),
      num_interrupts(&props_, fpgaPropertiesGetNumInterrupts,
                     fpgaPropertiesSetNumInterrupts),
      accelerator_state(&props_, fpgaPropertiesGetAcceleratorState,
                        fpgaPropertiesSetAcceleratorState),
      object_id(&props_, fpgaPropertiesGetObjectID, fpgaPropertiesSetObjectID),
      parent(&props_, fpgaPropertiesGetParent, fpgaPropertiesSetParent),
      guid(&props_) {
  if (alloc_props) {
    ASSERT_FPGA_OK(fpgaGetProperties(nullptr, &props_));
  }
}

properties::ptr_t properties::get() {
  properties::ptr_t props(new properties());
  return props;
}

properties::ptr_t properties::get(fpga_guid guid_in) {
  properties::ptr_t props(new properties());
  props->guid = guid_in;
  return props;
}

properties::ptr_t properties::get(fpga_objtype objtype) {
  properties::ptr_t props(new properties());
  props->type = objtype;
  return props;
}

properties::ptr_t properties::get(fpga_token tok) {
  ptr_t p(new properties(false));
  auto res = fpgaGetProperties(tok, &p->props_);
  if (res != FPGA_OK) {
    p.reset();
  }
  ASSERT_FPGA_OK(res);
  return p;
}

properties::ptr_t properties::get(handle::ptr_t h) {
  ptr_t p(new properties(false));
  auto res = fpgaGetPropertiesFromHandle(h->c_type(), &p->props_);
  if (res != FPGA_OK) {
    p.reset();
  }
  ASSERT_FPGA_OK(res);
  return p;
}

properties::ptr_t properties::get(token::ptr_t tok) {
  return get(tok->c_type());
}

properties::~properties() {
  if (props_ != nullptr) {
    auto res = fpgaDestroyProperties(&props_);
    if (res != FPGA_OK) {
      std::cerr << "Error while calling fpgaDestroyProperties: "
                << fpgaErrStr(res) << "\n";
    }
  }
}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
