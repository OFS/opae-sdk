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
#include <opae/types.h>
#include "option.h"
#include "log.h"
#include "option_map.h"
#include "any_value.h"

namespace intel
{
namespace fpga
{

class property_map
{
public:
    property_map(fpga_properties props)
    : props_(props)
    {
    }

    template<typename T>
    bool get(fpga_result (*func)(fpga_properties, T*), const std::string & property_name, T & value)
    {
        fpga_result res = func(props_, &value);
        if (res == FPGA_OK)
        {
            return true;
        }
        std::cerr << "Could not get property " << property_name << ". Result is: " << res << std::endl;
        return false;
    }

    template<typename T>
    bool set(fpga_result (*func)(fpga_properties, T), const std::string & property_name, T value)
    {
        fpga_result res = func(props_, value);
        if (res == FPGA_OK)
        {
            return true;
        }
        std::cerr <<  "Could not set property " << property_name << ". Result is: " << res << std::endl;
        return false;
    }

    template<typename T>
    bool set(fpga_result (*func)(fpga_properties, T), const std::string & property_name, intel::utils::option::ptr_t opt)
    {
        if (opt && opt->is_set())
        {
            T value;
            try
            {
                value = opt->value<T>();
            }
            catch(const intel::utils::any_value_cast_error &)
            {
                return false;
            }

            return set<T>(func, property_name, value);
        }
        return false;
    }

    fpga_properties get()
    {
        return props_;
    }

    bool from_options(intel::utils::option_map & opts);

private:
    fpga_properties props_;
};

} // end of namespace fpga
} // end of namespace intel
