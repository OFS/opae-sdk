//
// Copyright (c) 2017, Intel Corporation
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

/**
 * \file mpf_internal.h
 * \brief MPF internal data structures
 */

#ifndef __FPGA_MPF_INTERNAL_H__
#define __FPGA_MPF_INTERNAL_H__

#include <stdint.h>

/*
 * Convenience macros for printing messages and errors.
 */
#ifdef __MPF_SHORT_FILE__
#undef __MPF_SHORT_FILE__
#endif // __MPF_SHORT_FILE__
#define __MPF_SHORT_FILE__             \
({ const char *file = __FILE__;    \
   const char *p    = file;        \
   while ( *p ) { ++p; }           \
   while ( (p > file)  &&          \
           ('/'  != *p) &&         \
           ('\\' != *p) ) { --p; } \
   if ( p > file ) { ++p; }        \
   p;                              \
})

#ifdef MPF_FPGA_MSG
#undef MPF_FPGA_MSG
#endif // MPF_FPGA_MSG
#define MPF_FPGA_MSG(format, ...)\
        printf( "%s:%u:%s() : " format "\n", __MPF_SHORT_FILE__, __LINE__,\
                                                __func__, ## __VA_ARGS__ )

// Forward declaration to avoid circular dependence.
typedef struct _mpf_handle_t* _mpf_handle_p;

#include "mpf_os.h"
#include "shim_vtp_internal.h"


/**
 * Internal structure for maintaining connected MPF state
 */
struct _mpf_handle_t
{
    // Arguments passed to mpfConnect()
    fpga_handle handle;
    uint32_t mmio_num;
    uint64_t mmio_offset;

    // Base MMIO offset of each shim.  0 if shim not present.
    uint64_t shim_mmio_base[CCI_MPF_SHIM_LAST_IDX];

    // VTP state
    mpf_vtp_state vtp;

    // Debug mode requested in mpf_flags?
    bool dbg_mode;
};


#endif // __FPGA_MPF_INTERNAL_H__
