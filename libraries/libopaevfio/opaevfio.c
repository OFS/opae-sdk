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
#include <linux/pci_regs.h>

#include <opae/vfio.h>
#include "mock/opae_std.h"

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

#ifdef LIBOPAE_DEBUG
#define ERR(format, ...)                               \
fprintf(stderr, "%s:%u:%s() **ERROR** [%s] : " format, \
	__SHORT_FILE__, __LINE__, __func__, strerror(errno), ##__VA_ARGS__)
#else
#define ERR(format, ...) do { } while (0)
#endif

STATIC struct opae_vfio_sparse_info *
opae_vfio_create_sparse_info(uint32_t index, uint32_t offset, uint32_t size)
{
	struct opae_vfio_sparse_info *p = opae_malloc(sizeof(*p));
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
		opae_free(trash);
	}
}

STATIC struct opae_vfio_device_region *
opae_vfio_map_sparse_device_region(uint32_t index,
				   int fd,
				   uint64_t offset,
				   size_t sz,
				   struct opae_vfio_sparse_info *slist)
{
	struct opae_vfio_device_region *r = opae_malloc(sizeof(*r));
	if (r) {
		r->next = NULL;
		r->region_index = index;
		r->region_ptr = mmap(0, sz, PROT_READ|PROT_WRITE,
				     MAP_ANONYMOUS|MAP_PRIVATE,
				     -1, 0);
		if (r->region_ptr == MAP_FAILED) {
			opae_free(r);
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
	struct opae_vfio_device_region *r = opae_malloc(sizeof(*r));
	if (r) {
		r->next = NULL;
		r->region_index = index;
		r->region_ptr = mmap(0, sz, PROT_READ|PROT_WRITE,
				     MAP_SHARED, fd, (off_t)offset);
		r->region_size = sz;
		r->region_sparse = NULL;
		if (r->region_ptr == MAP_FAILED) {
			opae_free(r);
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
		opae_free(trash);
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

	buffer = opae_malloc(rinfo->argsz);
	if (!buffer) {
		ERR("malloc failed\n");
		return NULL;
	}

	memcpy(buffer, rinfo, sizeof(*rinfo));
	rinfo_ptr = (struct vfio_region_info *) buffer;

	if (opae_ioctl(d->device_fd, VFIO_DEVICE_GET_REGION_INFO, buffer)) {
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
	opae_free(buffer);

	return sparse_list;
}

STATIC struct opae_vfio_device_irq *
opae_vfio_device_get_irq_info(uint32_t flags,
			      uint32_t index,
			      uint32_t count)
{
	struct opae_vfio_device_irq *irq = opae_malloc(sizeof(*irq));
	if (irq) {
		irq->flags = flags;
		irq->index = index;
		irq->count = count;
		irq->event_fds = NULL;
		irq->masks = NULL;
		irq->next = NULL;
	}
	return irq;
}

STATIC void
opae_vfio_destroy_device_irq(struct opae_vfio_device_irq *i)
{
	while (i) {
		struct opae_vfio_device_irq *trash = i;
		i = i->next;
		if (trash->event_fds)
			opae_free(trash->event_fds);
		if (trash->masks)
			opae_free(trash->masks);
		opae_free(trash);
	}
}

STATIC void opae_vfio_device_destroy(struct opae_vfio_device *d)
{
	opae_vfio_destroy_device_region(d->regions);
	d->regions = NULL;

	opae_vfio_destroy_device_irq(d->irqs);
	d->irqs = NULL;

	if (d->device_fd >= 0) {
		opae_close(d->device_fd);
		d->device_fd = -1;
	}
}

STATIC int setup_pci_command(int fd, size_t cfg_offset)
{
	uint16_t cmd = 0;
	ssize_t sz = sizeof(uint16_t);

	if (pread(fd, &cmd, sz, cfg_offset + PCI_COMMAND) == sz) {
		if (!(cmd & PCI_COMMAND_MEMORY) ||
		    !(cmd & PCI_COMMAND_MASTER)) {
			cmd |= (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
			if (pwrite(fd, &cmd, sz,
				   cfg_offset + PCI_COMMAND) != sz)
				return 1;
		}
	} else {
		return 2;
	}

	return 0;
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
	struct vfio_irq_info irq_info;
	struct opae_vfio_device_irq **ilist = &d->irqs;

	if (token) {
		if (snprintf(arg, sizeof(arg),
			     "%s vf_token=%s", pciaddr, token) < 0) {
			ERR("snprintf failed\n");
			return 1;
		}
	} else {
		memcpy(arg, pciaddr, strlen(pciaddr) + 1);
	}

	d->device_fd = opae_ioctl(group_fd, VFIO_GROUP_GET_DEVICE_FD, arg);
	if (d->device_fd < 0) {
		ERR("ioctl(%d, VFIO_GROUP_GET_DEVICE_FD, \"%s\")\n",
		    group_fd, pciaddr);
		return 2;
	}

	memset(&region_info, 0, sizeof(region_info));
	region_info.argsz = sizeof(region_info);
	region_info.index = VFIO_PCI_CONFIG_REGION_INDEX;

	if (opae_ioctl(d->device_fd, VFIO_DEVICE_GET_REGION_INFO, &region_info)) {
		ERR("ioctl(%d, VFIO_DEVICE_GET_REGION_INFO, &region_info)\n",
		    d->device_fd);
		return 3;
	}

	d->device_config_offset = region_info.offset;

	// setup pci config space command register
	// for bus master and mem enable (mmio)
	if (setup_pci_command(d->device_fd, region_info.offset)) {
		ERR("[%s] Could not read/write pci config cmd register", pciaddr);
		return 4;
	}

	memset(&device_info, 0, sizeof(device_info));
	device_info.argsz = sizeof(device_info);

	if (opae_ioctl(d->device_fd, VFIO_DEVICE_GET_INFO, &device_info)) {
		ERR("[%s] ioctl(%d, VFIO_DEVICE_GET_INFO, &device_info)\n",
		    pciaddr, d->device_fd);
		return 5;
	}

	d->device_num_regions = device_info.num_regions;
	d->device_num_irqs = device_info.num_irqs;

	for (i = 0 ; i < d->device_num_regions ; ++i) {
		struct opae_vfio_sparse_info *sparse_list = NULL;

		memset(&region_info, 0, sizeof(region_info));
		region_info.argsz = sizeof(region_info);
		region_info.index = i;

		if (opae_ioctl(d->device_fd,
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

	for (i = 0 ; i < d->device_num_irqs ; ++i) {
		struct opae_vfio_device_irq *irq = NULL;

		memset(&irq_info, 0, sizeof(irq_info));
		irq_info.argsz = sizeof(irq_info);
		irq_info.index = i;

		if (opae_ioctl(d->device_fd,
			  VFIO_DEVICE_GET_IRQ_INFO,
			  &irq_info))
			continue;

		irq = opae_vfio_device_get_irq_info(irq_info.flags,
						    irq_info.index,
						    irq_info.count);
		if (irq) {
			*ilist = irq;
			ilist = &irq->next;
		}
	}

	return 0;
}

STATIC void opae_vfio_group_destroy(struct opae_vfio_group *g)
{
	if (g->group_fd >= 0) {
		if (opae_ioctl(g->group_fd, VFIO_GROUP_UNSET_CONTAINER)) {
			ERR("ioctl(%d, VFIO_GROUP_UNSET_CONTAINER)\n",
			    g->group_fd);
		}
		opae_close(g->group_fd);
		g->group_fd = -1;
	}

	if (g->group_device) {
		opae_free(g->group_device);
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

	g->group_fd = opae_open(g->group_device, O_RDWR);
	if (g->group_fd < 0) {
		ERR("open(\"%s\", O_RDWR)\n", g->group_device);
		res = 2;
		goto out_destroy;
	}

	memset(&group_status, 0, sizeof(group_status));
	group_status.argsz = sizeof(group_status);

	if (opae_ioctl(g->group_fd, VFIO_GROUP_GET_STATUS, &group_status)) {
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
		opae_free(trash);
	}
}

STATIC void
opae_vfio_destroy_buffer(struct opae_vfio *, struct opae_vfio_buffer *);

STATIC void opae_vfio_destroy(struct opae_vfio *v)
{
	// destroy buffers before we close any FDs
	opae_hash_map_destroy(&v->cont_buffers);

	opae_vfio_device_destroy(&v->device);
	opae_vfio_group_destroy(&v->group);
	opae_vfio_destroy_iova_range(v->cont_ranges);
	v->cont_ranges = NULL;

	mem_alloc_destroy(&v->iova_alloc);

	if (v->cont_fd >= 0) {
		opae_close(v->cont_fd);
		v->cont_fd = -1;
	}

	if (v->cont_device) {
		opae_free(v->cont_device);
		v->cont_device = NULL;
	}

	if (v->cont_pciaddr) {
		opae_free(v->cont_pciaddr);
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
	r = opae_malloc(sizeof(*r));
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

	if (opae_ioctl(v->cont_fd, VFIO_IOMMU_GET_INFO, &iommu_info)) {
		ERR("ioctl(%d, VFIO_IOMMU_GET_INFO, &iommu_info\n",
		    v->cont_fd);
		return NULL;
	}

	info_buf = opae_calloc(1, iommu_info.argsz);
	if (!info_buf) {
		ERR("calloc(1, iommu_info.argsz)\n");
		return NULL;
	}

	info_ptr = (struct vfio_iommu_type1_info *) info_buf;
	info_ptr->argsz = iommu_info.argsz;

	if (opae_ioctl(v->cont_fd, VFIO_IOMMU_GET_INFO, info_ptr)) {
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
	opae_free(info_buf);
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
			uint64_t iova,
			int flags)
{
	struct opae_vfio_buffer *b;
	b = opae_malloc(sizeof(*b));
	if (b) {
		b->buffer_ptr = vaddr;
		b->buffer_size = size;
		b->buffer_iova = iova;
		b->flags = flags;
	}
	return b;
}

STATIC void
opae_vfio_destroy_buffer(struct opae_vfio *v,
			 struct opae_vfio_buffer *b)
{
	struct vfio_iommu_type1_dma_unmap dma_unmap;

	memset(&dma_unmap, 0, sizeof(dma_unmap));
	dma_unmap.argsz = sizeof(dma_unmap);
	dma_unmap.iova = b->buffer_iova;
	dma_unmap.size = b->buffer_size;

	if (opae_ioctl(v->cont_fd, VFIO_IOMMU_UNMAP_DMA, &dma_unmap) < 0)
		ERR("ioctl(%d, VFIO_IOMMU_UNMAP_DMA, &dma_unmap)\n",
		    v->cont_fd);

	if (!(b->flags & OPAE_VFIO_BUF_PREALLOCATED) &&
	    munmap(b->buffer_ptr, b->buffer_size) < 0)
		ERR("munmap(%p, %lu) failed\n",
		    b->buffer_ptr, b->buffer_size);

	if (mem_alloc_put(&v->iova_alloc, b->buffer_iova))
		ERR("mem_alloc_put(..., 0x%lx) failed\n",
		    b->buffer_iova);

	opae_free(b);
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
#define ADDR ((void *)(0x8000000000000000UL))
#define FLAGS_4K (MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED)
#define FLAGS_2M (FLAGS_4K|MAP_2M_HUGEPAGE|MAP_HUGETLB)
#define FLAGS_1G (FLAGS_4K|MAP_1G_HUGEPAGE|MAP_HUGETLB)
#else
#define ADDR ((void *)(0x0UL))
#define FLAGS_4K (MAP_PRIVATE|MAP_ANONYMOUS)
#define FLAGS_2M (FLAGS_4K|MAP_2M_HUGEPAGE|MAP_HUGETLB)
#define FLAGS_1G (FLAGS_4K|MAP_1G_HUGEPAGE|MAP_HUGETLB)
#endif

STATIC int
opae_vfio_buffer_mmap(struct opae_vfio *v,
		      size_t *size,
		      uint8_t **buf,
		      uint64_t *iova,
		      int flags,
		      struct opae_vfio_buffer **node)
{
	uint8_t *vaddr = NULL;
	uint64_t ioaddr = 0;
	int res;
	struct vfio_iommu_type1_dma_map dma_map;
	struct vfio_iommu_type1_dma_unmap dma_unmap;

	if (opae_vfio_iova_reserve(v, size, &ioaddr)) {
		return 1;
	}

	if (!(flags & OPAE_VFIO_BUF_PREALLOCATED)) {

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
			mem_alloc_put(&v->iova_alloc, ioaddr);
			return 2;
		}

	} else if (!buf || !*buf) {
		ERR("got OPAE_VFIO_BUF_PREALLOCATED, but buf is NULL.\n");
		mem_alloc_put(&v->iova_alloc, ioaddr);
		return 3;
	} else {
		vaddr = *buf;
	}

	memset(&dma_map, 0, sizeof(dma_map));

	dma_map.argsz = sizeof(dma_map);
	dma_map.vaddr = (uint64_t) vaddr;
	dma_map.size = *size;
	dma_map.iova = ioaddr;
	dma_map.flags = VFIO_DMA_MAP_FLAG_READ|VFIO_DMA_MAP_FLAG_WRITE;

	if (opae_ioctl(v->cont_fd, VFIO_IOMMU_MAP_DMA, &dma_map) < 0) {
		ERR("ioctl(%d, VFIO_IOMMU_MAP_DMA, &dma_map)\n", v->cont_fd);
		mem_alloc_put(&v->iova_alloc, ioaddr);
		res = 4;
		goto out_munmap;
	}

	*node = opae_vfio_create_buffer(vaddr, *size, ioaddr, flags);
	if (!*node) {
		ERR("malloc failed\n");
		mem_alloc_put(&v->iova_alloc, ioaddr);
		res = 5;
		goto out_unmap_ioctl;
	}

	if (!(flags & OPAE_VFIO_BUF_PREALLOCATED) && buf)
		*buf = vaddr;
	if (iova)
		*iova = ioaddr;

	return 0;

out_unmap_ioctl:
	memset(&dma_unmap, 0, sizeof(dma_unmap));
	dma_unmap.argsz = sizeof(dma_unmap);
	dma_unmap.iova = ioaddr;
	dma_unmap.size = *size;
	opae_ioctl(v->cont_fd, VFIO_IOMMU_UNMAP_DMA, &dma_unmap);
out_munmap:
	if (!(flags & OPAE_VFIO_BUF_PREALLOCATED))
		munmap(vaddr, *size);
	return res;
}

int opae_vfio_buffer_allocate_ex(struct opae_vfio *v,
				 size_t *size,
				 uint8_t **buf,
				 uint64_t *iova,
				 int flags)
{
	struct opae_vfio_buffer *node = NULL;
	int res = 0;

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

	if (opae_vfio_buffer_mmap(v,
				  size,
				  buf,
				  iova,
				  flags,
				  &node)) {
		if (pthread_mutex_unlock(&v->lock))
			ERR("pthread_mutex_unlock() failed\n");
		return 4;
	}

	if (opae_hash_map_add(&v->cont_buffers, *buf, node)) {
		ERR("opae_hash_map_add() failed\n");
		res = 5;
	}

	if (pthread_mutex_unlock(&v->lock))
		ERR("pthread_mutex_unlock() failed\n");

	return res;
}

struct opae_vfio_buffer *opae_vfio_buffer_info(struct opae_vfio *v,
					       uint8_t *vaddr)
{
	struct opae_vfio_buffer *binfo = NULL;

	if (!v || !vaddr) {
		ERR("NULL param\n");
		return NULL;
	}

	if (pthread_mutex_lock(&v->lock)) {
		ERR("pthread_mutex_lock() failed\n");
		return NULL;
	}

	if (opae_hash_map_find(&v->cont_buffers,
			       vaddr,
			       (void **)&binfo))
		ERR("opae_vfio_buffer_info() failed for key %p\n", vaddr);

	if (pthread_mutex_unlock(&v->lock))
		ERR("pthread_mutex_unlock() failed\n");

	return binfo;
}

int opae_vfio_buffer_allocate(struct opae_vfio *v,
			      size_t *size,
			      uint8_t **buf,
			      uint64_t *iova)
{
	return opae_vfio_buffer_allocate_ex(v,
					    size,
					    buf,
					    iova,
					    0);
}

int opae_vfio_buffer_free(struct opae_vfio *v,
			  uint8_t *buf)
{
	int res = 0;

	if (!v) {
		ERR("NULL param\n");
		return 1;
	}

	if (pthread_mutex_lock(&v->lock)) {
		ERR("pthread_mutex_lock() failed\n");
		return 2;
	}

	if (opae_hash_map_remove(&v->cont_buffers, buf)) {
		ERR("hash key %p not found\n", buf);
		res = 3;
	}

	if (pthread_mutex_unlock(&v->lock))
		ERR("pthread_mutex_unlock() failed\n");

	return res;
}

STATIC int
opae_vfio_device_set_irqs(struct opae_vfio *v,
			  uint32_t index,
			  uint32_t subindex,
			  int event_fd,
			  int flags)
{
	struct opae_vfio_device_irq *irq;
	struct vfio_irq_set *i;
	size_t sz;
	char *buf;
	int res;
	uint32_t u;

	for (irq = v->device.irqs ; irq ; irq = irq->next) {
		if ((irq->index != index) ||
		    !(irq->flags & VFIO_IRQ_INFO_EVENTFD))
			continue;

		if (subindex >= irq->count) {
			ERR("subindex %u is out of range 0-%u\n",
			    subindex, irq->count ? irq->count - 1 : irq->count);
			return 3;
		}

		break;
	}

	if (!irq)
		return 4;

	if (flags & VFIO_IRQ_SET_DATA_EVENTFD) {
		if (!irq->event_fds) {

			irq->event_fds =
				(int32_t *)opae_malloc(irq->count * sizeof(int32_t));
			if (!irq->event_fds) {
				ERR("malloc() failed\n");
				return 5;
			}

			for (u = 0 ; u < irq->count ; ++u)
				irq->event_fds[u] = -1;
		}

		irq->event_fds[subindex] = event_fd;
	} else if (flags & VFIO_IRQ_SET_DATA_BOOL) {
		if (!irq->masks) {

			irq->masks =
				(int32_t *)opae_malloc(irq->count * sizeof(int32_t));
			if (!irq->masks) {
				ERR("malloc() failed\n");
				return 6;
			}

			for (u = 0 ; u < irq->count ; ++u)
				irq->masks[u] = 0;
		}

		irq->masks[subindex] = event_fd;
	}

	sz = sizeof(*i) + (irq->count * sizeof(int32_t));
	buf = opae_malloc(sz);
	if (!buf) {
		ERR("malloc() failed\n");
		return 7;
	}

	i = (struct vfio_irq_set *)buf;
	i->argsz = sz;
	i->flags = flags;
	i->index = irq->index;
	i->start = 0;
	i->count = irq->count;

	if (flags & VFIO_IRQ_SET_DATA_EVENTFD)
		memcpy(&i->data, irq->event_fds, irq->count * sizeof(int32_t));
	else if (flags & VFIO_IRQ_SET_DATA_BOOL)
		memcpy(&i->data, irq->masks, irq->count * sizeof(int32_t));

	res = opae_ioctl(v->device.device_fd, VFIO_DEVICE_SET_IRQS, i);

	opae_free(buf);

	return res;
}

int opae_vfio_irq_enable(struct opae_vfio *v,
			 uint32_t index,
			 uint32_t subindex,
			 int event_fd)
{
	int res;

	if (!v) {
		ERR("NULL param\n");
		return 1;
	}

	if (pthread_mutex_lock(&v->lock)) {
		ERR("pthread_mutex_lock() failed\n");
		return 2;
	}

	res = opae_vfio_device_set_irqs(v,
					index,
					subindex,
					event_fd,
					VFIO_IRQ_SET_DATA_EVENTFD |
					VFIO_IRQ_SET_ACTION_TRIGGER);

	if (res < 0)
		ERR("ioctl(fd, VFIO_DEVICE_SET_IRQS, i) [enable]\n");

	if (pthread_mutex_unlock(&v->lock))
		ERR("pthread_mutex_unlock() failed\n");

	return res;
}

int opae_vfio_irq_unmask(struct opae_vfio *v,
			 uint32_t index,
			 uint32_t subindex)
{
	int res;

	if (!v) {
		ERR("NULL param\n");
		return 1;
	}

	if (pthread_mutex_lock(&v->lock)) {
		ERR("pthread_mutex_lock() failed\n");
		return 2;
	}

	res = opae_vfio_device_set_irqs(v,
					index,
					subindex,
					0,
					VFIO_IRQ_SET_DATA_BOOL |
					VFIO_IRQ_SET_ACTION_UNMASK);

	if (res < 0)
		ERR("ioctl(fd, VFIO_DEVICE_SET_IRQS, i) [unmask]\n");

	if (pthread_mutex_unlock(&v->lock))
		ERR("pthread_mutex_unlock() failed\n");

	return res;
}

int opae_vfio_irq_mask(struct opae_vfio *v,
		       uint32_t index,
		       uint32_t subindex)
{
	int res;

	if (!v) {
		ERR("NULL param\n");
		return 1;
	}

	if (pthread_mutex_lock(&v->lock)) {
		ERR("pthread_mutex_lock() failed\n");
		return 2;
	}

	res = opae_vfio_device_set_irqs(v,
					index,
					subindex,
					1,
					VFIO_IRQ_SET_DATA_BOOL |
					VFIO_IRQ_SET_ACTION_MASK);

	if (res < 0)
		ERR("ioctl(fd, VFIO_DEVICE_SET_IRQS, i) [mask]\n");

	if (pthread_mutex_unlock(&v->lock))
		ERR("pthread_mutex_unlock() failed\n");

	return res;
}

int opae_vfio_irq_disable(struct opae_vfio *v,
			  uint32_t index,
			  uint32_t subindex)
{
	int res;

	if (!v) {
		ERR("NULL param\n");
		return 1;
	}

	if (pthread_mutex_lock(&v->lock)) {
		ERR("pthread_mutex_lock() failed\n");
		return 2;
	}

	res = opae_vfio_device_set_irqs(v,
					index,
					subindex,
					-1,
					VFIO_IRQ_SET_DATA_EVENTFD |
					VFIO_IRQ_SET_ACTION_TRIGGER);

	if (res < 0)
		ERR("ioctl(fd, VFIO_DEVICE_SET_IRQS, i) [disable]\n");

	if (pthread_mutex_unlock(&v->lock))
		ERR("pthread_mutex_unlock() failed\n");

	return res;
}

STATIC char *opae_vfio_group_for(const char *pciaddr)
{
	char path[256];
	char rlbuf[256];
	char *p;

	snprintf(path, sizeof(path),
		 "/sys/bus/pci/devices/%s/iommu_group", pciaddr);

	memset(rlbuf, 0, sizeof(rlbuf));
	if (opae_readlink(path, rlbuf, sizeof(rlbuf)) < 0) {
		ERR("readlink() failed\n");
		return opae_strdup("ERROR");
	}

	p = strrchr(rlbuf, '/');
	if (!p)
		return opae_strdup("ERROR");

	snprintf(path, sizeof(path), "/dev/vfio/%s", p + 1);

	return opae_strdup(path);
}

STATIC
void opae_vfio_value_cleanup(void *value, void *context)
{
	struct opae_vfio_buffer *b =
		(struct opae_vfio_buffer *)value;
	struct opae_vfio *v =
		(struct opae_vfio *)context;

	opae_vfio_destroy_buffer(v, b);
}

STATIC int opae_vfio_init(struct opae_vfio *v,
			  const char *pciaddr,
			  const char *token)
{
	int res = 0;
	pthread_mutexattr_t mattr;
	int cont_fd;
	fpga_result result;

	memset(v, 0, sizeof(*v));
	v->cont_fd = -1;
	v->group.group_fd = -1;
	v->device.device_fd = -1;

	mem_alloc_init(&v->iova_alloc);

	result = opae_hash_map_init(&v->cont_buffers,
				    16384, // num_buckets
				    0,     // hash_seed
				    opae_u64_key_hash,
				    opae_u64_key_compare,
				    NULL,  // key_cleanup
				    opae_vfio_value_cleanup);
	if (result) {
		ERR("opae_hash_map_init()\n");
		return 11;
	}
	v->cont_buffers.cleanup_context = v;

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

	v->cont_device = opae_strdup("/dev/vfio/vfio");
	v->cont_pciaddr = opae_strdup(pciaddr);
	v->cont_fd = opae_open(v->cont_device, O_RDWR);
	if (v->cont_fd < 0) {
		ERR("open(\"%s\")\n", v->cont_device);
		res = 4;
		goto out_destroy_container;
	}

	if (opae_ioctl(v->cont_fd, VFIO_GET_API_VERSION) != VFIO_API_VERSION) {
		ERR("ioctl(%d, VFIO_GET_API_VERSION)\n", v->cont_fd);
		res = 5;
		goto out_destroy_container;
	}

	if (!opae_ioctl(v->cont_fd, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)) {
		ERR("ioctl(%d, VFIO_CHECK_EXTENSION, VFIO_TYPE1_IOMMU)\n",
		    v->cont_fd);
		res = 6;
		goto out_destroy_container;
	}

	res = opae_vfio_group_init(&v->group, opae_vfio_group_for(pciaddr));
	if (res)
		goto out_destroy_container;

	cont_fd = v->cont_fd;
	if (opae_ioctl(v->group.group_fd, VFIO_GROUP_SET_CONTAINER, &cont_fd)) {
		ERR("ioctl(%d, VFIO_GROUP_SET_CONTAINER, &cont_fd)\n",
		    v->group.group_fd);
		res = 7;
		goto out_destroy_container;
	}

	if (opae_ioctl(v->cont_fd, VFIO_SET_IOMMU, VFIO_TYPE1_IOMMU) < 0) {
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

	for (region = v->device.regions ; region ; region = region->next) {
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
