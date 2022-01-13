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
#include <vector>
#include <memory>
#include <string>
#include "option.h"
#include "any_value.h"

namespace intel
{
namespace utils
{

template<typename T>
class choice : public typed_option<T>
{
public:
    typedef std::shared_ptr<choice<T>> ptr_t;

    choice(const std::string & name, const std::vector<T> & values)
    : name_(name)
    , values_(values)
    , it_(values_.begin())
    , random_(0, values_.size()-1)
    {
    }

    T next()
    {
        ++it_;
        if (it_ == values_.end())
            it_ = values_.begin();
        return *it_;
    }

    T random()
    {
        return *(values_.begin()+random_());
    }

    virtual const std::string & name() { return name_; }

    virtual any_value any() { return random(); }

private:
    std::string                             name_;
    std::vector<T>                          values_;
    intel::utils::random<size_t>            random_;
    typename std::vector<T>::const_iterator it_;
};

} // end of namespace utils
} // end of namespace intel
