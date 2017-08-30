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

#include "fpga_resource.h"
#include "property_map.h"
#include <opae/fpga.h>
#include <uuid/uuid.h>

namespace intel
{
namespace fpga
{

using namespace std;

fpga_resource::fpga_resource(shared_token token, fpga_properties props, fpga_resource::ptr_t parent)
: handle_(nullptr)
, log_()
, token_(token)
, props_(props)
, parent_(parent)
, guid_("")
, bus_(0)
, device_(0)
, function_(0)
, socket_id_(0)
, is_copy_(false)
{
    fpga_guid guid;

    property_map property(props_);
    if (property.get<fpga_guid>(fpgaPropertiesGetGUID, "guid", guid))
    {
        char c_str[37];
        uuid_unparse(guid, c_str);
        guid_.assign(c_str);
    }
    property.get(fpgaPropertiesGetBus, "bus", bus_);
    property.get(fpgaPropertiesGetDevice, "device", device_);
    property.get(fpgaPropertiesGetFunction, "function", function_);
    property.get(fpgaPropertiesGetSocketID, "socket_id",  socket_id_);
}

fpga_resource::fpga_resource(const fpga_resource & other)
: handle_(other.handle_)
, parent_(other.parent_)
, guid_(other.guid_)
, bus_(other.bus_)
, device_(other.device_)
, function_(other.function_)
, socket_id_(other.socket_id_)
, is_copy_(true)
{
    if (fpgaCloneProperties(other.props_, &props_) != FPGA_OK)
    {
        std::cerr << "Error cloning properties" << std::endl;
    }
}

fpga_resource & fpga_resource::operator=(const fpga_resource & other)
{
    if (this != &other)
    {
        handle_ = other.handle_;
        parent_ = other.parent_;
        guid_ = other.guid_;
        bus_ = other.bus_;
        device_ = other.device_;
        function_ = other.function_;
        socket_id_ = other.socket_id_;
        is_copy_ = true;
        if (fpgaCloneProperties(other.props_, &props_) != FPGA_OK)
        {
            std::cerr << "Error cloning properties" << std::endl;
        }
    }
    return *this;
}


fpga_resource::~fpga_resource()
{
    if (!is_copy_ && is_open())
    {
        close();
    }
    fpgaDestroyProperties(&props_);
    props_ = nullptr;
    handle_ = nullptr;
}


bool fpga_resource::enumerate_tokens(fpga_objtype objtype,
                                     const std::vector<intel::utils::option_map::ptr_t> & options,
                                     vector<shared_token> & tokens)
{
    bool result = false;
    uint32_t filter_size = 0;
    intel::utils::logger log;
    vector<fpga_properties> filters;
    vector<fpga_token>      real_tokens;

    if (options.empty())
    {
        filter_size = 1;
        filters.resize(1);
        fpgaGetProperties(nullptr, &filters[0]);
        fpgaPropertiesSetObjectType(filters[0], objtype);
    }
    else
    {
        filters.reserve(options.size());
        filters.resize(0);

        for (int i = 0; i < options.size(); ++i)
        {
            fpga_properties props;
            if (fpgaGetProperties(nullptr, &props) == FPGA_OK)
            {
                fpgaPropertiesSetObjectType(props, objtype);
                property_map pmap(props);
                if (pmap.from_options(*options[i]))
                {
                    filters.push_back(props);
                }
                else
                {
                    fpgaDestroyProperties(&props);
                }
            }
        }
    }
    uint32_t matches = 0;
    if (fpgaEnumerate(filters.data(), filters.size(), nullptr, 0, &matches) == FPGA_OK)
    {
        tokens.reserve(matches);
        real_tokens.resize(matches);
        if (fpgaEnumerate(filters.data(), filters.size(), real_tokens.data(), real_tokens.size(), &matches) == FPGA_OK)
        {
            result = true;

            // wrap the tokens with a shared_ptr and a custom deleter
            // these tokens are cloned by the fpgaEnumerate API
            for(auto t : real_tokens)
            {
                tokens.push_back(shared_token(new fpga_token(t),
                            [=](fpga_token* tok_)
                            {
                                if (fpgaDestroyToken(tok_) != FPGA_OK)
                                {
                                    throw new std::runtime_error("Bad Token");
                                }
                            }));
            }
        }
    }

    for (auto f : filters)
    {
        fpgaDestroyProperties(&f);
    }

    return result;
}

std::string fpga_resource::sysfs_path_from_token(fpga_token t)
{
   struct _fpga_token
   {
       uint64_t magic;
       char sysfspath[256];
       char devpath[256];
   };
   struct _fpga_token *_t = (struct _fpga_token *)t;
   return std::string(_t ? _t->sysfspath : "");
}

bool fpga_resource::open(bool shared)
{
    uint32_t flags = shared ? FPGA_OPEN_SHARED : 0;
    if (!is_open() && fpgaOpen(*token_, &handle_, flags) == FPGA_OK)
    {
        return true;
    }
    return false;
}

bool fpga_resource::close()
{
    if (is_open() && fpgaClose(handle_) == FPGA_OK)
    {
        handle_ = nullptr;
        return true;
    }
    return false;
}

bool fpga_resource::is_open()
{
    return handle_ != nullptr;
}

std::string fpga_resource::guid()
{
    return guid_;
}

fpga_resource::ptr_t fpga_resource::parent()
{
    return parent_;
}

uint8_t fpga_resource::bus()
{
    return bus_;
}

uint8_t fpga_resource::device()
{
    return device_;
}

uint8_t fpga_resource::function()
{
    return function_;
}

uint8_t fpga_resource::socket_id()
{
    return socket_id_;
}

} // end of namespace fpga
} // end of namespace intel
