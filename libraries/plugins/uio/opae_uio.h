// Copyright(c) 2023, Intel Corporation
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
#ifndef _OPAE_UIO_PLUGIN_H
#define _OPAE_UIO_PLUGIN_H
#include <opae/uio.h>
#include <opae/fpga.h>

#ifndef __FILENAME__
#define SLASHPTR strrchr(__FILE__, '/')
#define __FILENAME__ (SLASHPTR ? SLASHPTR+1 : __FILE__)
#endif

#ifdef LIBOPAE_DEBUG
#define ERR(format, ...)                                                       \
	fprintf(stderr, "%s:%u:%s() [ERROR][%s] : " format,                    \
	__FILENAME__, __LINE__, __func__, strerror(errno), ##__VA_ARGS__)
#else
#define ERR(format, ...) do { } while (0)
#endif

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
struct _uio_token;

#define PCIADDR_MAX 16
// dfl_dev.xyz
#define DFL_DEV_MAX 12
typedef struct _uio_pci_device {
	char addr[PCIADDR_MAX];
	char dfl_dev[DFL_DEV_MAX];
	uint64_t object_id;
	bdf_t bdf;
	uint32_t vendor;
	uint32_t device;
	uint32_t numa_node;
	uint16_t subsystem_vendor;
	uint16_t subsystem_device;
	struct _uio_token *tokens;
	struct _uio_pci_device *next;
} uio_pci_device_t;

typedef struct _uio_ops {
	fpga_result (*reset)(const uio_pci_device_t *p, volatile uint8_t *mmio);
} uio_ops;

#define USER_MMIO_MAX 8
typedef struct _uio_token {
	fpga_token_header hdr; //< Must appear at offset 0!
	fpga_guid compat_id;
	uio_pci_device_t *device;
	uint32_t region;
	uint32_t offset; // ?
	uint32_t mmio_size;
	uint32_t user_mmio_count;
	uint32_t user_mmio[USER_MMIO_MAX];
	uint64_t bitstream_id;
	uint64_t bitstream_mdata;
	uint8_t num_ports;
	fpga_accelerator_state afu_state;
	uint32_t num_afu_irqs;
	struct _uio_token *parent;
	struct _uio_token *next;
	uio_ops ops;
} uio_token;

typedef struct _uio_handle {
	uint32_t magic;
	uio_token *token;
	struct opae_uio uio;
	volatile uint8_t *mmio_base;
	size_t mmio_size;
	pthread_mutex_t lock;
#define OPAE_FLAG_HAS_AVX512 (1u << 0)
	uint32_t flags;
} uio_handle;

typedef struct _uio_event_handle {
	uint32_t magic;
	pthread_mutex_t lock;
	int fd;
	uint32_t flags;
} uio_event_handle;

int uio_pci_discover(void);
void uio_free_device_list(void);
uio_token *uio_get_token(uio_pci_device_t *dev,
			 uint32_t region,
			 fpga_objtype objtype);
fpga_result uio_get_guid(uint64_t *h, fpga_guid guid);
#endif // _OPAE_UIO_PLUGIN_H
