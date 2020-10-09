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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "opaevfio.h"

#define ERR(format, ...)                                \
fprintf(stderr, "%s:%u:%s() **ERROR** [%s] : " format , \
	__FILE__, __LINE__, __func__, strerror(errno), ##__VA_ARGS__)


STATIC int open_container_discover(struct opae_vfio_container *c)
{
	int res = 0;
	struct vfio_iommu_type1_info iommu_info;
	uint8_t *info_buf;
	struct vfio_iommu_type1_info *info_ptr;
	struct vfio_info_cap_header *hdr;
	struct opae_vfio_iova_range **range_ptr = &c->cont_ranges;

	memset(&iommu_info, 0, sizeof(iommu_info));
	iommu_info.argsz = sizeof(iommu_info);

	if (ioctl(c->cont_fd, VFIO_IOMMU_GET_INFO, &iommu_info)) {
		ERR("ioctl( , VFIO_IOMMU_GET_INFO, &iommu_info\n");
		return 1;
	}

	// Now that we have the size, create a temp buffer large enough..
	info_buf = calloc(1, iommu_info.argsz);
	if (!info_buf) {
		ERR("calloc(1, iommu_info.argsz)\n");
		return 2;
	}

	info_ptr = (struct vfio_iommu_type1_info *)info_buf;
	info_ptr->argsz = iommu_info.argsz;

	if (ioctl(c->cont_fd, VFIO_IOMMU_GET_INFO, info_ptr)) {
		ERR("ioctl( , VFIO_IOMMU_GET_INFO, info_ptr)\n");
		res = 3;
		goto out_free;
	}

	hdr = (struct vfio_info_cap_header *) (info_buf + info_ptr->cap_offset);

	while (1) {
		if (hdr->id == VFIO_IOMMU_TYPE1_INFO_CAP_IOVA_RANGE) {
			struct vfio_iommu_type1_info_cap_iova_range *io_range =
				(struct vfio_iommu_type1_info_cap_iova_range *) hdr;
			uint32_t i;

			for (i = 0 ; i < io_range->nr_iovas ; ++i) {
				*range_ptr = calloc(1, sizeof(struct opae_vfio_iova_range));
				if (!*range_ptr) {
					ERR("calloc()\n");
					res = 4;
					goto out_destroy_ranges;
				}

				(*range_ptr)->start = io_range->iova_ranges[i].start;
				(*range_ptr)->end = io_range->iova_ranges[i].end;
				(*range_ptr)->next_ptr = (*range_ptr)->start;

				range_ptr = &(*range_ptr)->next;
			}
		}

		if (!hdr->next)
			break;

		hdr = (struct vfio_info_cap_header *) (info_buf + hdr->next);
	}

	goto out_free;

out_destroy_ranges:
	range_ptr = &c->cont_ranges;
	while (*range_ptr) {
		struct opae_vfio_iova_range *trash = *range_ptr;
		range_ptr = &(*range_ptr)->next;
		free(trash);
	}

out_free:
	free(info_buf);
	return res;
}

int opae_vfio_open_container(struct opae_vfio_container *c)
{
	int res = 0;
	pthread_mutexattr_t mattr;

	memset(c, 0, sizeof(*c));

	if (pthread_mutexattr_init(&mattr)) {
		ERR("pthread_mutexattr_init()\n");
		return 1;
	}

	if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE)) {
		ERR("pthread_mutexattr_settype()\n");
		res = 2;
		goto out_destroy_attr;
	}

	if (pthread_mutex_init(&c->lock, &mattr)) {
		ERR("pthread_mutex_init()\n");
		res = 3;
		goto out_destroy_attr;
	}

	c->cont_fd = open("/dev/vfio/vfio", O_RDWR);
	if (c->cont_fd < 0) {
		ERR("open(\"/dev/vfio/vfio\")\n");
		res = 4;
		goto out_destroy_attr;
	}

	if (ioctl(c->cont_fd, VFIO_GET_API_VERSION) != VFIO_API_VERSION) {
		ERR("ioctl( , VFIO_GET_API_VERSION)\n");
		res = 5;
		goto out_close;
	}

	if (!ioctl(c->cont_fd, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)) {
		ERR("ioctl( , VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)\n");
		res = 6;
		goto out_close;
	}

	res = open_container_discover(c);
	if (res)
		goto out_close;

	goto out_destroy_attr;

out_close:
	close(c->cont_fd);
	c->cont_fd = -1;

out_destroy_attr:
	if (pthread_mutexattr_destroy(&mattr)) {
		ERR("pthread_mutexattr_destroy()\n");
		res = 9;
	}

	return res;
}

