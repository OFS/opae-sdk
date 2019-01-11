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
 * opae_ioctl.c
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#define _GNU_SOURCE
#include <pthread.h>
#include <glob.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <regex.h>
#include <errno.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#undef _GNU_SOURCE

#include <opae/fpga.h>
#include <safe_string/safe_string.h>
#include "common_int.h"
#include "opae_drv.h"
#include "intel-fpga.h"
#include "fpga-dfl.h"

#define PCIE_PATH_PATTERN "([0-9a-fA-F]{4}):([0-9a-fA-F]{2}):([0-9]{2})\\.([0-9])"
#define PCIE_PATH_PATTERN_GROUPS 5

#define RE_GROUP_INT(_p, _m, _v, _b, _l)                                       \
	do {                                                                   \
		errno = 0;                                                     \
		_v = strtoul(_p + _m.rm_so, NULL, _b);                         \
		if (errno) {                                                   \
			FPGA_MSG("error parsing int");                         \
			goto _l;                                               \
		}                                                              \
	} while (0)


typedef struct _sysfs_formats {
	const char *class_path;
	const char *region_fmt;
	const char *resource_fmt;
	const char *compat_id;
} sysfs_formats;

typedef struct _ioctl_ops {
	fpga_result (*get_fme_info)(int fd, opae_fme_info *info);
	fpga_result (*get_port_info)(int fd, opae_port_info *info);
	fpga_result (*get_port_region_info)(int fd, uint32_t index,
					    opae_port_region_info *info);

	fpga_result (*port_map)(int fd, void *addr, uint64_t len,
				uint64_t *io_addr);
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


fpga_result intel_port_map(int fd, void *addr, uint64_t len, uint64_t *io_addr)
{
	int res = 0;
	int req = 0;
	void *msg = NULL;
	/* Set ioctl fpga_port_dma_map struct parameters */
	struct fpga_port_dma_map dma_map = {.argsz = sizeof(dma_map),
					    .flags = 0,
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
	errno_t err = 0;
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
	err = memcpy32_s((uint32_t *)irq->evtfd, count, (uint32_t *)eventfd,
			 count);
	if (err) {
		res = FPGA_INVALID_PARAM;
		goto out_free;
	}

	res = opae_ioctl(fd, FPGA_PORT_UAFU_SET_IRQ, irq);

out_free:
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

fpga_result dfl_port_map(int fd, void *addr, uint64_t len, uint64_t *io_addr)
{
	int res = 0;
	/* Set ioctl fpga_port_dma_map struct parameters */
	struct dfl_fpga_port_dma_map dma_map = {.argsz = sizeof(dma_map),
						.flags = 0,
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
	struct dfl_fpga_fme_port_assign assign = {
		.argsz = sizeof(assign), .flags = 0, .port_id = port_id};
	if (flags) {
		OPAE_MSG(
			"flags currently not supported in DFL_FPGA_FME_PORT_ASSIGN");
	}
	return opae_ioctl(fd, DFL_FPGA_FME_PORT_ASSIGN, &assign);
}

fpga_result dfl_fme_port_release(int fd, uint32_t flags, uint32_t port_id)
{
	struct dfl_fpga_fme_port_assign assign = {
		.argsz = sizeof(assign), .flags = 0, .port_id = port_id};
	if (flags) {
		OPAE_MSG(
			"flags currently not supported in DFL_FPGA_FME_PORT_RELEASE");
	}
	return opae_ioctl(fd, DFL_FPGA_FME_PORT_RELEASE, &assign);
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

#define OPAE_KERNEL_DRIVERS 2

typedef struct _drv_iface {
	sysfs_formats sysfs;
	ioctl_ops io;
} drv_iface;

static drv_iface _opae_drivers[OPAE_KERNEL_DRIVERS] = {
	{ {"/sys/class/fpga_region", "region([0-9])+",
	  "dfl-(fme|port)\\.([0-9]+)",
	  "/dfl-fme-region.*/fpga_region/region*/compat_id"},
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
	  .fme_port_assign = dfl_fme_port_assign,
	  .fme_port_release = dfl_fme_port_release,
	  .fme_port_pr = dfl_fme_port_pr,
	  .fme_port_reset = dfl_fme_port_reset} },
	{ {"/sys/class/fpga", "intel-fpga-dev\\.([0-9]+)",
	  "intel-fpga-(fme|port)\\.([0-9]+)", "pr/interface_id"},
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
	  .fme_port_assign = intel_fme_port_assign,
	  .fme_port_release = intel_fme_port_release,
	  .fme_port_pr = intel_fme_port_pr,
	  .fme_port_reset = intel_fme_port_reset}
	} };

static drv_iface *_drv;


#define SYSFS_MAX_DEVICES 128
#define RE_REGION_GROUPS 2
#define RE_RESOURCE_GROUPS 3

static opae_device _devices[SYSFS_MAX_DEVICES];
static uint32_t _sysfs_device_count;
pthread_mutex_t _sysfs_device_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

STATIC int sysfs_parse_device_vendor_id(opae_device *device)
{
	uint64_t value = 0;
	int res = sysfs_parse_attribute64(device->sysfs_path, "device/device", &value);
	if (res) {
		FPGA_ERR("Error parsing device_id for region: %s",
			 device->sysfs_path);
		return res;
	}
	device->device_id = value;

	res = sysfs_parse_attribute64(device->sysfs_path, "device/vendor", &value);

	if (res) {
		FPGA_ERR("Error parsing vendor_id for region: %s",
			 device->sysfs_path);
		return res;
	}
	device->vendor_id = value;

	return FPGA_OK;
}

STATIC fpga_result sysfs_parse_pcie_info(regex_t *re, opae_device *device, char *buffer)
{
	char err[128] = {0};
	regmatch_t matches[PCIE_PATH_PATTERN_GROUPS] = { {0} };

	int reg_res = regexec(re, buffer, PCIE_PATH_PATTERN_GROUPS, matches, 0);
	if (reg_res) {
		regerror(reg_res, re, err, sizeof(err));
		FPGA_ERR("Error executing regex: %s", err);
		return FPGA_EXCEPTION;
	}

	RE_GROUP_INT(buffer, matches[1], device->segment, 16, out_err);
	RE_GROUP_INT(buffer, matches[2], device->bus, 16, out_err);
	RE_GROUP_INT(buffer, matches[3], device->device, 16, out_err);
	RE_GROUP_INT(buffer, matches[4], device->function, 10, out_err);
	return FPGA_OK;

out_err:
	return FPGA_EXCEPTION;
}

STATIC int add_device(opae_device *device, const char *sysfs_class_fpga,
		      char *dir_name, int num, regex_t *pcie_re)
{
	int res = FPGA_OK;
	char buffer[PATH_MAX] = {0};
	ssize_t sym_link_len = 0;

	if (snprintf_s_ss(device->sysfs_path, PATH_MAX, "%s/%s",
			  sysfs_class_fpga, dir_name)
	    < 0) {
		FPGA_ERR("Error formatting sysfs paths");
		return FPGA_EXCEPTION;
	}

	if (snprintf_s_s(device->sysfs_name, PATH_MAX, "%s", dir_name) < 0) {
		FPGA_ERR("Error formatting sysfs name");
		return FPGA_EXCEPTION;
	}

	sym_link_len = readlink(device->sysfs_path, buffer, PATH_MAX);
	if (sym_link_len < 0) {
		FPGA_ERR("Error reading sysfs link: %s", device->sysfs_path);
		return FPGA_EXCEPTION;
	}

	device->instance = num;
	res = sysfs_parse_pcie_info(pcie_re, device, buffer);

	if (res) {
		FPGA_ERR("Could not parse symlink");
		return res;
	}

	return sysfs_parse_device_vendor_id(device);
}

#define OPAE_FME "fme"
#define OPAE_FME_LEN 3

#define OPAE_PORT "port"
#define OPAE_PORT_LEN 4

STATIC opae_resource *make_opae_resource(opae_device *device, char *name,
				    int num, fpga_objtype type)
{
	struct stat st;
	opae_resource *res = (opae_resource *)malloc(sizeof(opae_resource));
	if (!res) {
		OPAE_MSG("error allocating resource struct");
		return NULL;
	}

	res->device = device;
	res->type = type;
	res->instance = num;
	// copy the full path to the parent region object
	strcpy_s(res->sysfs_path, SYSFS_PATH_MAX, device->sysfs_path);
	// add a trailing path seperator '/'
	int len = strlen(res->sysfs_path);
	char *ptr = res->sysfs_path + len;
	*ptr = '/';
	ptr++;
	*ptr = '\0';
	// append the name to get the full path to the res
	if (cat_sysfs_path(res->sysfs_path, name)) {
		FPGA_ERR("error concatenating path");
		free(res);
		return NULL;
	}
	if (stat(res->sysfs_path, &st)) {
		FPGA_ERR("Resource sysfs path does not exist: %s", res->sysfs_path);
		free(res);
		return NULL;
	}

	if (snprintf_s_s(res->sysfs_name, PATH_MAX, "%s", name) < 0) {
		FPGA_ERR("Error formatting sysfs name");
		free(res);
		return NULL;
	}
	if (snprintf_s_s(res->devfs_path, PATH_MAX, "/dev/%s", name) < 0) {
		FPGA_ERR("Error formatting devfs path");
		free(res);
		return NULL;
	}

	if (stat(res->devfs_path, &st)) {
		FPGA_ERR("Resource devfs path does not exist: %s", res->devfs_path);
		free(res);
		return NULL;
	}

	return res;
}

STATIC void sysfs_find_resources(opae_device *device)
{
	DIR *dir = NULL;
	struct dirent *dirent = NULL;
	regex_t re;
	int reg_res = -1;
	int num = -1;
	char err[128] = {0};
	regmatch_t matches[RE_RESOURCE_GROUPS];
	reg_res = regcomp(&re, _drv->sysfs.resource_fmt, REG_EXTENDED);
	if (reg_res) {
		regerror(reg_res, &re, err, 128);
		FPGA_MSG("Error compiling regex: %s", err);
	}

	dir = opendir(device->sysfs_path);
	while ((dirent = readdir(dir)) != NULL) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;
		reg_res = regexec(&re, dirent->d_name, SYSFS_MAX_RESOURCES,
				  matches, 0);
		if (!reg_res) {
			int type_beg = matches[1].rm_so;
			// int type_end = matches[1].rm_eo;
			int num_beg = matches[2].rm_so;
			// int num_end = matches[2].rm_eo;
			if (type_beg < 1 || num_beg < 1) {
				FPGA_MSG("Invalid sysfs resource format");
				continue;
			}
			num = strtoul(dirent->d_name + num_beg, NULL, 10);
			if (!strncmp(OPAE_FME, dirent->d_name + type_beg,
				     OPAE_FME_LEN)) {
				device->fme = make_opae_resource(
					device, dirent->d_name, num,
					FPGA_DEVICE);
			} else if (!strncmp(OPAE_PORT,
					    dirent->d_name + type_beg,
					    OPAE_PORT_LEN)) {
				device->port = make_opae_resource(
					device, dirent->d_name, num,
					FPGA_ACCELERATOR);
			}
		}
	}
	regfree(&re);
	closedir(dir);
}

STATIC void discover(void)
{
	int res = 0;
	uint32_t i = 0;
	char err[128];
	DIR *dir = NULL;
	struct dirent *dirent = NULL;
	const char *sysfs_class_path = _drv->sysfs.class_path;
	regex_t device_re, pcie_re;
	regmatch_t matches[RE_REGION_GROUPS];
	int reg_res = regcomp(&device_re, _drv->sysfs.region_fmt,
			      REG_EXTENDED);
	if (reg_res) {
		regerror(reg_res, &device_re, err, 128);
		FPGA_ERR("Error compling regex: %s", err);
		return;
	};

	reg_res = regcomp(&pcie_re, PCIE_PATH_PATTERN, REG_EXTENDED | REG_ICASE);
	if (reg_res) {
		regfree(&device_re);
		FPGA_ERR("Error compling regex");
		return;
	}

	dir = opendir(sysfs_class_path);
	while ((dirent = readdir(dir))) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;
		// if the current directory matches the device regex
		reg_res = regexec(&device_re, dirent->d_name, RE_REGION_GROUPS,
				  matches, 0);
		if (!reg_res) {
			int num_begin = matches[1].rm_so;
			if (num_begin < 0) {
				FPGA_ERR("sysfs format invalid: %s", dirent->d_name);
				continue;
			}
			int num = strtoul(dirent->d_name + num_begin, NULL, 10);
			// increment our device count after filling out details
			// of the discovered region in our _devices array
			if (opae_mutex_lock(res, &_sysfs_device_lock)) {
				goto out_free;
			}
			if (add_device(&_devices[_sysfs_device_count++],
					sysfs_class_path, dirent->d_name,
					num, &pcie_re)) {
				FPGA_MSG("Error processing device: %s",
					 dirent->d_name);
				_sysfs_device_count--;
			}
			if (opae_mutex_unlock(res, &_sysfs_device_lock)) {
				goto out_free;
			}
		}
	}

	if (opae_mutex_lock(res, &_sysfs_device_lock)) {
		goto out_free;
	} else if (!_sysfs_device_count) {
		FPGA_ERR("Error discovering fpga regions");
		goto out_unlock;
	}

	// now, for each discovered region, look inside for resources
	// (fme|port)
	for (i = 0; i < _sysfs_device_count; ++i) {
		sysfs_find_resources(&_devices[i]);
	}

out_unlock:
	if (pthread_mutex_unlock(&_sysfs_device_lock)) {
		FPGA_MSG("error unlocking sysfs region mutex");
	}
out_free:
	regfree(&device_re);
	regfree(&pcie_re);
	closedir(dir);
}

STATIC void sysfs_device_clear(opae_device *device)
{
	if (!device) {
		return;
	}

	if (device->fme) {
		free(device->fme);
		device->fme = NULL;
	}
	if (device->port) {
		free(device->port);
		device->port = NULL;
	}
}

fpga_result opae_drv_initialize(void)
{
	if (_drv) {
		// indicates that we have already initialized
		// let's finalize and re-initialize
		// this helps in cases when finalize isn't called
		opae_drv_finalize();
	}

	struct stat st;
	if (!stat("/sys/class/fpga_region", &st)) {
		_drv = &_opae_drivers[0];
	}
	if (!stat("/sys/class/fpga", &st)) {
		_drv = &_opae_drivers[1];
	}

	if (!_drv) {
		OPAE_ERR("Did not find OPAE sysfs path");
		return FPGA_NO_DRIVER;
	}

	discover();

	return FPGA_OK;
}

fpga_result opae_drv_finalize(void)
{
	uint32_t i = 0;
	fpga_result res = FPGA_OK;
	if (opae_mutex_lock(res, &_sysfs_device_lock)) {
		OPAE_ERR("error locking mutex");
		goto out;
	}
	for (; i < _sysfs_device_count; ++i) {
		sysfs_device_clear(&_devices[i]);
	}
	_sysfs_device_count = 0;
	if (opae_mutex_unlock(res, &_sysfs_device_lock)) {
		OPAE_ERR("error unlocking mutex");
		goto out;
	}
	_drv = NULL;
out:
	return res;
}

#define IOCTL(_FN, ...)                                                        \
	do {                                                                   \
		if (!_drv) {                                                   \
			OPAE_ERR("ioctl interface has not been initialized");  \
			return FPGA_EXCEPTION;                                 \
		}                                                              \
		if (!_drv->io._FN) {                                           \
			OPAE_MSG("ioctl function not yet supported");          \
			return FPGA_NOT_SUPPORTED;                             \
		}                                                              \
		return _drv->io._FN(__VA_ARGS__);                              \
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

fpga_result opae_port_map(int fd, void *addr, uint64_t len, uint64_t *io_addr)
{
	IOCTL(port_map, fd, addr, len, io_addr);
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
