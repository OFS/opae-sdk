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
#ifndef _OPAE_VFIO_PLUGIN_H
#define _OPAE_VFIO_PLUGIN_H
#include <opae/vfio.h>
#include <opae/fpga.h>

#define GUIDSTR_MAX 36

#ifdef __GNUC__
#define GCC_VERSION \
    (__GNUC__*10000 + __GNUC_MINOR__*100 + __GNUC_PATCHLEVEL__)
#else
#define GCC_VERSION 0
#endif

typedef union _bdf {
	struct {
		uint16_t segment;
		uint8_t bus;
		uint8_t device : 5;
		uint8_t function : 3;
	};
	uint32_t bdf;
} bdf_t;
struct _vfio_token;

#define PCIADDR_MAX 16
#define INVALID_NUMA_NODE 0xffffffff
typedef struct _vfio_pci_device {
	char addr[PCIADDR_MAX];
	bdf_t bdf;
	uint32_t vendor;
	uint32_t device;
	uint32_t numa_node;
	uint16_t subsystem_vendor;
	uint16_t subsystem_device;
	struct _vfio_token *tokens;
	struct _vfio_pci_device *next;
} vfio_pci_device_t;

typedef struct _vfio_ops {
	fpga_result (*reset)(const vfio_pci_device_t *p, volatile uint8_t *mmio);
} vfio_ops;

#define USER_MMIO_MAX 32
typedef struct _vfio_token {
	fpga_token_header hdr; //< Must appear at offset 0!
	fpga_guid compat_id;
	vfio_pci_device_t *device;
	uint32_t region;
	uint32_t mmio_size;
	uint32_t user_mmio_count;
	uint32_t user_mmio[USER_MMIO_MAX];
	uint64_t bitstream_id;
	uint64_t bitstream_mdata;
	uint8_t num_ports;
	fpga_accelerator_state afu_state;
	uint32_t num_afu_irqs;
	struct _vfio_token *parent;
	struct _vfio_token *next;
	vfio_ops ops;
} vfio_token;


typedef struct _vfio_pair {
	fpga_guid secret;
	struct opae_vfio *device;
	struct opae_vfio *physfn;
} vfio_pair_t;

typedef struct _vfio_handle {
	uint32_t magic;
	vfio_token *token;
	vfio_pair_t *vfio_pair;
	volatile uint8_t *mmio_base;
	size_t mmio_size;
	pthread_mutex_t lock;
	int sva_fd;
	int pasid;
#define OPAE_FLAG_HAS_AVX512 (1u << 0)
#define OPAE_FLAG_SVA_FD_VALID (1u << 1)  // Indicates sva_fd file handle is valid
#define OPAE_FLAG_PASID_VALID (1u << 2)   // Indicates pasid is set
	uint32_t flags;
} vfio_handle;

typedef struct _vfio_event_handle {
	uint32_t magic;
	pthread_mutex_t lock;
	int fd;
	uint32_t flags;
} vfio_event_handle;

int vfio_pci_discover(const char *gpattern);
void vfio_free_device_list(void);
vfio_token *vfio_get_token(vfio_pci_device_t *dev,
			   uint32_t region,
			   fpga_objtype type);
fpga_result vfio_get_guid(uint64_t *h, fpga_guid guid);
#endif // _OPAE_VFIO_PLUGIN_H
