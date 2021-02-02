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
#ifndef VFIO_DFL_H
#define VFIO_DFL_H
#include <stdint.h>
#include "opae_vfio.h"

typedef struct _dfh {
	uint64_t id : 12;
	uint64_t major_rev : 4;
	uint64_t  next : 24;
	uint64_t eol : 1;
	uint64_t reserved41 : 7;
	uint64_t minor_rev : 4;
	uint64_t version : 8;
	uint64_t type : 4;
} dfh;

typedef struct _dfh0 {
	uint64_t cci_version : 12;
	uint64_t cci_minor_rev : 4;
	uint64_t  next : 24;
	uint64_t eol : 1;
	uint64_t reserved41 : 20;
	uint64_t type : 4;
} dfh0;

typedef struct _port_offset {
	uint64_t offset : 24;
	uint64_t reserved24 : 8;
	uint64_t bar : 3;
	uint64_t reserved35 : 20;
	uint64_t access : 1;
	uint64_t access_ctrl : 1;
	uint64_t reserved57 : 3;
	uint64_t implemented : 1;
	uint64_t reserved61 : 3;
} port_offset;

#define PR_FEATURE_ID 0x5
#define PR_INTFC_ID_LO 0xA8
#define PR_INTFC_ID_HI 0xB0
#define PORT_STP_ID 0x13
#define BITSTREAM_ID 0x60
typedef struct _bitstream_id {
	uint64_t git_hash : 32;
	uint64_t hssi_id : 4;
	uint64_t reserved36 : 12;
	uint64_t ver_debug : 4;
	uint64_t ver_patch : 4;
	uint64_t ver_minor : 4;
	uint64_t ver_major : 4;
} bitstream_id;

#define BITSTREAM_MD 0x68
#define PORT_CONTROL 0x38
typedef struct _port_control {
	uint64_t port_reset : 1;
	uint64_t reserved1 : 1;
	uint64_t latency_tolerance : 1;
	uint64_t flr_port_reset : 1;
	uint64_t port_reset_ack : 1;
	uint64_t reserved5 : 59;
} port_control;

#define PORT_NEXT_AFU 0x18
typedef struct _port_next_afu {
	uint64_t port_afu_dfh_offset : 24;
	uint64_t reserved24: 40;
} port_next_afu;

#define FAB_CAPABILITY 0x30
typedef struct _fab_capability {
	uint64_t fab_version : 8;
	uint64_t reserved9 : 4;
	uint64_t pcie0_link : 1;
	uint64_t reserved13 : 4;
	uint64_t num_ports : 3;
	uint64_t reserved20 : 4;
	uint64_t address_width : 6;
	uint64_t reserved30 : 34;
} fab_capability;

#define PORT_CAPABILITY 0x30
typedef struct _port_capability {
	uint64_t port_number : 2;
	uint64_t reserved2 : 6;
	uint64_t mmio_size : 16;
	uint64_t resered24 : 8;
	uint64_t num_supported_int : 4;
	uint64_t reserved36 : 28;
} port_capability;

typedef struct _dfl {
	dfh h;
	dfh *next;
} dfl;

#define for_each_dfh(H, ADDR) for (dfh *H = (dfh *)ADDR; H; H = next_dfh(H))
int walk_fme(pci_device_t *p, struct opae_vfio *v, volatile uint8_t *mmio, int region);
int walk_port(vfio_token *parent, uint32_t region, volatile uint8_t *mmio);

#endif /* !VFIO_DFL_H */
