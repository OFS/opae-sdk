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
#include "option_map.h"
#include <memory>
#include <vector>
#include <opae/types.h>

namespace intel
{
namespace fpga
{


typedef std::shared_ptr<fpga_token> shared_token;

class fpga_resource
{
public:
    typedef std::shared_ptr<fpga_resource> ptr_t;

    enum type_t
    {
        fpga = 0,
        accelerator,
    };


    fpga_resource(shared_token token, fpga_properties props, ptr_t parent = ptr_t());
    virtual ~fpga_resource();

    static bool enumerate_tokens(fpga_objtype ojbtype, const std::vector<intel::utils::option_map::ptr_t> & options, std::vector<shared_token> & tokens);

    virtual type_t type() = 0;

    virtual bool is_open();

    virtual bool open(bool shared);

    virtual bool close();

    virtual std::string guid();

    virtual ptr_t parent();

    virtual uint8_t bus();

    virtual uint8_t device();

    virtual uint8_t function();

    virtual uint8_t socket_id();

    static std::string sysfs_path_from_token(fpga_token t);

protected:
    fpga_resource(const fpga_resource &other);
    fpga_resource & operator=(const fpga_resource & other);
    fpga_handle          handle_;
    intel::utils::logger log_;

private:
    shared_token         token_;
    fpga_properties      props_;
    ptr_t                parent_;
    std::string          guid_;
    uint8_t              bus_;
    uint8_t              device_;
    uint8_t              function_;
    uint8_t              socket_id_;
    bool                 is_copy_;
};

} // end of namespace fpga
} // end of namespace intel
