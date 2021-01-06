// Copyright(c) 2020-2021, Intel Corporation
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
#include <sys/mman.h>
#include <regex.h>

#include <opae/vfio.h>

#define __SHORT_FILE__                                    \
({                                                        \
	const char *file = __FILE__;                      \
	const char *p = file;                             \
	while (*p)                                        \
		++p;                                      \
	while ((p > file) && ('/' != *p) && ('\\' != *p)) \
		--p;                                      \
	if (p > file)                                     \
		++p;                                      \
	p;                                                \
})

#define ERR(format, ...)                                \
fprintf(stderr, "%s:%u:%s() **ERROR** [%s] : " format , \
	__SHORT_FILE__, __LINE__, __func__, strerror(errno), ##__VA_ARGS__)

STATIC struct opae_vfio_sparse_info *
opae_vfio_create_sparse_info(uint32_t index, uint32_t offset, uint32_t size)
{
	struct opae_vfio_sparse_info *p = malloc(sizeof(*p));
	if (p) {
		p->next = NULL;
		p->index = index;
		p->offset = offset;
		p->size = size;
		p->ptr = MAP_FAILED;
	}
	return p;
}

STATIC void opae_vfio_destroy_sparse_info(struct opae_vfio_sparse_info *s)
{
	while (s) {
		struct opae_vfio_sparse_info *trash = s;
		s = s->next;
		if (trash->ptr != MAP_FAILED) {
			if (munmap(trash->ptr, trash->size) < 0)
				ERR("munmap failed\n");
		}
		free(trash);
	}
}

STATIC struct opae_vfio_device_region *
opae_vfio_map_sparse_device_region(uint32_t index,
				   int fd,
				   uint64_t offset,
				   size_t sz,
				   struct opae_vfio_sparse_info *slist)
{
	struct opae_vfio_device_region *r = malloc(sizeof(*r));
	if (r) {
		r->next = NULL;
		r->region_index = index;
		r->region_ptr = mmap(0, sz, PROT_READ|PROT_WRITE,
				     MAP_ANONYMOUS|MAP_PRIVATE,
				     -1, 0);
		if (r->region_ptr == MAP_FAILED) {
			free(r);
			return NULL;
		}
		r->region_size = sz;
		r->region_sparse = slist;

		while (slist) {
			slist->ptr = mmap(r->region_ptr + slist->offset,
				    slist->size, PROT_READ|PROT_WRITE,
				    MAP_FIXED|MAP_SHARED, fd,
				    offset + slist->offset);
			if (slist->ptr == MAP_FAILED)
				ERR("mmap failed\n");
			slist = slist->next;
		}
	}
	return r;
}

STATIC struct opae_vfio_device_region *
opae_vfio_map_device_region(uint32_t index,
			    int fd,
			    uint64_t offset,
			    size_t sz)
{
	struct opae_vfio_device_region *r = malloc(sizeof(*r));
	if (r) {
		r->next = NULL;
		r->region_index = index;
		r->region_ptr = mmap(0, sz, PROT_READ|PROT_WRITE,
				     MAP_SHARED, fd, (off_t)offset);
		r->region_size = sz;
		r->region_sparse = NULL;
		if (r->region_ptr == MAP_FAILED) {
			free(r);
			return NULL;
		}
	}
	return r;
}

STATIC void
opae_vfio_destroy_device_region(struct opae_vfio_device_region *r)
{
	while (r) {
		struct opae_vfio_device_region *trash = r;
		r = r->next;
		if (munmap(trash->region_ptr, trash->region_size) < 0)
			ERR("munmap failed\n");
		opae_vfio_destroy_sparse_info(trash->region_sparse);
		free(trash);
	}
}

STATIC struct opae_vfio_sparse_info *
opae_vfio_device_get_sparse_info(struct opae_vfio_device *d,
				 struct vfio_region_info *rinfo)
{
	struct opae_vfio_sparse_info *sparse_list = NULL;
	struct opae_vfio_sparse_info **slist = &sparse_list;
	uint8_t *buffer;
	struct vfio_region_info *rinfo_ptr;
	struct vfio_info_cap_header *hdr;
	struct vfio_region_info_cap_sparse_mmap *sparse_mmap;
	uint32_t i;

	if (!(rinfo->flags & VFIO_REGION_INFO_FLAG_CAPS))
		return NULL;

	buffer = malloc(rinfo->argsz);
	if (!buffer) {
		ERR("malloc failed\n");
		return NULL;
	}

	memcpy(buffer, rinfo, sizeof(*rinfo));
	rinfo_ptr = (struct vfio_region_info *) buffer;

	if (ioctl(d->device_fd, VFIO_DEVICE_GET_REGION_INFO, buffer)) {
		ERR("ioctl(%d, VFIO_DEVICE_GET_REGION_INFO, buffer)\n",
		    d->device_fd);
		goto out_free_buffer;
	}

	hdr = (struct vfio_info_cap_header *)(buffer + rinfo_ptr->cap_offset);

	if (hdr->id != VFIO_REGION_INFO_CAP_SPARSE_MMAP)
		goto out_free_buffer;

	sparse_mmap = (struct vfio_region_info_cap_sparse_mmap *) hdr;

	for (i = 0 ; i < sparse_mmap->nr_areas ; ++i) {
		struct opae_vfio_sparse_info *node =
		opae_vfio_create_sparse_info(i,
					     sparse_mmap->areas[i].offset,
					     sparse_mmap->areas[i].size);
		if (!node) {
			ERR("malloc failed");
			goto out_free_buffer;
		}
		*slist = node;
		slist = &node->next;
	}

out_free_buffer:
	free(buffer);

	return sparse_list;
}

STATIC void opae_vfio_device_destroy(struct opae_vfio_device *d)
{
	opae_vfio_destroy_device_region(d->regions);
	d->regions = NULL;

	if (d->device_fd >= 0) {
		close(d->device_fd);
		d->device_fd = -1;
	}
}

STATIC int opae_vfio_device_init(struct opae_vfio_device *d,
				 int group_fd,
				 const char *pciaddr,
				 const char *token)
{
	struct vfio_region_info region_info;
	struct vfio_device_info device_info;
	uint32_t i;
	struct opae_vfio_device_region **rlist = &d->regions;
	char arg[256];

	if (token) {
		if (snprintf(arg, sizeof(arg),
			     "%s vf_token=%s", pciaddr, token) < 0) {
			ERR("snprintf failed\n");
			return 1;
		}
	} else {
		memcpy(arg, pciaddr, strlen(pciaddr) + 1);
	}

	d->device_fd = ioctl(group_fd, VFIO_GROUP_GET_DEVICE_FD, arg);
	if (d->device_fd < 0) {
		ERR("ioctl(%d, VFIO_GROUP_GET_DEVICE_FD, \"%s\")\n",
		    group_fd, pciaddr);
		return 2;
	}

	memset(&region_info, 0, sizeof(region_info));
	region_info.argsz = sizeof(region_info);
	region_info.index = VFIO_PCI_CONFIG_REGION_INDEX;

	if (ioctl(d->device_fd, VFIO_DEVICE_GET_REGION_INFO, &region_info)) {
		ERR("ioctl(%d, VFIO_DEVICE_GET_REGION_INFO, &region_info)\n",
		    d->device_fd);
		return 3;
	}

	d->device_config_offset = region_info.offset;

	memset(&device_info, 0, sizeof(device_info));
	device_info.argsz = sizeof(device_info);

	if (ioctl(d->device_fd, VFIO_DEVICE_GET_INFO, &device_info)) {
		ERR("ioctl(%d, VFIO_DEVICE_GET_INFO, &device_info)\n",
		    d->device_fd);
		return 4;
	}

	d->device_num_regions = device_info.num_regions;

	for (i = 0 ; i < d->device_num_regions ; ++i) {
		struct opae_vfio_sparse_info *sparse_list = NULL;

		memset(&region_info, 0, sizeof(region_info));
		region_info.argsz = sizeof(region_info);
		region_info.index = i;

		if (ioctl(d->device_fd,
			  VFIO_DEVICE_GET_REGION_INFO,
			  &region_info))
			continue;

		if (region_info.flags & VFIO_REGION_INFO_FLAG_MMAP) {
			struct opae_vfio_device_region *region = NULL;

			sparse_list = opae_vfio_device_get_sparse_info(
					d, &region_info);

			if (sparse_list) {
				// map_sparse
				region = opae_vfio_map_sparse_device_region(
						i, d->device_fd,
						region_info.offset,
						region_info.size,
						sparse_list);
			} else {
				// map
				region = opae_vfio_map_device_region(i,
					d->device_fd,
					region_info.offset,
					region_info.size);
			}

			if (region) {
				*rlist = region;
				rlist = &region->next;
			}
		}

	}

	return 0;
}

STATIC void opae_vfio_group_destroy(struct opae_vfio_group *g)
{
	if (g->group_fd >= 0) {
		if (ioctl(g->group_fd, VFIO_GROUP_UNSET_CONTAINER)) {
			ERR("ioctl(%d, VFIO_GROUP_UNSET_CONTAINER)\n",
			    g->group_fd);
		}
		close(g->group_fd);
		g->group_fd = -1;
	}

	if (g->group_device) {
		free(g->group_device);
		g->group_device = NULL;
	}
}

STATIC int opae_vfio_group_init(struct opae_vfio_group *g,
				char *device)
{
	struct vfio_group_status group_status;
	int res = 0;

	g->group_device = device;

	if (!g->group_device) {
		ERR("failed to find iommu group\n");
		res = 1;
		goto out_destroy;
	}

	g->group_fd = open(g->group_device, O_RDWR);
	if (g->group_fd < 0) {
		ERR("open(\"%s\", O_RDWR)\n", g->group_device);
		res = 2;
		goto out_destroy;
	}

	memset(&group_status, 0, sizeof(group_status));
	group_status.argsz = sizeof(group_status);

	if (ioctl(g->group_fd, VFIO_GROUP_GET_STATUS, &group_status)) {
		ERR("ioctl(%d, VFIO_GROUP_GET_STATUS, &group_status)\n",
		    g->group_fd);
		res = 3;
		goto out_destroy;
	}

	if (!(group_status.flags & VFIO_GROUP_FLAGS_VIABLE)) {
		ERR("VFIO group not viable\n");
		res = 4;
		goto out_destroy;
	}

	return 0;

out_destroy:
	opae_vfio_group_destroy(g);
	return res;
}

STATIC void opae_vfio_destroy_iova_range(struct opae_vfio_iova_range *r)
{
	while (r) {
		struct opae_vfio_iova_range *trash = r;
		r = r->next;
		free(trash);
	}
}

STATIC void
opae_vfio_destroy_buffer(struct opae_vfio * , struct opae_vfio_buffer * );

STATIC void opae_vfio_destroy(struct opae_vfio *v)
{
	opae_vfio_device_destroy(&v->device);
	opae_vfio_group_destroy(&v->group);
	opae_vfio_destroy_iova_range(v->cont_ranges);
	v->cont_ranges = NULL;
	opae_vfio_destroy_buffer(v, v->cont_buffers);
	v->cont_buffers = NULL;

	mem_alloc_destroy(&v->iova_alloc);

	if (v->cont_fd >= 0) {
		close(v->cont_fd);
		v->cont_fd = -1;
	}

	if (v->cont_device) {
		free(v->cont_device);
		v->cont_device = NULL;
	}

	if (v->cont_pciaddr) {
		free(v->cont_pciaddr);
		v->cont_pciaddr = NULL;
	}

	if (pthread_mutex_unlock(&v->lock))
		ERR("pthread_mutex_unlock() failed\n");

	if (pthread_mutex_destroy(&v->lock))
		ERR("pthread_mutex_destroy() failed\n");
}

STATIC struct opae_vfio_iova_range *
opae_vfio_create_iova_range(uint64_t start, uint64_t end)
{
	struct opae_vfio_iova_range *r;
	r = malloc(sizeof(*r));
	if (r) {
		r->start = start;
		r->end = end;
		r->next = NULL;
	}
	return r;
}

STATIC struct opae_vfio_iova_range *
opae_vfio_iova_discover(struct opae_vfio *v)
{
	struct opae_vfio_iova_range *iova_list = NULL;
	struct opae_vfio_iova_range **ilist = &iova_list;
	struct vfio_iommu_type1_info iommu_info;
	uint8_t *info_buf;
	struct vfio_iommu_type1_info *info_ptr;
	struct vfio_info_cap_header *hdr;

	memset(&iommu_info, 0, sizeof(iommu_info));
	iommu_info.argsz = sizeof(iommu_info);

	if (ioctl(v->cont_fd, VFIO_IOMMU_GET_INFO, &iommu_info)) {
		ERR("ioctl(%d, VFIO_IOMMU_GET_INFO, &iommu_info\n",
		    v->cont_fd);
		return NULL;
	}

	info_buf = calloc(1, iommu_info.argsz);
	if (!info_buf) {
		ERR("calloc(1, iommu_info.argsz)\n");
		return NULL;
	}

	info_ptr = (struct vfio_iommu_type1_info *) info_buf;
	info_ptr->argsz = iommu_info.argsz;

	if (ioctl(v->cont_fd, VFIO_IOMMU_GET_INFO, info_ptr)) {
		ERR("ioctl(%d, VFIO_IOMMU_GET_INFO, info_ptr)\n",
		    v->cont_fd);
		goto out_free;
	}

	hdr = (struct vfio_info_cap_header *) (info_buf + info_ptr->cap_offset);

	while (1) {
		if (hdr->id == VFIO_IOMMU_TYPE1_INFO_CAP_IOVA_RANGE) {
			struct vfio_iommu_type1_info_cap_iova_range *io_range =
				(struct vfio_iommu_type1_info_cap_iova_range *) hdr;
			uint32_t i;

			for (i = 0 ; i < io_range->nr_iovas ; ++i) {
				struct opae_vfio_iova_range *node;

				node = opae_vfio_create_iova_range(
						io_range->iova_ranges[i].start,
						io_range->iova_ranges[i].end);

				if (node) {
					*ilist = node;
					ilist = &node->next;

					if (mem_alloc_add_free(&v->iova_alloc,
							       node->start,
							       node->end + 1 -
							       node->start)) {
						ERR("mem_alloc_add_free()");
					}
				}
			}
		}

		if (!hdr->next)
			break;

		hdr = (struct vfio_info_cap_header *) (info_buf + hdr->next);
	}

out_free:
	free(info_buf);
	return iova_list;
}

STATIC int opae_vfio_iova_reserve(struct opae_vfio *v,
				  uint64_t *size,
				  uint64_t *iova)
{
	uint64_t page_size;

	page_size = sysconf(_SC_PAGE_SIZE);
	*size = page_size + ((*size - 1) & ~(page_size - 1));

	return mem_alloc_get(&v->iova_alloc,
			     iova,
			     *size);
}

STATIC struct opae_vfio_buffer *
opae_vfio_create_buffer(uint8_t *vaddr,
			size_t size,
			uint64_t iova)
{
	struct opae_vfio_buffer *b;
	b = malloc(sizeof(*b));
	if (b) {
		b->buffer_ptr = vaddr;
		b->buffer_size = size;
		b->buffer_iova = iova;
		b->next = NULL;
	}
	return b;
}

STATIC void
opae_vfio_destroy_buffer(struct opae_vfio *v,
			 struct opae_vfio_buffer *b)
{
	struct vfio_iommu_type1_dma_unmap dma_unmap;

	while (b) {
		struct opae_vfio_buffer *trash = b;
		b = b->next;

		memset(&dma_unmap, 0, sizeof(dma_unmap));
		dma_unmap.argsz = sizeof(dma_unmap);
		dma_unmap.iova = trash->buffer_iova;
		dma_unmap.size = trash->buffer_size;

		if (ioctl(v->cont_fd, VFIO_IOMMU_UNMAP_DMA, &dma_unmap) < 0)
			ERR("ioctl(%d, VFIO_IOMMU_UNMAP_DMA, &dma_unmap)\n",
			    v->cont_fd);

		if (munmap(trash->buffer_ptr, trash->buffer_size) < 0)
			ERR("munmap(%p, %lu) failed\n",
			    trash->buffer_ptr, trash->buffer_size);

		if (mem_alloc_put(&v->iova_alloc, trash->buffer_iova))
			ERR("mem_alloc_put(..., 0x%lx) failed\n",
			    trash->buffer_iova);

		free(trash);
	}
}

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000
#endif
#ifndef MAP_HUGE_SHIFT
#define MAP_HUGE_SHIFT 26
#endif
#define MAP_2M_HUGEPAGE (0x15 << MAP_HUGE_SHIFT)
#define MAP_1G_HUGEPAGE (0x1e << MAP_HUGE_SHIFT)
#ifdef __ia64__
#define ADDR (void *)(0x8000000000000000UL)
#define FLAGS_4K (MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED)
#define FLAGS_2M (FLAGS_4K|MAP_2M_HUGEPAGE|MAP_HUGETLB)
#define FLAGS_1G (FLAGS_4K|MAP_1G_HUGEPAGE|MAP_HUGETLB)
#else
#define ADDR (void *)(0x0UL)
#define FLAGS_4K (MAP_PRIVATE|MAP_ANONYMOUS)
#define FLAGS_2M (FLAGS_4K|MAP_2M_HUGEPAGE|MAP_HUGETLB)
#define FLAGS_1G (FLAGS_4K|MAP_1G_HUGEPAGE|MAP_HUGETLB)
#endif

int opae_vfio_buffer_allocate(struct opae_vfio *v,
			      size_t *size,
			      uint8_t **buf,
			      uint64_t *iova)
{
	int res = 0;
	uint64_t ioaddr = 0;
	uint8_t *vaddr;
	struct vfio_iommu_type1_dma_map dma_map;
	struct vfio_iommu_type1_dma_unmap dma_unmap;
	struct opae_vfio_buffer *node;

	if (!v || !size) {
		ERR("NULL param\n");
		return 1;
	}

	if (!*size) {
		ERR("size must be > 0\n");
		return 2;
	}

	if (pthread_mutex_lock(&v->lock)) {
		ERR("pthread_mutex_lock() failed\n");
		return 3;
	}

	if (opae_vfio_iova_reserve(v, size, &ioaddr)) {
		pthread_mutex_unlock(&v->lock);
		return 4;
	}

	if (*size > (2 * 1024 * 1024))
		vaddr = mmap(ADDR, *size, PROT_READ|PROT_WRITE,
			     FLAGS_1G, 0, 0);
	else if (*size > 4096)
		vaddr = mmap(ADDR, *size, PROT_READ|PROT_WRITE,
			     FLAGS_2M, 0, 0);
	else
		vaddr = mmap(ADDR, *size, PROT_READ|PROT_WRITE,
			     FLAGS_4K, 0, 0);

	if (vaddr == MAP_FAILED) {
		ERR("mmap() failed\n");
		pthread_mutex_unlock(&v->lock);
		return 5;
	}

	memset(&dma_map, 0, sizeof(dma_map));

	dma_map.argsz = sizeof(dma_map);
	dma_map.vaddr = (uint64_t) vaddr;
	dma_map.size = *size;
	dma_map.iova = ioaddr;
	dma_map.flags = VFIO_DMA_MAP_FLAG_READ|VFIO_DMA_MAP_FLAG_WRITE;

	if (ioctl(v->cont_fd, VFIO_IOMMU_MAP_DMA, &dma_map) < 0) {
		ERR("ioctl(%d, VFIO_IOMMU_MAP_DMA, &dma_map)\n",
		    v->cont_fd);
		res = 5;
		goto out_munmap;
	}

	node = opae_vfio_create_buffer(vaddr, *size, ioaddr);
	if (!node) {
		ERR("malloc failed\n");
		res = 6;
		goto out_unmap_ioctl;
	}

	node->next = v->cont_buffers;
	v->cont_buffers = node;

	if (buf)
		*buf = vaddr;
	if (iova)
		*iova = ioaddr;

	if (pthread_mutex_unlock(&v->lock))
		ERR("pthread_mutex_unlock() failed\n");

	return 0;

out_unmap_ioctl:
	memset(&dma_unmap, 0, sizeof(dma_unmap));
	dma_unmap.argsz = sizeof(dma_unmap);
	dma_unmap.iova = ioaddr;
	dma_unmap.size = *size;
	ioctl(v->cont_fd, VFIO_IOMMU_UNMAP_DMA, &dma_unmap);
out_munmap:
	munmap(vaddr, *size);
	if (pthread_mutex_unlock(&v->lock))
		ERR("pthread_mutex_unlock() failed\n");
	return res;
}

int opae_vfio_buffer_free(struct opae_vfio *v,
			  uint8_t *buf)
{
	struct opae_vfio_buffer *b;
	struct opae_vfio_buffer *prev = NULL;
	int res = 0;

	if (!v) {
		ERR("NULL param\n");
		return 1;
	}

	if (pthread_mutex_lock(&v->lock)) {
		ERR("pthread_mutex_lock() failed\n");
		return 2;
	}

	for (b = v->cont_buffers ; b ; prev = b, b = b->next) {
		if (b->buffer_ptr == buf) {
			if (!prev) { // b == v->cont_buffers
				v->cont_buffers = b->next;
			} else {
				prev->next = b->next;
			}
			b->next = NULL;
			opae_vfio_destroy_buffer(v, b);
			goto out_unlock;
		}
	}

	res = 3;

out_unlock:
	if (pthread_mutex_unlock(&v->lock))
		ERR("pthread_mutex_unlock() failed\n");

	return res;
}

STATIC char * opae_vfio_group_for(const char *pciaddr)
{
	char path[256];
	char rlbuf[256];
	char *p;

	snprintf(path, sizeof(path),
		 "/sys/bus/pci/devices/%s/iommu_group", pciaddr);

	memset(rlbuf, 0, sizeof(rlbuf));
	if (readlink(path, rlbuf, sizeof(rlbuf)) < 0) {
		ERR("readlink() failed\n");
		return strdup("ERROR");
	}

	p = strrchr(rlbuf, '/');
	if (!p)
		return strdup("ERROR");

	snprintf(path, sizeof(path), "/dev/vfio/%s", p + 1);

	return strdup(path);
}

STATIC int opae_vfio_init(struct opae_vfio *v,
			  const char *pciaddr,
			  const char *token)
{
	int res = 0;
	pthread_mutexattr_t mattr;
	int cont_fd;

	memset(v, 0, sizeof(*v));
	v->cont_fd = -1;
	v->group.group_fd = -1;
	v->device.device_fd = -1;

	mem_alloc_init(&v->iova_alloc);

	if (pthread_mutexattr_init(&mattr)) {
		ERR("pthread_mutexattr_init()\n");
		return 1;
	}

	if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE)) {
		ERR("pthread_mutexattr_settype()\n");
		res = 2;
		goto out_destroy_attr;
	}

	if (pthread_mutex_init(&v->lock, &mattr)) {
		ERR("pthread_mutex_init()\n");
		res = 3;
		goto out_destroy_attr;
	}

	v->cont_device = strdup("/dev/vfio/vfio");
	v->cont_pciaddr = strdup(pciaddr);
	v->cont_fd = open(v->cont_device, O_RDWR);
	if (v->cont_fd < 0) {
		ERR("open(\"%s\")\n", v->cont_device);
		res = 4;
		goto out_destroy_container;
	}

	if (ioctl(v->cont_fd, VFIO_GET_API_VERSION) != VFIO_API_VERSION) {
		ERR("ioctl(%d, VFIO_GET_API_VERSION)\n", v->cont_fd);
		res = 5;
		goto out_destroy_container;
	}

	if (!ioctl(v->cont_fd, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)) {
		ERR("ioctl(%d, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)\n",
		    v->cont_fd);
		res = 6;
		goto out_destroy_container;
	}

	res = opae_vfio_group_init(&v->group, opae_vfio_group_for(pciaddr));
	if (res)
		goto out_destroy_container;

	cont_fd = v->cont_fd;
	if (ioctl(v->group.group_fd, VFIO_GROUP_SET_CONTAINER, &cont_fd)) {
		ERR("ioctl(%d, VFIO_GROUP_SET_CONTAINER, &cont_fd)\n",
		    v->group.group_fd);
		res = 7;
		goto out_destroy_container;
	}

	if (ioctl(v->cont_fd, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU) < 0) {
		ERR("ioctl(%d, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU)\n",
		    v->cont_fd);
		res = 8;
		goto out_destroy_container;
	}

	res = opae_vfio_device_init(&v->device,
				    v->group.group_fd,
				    pciaddr,
				    token);
	if (res)
		goto out_destroy_container;

	v->cont_ranges = opae_vfio_iova_discover(v);

	if (pthread_mutexattr_destroy(&mattr)) {
		ERR("pthread_mutexattr_destroy()\n");
		return 9;
	}

	return 0;

out_destroy_container:
	pthread_mutex_lock(&v->lock);
	opae_vfio_destroy(v);

out_destroy_attr:
	if (pthread_mutexattr_destroy(&mattr)) {
		ERR("pthread_mutexattr_destroy()\n");
		res = 10;
	}

	return res;
}

int opae_vfio_open(struct opae_vfio *v,
		   const char *pciaddr)
{
	if (!v || !pciaddr) {
		ERR("NULL param\n");
		return 1;
	}

	return opae_vfio_init(v, pciaddr, NULL);
}

#define GUID_RE_PATTERN "[0-9a-fA-F]{8}-" \
                        "[0-9a-fA-F]{4}-" \
                        "[0-9a-fA-F]{4}-" \
                        "[0-9a-fA-F]{4}-" \
                        "[0-9a-fA-F]{12}"

int opae_vfio_secure_open(struct opae_vfio *v,
			  const char *pciaddr,
			  const char *token)
{
	int reg_res;
	regex_t re;
	regmatch_t matches[2];

	if (!v || !pciaddr || !token) {
		ERR("NULL param\n");
		return 1;
	}

	memset(&matches, 0, sizeof(matches));
	reg_res = regcomp(&re, GUID_RE_PATTERN, REG_EXTENDED);

	if (reg_res) {
		ERR("compiling GUID regex\n");
		return 2;
	}

	reg_res = regexec(&re, token, 2, matches, 0);
	if (reg_res) {
		char err[128] = { 0, };
		regerror(reg_res, &re, err, sizeof(err));
		ERR("invalid GUID: %s [%s]\n", token, err);
		regfree(&re);
		return 3;
	}

	regfree(&re);
	return opae_vfio_init(v, pciaddr, token);
}

int opae_vfio_region_get(struct opae_vfio *v,
			 uint32_t index,
			 uint8_t **ptr,
			 size_t *size)
{
	struct opae_vfio_device_region *region;

	if (!v) {
		ERR("NULL param\n");
		return 1;
	}

	for (region = v->device.regions ; region ; region = region->next)
	{
		if (index == region->region_index) {
			if (ptr)
				*ptr = region->region_ptr;
			if (size)
				*size = region->region_size;
			return 0;
		}
	}

	return 2;
}

void opae_vfio_close(struct opae_vfio *v)
{
	if (!v) {
		ERR("NULL param\n");
		return;
	}

	if (pthread_mutex_lock(&v->lock)) {
		ERR("pthread_mutex_lock() failed\n");
		return;
	}

	opae_vfio_destroy(v);
}
