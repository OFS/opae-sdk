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
#pragma once
#include <Python.h>

#include <opae/cxx/core/handle.h>
#include <pybind11/pybind11.h>

const char *handle_doc_open();
opae::fpga::types::handle::ptr_t handle_open(
    opae::fpga::types::token::ptr_t tok, int flags = 0);

const char *handle_doc_reconfigure();
void handle_reconfigure(opae::fpga::types::handle::ptr_t handle, uint32_t slot,
                        pybind11::object, int flags);

const char *handle_doc_valid();
bool handle_valid(opae::fpga::types::handle::ptr_t handle);

const char *handle_doc_context_enter();
opae::fpga::types::handle::ptr_t handle_context_enter(opae::fpga::types::handle::ptr_t hnd);
const char *handle_doc_context_exit();
void handle_context_exit(opae::fpga::types::handle::ptr_t hnd, pybind11::args args);

const char *handle_doc_close();
const char *handle_doc_reset();
const char *handle_doc_read_csr32();
const char *handle_doc_read_csr64();
const char *handle_doc_write_csr32();
const char *handle_doc_write_csr64();

