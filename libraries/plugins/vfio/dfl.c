// Copyright(c) 2020-2023, Intel Corporation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <time.h>

#include "opae_int.h"
#include "opae_vfio.h"
#include "dfl.h"

#define FME_PORTS 4
uint32_t fme_ports[FME_PORTS] = {0x38, 0x40, 0x48, 0x50};

STATIC fpga_result legacy_port_reset(const vfio_pci_device_t *dev,
				     volatile uint8_t *port_base)
{
	UNUSED_PARAM(dev);
	struct timespec ts = {.tv_sec = 0, .tv_nsec = 500};
	uint64_t cycles = 0;
	const uint64_t reset_timeout = 1000;
	port_control port_ctrl_reg;
	volatile uint64_t *port_ctrl_ptr =
		(volatile uint64_t *)(port_base + PORT_CONTROL);

	// Read CSR 0x38, Port Control register.
	port_ctrl_reg.data = read_csr64(port_ctrl_ptr);

	// Clear bit 0, port soft reset.
	port_ctrl_reg.bits.port_reset = 0;

	// Write the CSR back.
	write_csr64(port_ctrl_ptr, port_ctrl_reg.data);

	// Poll until hardware clears the reset ACK bit.
	port_ctrl_reg.data = read_csr64(port_ctrl_ptr);
	while (port_ctrl_reg.bits.port_reset_ack) {
		nanosleep(&ts, NULL);
		if (++cycles > reset_timeout) {
			return FPGA_EXCEPTION;
		}
		port_ctrl_reg.data = read_csr64(port_ctrl_ptr);
	}

	return FPGA_OK;
}

int walk_port(vfio_token *parent, uint32_t region, volatile uint8_t *mmio)
{
	vfio_token *port;
	volatile uint64_t *port_next_afu_ptr;
	port_next_afu port_next_afu_reg;
	volatile uint64_t *port_capability_ptr;
	port_capability port_capability_reg;
	volatile uint8_t *dfh_ptr;
	dfh dfh_reg;

	port = vfio_get_token(parent->device, region, FPGA_ACCELERATOR);
	if (!port) {
		OPAE_ERR("Failed to get port token");
		return -1;
	}

	port_next_afu_ptr = (volatile uint64_t *)(mmio + PORT_NEXT_AFU);
	port_next_afu_reg.data = read_csr64(port_next_afu_ptr);

	port_capability_ptr = (volatile uint64_t *)(mmio + PORT_CAPABILITY);
	port_capability_reg.data = read_csr64(port_capability_ptr);

	port->parent = parent;
	port->mmio_size = port_capability_reg.bits.mmio_size;
	port->user_mmio_count += 1;
	port->user_mmio[region] = port_next_afu_reg.bits.port_afu_dfh_offset;

	port->ops.reset = legacy_port_reset;
	// reset the port before attempting to read any afu registers
	if (port->ops.reset(port->device, mmio)) {
		OPAE_ERR("error resetting port");
	}
	vfio_get_guid(1 +
			(uint64_t *)(mmio +
				port_next_afu_reg.bits.port_afu_dfh_offset),
			port->hdr.guid);

	// look for stp and if found, add it to user_mmio offsets
	dfh_ptr = mmio;
	dfh_reg.data = read_csr64(dfh_ptr);
	while (dfh_ptr) {
		if (dfh_reg.bits.id == PORT_STP_ID) {
			port->user_mmio_count += 1;
			port->user_mmio[region + 1] = dfh_ptr - mmio;
			break;
		}
		if (!dfh_reg.bits.next || dfh_reg.bits.eol)
			dfh_ptr = NULL;
		else {
			dfh_ptr += dfh_reg.bits.next;
			dfh_reg.data = read_csr64(dfh_ptr);
		}
	}

	return 0;
}

int walk_fme(vfio_pci_device_t *dev, struct opae_vfio *v,
	     volatile uint8_t *mmio, int region)
{
	vfio_token *fme;
	fab_capability fab_capability_reg;
	volatile uint8_t *dfh_ptr;
	dfh dfh_reg;

	fme = vfio_get_token(dev, region, FPGA_DEVICE);
	if (!fme) {
		OPAE_ERR("Failed to get fme token");
		return -1;
	}

	vfio_get_guid(1 + (uint64_t *)mmio, fme->hdr.guid);
	fme->bitstream_id = read_csr64(mmio + BITSTREAM_ID);
	fme->bitstream_mdata = read_csr64(mmio + BITSTREAM_MD);

	fab_capability_reg.data = read_csr64(mmio + FAB_CAPABILITY);
	fme->num_ports = fab_capability_reg.bits.num_ports;

	// Find the PR feature and grab its guid. This will
	// be used as the FME guid.
	dfh_ptr = mmio;
	dfh_reg.data = read_csr64(dfh_ptr);
	while (dfh_ptr) {
		if (dfh_reg.bits.id == PR_FEATURE_ID) {
			vfio_get_guid((uint64_t *)(dfh_ptr + PR_INTFC_ID_LO),
					fme->compat_id);
			break;
		}
		if (!dfh_reg.bits.next || dfh_reg.bits.eol)
			dfh_ptr = NULL;
		else {
			dfh_ptr += dfh_reg.bits.next;
			dfh_reg.data = read_csr64(dfh_ptr);
		}
	}

	for (size_t i = 0; i < FME_PORTS; ++i) {
		int bar;
		uint8_t *port_mmio = NULL;
		size_t size = 0;
		volatile uint8_t *offset_ptr;
		port_offset port_offset_reg;

		offset_ptr = mmio + fme_ports[i];
		port_offset_reg.data = read_csr64(offset_ptr);

		if (!port_offset_reg.bits.implemented)
			continue;

		bar = port_offset_reg.bits.bar;
		if (opae_vfio_region_get(v, bar, &port_mmio, &size)) {
			OPAE_ERR("failed to get Port BAR %d", bar);
			continue;
		}

		walk_port(fme, bar, port_mmio + port_offset_reg.bits.offset);
	}

	return 0;
}
