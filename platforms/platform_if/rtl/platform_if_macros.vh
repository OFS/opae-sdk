//
// Copyright (c) 2018, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

`ifndef __PLATFORM_IF_MACROS_VH__
`define __PLATFORM_IF_MACROS_VH__

// ========================================================================
//
// The set of parameters passed to top-level modules may vary, depending on
// both the needs of an AFU and the interfaces offered by individual
// platforms. The problem with this variable number of parameters is
// syntactic. It is difficult to know when commas have to be inserted
// between parameters. These macros maintain state that properly inserts
// commas between parameters that may or may not be set, based on
// preprocessor macros.
//
// ========================================================================

// Begin a list of parameters with `PLATFORM_ARG_LIST_BEGIN
`define PLATFORM_ARG_LIST_BEGIN \
    `undef PLATFORM_ARG_LIST_SEPARATOR \
    `define PLATFORM_ARG_LIST_SEPARATOR

// Add a parameter with `PLATFORM_ARG_APPEND(parameter text)
`define PLATFORM_ARG_APPEND(arg) \
    `PLATFORM_ARG_LIST_SEPARATOR arg \
    `undef PLATFORM_ARG_LIST_SEPARATOR \
    `define PLATFORM_ARG_LIST_SEPARATOR ,

`endif // __PLATFORM_IF_MACROS_VH__
