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
#pragma once
#include <map>
#include <memory>
#include <iostream>

#include <opae/properties.h>
#include <opaec++/pvalue.h>

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
    properties(fpga_objtype objtype);

    ~properties();

    fpga_properties get() const { return props_; }

    static properties::ptr_t read(std::shared_ptr<token> t);
    static properties::ptr_t read(fpga_token t);

    pvalue<fpga_objtype>           type;
    pvalue<uint8_t>                bus;
    pvalue<uint8_t>                device;
    pvalue<uint8_t>                function;
    pvalue<uint8_t>                socket_id;
    pvalue<uint32_t>               num_slots;
    pvalue<uint64_t>               bbs_id;
    pvalue<fpga_version>           bbs_version;
    pvalue<uint16_t>               vendor_id;
    pvalue<char*>                  model;
    pvalue<uint64_t>               local_memory_size;
    pvalue<uint64_t>               capabilities;
    pvalue<uint32_t>               num_mmio;
    pvalue<uint32_t>               num_interrupts;
    pvalue<fpga_accelerator_state> accelerator_state;
    pvalue<uint64_t>               object_id;
    pvalue<fpga_token>             parent;
    guid_t                         guid;

private:
    fpga_properties props_;
};

} // end of namespace types
} // end of namespace fpga
} // end of namespace opae
