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
#include "opae_vfio.h"
#include "dfh.h"

#define FME_PORTS 4
uint32_t fme_ports[FME_PORTS] = {
	0x38,
	0x40,
	0x48,
	0x50
};

#define PORT_ERRORS 0x1010
static fpga_result legacy_port_errors_clear(vfio_token *t)
{
	volatile uint8_t *port_base = t->address;
	uint64_t value = *(uint64_t*)(port_base+PORT_ERRORS);
	*(uint64_t*)(port_base+PORT_ERRORS) = value;
	return FPGA_OK;
}

static fpga_result legacy_port_reset(vfio_token *t)
{
	volatile uint8_t *port_base = t->address;
	port_control *cntrl = (port_control*)(port_base + PORT_CONTROL);
	cntrl->port_reset = 1;
	(void)*cntrl;
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 500 };
	uint64_t cycles = 0;
	const uint64_t port_timeout = 1000;
	while(!cntrl->port_reset_ack) {
		(void)*cntrl;
		nanosleep(&ts, NULL);
		if (++cycles > port_timeout) {
			return FPGA_EXCEPTION;
		}
	}
	cntrl->port_reset = 0;
	(void)*cntrl;
	return FPGA_OK;
}



int walk_port(vfio_token *parent, uint32_t region, volatile uint8_t *mmio)
{
	//walk_port
	vfio_token *port = get_token(parent->device, region, mmio,
				     FPGA_ACCELERATOR);
	port_next_afu *afu = (port_next_afu*)(mmio+PORT_NEXT_AFU);
	port_capability *cap = (port_capability*)(mmio+PORT_CAPABILITY);
	port->parent = parent;
	port->user_mmio_count = 2;
	port->user_mmio[0].base = mmio + afu->port_afu_dfh_offset;
	port->user_mmio[0].size = cap->mmio_size;
	get_guid(1+(uint64_t*)port->user_mmio[0].base, port->guid);
	port->ops.reset = legacy_port_reset;
	port->ops.clear_errors = legacy_port_errors_clear;
	if (port->ops.reset(port)) {
		printf("error resetting port\n");
	}
	return 0;
}

static inline dfh *next_dfh(dfh* h)
{
	if (h && h->next && !h->eol)
		return (dfh*)(((uint8_t*)h)+h->next);
	return NULL;
}


int walk_fme(pci_device_t *p, volatile uint8_t *mmio, int region)
{
	vfio_token *fme = get_token(p, region, mmio, FPGA_DEVICE);
	get_guid(1+(uint64_t*)mmio, fme->guid);
	for(dfh *h = (dfh*)mmio; h; h = next_dfh(h)) {
		if (h->id == PR_FEATURE_ID) {
			printf("fme dfh: 0x%lx\n", *(uint64_t*)h);
			fme->pr_control = (volatile uint8_t*)h;
			uint8_t *pr_id = PR_INTFC_ID_LO+(uint8_t*)h;
			get_guid((uint64_t*)pr_id, fme->compat_id);
		}
	}
	for (size_t i = 0; i < FME_PORTS; ++i) {
		port_offset *offset_r = (port_offset*)(mmio+fme_ports[i]);
		if (!offset_r->implemented) continue;
		int bar = offset_r->bar;
		uint8_t *port_mmio;
		size_t size = 0;
		if (opae_vfio_region_get(&p->vfio_device, bar, &port_mmio, &size)) {
			printf("error getting port %lu\n", i);
			continue;
		}

		walk_port(fme, bar, port_mmio+offset_r->offset);
	}
	return 0;
}

