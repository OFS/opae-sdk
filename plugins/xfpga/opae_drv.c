// Copyright(c) 2017-2020, Intel Corporation
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

#include <string.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>

#include <opae/fpga.h>

#include "common_int.h"
#include "opae_drv.h"
#include "intel-fpga.h"
#include "fpga-dfl.h"

typedef struct _ioctl_ops {
	fpga_result (*get_fme_info)(int fd, opae_fme_info *info);
	fpga_result (*get_port_info)(int fd, opae_port_info *info);
	fpga_result (*get_port_region_info)(int fd, uint32_t index,
					    opae_port_region_info *info);

	fpga_result (*port_map)(int fd, void *addr, uint64_t len,
				uint32_t flags, uint64_t *io_addr);
	fpga_result (*port_unmap)(int fd, uint64_t io_addr);

	fpga_result (*port_umsg_cfg)(int fd, uint32_t flags,
				     uint32_t hint_bitmap);
	fpga_result (*port_umsg_set_base_addr)(int fd, uint32_t flags,
					       uint64_t io_addr);
	fpga_result (*port_umsg_enable)(int fd);
	fpga_result (*port_umsg_disable)(int fd);

	fpga_result (*fme_set_err_irq)(int fd, uint32_t flags, int32_t eventfd);
	fpga_result (*port_set_err_irq)(int fd, uint32_t flags,
					int32_t eventfd);
	fpga_result (*port_set_user_irq)(int fd, uint32_t flags, uint32_t start,
					 uint32_t count, int32_t *eventfd);

	fpga_result (*fme_port_assign)(int fd, uint32_t flags,
				       uint32_t port_id);
	fpga_result (*fme_port_release)(int fd, uint32_t flags,
					uint32_t port_id);
	fpga_result (*fme_port_pr)(int fd, uint32_t flags, uint32_t port_id,
				   uint32_t sz, uint64_t addr,
				   uint64_t *status);
	fpga_result (*fme_port_reset)(int fd);
	fpga_result(*dfl_port_get_err_irq)(int fd, uint32_t *num_irqs);

	fpga_result(*dfl_port_get_user_irq)(int fd, uint32_t *num_irqs);

	fpga_result(*dfl_fme_get_err_irq)(int fd, uint32_t *num_irqs);

	fpga_result(*dfl_fme_set_err_irq)(int fd, uint32_t start,
		uint32_t count, int32_t *eventfd);

	fpga_result(*dfl_port_set_err_irq)(int fd, uint32_t start,
		uint32_t count, int32_t *eventfd);

	fpga_result(*dfl_port_set_user_irq)(int fd, uint32_t start,
		uint32_t count, int32_t *eventfd);
} ioctl_ops;

fpga_result opae_ioctl(int fd, int request, ...)
{
	fpga_result res = FPGA_OK;
	va_list argp;
	va_start(argp, request);
	void *msg = va_arg(argp, void *);
	errno = 0;
	if (ioctl(fd, request, msg) != 0) {
		OPAE_MSG("error executing ioctl: %s", strerror(errno));
		switch (errno) {
		case EINVAL:
			res = FPGA_INVALID_PARAM;
			break;
		case ENOTSUP:
			res = FPGA_NOT_SUPPORTED;
			break;
		default:
			// other errors could be
			// EBADF - fd is bad file descriptor
			// EFAULT - argp references an inaccessible
			// memory area
			// ENOTTY - fd is not associated with a char.
			// special device
			res = FPGA_EXCEPTION;
			break;
		}
	}
	va_end(argp);

	return res;
}


fpga_result intel_fpga_version(int fd)
{
	return opae_ioctl(fd, FPGA_GET_API_VERSION, NULL);
}

fpga_result intel_get_fme_info(int fd, opae_fme_info *info)
{
	ASSERT_NOT_NULL(info);
	struct fpga_fme_info fme_info = {.argsz = sizeof(fme_info), .flags = 0};
	int res = opae_ioctl(fd, FPGA_FME_GET_INFO, &fme_info);
	if (!res) {
		info->flags = fme_info.flags;
		info->capability = fme_info.capability;
	}
	return res;
}

fpga_result intel_get_port_info(int fd, opae_port_info *info)
{
	ASSERT_NOT_NULL(info);
	struct fpga_port_info pinfo = {.argsz = sizeof(pinfo), .flags = 0};
	int res = opae_ioctl(fd, FPGA_PORT_GET_INFO, &pinfo);
	if (!res) {
		info->flags = pinfo.flags;
		info->capability = pinfo.capability;
		if (pinfo.capability & FPGA_PORT_CAP_UAFU_IRQ)
			info->capability |= OPAE_PORT_CAP_UAFU_IRQS;
		info->num_regions = pinfo.num_regions;
		info->num_umsgs = pinfo.num_umsgs;
		info->num_uafu_irqs = pinfo.num_uafu_irqs;
	}
	return res;
}


fpga_result intel_get_port_region_info(int fd, uint32_t index,
				       opae_port_region_info *info)
{
	ASSERT_NOT_NULL(info);
	struct fpga_port_region_info rinfo = {
		.argsz = sizeof(rinfo), .padding = 0, .index = index};
	int res = opae_ioctl(fd, FPGA_PORT_GET_REGION_INFO, &rinfo);
	if (!res) {
		info->flags = rinfo.flags;
		info->size = rinfo.size;
		info->offset = rinfo.offset;
	}
	return res;
}


fpga_result intel_port_map(int fd, void *addr, uint64_t len, uint32_t flags,
			   uint64_t *io_addr)
{
	int res = 0;
	int req = 0;
	void *msg = NULL;
	/* Set ioctl fpga_port_dma_map struct parameters */
	struct fpga_port_dma_map dma_map = {.argsz = sizeof(dma_map),
					    .flags = flags,
					    .user_addr = (__u64)addr,
					    .length = (__u64)len,
					    .iova = 0};
	ASSERT_NOT_NULL(io_addr);
	/* Dispatch ioctl command */
	req = FPGA_PORT_DMA_MAP;
	msg = &dma_map;

	res = opae_ioctl(fd, req, msg);
	if (!res) {
		*io_addr = dma_map.iova;
	}
	return res;
}

fpga_result intel_port_unmap(int fd, uint64_t io_addr)
{
	/* Set ioctl fpga_port_dma_unmap struct parameters */
	struct fpga_port_dma_unmap dma_unmap = {
		.argsz = sizeof(dma_unmap), .flags = 0, .iova = io_addr};

	/* Dispatch ioctl command */
	return opae_ioctl(fd, FPGA_PORT_DMA_UNMAP, &dma_unmap);
}

fpga_result intel_port_umsg_cfg(int fd, uint32_t flags, uint32_t hint_bitmap)
{
	if (flags) {
		OPAE_MSG(
			"flags currently not supported in FPGA_PORT_UMSG_SET_MODE");
	}

	struct fpga_port_umsg_cfg umsg_cfg = {.argsz = sizeof(umsg_cfg),
					      .flags = 0,
					      .hint_bitmap = hint_bitmap};
	return opae_ioctl(fd, FPGA_PORT_UMSG_SET_MODE, &umsg_cfg);
}

fpga_result intel_port_umsg_set_base_addr(int fd, uint32_t flags,
					  uint64_t io_addr)
{
	if (flags) {
		OPAE_MSG(
			"flags currently not supported in FPGA_PORT_UMSG_SET_BASE_ADDR");
	}

	struct fpga_port_umsg_base_addr baseaddr = {
		.argsz = sizeof(baseaddr), .flags = 0, .iova = io_addr};
	return opae_ioctl(fd, FPGA_PORT_UMSG_SET_BASE_ADDR, &baseaddr);
}

fpga_result intel_port_umsg_enable(int fd)
{
	return opae_ioctl(fd, FPGA_PORT_UMSG_ENABLE, NULL);
}

fpga_result intel_port_umsg_disable(int fd)
{
	return opae_ioctl(fd, FPGA_PORT_UMSG_DISABLE, NULL);
}

fpga_result intel_fme_set_err_irq(int fd, uint32_t flags, int32_t evtfd)
{
	if (flags) {
		OPAE_MSG(
			"flags currently not supported in FPGA_FME_ERR_SET_IRQ");
	}

	struct fpga_fme_err_irq_set irq = {
		.argsz = sizeof(irq), .flags = flags, .evtfd = evtfd};
	return opae_ioctl(fd, FPGA_FME_ERR_SET_IRQ, &irq);
}

fpga_result intel_port_set_err_irq(int fd, uint32_t flags, int32_t evtfd)
{
	if (flags) {
		OPAE_MSG(
			"flags currently not supported in FPGA_FME_ERR_SET_IRQ");
	}

	struct fpga_port_err_irq_set irq = {
		.argsz = sizeof(irq), .flags = flags, .evtfd = evtfd};
	return opae_ioctl(fd, FPGA_PORT_ERR_SET_IRQ, &irq);
}

fpga_result intel_port_set_user_irq(int fd, uint32_t flags, uint32_t start,
				    uint32_t count, int32_t *eventfd)
{
	uint32_t sz =
		sizeof(struct fpga_port_uafu_irq_set) + count * sizeof(int32_t);
	struct fpga_port_uafu_irq_set *irq = NULL;
	int res = 0;

	ASSERT_NOT_NULL(eventfd);
	if (!count) {
		OPAE_ERR("set_user irq with emtpy count");
		return FPGA_INVALID_PARAM;
	}

	if (flags) {
		OPAE_MSG(
			"flags currently not supported in FPGA_FME_ERR_SET_IRQ");
	}

	irq = malloc(sz);
	if (!irq) {
		OPAE_ERR("Could not allocate memory for irq request");
		return FPGA_NO_MEMORY;
	}

	irq->argsz = sz;
	irq->flags = 0;
	irq->start = start;
	irq->count = count;

	memcpy(irq->evtfd, eventfd, count * sizeof(int32_t));

	res = opae_ioctl(fd, FPGA_PORT_UAFU_SET_IRQ, irq);

	free(irq);
	return res;
}

fpga_result intel_fme_port_assign(int fd, uint32_t flags, uint32_t port_id)
{
	struct fpga_fme_port_assign assign = {
		.argsz = sizeof(assign), .flags = 0, .port_id = port_id};
	if (flags) {
		OPAE_MSG(
			"flags currently not supported in FPGA_FME_PORT_ASSIGN");
	}
	return opae_ioctl(fd, FPGA_FME_PORT_ASSIGN, &assign);
}

fpga_result intel_fme_port_release(int fd, uint32_t flags, uint32_t port_id)
{
	struct fpga_fme_port_assign assign = {
		.argsz = sizeof(assign), .flags = 0, .port_id = port_id};
	if (flags) {
		OPAE_MSG(
			"flags currently not supported in FPGA_FME_PORT_RELEASE");
	}
	return opae_ioctl(fd, FPGA_FME_PORT_RELEASE, &assign);
}

fpga_result intel_fme_port_pr(int fd, uint32_t flags, uint32_t port_id,
			      uint32_t sz, uint64_t addr, uint64_t *status)
{
	struct fpga_fme_port_pr port_pr = {.argsz = sizeof(port_pr),
					   .flags = 0,
					   .port_id = port_id,
					   .buffer_size = sz,
					   .buffer_address = addr};
	int res = FPGA_OK;
	if (flags) {
		OPAE_MSG("flags currently not supported in FPGA_FME_PORT_PR");
	}
	ASSERT_NOT_NULL(status);
	res = opae_ioctl(fd, FPGA_FME_PORT_PR, &port_pr);
	*status = port_pr.status;
	return res;
}

fpga_result intel_fme_port_reset(int fd)
{
	return opae_ioctl(fd, FPGA_PORT_RESET, NULL);
}

fpga_result dfl_fpga_version(int fd)
{
	return opae_ioctl(fd, DFL_FPGA_GET_API_VERSION, NULL);
}

fpga_result dfl_get_port_info(int fd, opae_port_info *info)
{
	ASSERT_NOT_NULL(info);
	struct dfl_fpga_port_info pinfo = {.argsz = sizeof(pinfo), .flags = 0};
	int res = opae_ioctl(fd, DFL_FPGA_PORT_GET_INFO, &pinfo);
	if (!res) {
		info->flags = pinfo.flags;
		info->num_regions = pinfo.num_regions;
		info->num_umsgs = pinfo.num_umsgs;
	}
	return res;
}

fpga_result dfl_get_port_region_info(int fd, uint32_t index,
				     opae_port_region_info *info)
{
	ASSERT_NOT_NULL(info);
	struct dfl_fpga_port_region_info rinfo = {
		.argsz = sizeof(rinfo), .padding = 0, .index = index};
	int res = opae_ioctl(fd, DFL_FPGA_PORT_GET_REGION_INFO, &rinfo);
	if (!res) {
		info->flags = rinfo.flags;
		info->size = rinfo.size;
		info->offset = rinfo.offset;
	}
	return res;
}

fpga_result dfl_port_map(int fd, void *addr, uint64_t len, uint32_t flags,
			 uint64_t *io_addr)
{
	int res = 0;
	/* Set ioctl fpga_port_dma_map struct parameters */
	struct dfl_fpga_port_dma_map dma_map = {.argsz = sizeof(dma_map),
						.flags = flags,
						.user_addr = (__u64)addr,
						.length = (__u64)len,
						.iova = 0};
	ASSERT_NOT_NULL(io_addr);
	/* Dispatch ioctl command */
	res = opae_ioctl(fd, DFL_FPGA_PORT_DMA_MAP, &dma_map);
	if (!res) {
		*io_addr = dma_map.iova;
	}
	return res;
}

fpga_result dfl_port_unmap(int fd, uint64_t io_addr)
{
	/* Set ioctl fpga_port_dma_unmap struct parameters */
	struct dfl_fpga_port_dma_unmap dma_unmap = {
		.argsz = sizeof(dma_unmap), .flags = 0, .iova = io_addr};

	/* Dispatch ioctl command */
	return opae_ioctl(fd, DFL_FPGA_PORT_DMA_UNMAP, &dma_unmap);
}


fpga_result dfl_fme_port_assign(int fd, uint32_t flags, uint32_t port_id)
{
	UNUSED_PARAM(flags);
	return opae_ioctl(fd, DFL_FPGA_FME_PORT_ASSIGN, port_id);
}

fpga_result dfl_fme_port_release(int fd, uint32_t flags, uint32_t port_id)
{
	UNUSED_PARAM(flags);
	return opae_ioctl(fd, DFL_FPGA_FME_PORT_RELEASE, port_id);
}

fpga_result dfl_fme_port_pr(int fd, uint32_t flags, uint32_t port_id,
			    uint32_t sz, uint64_t addr, uint64_t *status)
{
	struct dfl_fpga_fme_port_pr port_pr = {.argsz = sizeof(port_pr),
					       .flags = 0,
					       .port_id = port_id,
					       .buffer_size = sz,
					       .buffer_address = addr};
	int res = FPGA_OK;
	if (flags) {
		OPAE_MSG("flags currently not supported in FPGA_FME_PORT_PR");
	}
	ASSERT_NOT_NULL(status);
	res = opae_ioctl(fd, DFL_FPGA_FME_PORT_PR, &port_pr);
	*status = 0;
	return res;
}

fpga_result dfl_fme_port_reset(int fd)
{
	return opae_ioctl(fd, DFL_FPGA_PORT_RESET, NULL);
}

fpga_result dfl_port_get_err_irq(int fd, uint32_t *num_irqs)
{
	ASSERT_NOT_NULL(num_irqs);
	int res = opae_ioctl(fd, DFL_FPGA_PORT_ERR_GET_IRQ_NUM, num_irqs);
	if (!res) {
		OPAE_MSG("Ioctl DFL_FPGA_PORT_ERR_GET_IRQ_NUM error=%d", res);
	}
	return res;
}

fpga_result dfl_port_get_user_irq(int fd, uint32_t *num_irqs)
{
	ASSERT_NOT_NULL(num_irqs);
	int res = opae_ioctl(fd, DFL_FPGA_PORT_UINT_GET_IRQ_NUM, num_irqs);
	if (!res) {
		OPAE_MSG("Ioctl DFL_FPGA_PORT_UINT_GET_IRQ_NUM error=%d", res);
	}
	return res;
}

fpga_result dfl_fme_get_err_irq(int fd, uint32_t *num_irqs)
{
	ASSERT_NOT_NULL(num_irqs);
	int res = opae_ioctl(fd, DFL_FPGA_FME_ERR_GET_IRQ_NUM, num_irqs);
	if (!res) {
		OPAE_MSG("Ioctl DFL_FPGA_FME_ERR_GET_IRQ_NUM error=%d", res);
	}
	return res;
}

fpga_result dfl_set_irq(int fd,uint32_t start,
	uint32_t count, int32_t *eventfd,int ioctl_id)
{
	uint32_t sz = sizeof(struct dfl_fpga_irq_set)
		+ count * sizeof(int32_t);
	struct dfl_fpga_irq_set *irq = NULL;
	int res = 0;

	ASSERT_NOT_NULL(eventfd);
	if (!count) {
		OPAE_ERR("set_user irq with emtpy count");
		return FPGA_INVALID_PARAM;
	}

	irq = malloc(sz);

	if (!irq) {
		OPAE_ERR("Could not allocate memory for irq request");
		return FPGA_NO_MEMORY;
	}

	irq->start = start;
	irq->count = count;
	memcpy(irq->evtfds, eventfd, count * sizeof(int32_t));

	res = opae_ioctl(fd, ioctl_id, irq);

	free(irq);
	return res;
}

fpga_result dfl_fme_set_err_irq(int fd,  uint32_t start,
	uint32_t count, int32_t *eventfd)
{
	return dfl_set_irq(fd, start, count, eventfd, DFL_FPGA_FME_ERR_SET_IRQ);
}

fpga_result dfl_port_set_err_irq(int fd, uint32_t start,
	uint32_t count, int32_t *eventfd)
{
	return dfl_set_irq(fd,  start, count, eventfd, DFL_FPGA_PORT_ERR_SET_IRQ);
}


fpga_result dfl_port_set_user_irq(int fd, uint32_t start,
	uint32_t count, int32_t *eventfd)
{
	return dfl_set_irq(fd, start, count, eventfd, DFL_FPGA_PORT_UINT_SET_IRQ);
}

#define MAX_KERNEL_DRIVERS 2
static ioctl_ops ioctl_table[MAX_KERNEL_DRIVERS] = {
	{.get_fme_info = NULL,
	 .get_port_info = dfl_get_port_info,
	 .get_port_region_info = dfl_get_port_region_info,
	 .port_map = dfl_port_map,
	 .port_unmap = dfl_port_unmap,
	 .port_umsg_cfg = NULL,
	 .port_umsg_set_base_addr = NULL,
	 .port_umsg_enable = NULL,
	 .port_umsg_disable = NULL,
	 .fme_set_err_irq = NULL,
	 .port_set_err_irq = NULL,
	 .port_set_user_irq = NULL,
	 .dfl_port_get_err_irq = dfl_port_get_err_irq,
	 .dfl_port_get_user_irq = dfl_port_get_user_irq,
	 .dfl_fme_get_err_irq = dfl_fme_get_err_irq,
	 .dfl_fme_set_err_irq = dfl_fme_set_err_irq,
	 .dfl_port_set_err_irq = dfl_port_set_err_irq,
	 .dfl_port_set_user_irq = dfl_port_set_err_irq,
	 .fme_port_assign = dfl_fme_port_assign,
	 .fme_port_release = dfl_fme_port_release,
	 .fme_port_pr = dfl_fme_port_pr,
	 .fme_port_reset = dfl_fme_port_reset},
	{.get_fme_info = intel_get_fme_info,
	 .get_port_info = intel_get_port_info,
	 .get_port_region_info = intel_get_port_region_info,
	 .port_map = intel_port_map,
	 .port_unmap = intel_port_unmap,
	 .port_umsg_cfg = intel_port_umsg_cfg,
	 .port_umsg_set_base_addr = intel_port_umsg_set_base_addr,
	 .port_umsg_enable = intel_port_umsg_enable,
	 .port_umsg_disable = intel_port_umsg_disable,
	 .fme_set_err_irq = intel_fme_set_err_irq,
	 .port_set_err_irq = intel_port_set_err_irq,
	 .port_set_user_irq = intel_port_set_user_irq,
	 .dfl_port_get_err_irq = NULL,
	 .dfl_port_get_user_irq = NULL,
	 .dfl_fme_get_err_irq = NULL,
	 .dfl_fme_set_err_irq = NULL,
	 .dfl_port_set_err_irq = NULL,
	 .fme_port_assign = intel_fme_port_assign,
	 .fme_port_release = intel_fme_port_release,
	 .fme_port_pr = intel_fme_port_pr,
	 .fme_port_reset = intel_fme_port_reset} };

static ioctl_ops *io_ptr;

int opae_ioctl_initialize(void)
{
	struct stat st;
	if (!stat("/sys/class/fpga_region", &st)) {
		io_ptr = &ioctl_table[0];
		return 0;
	}
	if (!stat("/sys/class/fpga", &st)) {
		io_ptr = &ioctl_table[1];
		return 0;
	}
	return 1;
}

#define IOCTL(_FN, ...)                                                        \
	do {                                                                   \
		if (!io_ptr) {                                                 \
			OPAE_ERR("ioctl interface has not been initialized");  \
			return FPGA_EXCEPTION;                                 \
		}                                                              \
		if (!io_ptr->_FN) {                                            \
			OPAE_MSG("ioctl function not yet supported");          \
			return FPGA_NOT_SUPPORTED;                             \
		}                                                              \
		return io_ptr->_FN(__VA_ARGS__);                               \
	} while (0);

fpga_result opae_get_fme_info(int fd, opae_fme_info *info)
{
	IOCTL(get_fme_info, fd, info);
}

fpga_result opae_get_port_info(int fd, opae_port_info *info)
{
	IOCTL(get_port_info, fd, info);
}

fpga_result opae_get_port_region_info(int fd, uint32_t index,
				      opae_port_region_info *info)
{
	IOCTL(get_port_region_info, fd, index, info);
}

fpga_result opae_port_map(int fd, void *addr, uint64_t len, uint32_t flags,
			  uint64_t *io_addr)
{
	IOCTL(port_map, fd, addr, len, flags, io_addr);
}

fpga_result opae_port_unmap(int fd, uint64_t io_addr)
{
	IOCTL(port_unmap, fd, io_addr);
}

fpga_result opae_port_umsg_cfg(int fd, uint32_t flags, uint32_t hint_bitmap)
{
	IOCTL(port_umsg_cfg, fd, flags, hint_bitmap);
}

fpga_result opae_port_umsg_set_base_addr(int fd, uint32_t flags,
					 uint64_t io_addr)
{
	IOCTL(port_umsg_set_base_addr, fd, flags, io_addr);
}

fpga_result opae_port_umsg_enable(int fd)
{
	IOCTL(port_umsg_enable, fd);
}

fpga_result opae_port_umsg_disable(int fd)
{
	IOCTL(port_umsg_disable, fd);
}

fpga_result opae_fme_set_err_irq(int fd, uint32_t flags, int32_t eventfd)
{
	IOCTL(fme_set_err_irq, fd, flags, eventfd);
}

fpga_result opae_port_set_err_irq(int fd, uint32_t flags, int32_t eventfd)
{
	IOCTL(port_set_err_irq, fd, flags, eventfd);
}

fpga_result opae_port_set_user_irq(int fd, uint32_t flags, uint32_t start,
				   uint32_t count, int32_t *eventfd)
{
	IOCTL(port_set_user_irq, fd, flags, start, count, eventfd);
}

fpga_result opae_fme_port_assign(int fd, uint32_t flags, uint32_t port_id)
{
	IOCTL(fme_port_assign, fd, flags, port_id);
}

fpga_result opae_fme_port_release(int fd, uint32_t flags, uint32_t port_id)
{
	IOCTL(fme_port_release, fd, flags, port_id);
}

fpga_result opae_fme_port_pr(int fd, uint32_t flags, uint32_t port_id,
			     uint32_t sz, uint64_t addr, uint64_t *status)
{
	IOCTL(fme_port_pr, fd, flags, port_id, sz, addr, status);
}

fpga_result opae_fme_port_reset(int fd)
{
	IOCTL(fme_port_reset, fd);
}
fpga_result opae_dfl_port_get_err_irq(int fd, uint32_t *num_irqs)
{
	IOCTL(dfl_port_get_err_irq, fd, num_irqs);
}

fpga_result opae_dfl_port_get_user_irq(int fd, uint32_t *num_irqs)
{
	IOCTL(dfl_port_get_user_irq, fd, num_irqs);
}

fpga_result opae_dfl_fme_get_err_irq(int fd, uint32_t *num_irqs)
{
	IOCTL(dfl_fme_get_err_irq, fd, num_irqs);
}

fpga_result opae_dfl_port_set_err_irq(int fd, uint32_t start,
	uint32_t count, int32_t *eventfd)
{
	IOCTL(dfl_port_set_err_irq, fd, start, count, eventfd);
}

fpga_result opae_dfl_port_set_user_irq(int fd, uint32_t start,
	uint32_t count, int32_t *eventfd)
{
	IOCTL(dfl_port_set_user_irq, fd, start, count, eventfd);

}
fpga_result opae_dfl_fme_set_err_irq(int fd, uint32_t start,
	uint32_t count, int32_t *eventfd)
{
	IOCTL(dfl_fme_set_err_irq, fd, start, count, eventfd);
}
