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
#ifndef STI_NOSYS_PROT_PACKET_H_INCLUDED
#define STI_NOSYS_PROT_PACKET_H_INCLUDED

#include "common.h"

#define SIZEOF_PACKET_GUARDBAND 4

#define SIZEOF_H2T_PACKET_HEADER ((unsigned char)sizeof(H2T_PACKET_HEADER))
#define H2T_PACKET_HEADER_MASK_SOP 0x01
#define H2T_PACKET_HEADER_MASK_EOP 0x02
#define H2T_PACKET_HEADER_MASK_CONN_ID 0xFF
#define H2T_PACKET_HEADER_MASK_CHANNEL 0x07FF
#define H2T_PACKET_HEADER_MASK_DATA_LEN_BYTES 0x1FFF
#define H2T_PACKET_MAX_PAYLOAD_BYTES 0x1000

#define SIZEOF_MGMT_PACKET_HEADER ((unsigned char)sizeof(MGMT_PACKET_HEADER))
#define MGMT_PACKET_HEADER_MASK_SOP 0x01
#define MGMT_PACKET_HEADER_MASK_EOP 0x02
#define MGMT_PACKET_HEADER_MASK_CHANNEL 0x07FF
#define MGMT_PACKET_HEADER_MASK_DATA_LEN_BYTES 0xFFFF
#define MGMT_PACKET_MAX_PAYLOAD_BYTES MGMT_PACKET_HEADER_MASK_DATA_LEN_BYTES

//#define DEBUG 1

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char *const PACKET_GUARDBAND;

typedef struct {
    unsigned char SOP_EOP;
    unsigned char CONN_ID;
    unsigned short CHANNEL;
    unsigned short DATA_LEN_BYTES;
} H2T_PACKET_HEADER;

typedef struct {
    unsigned char SOP_EOP;
    unsigned char RESERVED;
    unsigned short CHANNEL;
    unsigned short DATA_LEN_BYTES;
} MGMT_PACKET_HEADER;

void populate_guardband(unsigned char *bytes);

// H2T / T2H
void populate_h2t_packet_header
(
    H2T_PACKET_HEADER *packet,
    unsigned char sop,
    unsigned char eop,
    unsigned char conn_id,
    unsigned short channel,
    unsigned short data_len_bytes
);
void populate_h2t_packet_header_bytes
(
    unsigned char *bytes,
    unsigned char sop,
    unsigned char eop,
    unsigned char conn_id,
    unsigned short channel,
    unsigned short data_len_bytes
);
void populate_h2t_packet_bytes
(
    unsigned char *bytes,
    unsigned char sop,
    unsigned char eop,
    unsigned char conn_id,
    unsigned short channel,
    unsigned short data_len_bytes
);

// MGMT
void populate_mgmt_packet_header
(
    MGMT_PACKET_HEADER *packet,
    unsigned char sop,
    unsigned char eop,
    unsigned short channel,
    unsigned short data_len_bytes
);
void populate_mgmt_packet_header_bytes
(
    unsigned char *bytes,
    unsigned char sop,
    unsigned char eop,
    unsigned short channel,
    unsigned short data_len_bytes
);
void populate_mgmt_packet_bytes
(
    unsigned char *bytes,
    unsigned char sop,
    unsigned char eop,
    unsigned short channel,
    unsigned short data_len_bytes
);

// Debug stuff
#ifdef STI_DEBUG
void dump_h2t_packet_header(H2T_PACKET_HEADER *packet);
void dump_mgmt_packet_header(MGMT_PACKET_HEADER *packet);
void dump_bytes(char *bytes, size_t len);
#endif

#ifdef __cplusplus    
}
#endif

#endif //STI_NOSYS_PROT_PACKET_H_INCLUDED
