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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "common.h"

void generate_expected_handle_message(char *buff, size_t buff_size, const char *sock_name, int handle) {
    snprintf(buff, buff_size, "%s HANDLE=%d", sock_name, handle);
}

int parse_handle_id(const char *buff) {
    const char *HANDLE_MSG = "HANDLE=";
    const size_t handle_msg_strlen = 7;
    char *pos = strstr(buff, HANDLE_MSG);
    if (pos != NULL) {
        return atoi(pos + handle_msg_strlen);
    }
    return -1;
}

void zero_mem(void *a, size_t length) {
    typedef size_t big_type;
    size_t big_size = sizeof(big_type);
    size_t leftover_offset = 0;

    if (length > big_size) {
        size_t fast_transfers = length / big_size;
        leftover_offset = fast_transfers * big_size;
        big_type *big_a = (big_type *)a;
        for (size_t i = 0; i < fast_transfers; ++i) {
            big_a[i] = 0;
        }

    }
    size_t leftover = length % big_size;
    for (size_t i = 0; i < leftover; ++i) {
        ((char *)a)[i + leftover_offset] = 0;
    }
}

void fill_mem(void *a, char c, size_t length) {
    typedef size_t big_type;
    size_t big_size = sizeof(big_type);
    size_t leftover_offset = 0;
    char stamp[32];
    for (unsigned char i = 0; i < big_size; ++i) {
        stamp[i] = c;
    }
    big_type big_stamp = *((big_type *)stamp);

    if (length > big_size) {
        size_t fast_transfers = length / big_size;
        leftover_offset = fast_transfers * big_size;
        big_type *big_a = (big_type *)a;
        for (size_t i = 0; i < fast_transfers; ++i) {
            big_a[i] = big_stamp;
        }

    }
    size_t leftover = length % big_size;
    for (size_t i = 0; i < leftover; ++i) {
        ((char *)a)[i + leftover_offset] = c;
    }
}

int get_random_id() {
    srand((unsigned int)clock() * (unsigned int)time(NULL));
    // We never want a 0 ID
    int result = 0;
    while (result == 0)
        result = rand();

    return result;
}
