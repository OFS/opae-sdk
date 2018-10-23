// Copyright(c) 2018, Intel Corporation
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

/**
 * \buffer_mgmt.c
 * \brief FPGA Streaming DMA buffer management including NUMA
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <opae/fpga.h>
#include <stddef.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <math.h>
#include <hwloc.h>
#include <safe_string/safe_string.h>
#include "fpga_dma_internal.h"
#include "fpga_dma.h"

#define ERR_PROPS -1
#define ERR_MEMBIND -2
#define ERR_CPUBIND -3

static pthread_mutex_t hwloc_mutex = PTHREAD_MUTEX_INITIALIZER;

// Bind the calling thread to the NUMA node of the device
int setNUMABindings(fpga_handle fpga_h)
{
	fpga_result res = FPGA_OK;
	int cres = 0;
#ifndef USE_ASE
	// Set up proper affinity
	unsigned dom = 0, bus = 0, dev = 0, func = 0;
	fpga_properties props;

	hwloc_topology_t topology = NULL;
#ifdef FPGA_DMA_DEBUG
	char str[4096];
#endif
	res = fpgaGetPropertiesFromHandle(fpga_h, &props);
	if (FPGA_OK != res) {
		return ERR_PROPS;
	}
	res += fpgaPropertiesGetSegment(props, (uint16_t *)&dom);
	res += fpgaPropertiesGetBus(props, (uint8_t *)&bus);
	res += fpgaPropertiesGetDevice(props, (uint8_t *)&dev);
	res += fpgaPropertiesGetFunction(props, (uint8_t *)&func);
	fpgaDestroyProperties(&props);
	if (FPGA_OK != res) {
		return ERR_PROPS;
	}

	// Find the device from the topology
	hwloc_topology_init(&topology);
	hwloc_topology_set_flags(topology, HWLOC_TOPOLOGY_FLAG_IO_DEVICES);
	hwloc_topology_load(topology);
	hwloc_obj_t obj =
		hwloc_get_pcidev_by_busid(topology, dom, bus, dev, func);
	hwloc_obj_t obj2 = hwloc_get_non_io_ancestor_obj(topology, obj);
#ifdef FPGA_DMA_DEBUG
	hwloc_obj_type_snprintf(str, 4096, obj2, 1);
	printf("%s\n", str);
	hwloc_obj_attr_snprintf(str, 4096, obj2, " :: ", 1);
	printf("%s\n", str);
	hwloc_bitmap_taskset_snprintf(str, 4096, obj2->cpuset);
	printf("CPUSET is %s\n", str);
	hwloc_bitmap_taskset_snprintf(str, 4096, obj2->nodeset);
	printf("NODESET is %s\n", str);
#endif

#if HWLOC_API_VERSION > 0x00020000
	if (hwloc_set_membind(topology, obj2->nodeset, HWLOC_MEMBIND_THREAD,
			      HWLOC_MEMBIND_MIGRATE
				      | HWLOC_MEMBIND_BYNODESET)) {
		return ERR_MEMBIND;
	}
#else
	if (hwloc_set_membind_nodeset(topology, obj2->nodeset,
				      HWLOC_MEMBIND_THREAD,
				      HWLOC_MEMBIND_MIGRATE)) {
		return ERR_MEMBIND;
	}
#endif

	if (hwloc_set_cpubind(topology, obj2->cpuset, HWLOC_CPUBIND_STRICT)) {
		return ERR_CPUBIND;
	}
#endif

	return cres;
}

fpga_result fpgaDMAAllocateAndPinBuffers(fpga_dma_handle_t *dma_h)
{
	fpga_result res = FPGA_OK;
	int retval = 0;

	pthread_mutex_lock(&hwloc_mutex);

	retval = setNUMABindings(dma_h->main_header.fpga_h);

	if (retval == ERR_PROPS) {
		return FPGA_INVALID_PARAM;
	}

	pthread_mutex_unlock(&hwloc_mutex);

	return res;
}

