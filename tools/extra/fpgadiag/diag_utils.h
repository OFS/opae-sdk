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
#include <string.h>
#include <vector>
#include <initializer_list>
#include <chrono>
#include <thread>
#include <opae/fpga.h>
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/shared_buffer.h>

#include "safe_string/safe_string.h"

#include "option_map.h"
#include "option.h"

namespace intel
{
namespace fpga
{

const std::chrono::microseconds FPGA_DSM_TIMEOUT{1000000};
const std::chrono::microseconds ASE_DSM_TIMEOUT{1000000L*100000};


template<typename T>
bool buffer_poll(opae::fpga::types::shared_buffer::ptr_t buffer, std::size_t offset, std::chrono::microseconds timeout, T mask, T value)
{
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    std::chrono::microseconds elapsed;

    do
    {
        if ((buffer->read<T>(offset) & mask) == value)
            return true;

        elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now() - start);

    }while(elapsed < timeout);

    return false;
}

template<typename T>
bool buffer_wait(opae::fpga::types::shared_buffer::ptr_t buffer, std::size_t offset, std::chrono::microseconds each, std::chrono::microseconds timeout, T mask, T value)
{
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    std::chrono::microseconds elapsed;

    do
    {
        if ((buffer->read<T>(offset) & mask) == value)
            return true;

        std::this_thread::sleep_for(each);

        elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now() - start);

    }while(elapsed < timeout);

    return false;
}

class split_buffer : public opae::fpga::types::shared_buffer {
public:
  typedef std::shared_ptr<split_buffer> ptr_t;

  split_buffer(opae::fpga::types::shared_buffer::ptr_t parent, size_t size, uint8_t *virt, uint64_t wsid, uint64_t io_address)
    : opae::fpga::types::shared_buffer(opae::fpga::types::handle::ptr_t() , size, virt, wsid, io_address), parent_(parent){}
	template <typename T>
	static std::vector<opae::fpga::types::shared_buffer::ptr_t> split(opae::fpga::types::shared_buffer::ptr_t parent,
					std::initializer_list<T> sizes)
	{
		std::vector<opae::fpga::types::shared_buffer::ptr_t> v;
		std::size_t offset = 0;

		v.reserve(sizes.size());

		for (const auto &sz : sizes) {
			ptr_t p;
			p.reset(new split_buffer(
				parent, sz,
				const_cast<uint8_t *>(parent->c_type())
					+ offset,
				parent->wsid(), parent->io_address() + offset));
			v.push_back(p);
			offset += sz;
		}

		return v;
	}
private:
  opae::fpga::types::shared_buffer::ptr_t parent_;
};


opae::fpga::types::properties::ptr_t get_properties(intel::utils::option_map::ptr_t opts, fpga_objtype otype);

opae::fpga::types::token::ptr_t get_parent_token(opae::fpga::types::handle::ptr_t h);

uint64_t umsg_num(opae::fpga::types::handle::ptr_t h);

bool umsg_set_mask(opae::fpga::types::handle::ptr_t h, uint64_t mask);

uint64_t * umsg_get_ptr(opae::fpga::types::handle::ptr_t h);


} // end of namespace fpga
} // end of namespace intel
