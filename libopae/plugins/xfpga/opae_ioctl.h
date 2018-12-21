// Copyright(c) 2017-2018, Intel Corporation
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
/*
 * opae_ioctl.h
 *
 */

#ifndef OPAE_IOCTL_H
#define OPAE_IOCTL_H
#include <stdint.h>

typedef struct _opae_fme_info {
	uint32_t flags;
	uint32_t capability;
} opae_fme_info;

typedef struct _opae_port_info {
	uint32_t flags;
	uint32_t capability;
	uint32_t num_regions;
	uint32_t num_umsgs;
	uint32_t num_uafu_irqs;
} opae_port_info;

typedef struct _opae_port_region_info {
	uint32_t flags;
	uint64_t size;
	uint64_t offset;
} opae_port_region_info;

int opae_get_fme_info(int fd, opae_fme_info *info);
int opae_get_port_info(int fd, opae_port_info *info);
int opae_get_port_region_info(int fd, uint32_t index, opae_port_region_info *info);

int opae_port_map(int fd, void *addr, uint64_t len, uint64_t *io_addr);
int opae_port_unmap(int fd, uint64_t io_addr);


#endif /* !OPAE_IOCTL_H */
