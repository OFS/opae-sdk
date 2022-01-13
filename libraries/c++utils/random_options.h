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
#include <string>
#include <memory>
#include "option.h"
#include "log.h"
#include <vector>
#include "choice.h"
#include "range.h"

namespace intel
{
namespace utils
{

class random_options
{
public:
    template<typename T>
    void add_choices(const std::string & key, const std::vector<T> & values)
    {
        typename choice<T>::ptr_t choices(new choice<T>(key, values));
        options_.add_option(std::dynamic_pointer_cast<option>(choices));
    }

    template<typename T>
    void add_range(const std::string & key, T minval, T maxval)
    {
        typename range<T>::ptr_t values(new range<T>(key, minval, maxval));
        options_.add_option(std::dynamic_pointer_cast<option>(values));
    }

    void random(option_map & opts)
    {
        for (auto o : options_)
        {
            auto o2 = opts.find(o->name());
            if (o2)
            {
                *o2 = o->any();
            }

        }

    }

private:
    option_map options_;
};

} // end of namespace utils
} // end of namespace intel
