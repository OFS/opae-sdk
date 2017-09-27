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

using namespace intel::utils;
using namespace std;

namespace intel
{
namespace fpga
{

accelerator::accelerator(shared_token token, fpga_properties props,
          const std::string &par_sysfs, fpga_resource::ptr_t parent)
: fpga_resource(token, props, parent)
, status_(accelerator::unknown)
, parent_sysfs_(par_sysfs)
, mmio_base_(nullptr)
, port_errors_(0)
, error_event_(0)
{
}

accelerator::accelerator(const accelerator & other)
: fpga_resource(other)
, status_(other.status_)
, parent_sysfs_(other.parent_sysfs_)
, mmio_base_(other.mmio_base_)
, port_errors_(0)
, error_event_(0)
{

}

accelerator & accelerator::operator=(const accelerator & other)
{
    if (this != &other)
    {
        status_ = other.status_;
        parent_sysfs_ = other.parent_sysfs_;
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
    port_errors_ = port_error::read(socket_id());
    if (port_errors_) throw port_error(port_errors_.load());
    if (fpga_resource::open(shared))
    {
        auto result = fpgaMapMMIO(handle_, 0, reinterpret_cast<uint64_t**>(&mmio_base_));
        if (result == FPGA_NOT_SUPPORTED)
        {
            log_.warn("accelerator::open") << "Direct MMIO pointer may not be supported. Use API for MMIO access" << std::endl;
            result = fpgaMapMMIO(handle_, 0, nullptr);
        }

        if (result == FPGA_OK)
        {
            error_event_ = register_event(fpga_event::event_type::error);
            if (error_event_)
            {
                error_event_->notify([](void* data, fpga_event::event_type etype){
                    accelerator* this_guy = reinterpret_cast<accelerator*>(data);
                    if (etype == fpga_event::event_type::error){
                        this_guy->port_errors_ = port_error::read(this_guy->socket_id());
                    }
                }, this);
            }
            return true;
        }
    }
    log_.warn("accelerator::open") << "Errors encountered while opening accelerator resource" << std::endl;
    return false;
}

bool accelerator::close()
{
    return FPGA_OK == fpgaUnmapMMIO(handle_, 0) &&
           fpga_resource::close();
}

vector<accelerator::ptr_t> accelerator::enumerate(vector<option_map::ptr_t> options)
{
    vector<accelerator::ptr_t> accelerator_list;
    vector<shared_token> tokens;
    if (enumerate_tokens(FPGA_ACCELERATOR, options, tokens))
    {
        for (const shared_token & tok : tokens)
        {
            fpga_properties props;

            if (FPGA_OK == fpgaGetProperties(*tok, &props))
            {
                fpga_token parent = nullptr;
                fpga_resource::ptr_t parent_obj;
                fpga_properties parent_props = nullptr;

                std::string parent_sysfs_path = "";
                if (fpgaPropertiesGetParent(props, &parent) == FPGA_OK)
                {
                    if (fpgaGetProperties(parent, &parent_props) == FPGA_OK)
                    {
                        parent_sysfs_path = sysfs_path_from_token(parent);
                        // TODO: create parent object
                    }
                    else
                    {
                        parent_props = nullptr;
                    }
                }
                else
                {
                    parent = nullptr;
                }

                accelerator_list.push_back(accelerator::ptr_t(new accelerator(tok, props, parent_sysfs_path, parent_obj)));

                // TODO this may not be required when saving to another data struct..
                if (parent_props != nullptr)
                {
                    fpgaDestroyProperties(&parent_props);
                }
                if (parent != nullptr)
                {
                    fpgaDestroyToken(&parent);
                }
            }
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
    if (port_errors_) throw port_error(port_errors_.load());
    return FPGA_OK == fpgaWriteMMIO32(handle_, 0, offset, value);
}

bool accelerator::write_mmio64(uint32_t offset, uint64_t value)
{
    if (port_errors_) throw port_error(port_errors_.load());
    return FPGA_OK == fpgaWriteMMIO64(handle_, 0, offset, value);
}

bool accelerator::read_mmio32(uint32_t offset, uint32_t & value)
{
    if (port_errors_) throw port_error(port_errors_.load());
    return FPGA_OK == fpgaReadMMIO32(handle_, 0, offset, &value);
}

bool accelerator::read_mmio64(uint32_t offset, uint64_t & value)
{
    if (port_errors_) throw port_error(port_errors_.load());
    return FPGA_OK == fpgaReadMMIO64(handle_, 0, offset, &value);
}

uint8_t* accelerator::mmio_pointer(uint32_t offset)
{
    if (port_errors_) throw port_error(port_errors_.load());
    return mmio_base_ + offset;
}

bool accelerator::reset()
{
    if (port_errors_) throw port_error(port_errors_.load());
    fpga_result res = fpgaReset(handle_);
    if (res == FPGA_OK)
    {
        return true;
    }
    log_.warn("fpga_app::reset") << "Reset operation resulted in " << res << std::endl;
    return false;
}

void accelerator::release()
{
    if (handle_ != nullptr)
    {
        fpgaClose(handle_);
        handle_ = nullptr;
        status_ = status_t::released;
    }
}

dma_buffer::ptr_t accelerator::allocate_buffer(std::size_t size)
{
    dma_buffer::ptr_t buffer;
    uint64_t wsid;
    uint8_t *virt;
    uint64_t iova;
    fpga_result res = fpgaPrepareBuffer(handle_, size, reinterpret_cast<void**>(&virt), &wsid, 0);
    if (res == FPGA_OK)
    {
        res = fpgaGetIOAddress(handle_, wsid, &iova);
    }

    if (res == FPGA_OK)
    {
        buffer.reset(new dma_buffer(handle_, wsid, virt, iova, size));
    }

    return buffer;
}

uint64_t accelerator::umsg_num()
{
    uint64_t num = 0;
    fpga_result res = fpgaGetNumUmsg(handle_, &num);
    if (res == FPGA_OK)
       return num;
    return 0;
}

bool accelerator::umsg_set_mask(uint64_t mask)
{
    return FPGA_OK == fpgaSetUmsgAttributes(handle_, mask);
}

uint64_t * accelerator::umsg_get_ptr()
{
    uint64_t *p = NULL;
    fpga_result res = fpgaGetUmsgPtr(handle_, &p);
    if (res == FPGA_OK)
        return p;
    return NULL;
}

fpga_cache_counters accelerator::cache_counters() const
{
    return fpga_cache_counters(parent_sysfs_);
}

fpga_fabric_counters accelerator::fabric_counters() const
{
    return fpga_fabric_counters(parent_sysfs_);
}

} // end of namespace fpga
} // end of namespace intel

