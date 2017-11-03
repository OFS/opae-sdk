
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
#include "resource_properties.h"
#include <uuid/uuid.h>

namespace intel
{
namespace fpga
{

resource_properties::resource_properties()
: props_(nullptr)
{
    if (FPGA_OK != fpgaGetProperties(nullptr, &props_))
    {
        log_.error("resource_properties") << "Error creating fpga_resource_properties";
    }
}

resource_properties::resource_properties(fpga_properties props)
: props_(nullptr)
{
    if (FPGA_OK != fpgaCloneProperties(props, &props_))
    {
        log_.error("resource_properties") << "Error cloning fpga_resource_properties";
    }
}

resource_properties::resource_properties(fpga_objtype objtype)
: props_(nullptr)
{
    if (FPGA_OK != fpgaGetProperties(nullptr, &props_))
    {
        log_.error("resource_properties") << "Error creating fpga_resource_properties";
    }
    else
    {
        set<fpga_objtype>(fpgaPropertiesSetObjectType, objtype, "objtype");
    }
}

resource_properties::resource_properties(const resource_properties & other)
: props_(nullptr)
{
    if (FPGA_OK != fpgaCloneProperties(other.props_, &props_))
    {
        log_.error("resource_properties") << "Error cloning fpga_resource_properties";
    }
}

resource_properties & resource_properties::operator=(const resource_properties & other)
{
    if (&other != this)
    {
        if (FPGA_OK != fpgaCloneProperties(other.props_, &props_))
        {
            log_.error("resource_properties") << "Error cloning fpga_resource_properties";
        }
    }

    return *this;
}

resource_properties::~resource_properties()
{
    if (props_ != nullptr &&
        FPGA_OK != fpgaDestroyProperties(&props_))
    {
        log_.error("resource_properties") << "Error destroying fpga_resource_properties";
    }
}

bool resource_properties::set_type(fpga_objtype objtype)
{
    return set<fpga_objtype>(fpgaPropertiesSetObjectType, objtype, "objtype");
}

bool resource_properties::get_type(fpga_objtype &objtype)
{
    return get<fpga_objtype>(fpgaPropertiesGetObjectType, objtype, "objtype");
}

bool resource_properties::set_bus(uint8_t bus)
{
    return set<uint8_t>(fpgaPropertiesSetBus, bus, "bus");
}

bool resource_properties::get_bus(uint8_t &bus)
{
    return get<uint8_t>(fpgaPropertiesGetBus, bus, "bus");
}

bool resource_properties::set_device(uint8_t device)
{
    return set<uint8_t>(fpgaPropertiesSetDevice, device, "device");
}

bool resource_properties::get_device(uint8_t &device)
{
    return get<uint8_t>(fpgaPropertiesGetDevice, device, "device");
}

bool resource_properties::set_function(uint8_t function)
{
    return set<uint8_t>(fpgaPropertiesSetFunction, function, "function");
}

bool resource_properties::get_function(uint8_t &function)
{
    return get<uint8_t>(fpgaPropertiesGetFunction, function, "function");
}

bool resource_properties::set_socket_id(uint8_t socket_id)
{
    return set<uint8_t>(fpgaPropertiesSetSocketID, socket_id, "socket_id");
}

bool resource_properties::get_socket_id(uint8_t &socket_id)
{
    return get<uint8_t>(fpgaPropertiesGetSocketID, socket_id, "socket_id");
}

bool resource_properties::set_guid(const std::string & guid)
{
    fpga_guid _guid;
    if (uuid_parse(guid.c_str(), _guid) < 0)
    {
        log_.warn("set_guid") << "Could not parse guid string: " << guid << "\n";
        return false;
    }
    return set_guid(_guid);

}

bool resource_properties::set_guid(fpga_guid guid)
{
    return set<fpga_guid>(fpgaPropertiesSetGUID, guid, "guid");
}

bool resource_properties::get_guid(std::string & guid)
{
    fpga_guid _guid;
    if (get_guid(_guid))
    {
        uuid_unparse(_guid, const_cast<char*>(guid.data()));
    }

    return false;
}

bool resource_properties::get_guid(fpga_guid & guid)
{
    return get<fpga_guid>(fpgaPropertiesGetGUID, guid, "guid");
}

} // end of namespace fpga
} // end of namespace intel
