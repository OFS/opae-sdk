#pragma once
#include <map>
#include <memory>
#include <iostream>
#include "pvalue.h"
#include <opae/properties.h>

namespace opae
{
namespace fpga
{
namespace types
{

class token;

class properties
{
public:
    typedef std::shared_ptr<properties> ptr_t;

    properties();

    ~properties();

    fpga_properties get() const { return props_; }

    static properties::ptr_t read(std::shared_ptr<token> t);
    static properties::ptr_t read(fpga_token t);

    pvalue<fpga_objtype> 	       type;
    pvalue<uint8_t> 		       bus;
    pvalue<uint8_t>                device;
    pvalue<uint8_t>                function;
    pvalue<uint8_t>                socket_id;
    pvalue<uint32_t>               num_slots;
    pvalue<uint64_t>               bbs_id;
    pvalue<fpga_version> 	       bbs_version;
    pvalue<uint16_t>               vendor_id;
    pvalue<char*>                  model;
    pvalue<uint64_t>               local_memory_size;
    pvalue<uint64_t>               capabilities;
    pvalue<fpga_guid>              guid;
    pvalue<uint32_t>               num_mmio;
    pvalue<uint32_t>               num_interrupts;
    pvalue<fpga_accelerator_state> accelerator_state;
    pvalue<uint64_t> 		       object_id;
    pvalue<fpga_token>             parent;

private:
    fpga_properties props_;
};

} // end of namespace types
} // end of namespace fpga
} // end of namespace opae
