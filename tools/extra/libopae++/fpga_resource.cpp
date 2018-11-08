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
#include <opae/fpga.h>

using namespace opae::fpga::types;

namespace intel
{
namespace fpga
{

using namespace std;

fpga_resource::fpga_resource(token::ptr_t token, properties::ptr_t props, token::ptr_t parent)
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
    char guid_str[84];
    uuid_unparse(props->guid.c_type(), guid_str);
    guid_ = guid_str;
    bus_ = props->bus;
    device_ = props->device;
    socket_id_ = props->socket_id;
}

fpga_resource::fpga_resource(const fpga_resource & other)
: handle_(other.handle_)
, props_(other.props_)
, parent_(other.parent_)
, guid_(other.guid_)
, bus_(other.bus_)
, device_(other.device_)
, function_(other.function_)
, socket_id_(other.socket_id_)
, is_copy_(true)
{
}

fpga_resource & fpga_resource::operator=(const fpga_resource & other)
{
    if (this != &other)
    {
        handle_ = other.handle_;
        props_ = other.props_;
        parent_ = other.parent_;
        guid_ = other.guid_;
        bus_ = other.bus_;
        device_ = other.device_;
        function_ = other.function_;
        socket_id_ = other.socket_id_;
        is_copy_ = true;
    }
    return *this;
}


fpga_resource::~fpga_resource()
{
    if (!is_copy_ && is_open())
    {
        close();
    }
    props_.reset();
    handle_.reset();
}


bool fpga_resource::enumerate_tokens(fpga_objtype objtype,
                                     const std::vector<intel::utils::option_map::ptr_t> & options,
                                     vector<token::ptr_t> & tokens)
{
    std::vector<properties::ptr_t> props_vector;
    for (auto optmap : options) {
      properties::ptr_t p = properties::get(objtype);
      auto opt = optmap->find("bus");
      if (opt && opt->is_set()) {
        p->bus = opt->value<uint8_t>();
      }
      opt = optmap->find("device");
      if (opt && opt->is_set()) {
        p->device = opt->value<uint8_t>();
      }
      opt = optmap->find("function");
      if (opt && opt->is_set()) {
        p->function = opt->value<uint8_t>();
      }
      opt = optmap->find("socket_id");
      if (opt && opt->is_set()) {
        p->socket_id = opt->value<uint8_t>();
      }
      opt = optmap->find("guid");
      if (opt && opt->is_set()) {
        p->guid.parse(opt->value<const char*>());
      }
      props_vector.push_back(p);
    }

    tokens = token::enumerate(props_vector);
    return !tokens.empty();
}

bool fpga_resource::open(bool shared)
{
    uint32_t flags = shared ? FPGA_OPEN_SHARED : 0;
    handle_ = handle::open(token_, flags);
    return handle_ != nullptr;
}

bool fpga_resource::close()
{
    handle_->close();
    return true;
}

bool fpga_resource::is_open()
{
    return handle_ != nullptr;
}

std::string fpga_resource::guid()
{
    return guid_;
}

token::ptr_t fpga_resource::parent()
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

event::ptr_t fpga_resource::register_event(fpga_event_type event_type) const
{
    return event::register_event(handle_, event_type, 0);
}

} // end of namespace fpga
} // end of namespace intel
