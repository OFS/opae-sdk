#include "opaec++/properties.h"
#include "opaec++/token.h"
#include "opaec++/handle.h"
#include <uuid/uuid.h>
#include <opae/mmio.h>
#include <iostream>

using namespace opae::fpga::types;

int main(int argc, char* argvp[])
{
    const char* NLB0 = "D8424DC4-A4A3-C413-F89E-433683F9040B";
    const char* NLB3 = "F7DF405C-BD7A-CF72-22F1-44B0B93ACD18";
   
    properties p;
        
    p.socket_id = 1;
    p.type = FPGA_ACCELERATOR;
    uuid_t uuid;
    if (uuid_parse(NLB0, uuid) == 0){
        p.guid = uuid;
    }
    p.bbs_id = 0;
    auto tokens = token::enumerate({ p });
    if (tokens.size() > 0){
        auto tok = tokens[0];
        auto props = properties::read(tok->get());
        fpga_guid g;
        auto r = props->guid.get_value(g);
        if (r == FPGA_OK){
            char guid_str[84];
            uuid_unparse(g, guid_str);
            std::cout << "guid prop read: " << guid_str << "\n";
        }
        fpga_token p;
        r = props->parent.get_value(p);
        if (r == FPGA_OK){
            //char guid_str[84];
            //uuid_unparse(g, guid_str);
            //std::cout << "parent prop read: " << guid_str << "\n";
        }

        std::cout << "bus: 0x" << std::hex << props->bus << "\n";
        handle::ptr_t h = handle::open(tok, FPGA_OPEN_SHARED);
        uint8_t *mmio_ptr = 0;
        auto res = fpgaMapMMIO(h->get(), 0, reinterpret_cast<uint64_t**>(&mmio_ptr));
        if (res == FPGA_OK){
            uint64_t scratch = 0;
            if (fpgaWriteMMIO64(h->get(), 0, 0x100, 0xdeadbeef) == FPGA_OK &&
                fpgaReadMMIO64(h->get(), 0, 0x100, &scratch) == FPGA_OK){
                std::cout << "mmio @0x100: 0x" << std::hex << scratch << std::endl;
                std::cout << "mmio @0x100: 0x" << std::hex << *reinterpret_cast<uint64_t*>(mmio_ptr+0x100) << std::endl;
     
            }
            
        }
    }

    return  0;
}
