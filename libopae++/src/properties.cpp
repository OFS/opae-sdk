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
  ASSERT_FPGA_OK(fpgaGetProperties(nullptr, &props_));
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
  ASSERT_FPGA_OK(fpgaCloneProperties(p.props_, &props_));
  // make sure that we have a cached copy of
  // everything that p has cached.
  if (p.type.is_set()) type.update();
  if (p.bus.is_set()) bus.update();
  if (p.device.is_set()) device.update();
  if (p.function.is_set()) function.update();
  if (p.socket_id.is_set()) socket_id.update();
  if (p.num_slots.is_set()) num_slots.update();
  if (p.bbs_id.is_set()) bbs_id.update();
  if (p.bbs_version.is_set()) bbs_version.update();
  if (p.vendor_id.is_set()) vendor_id.update();
  if (p.model.is_set()) model.update();
  if (p.local_memory_size.is_set()) local_memory_size.update();
  if (p.capabilities.is_set()) capabilities.update();
  if (p.num_mmio.is_set()) num_mmio.update();
  if (p.num_interrupts.is_set()) num_interrupts.update();
  if (p.accelerator_state.is_set()) accelerator_state.update();
  if (p.object_id.is_set()) object_id.update();
  if (p.parent.is_set()) parent.update();
  if (p.guid.is_set()) guid.update();

}

properties & properties::operator =(const properties &p)
{
  if (this != &p) {

    if (props_) {
      ASSERT_FPGA_OK(fpgaDestroyProperties(&props_));
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
      ASSERT_FPGA_OK(fpgaCloneProperties(p.props_, &props_));

      // make sure that we have a cached copy of
      // everything that p has cached.
      if (p.type.is_set()) type.update();
      if (p.bus.is_set()) bus.update();
      if (p.device.is_set()) device.update();
      if (p.function.is_set()) function.update();
      if (p.socket_id.is_set()) socket_id.update();
      if (p.num_slots.is_set()) num_slots.update();
      if (p.bbs_id.is_set()) bbs_id.update();
      if (p.bbs_version.is_set()) bbs_version.update();
      if (p.vendor_id.is_set()) vendor_id.update();
      if (p.model.is_set()) model.update();
      if (p.local_memory_size.is_set()) local_memory_size.update();
      if (p.capabilities.is_set()) capabilities.update();
      if (p.num_mmio.is_set()) num_mmio.update();
      if (p.num_interrupts.is_set()) num_interrupts.update();
      if (p.accelerator_state.is_set()) accelerator_state.update();
      if (p.object_id.is_set()) object_id.update();
      if (p.parent.is_set()) parent.update();
      if (p.guid.is_set()) guid.update();
    }
  }
  return *this;
}

properties::properties(fpga_guid guid_in) : properties(){ guid = guid_in; }

properties::properties(fpga_objtype objtype) : properties() { type = objtype; }

properties::~properties() {
  if (props_ != nullptr) {
    ASSERT_FPGA_OK(fpgaDestroyProperties(&props_));
  }
}

properties::ptr_t properties::read(fpga_token tok) {
  opae::fpga::internal::logger log("properties::read()");
  ptr_t p(new properties());
  ASSERT_FPGA_OK(fpgaGetProperties(tok, &p->props_));
  return p;
}

properties::ptr_t properties::read(token::ptr_t tok) {
  return read(tok->get());
}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
