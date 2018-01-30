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
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/except.h>

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
    res = type.update();
    if (res)
      goto log_err;
  }
  if (p.bus.is_set()) {
    res = bus.update();
    if (res)
      goto log_err;
  }
  if (p.device.is_set()) {
    res = device.update();
    if (res)
      goto log_err;
  }
  if (p.function.is_set()) {
    res = function.update();
    if (res)
      goto log_err;
  }
  if (p.socket_id.is_set()) {
    res = socket_id.update();
    if (res)
      goto log_err;
  }
  if (p.num_slots.is_set()) {
    res = num_slots.update();
    if (res)
      goto log_err;
  }
  if (p.bbs_id.is_set()) {
    res = bbs_id.update();
    if (res)
      goto log_err;
  }
  if (p.bbs_version.is_set()) {
    res = bbs_version.update();
    if (res)
      goto log_err;
  }
  if (p.vendor_id.is_set()) {
    res = vendor_id.update();
    if (res)
      goto log_err;
  }
  if (p.model.is_set()) {
    res = model.update();
    if (res)
      goto log_err;
  }
  if (p.local_memory_size.is_set()) {
    res = local_memory_size.update();
    if (res)
      goto log_err;
  }
  if (p.capabilities.is_set()) {
    res = capabilities.update();
    if (res)
      goto log_err;
  }
  if (p.num_mmio.is_set()) {
    res = num_mmio.update();
    if (res)
      goto log_err;
  }
  if (p.num_interrupts.is_set()) {
    res = num_interrupts.update();
    if (res)
      goto log_err;
  }
  if (p.accelerator_state.is_set()) {
    res = accelerator_state.update();
    if (res)
      goto log_err;
  }
  if (p.object_id.is_set()) {
    res = object_id.update();
    if (res)
      goto log_err;
  }
  if (p.parent.is_set()) {
    res = parent.update();
    if (res)
      goto log_err;
  }
  if (p.guid.is_set()) {
    res = guid.update();
    if (res)
      goto log_err;
  }
  return;

log_err:
  log_.error("copy ctor") << "property getter failed with (" << res
               << ") " << fpgaErrStr(res);
  throw except(res, OPAECXX_HERE);
}

properties & properties::operator =(const properties &p)
{
  fpga_result res;
  if (this != &p) {

    if (props_) {
      if ((res = fpgaDestroyProperties(&props_)) != FPGA_OK) {
        log_.error() << "fpgaDestroyProperties() failed with (" << res
                     << ") " << fpgaErrStr(res);
        throw except(res, OPAECXX_HERE); 
      }
      props_ = nullptr;
      type.invalidate();
      bus.invalidate();
      device.invalidate();
      function.invalidate();
      socket_id.invalidate();
      num_slots.invalidate();
      bbs_id.invalidate();
      bbs_version.invalidate();
      vendor_id.invalidate();
      model.invalidate();
      local_memory_size.invalidate();
      capabilities.invalidate();
      num_mmio.invalidate();
      num_interrupts.invalidate();
      accelerator_state.invalidate();
      object_id.invalidate();
      parent.invalidate();
      guid.invalidate();
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
        res = type.update();
        if (res)
          goto log_err;
      }
      if (p.bus.is_set()) {
        res = bus.update();
        if (res)
          goto log_err;
      }
      if (p.device.is_set()) {
        res = device.update();
        if (res)
          goto log_err;
      }
      if (p.function.is_set()) {
        res = function.update();
        if (res)
          goto log_err;
      }
      if (p.socket_id.is_set()) {
        res = socket_id.update();
        if (res)
          goto log_err;
      }
      if (p.num_slots.is_set()) {
        res = num_slots.update();
        if (res)
          goto log_err;
      }
      if (p.bbs_id.is_set()) {
        res = bbs_id.update();
        if (res)
          goto log_err;
      }
      if (p.bbs_version.is_set()) {
        res = bbs_version.update();
        if (res)
          goto log_err;
      }
      if (p.vendor_id.is_set()) {
        res = vendor_id.update();
        if (res)
          goto log_err;
      }
      if (p.model.is_set()) {
        res = model.update();
        if (res)
          goto log_err;
      }
      if (p.local_memory_size.is_set()) {
        res = local_memory_size.update();
        if (res)
          goto log_err;
      }
      if (p.capabilities.is_set()) {
        res = capabilities.update();
        if (res)
          goto log_err;
      }
      if (p.num_mmio.is_set()) {
        res = num_mmio.update();
        if (res)
          goto log_err;
      }
      if (p.num_interrupts.is_set()) {
        res = num_interrupts.update();
        if (res)
          goto log_err;
      }
      if (p.accelerator_state.is_set()) {
        res = accelerator_state.update();
        if (res)
          goto log_err;
      }
      if (p.object_id.is_set()) {
        res = object_id.update();
        if (res)
          goto log_err;
      }
      if (p.parent.is_set()) {
        res = parent.update();
        if (res)
          goto log_err;
      }
      if (p.guid.is_set()) {
        res = guid.update();
        if (res)
          goto log_err;
      }
    }
  }
  return *this;

log_err:
  log_.error("operator =") << "property getter failed with (" << res
                           << ") " << fpgaErrStr(res);
  throw except(res, OPAECXX_HERE);
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
