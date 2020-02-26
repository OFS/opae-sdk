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
#include <opae/cxx/core/sysobject.h>
#include <opae/cxx/core/token.h>
#include <pybind11/pybind11.h>

const char *sysobject_doc();
const char *sysobject_doc_token_get();
const char *sysobject_doc_handle_get();
const char *sysobject_doc_object_get();
const char *sysobject_doc_token_find();
const char *sysobject_doc_handle_find();
const char *sysobject_doc_object_find();
const char *sysobject_doc_bytes();
const char *sysobject_doc_getitem();
const char *sysobject_doc_getslice();

opae::fpga::types::sysobject::ptr_t token_get_sysobject(
    opae::fpga::types::token::ptr_t tok, const std::string &name);
opae::fpga::types::sysobject::ptr_t handle_get_sysobject(
    opae::fpga::types::handle::ptr_t tok, const std::string &name);
opae::fpga::types::sysobject::ptr_t sysobject_get_sysobject(
    opae::fpga::types::sysobject::ptr_t tok, const std::string &name);
opae::fpga::types::sysobject::ptr_t token_find_sysobject(
    opae::fpga::types::token::ptr_t tok, const std::string &name,
    int flags = 0);
opae::fpga::types::sysobject::ptr_t handle_find_sysobject(
    opae::fpga::types::handle::ptr_t tok, const std::string &name,
    int flags = 0);
opae::fpga::types::sysobject::ptr_t sysobject_find_sysobject(
    opae::fpga::types::sysobject::ptr_t tok, const std::string &name,
    int flags = 0);
std::string sysobject_bytes(opae::fpga::types::sysobject::ptr_t obj);
pybind11::object sysobject_getitem(opae::fpga::types::sysobject::ptr_t obj,
                          uint32_t offset);
std::string sysobject_getslice(opae::fpga::types::sysobject::ptr_t obj,
                                    pybind11::slice slice);
