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

#include "property_map.h"
#include <uuid/uuid.h>
#include <opae/fpga.h>

using namespace intel::utils;

namespace intel
{
namespace fpga
{

bool property_map::from_options(intel::utils::option_map & opts)
{
    bool res = false;

    // Return true if any of GUID, bus, device, function, and socket-id is set.
    // Return false if none of them is set.
    option::ptr_t opt = opts.find("guid");
    if (opt)
    {
        fpga_guid guid;
        if (uuid_parse(opt->value<std::string>().c_str(), guid) < 0)
        {
            std::cerr << "Could not parse guid: " << opt->value<std::string>() << std::endl;
        }
        else
        {
            res = set<fpga_guid>(fpgaPropertiesSetGUID, "guid", guid) || res;
        }
    }

    opt = opts.find("bus");
    if (opt && opt->is_set())
    {
        res = set<uint8_t>(fpgaPropertiesSetBus, "bus", opt) || res;
    }
    else
    {
        opt = opts.find("bus-number");
        res = set<uint8_t>(fpgaPropertiesSetBus, "bus-number", opt) || res;
    }

    opt = opts.find("device");
    res = set<uint8_t>(fpgaPropertiesSetDevice, "device", opt) || res;

    opt = opts.find("function");
    res = set<uint8_t>(fpgaPropertiesSetFunction, "function", opt) || res;

    opt = opts.find("socket-id");
    res = set<uint8_t>(fpgaPropertiesSetSocketID, "socket-id", opt) || res;

    // TODO: Set accelerator/FPGA specific properties if in options

    return res;
}


} // end of namespace fpga
} // end of namespace intel
