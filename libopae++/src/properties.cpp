#include "opaec++/properties.h"
#include "opaec++/token.h"

namespace opae
{
namespace fpga
{
namespace types
{

properties::properties()
: props_(nullptr)
, type(&props_, fpgaPropertiesGetObjectType, fpgaPropertiesSetObjectType)
, bus(&props_, fpgaPropertiesGetBus, fpgaPropertiesSetBus)
, device(&props_, fpgaPropertiesGetDevice, fpgaPropertiesSetDevice)
, function(&props_, fpgaPropertiesGetFunction, fpgaPropertiesSetFunction)
, socket_id(&props_, fpgaPropertiesGetSocketID, fpgaPropertiesSetSocketID)
, num_slots(&props_, fpgaPropertiesGetNumSlots, fpgaPropertiesSetNumSlots)
, bbs_id(&props_, fpgaPropertiesGetBBSID, fpgaPropertiesSetBBSID)
, bbs_version(&props_, fpgaPropertiesGetBBSVersion, fpgaPropertiesSetBBSVersion)
, vendor_id(&props_, fpgaPropertiesGetVendorID, fpgaPropertiesSetVendorID)
, model(&props_, fpgaPropertiesGetModel, fpgaPropertiesSetModel)
, local_memory_size(&props_, fpgaPropertiesGetLocalMemorySize, fpgaPropertiesSetLocalMemorySize)
, capabilities(&props_, fpgaPropertiesGetCapabilities, fpgaPropertiesSetCapabilities)
, guid(&props_, fpgaPropertiesGetGUID, fpgaPropertiesSetGUID)
, num_mmio(&props_, fpgaPropertiesGetNumMMIO, fpgaPropertiesSetNumMMIO)
, num_interrupts(&props_, fpgaPropertiesGetNumInterrupts, fpgaPropertiesSetNumInterrupts)
, accelerator_state(&props_, fpgaPropertiesGetAcceleratorState, fpgaPropertiesSetAcceleratorState)
, object_id(&props_, fpgaPropertiesGetObjectID, fpgaPropertiesSetObjectID)
, parent(&props_, fpgaPropertiesGetParent, fpgaPropertiesSetParent)
{
    auto res = fpgaGetProperties(nullptr, &props_);
    if (res != FPGA_OK){
        props_ = nullptr;
        // TODO: Throw fpga_error
    }
}

properties::~properties(){
    if (props_ != nullptr){
        auto res = fpgaDestroyProperties(&props_);
        if (res != FPGA_OK){
            // TODO: Throw fpga_error
        }
    }

}

properties::ptr_t properties::read(fpga_token tok){
    ptr_t p(new properties());
    auto res = fpgaGetProperties(tok, &p->props_);
    if (res != FPGA_OK){
        // TODO: Throw fpga_error
        p.reset();
    }
    return p;
}

properties::ptr_t properties::read(token::ptr_t tok){
    return read(tok->get());
}

} // end of namespace types
} // end of namespace fpga
} // end of namespace opae