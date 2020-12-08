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
#ifndef STI_NOSYS_PROT_ST_DBG_IP_ALLOCATOR_H_INCLUDED
#define STI_NOSYS_PROT_ST_DBG_IP_ALLOCATOR_H_INCLUDED

#include <stdlib.h>

typedef struct {
    char *raw_buff;
    size_t span;
    size_t write_offset;
    size_t read_offset;
    size_t space_available;
} CIRCLE_BUFF;

inline void cbuff_init(CIRCLE_BUFF *cbuff, char *raw_buff, size_t raw_buff_sz) {
    cbuff->raw_buff = raw_buff;
    cbuff->span = raw_buff_sz;
    cbuff->write_offset = 0;
    cbuff->read_offset = 0;
    cbuff->space_available = raw_buff_sz;
}

// No safety here. Assumptions are there is enough valid data to prevent underflowing, and 'amt' is > 0.
inline void cbuff_free(CIRCLE_BUFF *cbuff, size_t amt) {
    cbuff->space_available += amt;
    cbuff->read_offset = (cbuff->read_offset + amt) % cbuff->span;
}

// No safety here. Assumptions are there is enough space to prevent overflowing, and 'amt' is > 0.
inline char *cbuff_alloc(CIRCLE_BUFF *cbuff, size_t amt) {
    cbuff->space_available -= amt;
    char *result = cbuff->raw_buff + cbuff->write_offset;
    cbuff->write_offset = (cbuff->write_offset + amt) % cbuff->span;
    return result;
}

#endif //STI_NOSYS_PROT_ST_DBG_IP_ALLOCATOR_H_INCLUDED
