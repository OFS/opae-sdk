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

#include "pycontext.h"

buffer_registry* buffer_registry::instance_ = nullptr;

buffer_registry::buffer_registry() : buffers_() {}

buffer_registry& buffer_registry::instance() {
  if (buffer_registry::instance_ == nullptr) {
    buffer_registry::instance_ = new buffer_registry();
  }
  return *buffer_registry::instance_;
}

void buffer_registry::register_handle(handle_t handle) {
  buffers_[handle] = buffer_list_t();
}

void buffer_registry::add_buffer(handle_t handle, shared_buffer_t buffer) {
  buffers_[handle].push_back(buffer);
}

void buffer_registry::unregister_handle(handle_t handle) {
  // find the handle in the internal map
  // and release all buffers associated with the handle
  auto it = buffers_.find(handle);
  if (it != buffers_.end()) {
    for (auto b : it->second) {
      b->release();
    }
    buffers_.erase(it);
  }
}
