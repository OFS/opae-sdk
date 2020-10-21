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

#include "packet.h"

static unsigned char guardband_private[SIZEOF_PACKET_GUARDBAND] = { 0xDE, 0xAD, 0xBE, 0xEF };
const unsigned char *const PACKET_GUARDBAND = guardband_private;

void populate_guardband(unsigned char *bytes) {
    int i;
    for (i = 0; i < SIZEOF_PACKET_GUARDBAND; ++i) {
        bytes[i] = PACKET_GUARDBAND[i];
    }
}

void populate_h2t_packet_header
(
    H2T_PACKET_HEADER *packet,
    unsigned char sop,
    unsigned char eop,
    unsigned char conn_id,
    unsigned short channel,
    unsigned short data_len_bytes
) {
    packet->SOP_EOP = (sop & 0x01) | ((eop & 0x01) << 1);
    packet->CONN_ID = conn_id;
    packet->CHANNEL = channel;
    packet->DATA_LEN_BYTES = data_len_bytes;
}

void populate_h2t_packet_header_bytes
(
    unsigned char *bytes,
    unsigned char sop,
    unsigned char eop,
    unsigned char conn_id,
    unsigned short channel,
    unsigned short data_len_bytes
) {
    bytes[0] = (sop & 0x01) | ((eop & 0x01) << 1);
    bytes[1] = conn_id;
    bytes[2] = (unsigned char)channel;
    bytes[3] = (channel & 0xFF00) >> 8;
    bytes[4] = (unsigned char)data_len_bytes;
    bytes[5] = (data_len_bytes & 0xFF00) >> 8;
}

void populate_h2t_packet_bytes
(
    unsigned char *bytes,
    unsigned char sop,
    unsigned char eop,
    unsigned char conn_id,
    unsigned short channel,
    unsigned short data_len_bytes
) {
    populate_guardband(bytes);
    populate_h2t_packet_header_bytes(bytes + SIZEOF_PACKET_GUARDBAND, sop, eop, conn_id, channel, data_len_bytes);
}

void populate_mgmt_packet_header
(
    MGMT_PACKET_HEADER *packet,
    unsigned char sop,
    unsigned char eop,
    unsigned short channel,
    unsigned short data_len_bytes
) {
    packet->SOP_EOP = (sop & 0x01) | ((eop & 0x01) << 1);
    packet->CHANNEL = channel;
    packet->DATA_LEN_BYTES = data_len_bytes;
}

void populate_mgmt_packet_header_bytes
(
    unsigned char *bytes,
    unsigned char sop,
    unsigned char eop,
    unsigned short channel,
    unsigned short data_len_bytes
) {
    bytes[0] = (sop & 0x01) | ((eop & 0x01) << 1);
    bytes[2] = (unsigned char)channel;
    bytes[3] = (channel & 0xFF00) >> 8;
    bytes[4] = (unsigned char)data_len_bytes;
    bytes[5] = (data_len_bytes & 0xFF00) >> 8;
}

void populate_mgmt_packet_bytes
(
    unsigned char *bytes,
    unsigned char sop,
    unsigned char eop,
    unsigned short channel,
    unsigned short data_len_bytes
) {
    populate_guardband(bytes);
    populate_mgmt_packet_header_bytes(bytes + SIZEOF_PACKET_GUARDBAND, sop, eop, channel, data_len_bytes);
}

#ifdef STI_DEBUG
void dump_mgmt_packet_header(MGMT_PACKET_HEADER *packet) {
    printf("MGMT Packet header (size = %d): \n", SIZEOF_MGMT_PACKET_HEADER);
    printf("\tSOP: 0x%01hhu\n", packet->SOP_EOP & MGMT_PACKET_HEADER_MASK_SOP);
    printf("\tEOP: 0x%01hhu\n", (packet->SOP_EOP & MGMT_PACKET_HEADER_MASK_EOP) ? 1 : 0);
    printf("\tCHANNEL: 0x%03hx\n", packet->CHANNEL);
    printf("\tDATA_LEN_BYTES: 0x%04hx\n", packet->DATA_LEN_BYTES);    
}

void dump_h2t_packet_header(H2T_PACKET_HEADER *packet) {
    printf("H2T Packet header (size = %d): \n", SIZEOF_H2T_PACKET_HEADER);
    printf("\tSOP: 0x%01hhu\n", packet->SOP_EOP & H2T_PACKET_HEADER_MASK_SOP);
    printf("\tEOP: 0x%01hhu\n", (packet->SOP_EOP & H2T_PACKET_HEADER_MASK_EOP) ? 1 : 0);
    printf("\tCONN_ID: 0x%02hx\n", packet->CONN_ID);
    printf("\tCHANNEL: 0x%03hx\n", packet->CHANNEL);
    printf("\tDATA_LEN_BYTES: 0x%03hx\n", packet->DATA_LEN_BYTES);
}

void dump_bytes(char *bytes, size_t len) {
    const int COL_WIDTH = 16;
    size_t i;
    for (i = 0; i < len; ++i) {
        if ((i % COL_WIDTH == 0) && (i > 0)) {
            printf("\n");
        }
        printf("0x%02hhx ", bytes[i]);
    }
    printf("\n");
}
#endif
