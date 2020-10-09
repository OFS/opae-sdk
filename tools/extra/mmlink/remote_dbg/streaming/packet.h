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