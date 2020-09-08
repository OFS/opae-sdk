// Copyright(c) 2020, Intel Corporation
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
#include "test_afu.h"
#include "dummy_afu.h"

using namespace opae::app;
using opae::fpga::types::shared_buffer;

namespace dummy_afu {

class lpbk_test : public test_command
{
public:
  lpbk_test(){}
  virtual ~lpbk_test(){}
  virtual const char *name() const
  {
    return "lpbk";
  }

  virtual const char *description() const
  {
    return "run simple loopback test";
  }

  virtual int run(test_afu *afu, CLI::App *app)
  {
    (void)app;
    auto d_afu = dynamic_cast<dummy_afu*>(afu);
    auto done = d_afu->register_interrupt();
    source_ = d_afu->allocate(64);
    d_afu->fill(source_);
    destination_ = d_afu->allocate(64);
    d_afu->write64(MEM_TEST_SRC_ADDR, source_->io_address());
    d_afu->write64(MEM_TEST_DST_ADDR, destination_->io_address());
    d_afu->write64(MEM_TEST_CTRL, 0x0);
    d_afu->write64(MEM_TEST_CTRL, 0b1);
    d_afu->interrupt_wait(done, 1000);
    d_afu->compare(source_, destination_);
    return 0;
  }

protected:
  shared_buffer::ptr_t source_;
  shared_buffer::ptr_t destination_;
};

} // end of namespace dummy_afu
