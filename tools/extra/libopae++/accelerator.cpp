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

#include "accelerator.h"
#include "property_map.h"
#include "fpga_event.h"
#include "fpga_errors.h"

using namespace opae::fpga::types;
using namespace intel::utils;
using namespace std;

namespace intel
{
namespace fpga
{

accelerator::accelerator(token::ptr_t token, properties::ptr_t props,
          token::ptr_t parent)
: fpga_resource(token, props, parent)
, status_(accelerator::unknown)
, mmio_base_(nullptr)
, error_event_(0)
, port_errors_(0)
, throw_errors_(false)
{
}

accelerator::accelerator(const accelerator & other)
: fpga_resource(other)
, status_(other.status_)
, mmio_base_(other.mmio_base_)
, error_event_(0)
, port_errors_(0)
, throw_errors_(false)
{

}

accelerator & accelerator::operator=(const accelerator & other)
{
    if (this != &other)
    {
        status_ = other.status_;
        mmio_base_ = other.mmio_base_;
        fpga_resource::operator=(other);
    }
    return *this;
}
fpga_resource::type_t accelerator::type()
{
    return fpga_resource::accelerator;
}

bool accelerator::open(bool shared)
{

    port_errors_ = port_error::read(token_);
    error_assert();
    if (fpga_resource::open(shared))
    {
        error_event_ = register_event(FPGA_EVENT_ERROR);
        return true;
    }
    log_.warn("accelerator::open") << "Errors encountered while opening accelerator resource" << std::endl;
    return false;
}

bool accelerator::close()
{
    return fpga_resource::close();
}

vector<accelerator::ptr_t> accelerator::enumerate(vector<option_map::ptr_t> options)
{
    vector<accelerator::ptr_t> accelerator_list;
    vector<token::ptr_t> tokens;
    if (enumerate_tokens(FPGA_ACCELERATOR, options, tokens))
    {
        for (const token::ptr_t & tok : tokens)
        {
            properties::ptr_t props = properties::get(tok);
            fpga_token parent_tok = props->parent;
            auto parent_props = properties::get(parent_tok);
            auto tokens = token::enumerate({parent_props});

            accelerator_list.push_back(accelerator::ptr_t(new accelerator(tok, props, tokens[0])));

        }
    }

    return accelerator_list;
}

bool accelerator::ready()
{
    return status_ == accelerator::opened;
}

bool accelerator::write_mmio32(uint32_t offset, uint32_t value)
{
    error_assert();
    handle_->write_csr32(offset, value);
    return true;
}

bool accelerator::write_mmio64(uint32_t offset, uint64_t value)
{
    error_assert();
    handle_->write_csr64(offset, value);
    return true;
}

bool accelerator::read_mmio32(uint32_t offset, uint32_t & value)
{
    error_assert();
    value = handle_->read_csr32(offset);
    return true;
}

bool accelerator::read_mmio64(uint32_t offset, uint64_t & value)
{
    error_assert();
    value = handle_->read_csr64(offset);
    return true;
}

uint8_t* accelerator::mmio_pointer(uint32_t offset)
{
    (void)offset;
    error_assert();
    return nullptr;
}

bool accelerator::reset()
{
    handle_->reset();
    return true;
}

void accelerator::release()
{
    if (handle_ != nullptr)
    {
        handle_->close();
        handle_ = nullptr;
        status_ = status_t::released;
    }
}

shared_buffer::ptr_t accelerator::allocate_buffer(std::size_t size)
{
    return shared_buffer::allocate(handle_, size);
}

uint64_t accelerator::umsg_num()
{
    uint64_t num = 0;
    fpga_result res = fpgaGetNumUmsg(*handle_, &num);
    if (res == FPGA_OK)
       return num;
    return 0;
}

bool accelerator::umsg_set_mask(uint64_t mask)
{
    return FPGA_OK == fpgaSetUmsgAttributes(*handle_, mask);
}

uint64_t * accelerator::umsg_get_ptr()
{
    uint64_t *p = NULL;
    fpga_result res = fpgaGetUmsgPtr(*handle_, &p);
    if (res == FPGA_OK)
        return p;
    return NULL;
}

fpga_cache_counters accelerator::cache_counters()
{
    return fpga_cache_counters(parent());
}

fpga_fabric_counters accelerator::fabric_counters()
{
    return fpga_fabric_counters(parent());
}

void accelerator::error_assert(bool update)
{
  (void)update;
}

} // end of namespace fpga
} // end of namespace intel

