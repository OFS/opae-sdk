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
#ifndef _OPAE_VFIO_PLUGIN_H
#define _OPAE_VFIO_PLUGIN_H
#include <opae/vfio.h>
#include <opae/fpga.h>

#ifndef __FILENAME__
#define SLASHPTR strrchr(__FILE__, '/')
#define __FILENAME__ (SLASHPTR ? SLASHPTR+1 : __FILE__)
#endif

#define ERR(format, ...)                                                       \
	fprintf(stderr, "%s:%u:%s() [ERROR][%s] : " format,                    \
	__FILENAME__, __LINE__, __func__, strerror(errno), ##__VA_ARGS__)

#ifndef ASSERT_NOT_NULL_MSG_RESULT
#define ASSERT_NOT_NULL_MSG_RESULT(__arg, __msg, __res) \
  do {                                                  \
    if (!__arg) {                                       \
      OPAE_ERR(__msg);                                  \
      return __res;                                     \
    }                                                   \
  } while (0)
#endif

#ifndef ASSERT_NOT_NULL_MSG
#define ASSERT_NOT_NULL_MSG(__arg, __msg) \
  ASSERT_NOT_NULL_MSG_RESULT(__arg, __msg, FPGA_INVALID_PARAM)
#endif

#ifndef ASSERT_NOT_NULL
#define ASSERT_NOT_NULL(__arg) ASSERT_NOT_NULL_MSG(__arg, #__arg " is NULL")
#endif

typedef union _bdf
{
	struct
	{
		uint16_t segment;
		uint8_t bus;
		uint8_t device : 5;
		uint8_t function : 3;
	};
	uint32_t bdf;
} bdf_t;

#define PCIADDR_MAX 16
typedef struct _pci_device
{
	char addr[PCIADDR_MAX];
	bdf_t bdf;
	uint32_t vendor;
	uint32_t device;
	uint32_t numa_node;
	struct _pci_device *next;
} pci_device_t;

typedef struct _vfio_ops
{
	fpga_result (*reset)(const pci_device_t *p, volatile uint8_t *mmio);
} vfio_ops;

#define USER_MMIO_MAX 8
typedef struct _vfio_token
{
	uint32_t magic;
	fpga_guid guid;
	fpga_guid compat_id;
	const pci_device_t *device;
	uint32_t region;
	uint32_t offset;
	uint32_t mmio_size;
	uint32_t pr_control;
	uint32_t user_mmio_count;
	uint32_t user_mmio[USER_MMIO_MAX];
	uint64_t bitstream_id;
	uint64_t bitstream_mdata;
	uint8_t num_ports;
	uint32_t type;
	struct _vfio_token *parent;
	struct _vfio_token *next;
	vfio_ops ops;
} vfio_token;

typedef struct _vfio_handle
{
	uint32_t magic;
	struct _vfio_token *token;
	struct opae_vfio vfio_device;
	volatile uint8_t *mmio_base;
	size_t mmio_size;
  pthread_mutex_t lock;
} vfio_handle;


int pci_discover();
int features_discover();
pci_device_t *get_pci_device(char addr[PCIADDR_MAX]);
void free_device_list();
void free_token_list();
void free_buffer_list();
vfio_token *get_token(const pci_device_t *p, uint32_t region, int type);
fpga_result get_guid(uint64_t* h, fpga_guid guid);
#endif
