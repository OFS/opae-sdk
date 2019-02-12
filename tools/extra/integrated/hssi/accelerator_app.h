// Copyright(c) 2017-2018, Intel Corporation
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
#include <memory>
#include "option_map.h"
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/shared_buffer.h>
#include <thread>
#include <future>


namespace intel
{
namespace fpga
{
class accelerator_app
{
public:
    accelerator_app()
    : name_("none")
    , disabled_(false)
    {

    }

    explicit accelerator_app(const std::string &name)
    : name_(name)
    , disabled_(false)
    {
    }

    virtual const std::string & name()
    {
        return name_;
    }

    virtual void disabled(bool value)
    {
        disabled_ = value;
    }

    virtual bool disabled() const
    {
        return disabled_;
    }


    typedef std::shared_ptr<accelerator_app> ptr_t;
    virtual const std::string & afu_id() = 0;
    virtual intel::utils::option_map & get_options() = 0;
    virtual void assign(opae::fpga::types::handle::ptr_t accelerator) = 0;
    virtual bool setup() = 0;
    virtual bool run() = 0;
    virtual std::future<bool> run_async()
    {
        return std::async(std::launch::async, &accelerator_app::run, this);
    }

    virtual opae::fpga::types::shared_buffer::ptr_t dsm() const = 0;
    virtual uint64_t cachelines()    const  = 0;

private:
    std::string name_;
    bool disabled_;
};

} // end of namespace fpga
} // end of namespace intel

