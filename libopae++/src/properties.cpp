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
#include <opae/utils.h>
#include "opaec++/properties.h"
#include "opaec++/token.h"
#include "opaec++/except.h"

namespace opae {
namespace fpga {
namespace types {

const std::vector<properties> properties::none = {};

properties::properties()
    : props_(nullptr),
      log_("properties"),
      type(&props_, fpgaPropertiesGetObjectType, fpgaPropertiesSetObjectType),
      bus(&props_, fpgaPropertiesGetBus, fpgaPropertiesSetBus),
      device(&props_, fpgaPropertiesGetDevice, fpgaPropertiesSetDevice),
      function(&props_, fpgaPropertiesGetFunction, fpgaPropertiesSetFunction),
      socket_id(&props_, fpgaPropertiesGetSocketID, fpgaPropertiesSetSocketID),
      num_slots(&props_, fpgaPropertiesGetNumSlots, fpgaPropertiesSetNumSlots),
      bbs_id(&props_, fpgaPropertiesGetBBSID, fpgaPropertiesSetBBSID),
      bbs_version(&props_, fpgaPropertiesGetBBSVersion,
                  fpgaPropertiesSetBBSVersion),
      vendor_id(&props_, fpgaPropertiesGetVendorID, fpgaPropertiesSetVendorID),
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
  auto res = fpgaGetProperties(nullptr, &props_);
  if (res != FPGA_OK) {
    props_ = nullptr;
    log_.error() << "fpgaGetProperties() failed with (" << res
                 << ") " << fpgaErrStr(res);
    throw except(res, OPAECXX_HERE);
  }
}

properties::properties(const properties &p)
    : props_(nullptr),
      log_("properties"),
      type(&props_, fpgaPropertiesGetObjectType, fpgaPropertiesSetObjectType),
      bus(&props_, fpgaPropertiesGetBus, fpgaPropertiesSetBus),
      device(&props_, fpgaPropertiesGetDevice, fpgaPropertiesSetDevice),
      function(&props_, fpgaPropertiesGetFunction, fpgaPropertiesSetFunction),
      socket_id(&props_, fpgaPropertiesGetSocketID, fpgaPropertiesSetSocketID),
      num_slots(&props_, fpgaPropertiesGetNumSlots, fpgaPropertiesSetNumSlots),
      bbs_id(&props_, fpgaPropertiesGetBBSID, fpgaPropertiesSetBBSID),
      bbs_version(&props_, fpgaPropertiesGetBBSVersion,
                  fpgaPropertiesSetBBSVersion),
      vendor_id(&props_, fpgaPropertiesGetVendorID, fpgaPropertiesSetVendorID),
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
  auto res = fpgaCloneProperties(p.props_, &props_);
  if (res != FPGA_OK) {
    log_.error() << "fpgaCloneProperties() failed with (" << res
                 << ") " << fpgaErrStr(res);
    throw except(res, OPAECXX_HERE);
  }
  // make sure that we have a cached copy of
  // everything that p has cached.
  if (p.type.is_set()) {
    type.operator fpga_objtype();
  }
  if (p.bus.is_set()) {
    bus.operator uint8_t();
  }
  if (p.device.is_set()) {
    device.operator uint8_t();
  }
  if (p.function.is_set()) {
    function.operator uint8_t();
  }
  if (p.socket_id.is_set()) {
    socket_id.operator uint8_t();
  }
  if (p.num_slots.is_set()) {
    num_slots.operator uint32_t();
  }
  if (p.bbs_id.is_set()) {
    bbs_id.operator uint64_t();
  }
  if (p.bbs_version.is_set()) {
    bbs_version.operator fpga_version();
  }
  if (p.vendor_id.is_set()) {
    vendor_id.operator uint16_t();
  }
  if (p.model.is_set()) {
    model.operator std::string();
  }
  if (p.local_memory_size.is_set()) {
    local_memory_size.operator uint64_t();
  }
  if (p.capabilities.is_set()) {
    capabilities.operator uint64_t();
  }
  if (p.num_mmio.is_set()) {
    num_mmio.operator uint32_t();
  }
  if (p.num_interrupts.is_set()) {
    num_interrupts.operator uint32_t();
  }
  if (p.accelerator_state.is_set()) {
    accelerator_state.operator fpga_accelerator_state();
  }
  if (p.object_id.is_set()) {
    object_id.operator uint64_t();
  }
  if (p.parent.is_set()) {
    parent.operator fpga_token();
  }
  if (p.guid.is_set()) {
    guid.operator uint8_t *();
  }
}

properties & properties::operator =(const properties &p)
{
  if (this != &p) {
    fpga_result res;

    if (props_) {
      if ((res = fpgaDestroyProperties(&props_)) != FPGA_OK) {
        log_.error() << "fpgaDestroyProperties() failed with (" << res
                     << ") " << fpgaErrStr(res);
        throw except(res, OPAECXX_HERE); 
      }
      props_ = nullptr;
      type.is_set(false);
      bus.is_set(false);
      device.is_set(false);
      function.is_set(false);
      socket_id.is_set(false);
      num_slots.is_set(false);
      bbs_id.is_set(false);
      bbs_version.is_set(false);
      vendor_id.is_set(false);
      model.is_set(false);
      local_memory_size.is_set(false);
      capabilities.is_set(false);
      num_mmio.is_set(false);
      num_interrupts.is_set(false);
      accelerator_state.is_set(false);
      object_id.is_set(false);
      parent.is_set(false);
      guid.is_set(false);
    }

    if (p.props_) {
      if ((res = fpgaCloneProperties(p.props_, &props_)) != FPGA_OK) {
        log_.error() << "fpgaCloneProperties() failed with (" << res
                     << ") " << fpgaErrStr(res);
        throw except(res, OPAECXX_HERE);
      }

      // make sure that we have a cached copy of
      // everything that p has cached.
      if (p.type.is_set()) {
        type.operator fpga_objtype();
      }
      if (p.bus.is_set()) {
        bus.operator uint8_t();
      }
      if (p.device.is_set()) {
        device.operator uint8_t();
      }
      if (p.function.is_set()) {
        function.operator uint8_t();
      }
      if (p.socket_id.is_set()) {
        socket_id.operator uint8_t();
      }
      if (p.num_slots.is_set()) {
        num_slots.operator uint32_t();
      }
      if (p.bbs_id.is_set()) {
        bbs_id.operator uint64_t();
      }
      if (p.bbs_version.is_set()) {
        bbs_version.operator fpga_version();
      }
      if (p.vendor_id.is_set()) {
        vendor_id.operator uint16_t();
      }
      if (p.model.is_set()) {
        model.operator std::string();
      }
      if (p.local_memory_size.is_set()) {
        local_memory_size.operator uint64_t();
      }
      if (p.capabilities.is_set()) {
        capabilities.operator uint64_t();
      }
      if (p.num_mmio.is_set()) {
        num_mmio.operator uint32_t();
      }
      if (p.num_interrupts.is_set()) {
        num_interrupts.operator uint32_t();
      }
      if (p.accelerator_state.is_set()) {
        accelerator_state.operator fpga_accelerator_state();
      }
      if (p.object_id.is_set()) {
        object_id.operator uint64_t();
      }
      if (p.parent.is_set()) {
        parent.operator fpga_token();
      }
      if (p.guid.is_set()) {
        guid.operator uint8_t *();
      }
    }
  }
  return *this;
}

properties::properties(fpga_guid guid_in) : properties(){ guid = guid_in; }

properties::properties(fpga_objtype objtype) : properties() { type = objtype; }

properties::~properties() {
  if (props_ != nullptr) {
    auto res = fpgaDestroyProperties(&props_);
    if (res != FPGA_OK) {
      log_.error() << "fpgaDestroyProperties() failed with (" << res
                   << ") " << fpgaErrStr(res);
      throw except(res, OPAECXX_HERE); 
    }
  }
}

properties::ptr_t properties::read(fpga_token tok) {
  opae::fpga::internal::logger log("properties::read()");
  ptr_t p(new properties());
  auto res = fpgaGetProperties(tok, &p->props_);
  if (res != FPGA_OK) {
    p.reset();
    log.error() << "fpgaGetProperties() failed with (" << res
                << ") " << fpgaErrStr(res);
    throw except(res, OPAECXX_HERE); 
  }
  return p;
}

properties::ptr_t properties::read(token::ptr_t tok) {
  return read(tok->get());
}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
