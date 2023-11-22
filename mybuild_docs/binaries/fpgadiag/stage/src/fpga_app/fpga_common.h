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
#ifndef CACHELINE_BYTES
# define CACHELINE_BYTES 64
#endif // CACHELINE_BYTES
#ifndef LOG2_CL
# define LOG2_CL         6
#endif // LOG2_CL
#ifndef CACHELINE_ALIGNED_ADDR
#define CACHELINE_ALIGNED_ADDR(p) (reinterpret_cast<uint64_t>(p) >> LOG2_CL)
#endif // CACHELINE_ALIGNED_ADDR
#ifndef CL
# define CL(x) ((x)   * CACHELINE_BYTES)
#endif // CL
#ifndef MAX_CL
# define MAX_CL 65535
#endif // MAX_CL
#ifndef DEFAULT_FREQ
# define DEFAULT_FREQ MHZ(400)
#endif // DEFAULT_FREQ

#define GHZ(x) ((x) * 1000000000ULL)
#define MHZ(x) ((x) * 1000000ULL)
#define KHZ(x) ((x) * 1000ULL)

#define GB(x) ((x) * 1024*1024*1024)
#define MB(x) ((x) * 1024*1024)
#define KB(x) ((x) * 1024)

#define UNUSED_PARAM(x) (void)x
