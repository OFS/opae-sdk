
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
#include <opae/properties.h>
#include "log.h"

namespace intel
{
namespace fpga
{

class resource_properties
{
public:
    resource_properties();
    resource_properties(fpga_properties props);
    resource_properties(fpga_objtype objtype);
    resource_properties(const resource_properties & other);
    virtual ~resource_properties();

    resource_properties & operator=(const resource_properties & other);

    fpga_properties to_properties() const
    {
        return props_;
    }

    bool get_type(fpga_objtype &objype);
    bool set_type(fpga_objtype objtype);
    bool set_bus(uint8_t bus);
    bool get_bus(uint8_t &bus);
    bool set_device(uint8_t device);
    bool get_device(uint8_t &device);
    bool set_function(uint8_t function);
    bool get_function(uint8_t &function);
    bool set_socket_id(uint8_t socket_id);
    bool get_socket_id(uint8_t &socket_id);
    bool set_guid(const std::string & guid);
    bool set_guid(fpga_guid guid);
    bool get_guid(std::string & guid);
    bool get_guid(fpga_guid & guid);

private:

    template<typename T>
    bool get(fpga_result (*func)(fpga_properties, T*), T & value, const std::string & property_name)
    {
        fpga_result res = func(props_, &value);
        if (res == FPGA_OK)
        {
            return true;
        }
        log_.warn("properties") << "Could not get property " << property_name << ". Result is: " << res << "\n";
        return false;
    }

    template<typename T>
    bool set(fpga_result (*func)(fpga_properties, T), T value, const std::string & property_name)
    {
        fpga_result res = func(props_, value);
        if (res == FPGA_OK)
        {
            return true;
        }
        log_.warn("properties") << "Could not set property " << property_name << ". Result is: " << res << "\n";
        return false;
    }

    fpga_properties props_;
    intel::utils::logger log_;

};

} // end of namespace fpga
} // end of namespace intel
