// Copyright(c) 2018, Intel Corporation
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

#include <opae/event.h>

#include <opae/cxx/core/events.h>
#include <opae/cxx/core/except.h>

namespace opae {
namespace fpga {
namespace types {

event::~event() {
  auto res = fpgaUnregisterEvent(*handle_, type_, event_handle_);
  if (res != FPGA_OK) {
    std::cerr << "Error while calling fpgaUnregisterEvent: " << fpgaErrStr(res)
              << "\n";
  }

  res = fpgaDestroyEventHandle(&event_handle_);
  if (res != FPGA_OK) {
    std::cerr << "Error while calling fpgaDestroyEventHandle: "
              << fpgaErrStr(res) << "\n";
  }
}

event::operator fpga_event_handle() { return event_handle_; }

event::ptr_t event::register_event(handle::ptr_t h, event::type_t t,
                                   int flags) {
  if (!h) {
    throw std::invalid_argument("handle object is null");
  }

  event::ptr_t evptr;
  fpga_event_handle eh;
  ASSERT_FPGA_OK(fpgaCreateEventHandle(&eh));
  ASSERT_FPGA_OK(fpgaRegisterEvent(*h, t, eh, flags));
  evptr.reset(new event(h, t, eh));
  ASSERT_FPGA_OK(fpgaGetOSObjectFromEventHandle(eh, &evptr->os_object_));
  return evptr;
}

int event::os_object() const { return os_object_; }

event::event(handle::ptr_t h, event::type_t t, fpga_event_handle eh)
    : handle_(h), type_(t), event_handle_(eh), os_object_(-1) {}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
