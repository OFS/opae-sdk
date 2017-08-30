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

#include "fpga_app.h"
#include "accelerator.h"
#include "utils.h"
#include <uuid/uuid.h>
#include <fpga/types.h>

using namespace intel::utils;
using namespace std;

namespace intel
{
namespace fpga
{

fpga_app::fpga_app()
{
}

fpga_app::~fpga_app()
{
    shutdown();
}

std::vector<option_map::ptr_t> enumerate(intel::utils::option_map & filter)
{
    std::vector<option_map::ptr_t> resources;
    fpga_properties props = nullptr;
    //uint32_t        num_matches = 0;
    //std::vector<option::ptr_t> opts;
    //for (auto key : property_list)
    //{
    //    auto prop = filter.find(key);
    //    if (prop)
    //    {
    //        opts.push_back(prop);
    //    }
    //}
    //
    //fpgaPropertiesSetObjectType(props, FPGA_ACCELERATOR);
    //res = fpgaEnumerate(&props, 1, nullptr, &num_matches);
    //fpga_token      tokens[num_matches];
    //res = fpgaEnumerate(&props, 1, tokens, &num_matches);
    //list return_list;
    //char guid_str[36];
    //for (int i = 0; i < num_matches; ++i)
    //{
    //    dict obj;
    //    fpga_guid guid;
    //    res = fpgaGetPropertiesFromToken(tokens[i], props);
    //    res = fpgaPropertiesGetGUID(props, &guid);
    //    uuid_unparse(guid, guid_str);
    //    obj["guid"] = guid_str;
    //    return_list.append(obj);
    //}
    return resources;


}

void fpga_app::start(hwenv_t env)
{
}

accelerator::ptr_t fpga_app::open(const std::string & afu_id, const option_map & options )
{
    auto it = open_accelerators_.find(afu_id);
    if (it != open_accelerators_.end())
    {
        return it->second;
    }

    accelerator::ptr_t accelerator_ptr;
    fpga_properties filter = nullptr;
    fpga_token token;
    fpga_handle handle;
    fpga_guid guid;
    uint32_t matches = 1;

    if (uuid_parse(afu_id.c_str(), guid) < 0)
    {
        log_.error("fpga_app::open") << "Could not parse guid: " << afu_id << std::endl;
        return accelerator_ptr;
    }

    fpga_result res = fpgaCreateProperties(&filter);
    if (res != FPGA_OK)
    {
        log_.error("fpga_app::open") << "Could not create properties" << std::endl;
        return accelerator_ptr;
    }

    res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
    res = fpgaPropertiesSetGUID(filter, guid);

    option::ptr_t opt = options.find("bus-number");
    if (opt)
    {
        fpgaPropertiesSetBus(filter, opt->value<uint8_t>());
    }


    opt = options.find("device");
    if (opt)
    {
        fpgaPropertiesSetDevice(filter, opt->value<uint8_t>());
    }


    opt = options.find("function");
    if (opt)
    {
        fpgaPropertiesSetFunction(filter, opt->value<uint8_t>());
    }

    opt = options.find("socket-id");
    if (opt)
    {
        fpgaPropertiesSetSocketID(filter, opt->value<uint8_t>());
    }


    res = fpgaEnumerate(&filter, 1, &token, &matches);
    fpgaDestroyProperties(&filter);

    if (res != FPGA_OK)
    {
        log_.error("fpga_app::open") << "Enumerating for accelerator" << std::endl;
        return accelerator_ptr;
    }

    if (matches < 1)
    {
        log_.warn("fpga_app::open") << "Could not find accelerator" << std::endl;
        return accelerator_ptr;
    }

    res = fpgaOpen(token, &handle, 0);
    if (res != FPGA_OK)
    {
        log_.error("fpga_app::open") << "Could not open accelerator" << std::endl;
        return accelerator_ptr;
    }
    accelerator_ptr.reset(new accelerator(handle));

    open_accelerators_[afu_id] = accelerator_ptr;
    return accelerator_ptr;
}

void
fpga_app::shutdown()
{
    for (auto it : open_accelerators_)
    {
        it.second->release();
    }
}

} // end of namespace fpga
} // end of namespace intel
