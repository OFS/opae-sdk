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
        std::cout << "guid prop read: " << props->guid << "\n";
        fpga_token p = props->parent;

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
