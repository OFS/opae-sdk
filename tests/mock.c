// Copyright(c) 2017, Intel Corporation
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
 * Mock up driver interactions for testing
 *
 * Involves redefining ioctl(), open(), close(), others?
 */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <opae/types.h>
#include "common_int.h"
#include "intel-fpga.h"
#include <assert.h>
#include <stdint.h>
#include <safe_string/safe_string.h>

#define __USE_GNU
#include <dlfcn.h>

#define MAX_FD 1024
#define MAX_STRLEN 256
#define FPGA_MOCK_IOVA 0xDECAFBADDEADBEEF
#define FPGA_MOCK_NUM_UMSGS 8
#define FPGA_MOCK_DEV_PATH "/tmp"
#define MOCK_SYSFS_FPGA_CLASS_PATH "/tmp/class/fpga"
#define FPGA_FME_DEV_PREFIX "intel-fpga-fme."
#define FPGA_PORT_DEV_PREFIX "intel-fpga-port."
#define HASH_SUFFIX ".gbshash"

#undef FPGA_MSG
#define FPGA_MSG(fmt, ...) \
	printf("MOCK " fmt "\n", ## __VA_ARGS__)

#undef FPGA_ERR
#define FPGA_ERR(fmt, ...) \
	printf("MOCK ERROR " fmt "\n", ## __VA_ARGS__)

#undef FPGA_DBG
#ifdef LIBFPGA_DEBUG
#define FPGA_DBG(fmt, ...) \
	printf("MOCK DEBUG " fmt "\n", ## __VA_ARGS__)
#else
#define FPGA_DBG(fmt, ...) {}
#endif

/* TODO: track mock devices with dynamic data structure */
static struct mock_dev {
	int valid;
	fpga_objtype objtype;
	char pathname[MAX_STRLEN];
} mock_devs[MAX_FD] = {{0}};

static bool gEnableIRQ = false;
bool mock_enable_irq(bool enable)
{
	bool res = gEnableIRQ;
	gEnableIRQ = enable;
	return res;
}

static bool gEnableErrInj = false;
bool mock_enable_errinj(bool enable)
{
	bool res = gEnableErrInj;
	gEnableErrInj = enable;
	return res;
}

typedef int (*open_func)(const char *pathname, int flags);
typedef int (*open_mode_func)(const char *pathname, int flags, mode_t m);

typedef int (*close_func)(int fd);
typedef int (*ioctl_func)(int fd, unsigned long request, char *argp);
typedef DIR * (*opendir_func)(const char *name);
typedef ssize_t (*readlink_func)(const char *pathname, char *buf, size_t bufsiz);
typedef int (*__xstat_func)(int ver, const char *pathname, struct stat *buf);


uint32_t stupid_hash(uint32_t *buf, uint32_t len_in_words) {

	uint32_t i;
	uint32_t hash = 0;

	for (i = 0; i < len_in_words; ++i)
		hash ^= buf[i];

	return hash;
}

static char* rewrite_sysfs_path(const char* src, char* dst, int len) {
    int prefix_len = strlen(SYSFS_FPGA_CLASS_PATH);

    if (strncmp(SYSFS_FPGA_CLASS_PATH, src, prefix_len) == 0) {
        strncpy_s(dst, len, MOCK_SYSFS_FPGA_CLASS_PATH, strlen(MOCK_SYSFS_FPGA_CLASS_PATH));
        strncpy_s(dst + prefix_len, len - prefix_len, src + prefix_len, len - prefix_len);
    } else {
        strncpy_s(dst, len, src, len);
    }

    return dst;
}

int ioctl(int fd, unsigned long request, ...)
{
	va_list argp;
	int retval = -1;
	char *err;
	errno = EINVAL;
	uint32_t hash;
	char hashfilename[MAX_STRLEN];

	va_start(argp, request);

	/* check where ioctl is going */
	if (fd >= MAX_FD || !mock_devs[fd].valid) {
		FPGA_DBG("real ioctl() called");
		dlerror(); /* clear errors */
		ioctl_func real_ioctl = (ioctl_func)dlsym(RTLD_NEXT, "ioctl");
		err = dlerror();

		if (NULL != err){
			FPGA_ERR("dlsym() failed: %s", err);
			goto out_EINVAL;
		}
		char *arg = va_arg(argp, char *);

        if (NULL != real_ioctl) {
            return real_ioctl(fd, request, arg);
        }
    }

	if (fd >= MAX_FD)
		return -1;

	FPGA_DBG("mock ioctl() called");

	// Returns error when Error injection enabled
	if (gEnableErrInj) {
		goto out_EINVAL;
	}

	switch (mock_devs[fd].objtype) {
	case FPGA_DEVICE: /* FME */
		switch (request) {
		case FPGA_FME_PORT_RELEASE:
			FPGA_DBG("got FPGA_FME_PORT_RELEASE");
			struct fpga_fme_port_release *port_release =
				va_arg(argp, struct fpga_fme_port_release *);
			if (!port_release) {
				FPGA_MSG("port_release is NULL");
				goto out_EINVAL;
			}
			if (port_release->argsz != sizeof(*port_release)) {
				FPGA_MSG("wrong structure size");
				goto out_EINVAL;
			}
			if (port_release->flags != 0) {
				FPGA_MSG("unexpected flags %u", port_release->flags);
				goto out_EINVAL;
			}
			if (port_release->port_id != 0) {
				FPGA_MSG("unexpected port ID %u", port_release->port_id);
				goto out_EINVAL;
			}
			retval = 0;
			errno = 0;
			break;
		case FPGA_FME_PORT_PR:
			FPGA_DBG("got FPGA_FME_PORT_PR");
			struct fpga_fme_port_pr *pr = va_arg(argp, struct fpga_fme_port_pr *);
			if (!pr) {
				FPGA_MSG("pr is NULL");
				goto out_EINVAL;
			}
			if (pr->argsz != sizeof(*pr)) {
				FPGA_MSG("wrong structure size");
				goto out_EINVAL;
			}
			if (pr->flags != 0) {
				FPGA_MSG("unexpected flags %u", pr->flags);
				goto out_EINVAL;
			}
			if (pr->port_id != 0) {
				FPGA_MSG("unexpected port ID %u", pr->port_id);
				goto out_EINVAL;
			}
			if (pr->buffer_size == 0) {
				FPGA_MSG("buffer size is 0");
				goto out_EINVAL;
			}
			if (!pr->buffer_address) {
				FPGA_MSG("buffer address is NULL");
				goto out_EINVAL;
			}
			pr->status = 0; /* return success */
			/* TODO: reflect reconfiguration (change afu_id?) */
			/* generate hash for bitstream data */
			hash = stupid_hash((uint32_t*)pr->buffer_address, pr->buffer_size / 4);
			/* write hash to file in tmp */
			strncpy_s(hashfilename, MAX_STRLEN, mock_devs[fd].pathname, strlen(mock_devs[fd].pathname) + 1);
			strcat_s(hashfilename, MAX_STRLEN, HASH_SUFFIX);

			FILE* hashfile = fopen(hashfilename, "w");
			if (hashfile) {
		                fwrite(&hash, sizeof(hash), 1, hashfile);
				fclose(hashfile);
			}
			retval = 0;
			errno = 0;
			break;
		case FPGA_FME_PORT_ASSIGN:
			FPGA_DBG("got FPGA_FME_PORT_ASSIGN");
			struct fpga_fme_port_assign *port_assign =
				va_arg(argp, struct fpga_fme_port_assign *);
			if (!port_assign) {
				FPGA_MSG("port_assign is NULL");
				goto out_EINVAL;
			}
			if (port_assign->argsz != sizeof(*port_assign)) {
				FPGA_MSG("wrong structure size");
				goto out_EINVAL;
			}
			if (port_assign->flags != 0) {
				FPGA_MSG("unexpected flags %u", port_assign->flags);
				goto out_EINVAL;
			}
			if (port_assign->port_id != 0) {
				FPGA_MSG("unexpected port ID %u", port_assign->port_id);
				goto out_EINVAL;
			}
			retval = 0;
			errno = 0;
			break;
		case FPGA_FME_GET_INFO:
			FPGA_DBG("got FPGA_FME_GET_INFO");
			struct fpga_fme_info *fme_info =
				va_arg(argp, struct fpga_fme_info *);
			if (!fme_info) {
				FPGA_MSG("fme_info is NULL");
				goto out_EINVAL;
			}
			if (fme_info->argsz != sizeof(*fme_info)) {
				FPGA_MSG("wrong structure size");
				goto out_EINVAL;
			}
			if (fme_info->flags != 0) {
				FPGA_MSG("unexpected flags %u", fme_info->flags);
				goto out_EINVAL;
			}
			if (fme_info->capability != 0) {
				FPGA_MSG("unexpected capability %u", fme_info->capability);
				goto out_EINVAL;
			}
			fme_info->capability = gEnableIRQ ? FPGA_FME_CAP_ERR_IRQ : 0;
			retval = 0;
			errno = 0;
			break;
		case FPGA_FME_ERR_SET_IRQ:
			FPGA_DBG("got FPGA_FME_ERR_SET_IRQ");
			struct fpga_fme_err_irq_set *fme_irq =
				va_arg(argp, struct fpga_fme_err_irq_set *);
			if (!fme_irq) {
				FPGA_MSG("fme_irq is NULL");
				goto out_EINVAL;
			}
			if (fme_irq->argsz != sizeof(*fme_irq)) {
				FPGA_MSG("wrong structure size");
				goto out_EINVAL;
			}
			if (fme_irq->flags != 0) {
				FPGA_MSG("unexpected flags %u", fme_irq->flags);
				goto out_EINVAL;
			}
			if (gEnableIRQ && fme_irq->evtfd >= 0) {
				uint64_t data = 1;
				// Write to the eventfd to signal one IRQ event.
				if (write(fme_irq->evtfd, &data, sizeof(data)) != sizeof(data)) {
					FPGA_ERR("IRQ write < 8 bytes");
				}
			}
			retval = 0;
			errno = 0;
			break;
		default:
			FPGA_DBG("Unknown FME IOCTL request %lu", request);
			break;
		}
		break;
	case FPGA_ACCELERATOR: /* PORT */
		switch (request) {
		case FPGA_PORT_DMA_MAP:
			FPGA_DBG("got FPGA_PORT_DMA_MAP");
			struct fpga_port_dma_map *dma_map = va_arg(argp, struct fpga_port_dma_map *);
			if (!dma_map) {
				FPGA_MSG("dma_map is NULL");
				goto out_EINVAL;
			}
			if (dma_map->argsz != sizeof(*dma_map)) {
				FPGA_MSG("wrong structure size");
				goto out_EINVAL;
			}
			if (!dma_map->user_addr) {
				FPGA_MSG("mapping address is NULL");
				goto out_EINVAL;
			}
			/* TODO: check alignment */
			if (dma_map->length == 0) {
				FPGA_MSG("mapping size is 0");
				goto out_EINVAL;
			}
			dma_map->iova = FPGA_MOCK_IOVA; /* return something */
			retval = 0;
			errno = 0;
			break;
		case FPGA_PORT_DMA_UNMAP:
			FPGA_DBG("got FPGA_PORT_DMA_UNMAP");
			struct fpga_port_dma_unmap *dma_unmap = va_arg(argp, struct fpga_port_dma_unmap *);
			if (!dma_unmap) {
				FPGA_MSG("dma_unmap is NULL");
				goto out_EINVAL;
			}
			if (dma_unmap->argsz != sizeof(*dma_unmap)) {
				FPGA_MSG("wrong structure size");
				goto out_EINVAL;
			}
			if (dma_unmap->iova != FPGA_MOCK_IOVA) {
				FPGA_MSG("unexpected IOVA (0x%llx)", dma_unmap->iova);
				goto out_EINVAL;
			}
			retval = 0;
			errno = 0;
			break;
		case FPGA_PORT_RESET:
			FPGA_DBG("got FPGA_PORT_RESET");
			retval = 0;
			break;
		case FPGA_PORT_GET_REGION_INFO:
			FPGA_DBG("got FPGA_PORT_GET_REGION_INFO");
			struct fpga_port_region_info *rinfo = va_arg(argp, struct fpga_port_region_info *);
			if (!rinfo) {
				FPGA_MSG("rinfo is NULL");
				goto out_EINVAL;
			}
			if (rinfo->argsz != sizeof(*rinfo)) {
				FPGA_MSG("wrong structure size");
				goto out_EINVAL;
			}
			if (rinfo->index != 0) {
				FPGA_MSG("unsupported MMIO index");
				goto out_EINVAL;
			}
			if (rinfo->padding != 0) {
				FPGA_MSG("unsupported padding");
				goto out_EINVAL;
			}
			rinfo->flags = FPGA_REGION_READ | FPGA_REGION_WRITE | FPGA_REGION_MMAP;
			rinfo->size = 0x40000;
			rinfo->offset = 0;
			retval = 0;
			errno = 0;
			break;
		case FPGA_PORT_GET_INFO:
			FPGA_DBG("got FPGA_PORT_GET_INFO");
			struct fpga_port_info *pinfo = va_arg(argp, struct fpga_port_info *);
			if (!pinfo) {
				FPGA_MSG("pinfo is NULL");
				goto out_EINVAL;
			}
			if (pinfo->argsz != sizeof(*pinfo)) {
				FPGA_MSG("wrong structure size");
				goto out_EINVAL;
			}
			pinfo->flags = 0;
			pinfo->num_regions = 1;
			pinfo->num_umsgs = 8;
			if (gEnableIRQ) {
				pinfo->capability = FPGA_PORT_CAP_ERR_IRQ | FPGA_PORT_CAP_UAFU_IRQ;
				pinfo->num_uafu_irqs = 1;
			} else {
				pinfo->capability = 0;
				pinfo->num_uafu_irqs = 0;
			}
			retval = 0;
			errno = 0;
			break;

		case FPGA_PORT_ERR_SET_IRQ:
			FPGA_DBG("got FPGA_PORT_ERR_SET_IRQ");
			struct fpga_port_err_irq_set *port_irq =
				va_arg(argp, struct fpga_port_err_irq_set *);
			if (!port_irq) {
				FPGA_MSG("port_irq is NULL");
				goto out_EINVAL;
			}
			if (port_irq->argsz != sizeof(*port_irq)) {
				FPGA_MSG("wrong structure size");
				goto out_EINVAL;
			}
			if (port_irq->flags != 0) {
				FPGA_MSG("unexpected flags %u", port_irq->flags);
				goto out_EINVAL;
			}
			if (gEnableIRQ && port_irq->evtfd >= 0) {
				uint64_t data = 1;
				// Write to the eventfd to signal one IRQ event.
				if (write(port_irq->evtfd, &data, sizeof(data)) != sizeof(data)) {
					FPGA_ERR("IRQ write < 8 bytes");
				}
			}
			retval = 0;
			errno = 0;
			break;
		case FPGA_PORT_UAFU_SET_IRQ:
			FPGA_DBG("got FPGA_PORT_UAFU_SET_IRQ");
			struct fpga_port_uafu_irq_set *uafu_irq =
				va_arg(argp, struct fpga_port_uafu_irq_set *);
			if (!uafu_irq) {
				FPGA_MSG("uafu_irq is NULL");
				goto out_EINVAL;
			}
			if (uafu_irq->argsz < sizeof(*uafu_irq)) {
				FPGA_MSG("wrong structure size");
				goto out_EINVAL;
			}
			if (uafu_irq->flags != 0) {
				FPGA_MSG("unexpected flags %u", uafu_irq->flags);
				goto out_EINVAL;
			}
			if (gEnableIRQ) {
				uint32_t i;
				uint64_t data = 1;
				// Write to each eventfd to signal one IRQ event.
				for (i = 0 ; i < uafu_irq->count ; ++i) {
					if (uafu_irq->evtfd[i] >= 0)
						if (write(uafu_irq->evtfd[i], &data, sizeof(data)) !=
								sizeof(data)) {
							FPGA_ERR("IRQ write < 8 bytes");
						}
				}
			}
			retval = 0;
			errno = 0;
			break;
		case FPGA_PORT_UMSG_SET_MODE:
			FPGA_DBG("got FPGA_PORT_UMSG_SET_MODE");
			struct fpga_port_umsg_cfg *ucfg = va_arg(argp, struct fpga_port_umsg_cfg *);
			if (!ucfg) {
				FPGA_MSG("ucfg is NULL");
				goto out_EINVAL;
			}
			if (ucfg->argsz != sizeof(*ucfg)) {
				FPGA_MSG("wrong structure size");
				goto out_EINVAL;
			}
			if (ucfg->flags != 0) {
				FPGA_MSG("unexpected flags %u", ucfg->flags);
				goto out_EINVAL;
			}
			/* TODO: check hint_bitmap */
			if (ucfg->hint_bitmap >> FPGA_MOCK_NUM_UMSGS) {
				FPGA_MSG("invalid hint_bitmap 0x%x", ucfg->hint_bitmap);
				goto out_EINVAL;
			}
			retval = 0;
			errno = 0;
			break;
		case FPGA_PORT_UMSG_SET_BASE_ADDR:
			FPGA_DBG("got FPGA_PORT_UMSG_SET_BASE_ADDR");
			struct fpga_port_umsg_base_addr *ubase = va_arg(argp, struct fpga_port_umsg_base_addr *);
			if (!ubase) {
				FPGA_MSG("ubase is NULL");
				goto out_EINVAL;
			}
			if (ubase->argsz != sizeof(*ubase)) {
				FPGA_MSG("wrong structure size");
				goto out_EINVAL;
			}
			if (ubase->flags != 0) {
				FPGA_MSG("unexpected flags %u", ubase->flags);
				goto out_EINVAL;
			}
			/* TODO: check iova */
			retval = 0;
			errno = 0;
			break;
		case FPGA_PORT_UMSG_ENABLE:
			FPGA_DBG("got FPGA_PORT_UMSG_ENABLE");
			retval = 0;
			break;
		case FPGA_PORT_UMSG_DISABLE:
			FPGA_DBG("got FPGA_PORT_UMSG_DISABLE");
			retval = 0;
			break;
		default:
			FPGA_DBG("Unknown PORT IOCTL request %lu", request);
			break;
		}
		break;
	}

out:
	va_end(argp);
	return retval;

out_EINVAL:
	retval = -1;
	errno = EINVAL;
	goto out;
}

struct afu_header {
    uint64_t afu_dfh;
    uint64_t afu_id_l;
    uint64_t afu_id_h;
} __attribute__((packed));

int open(const char* pathname, int flags, ...) {
    int fd;
    char path[MAX_STRLEN];
    char* err;
    int prefix_len = strlen(FPGA_DEV_PATH);
    va_list argp;

    dlerror(); /* clear errors */
    open_func real_open = (open_func)dlsym(RTLD_NEXT, "open");
    assert(real_open);
    err = dlerror();
    if (err) {
        FPGA_ERR("dlsym() failed: %s", err);
        errno = EINVAL;
        return -1;
    }

    FPGA_DBG("open(\"%s\", %i)", pathname, flags);

    if (strncmp(FPGA_DEV_PATH "/" FPGA_FME_DEV_PREFIX, pathname, prefix_len + strlen(FPGA_FME_DEV_PREFIX) - 2) == 0 ) {
        FPGA_DBG("accessing FME device");
        /* rewrite path */
        strncpy_s(path, sizeof(path), FPGA_MOCK_DEV_PATH, prefix_len);
        strncpy_s(path + prefix_len, sizeof(path) - prefix_len,
                  pathname + prefix_len, (MAX_STRLEN - 1 - prefix_len));
        /* call real open */
        FPGA_DBG("-> open(\"%s\", %i)", path, flags);
        fd = real_open(path, flags);
        /* store info */
        strncpy_s(mock_devs[fd].pathname, strlen(mock_devs[fd].pathname), path, MAX_STRLEN);
        mock_devs[fd].objtype = FPGA_DEVICE;
        mock_devs[fd].valid = 1;

    } else if (strncmp(FPGA_DEV_PATH "/" FPGA_PORT_DEV_PREFIX, pathname, prefix_len + 1 + strlen(FPGA_PORT_DEV_PREFIX)) == 0 ) {
        struct afu_header header;
        ssize_t sz;
        ssize_t res;

        FPGA_DBG("accessing PORT device");
        /* rewrite path */
        strncpy_s(path, sizeof(path), FPGA_MOCK_DEV_PATH, prefix_len);
        strncpy_s(path + prefix_len, sizeof(path) - prefix_len, pathname + prefix_len, MAX_STRLEN - prefix_len);
        /* call real open */
        FPGA_DBG("-> open(\"%s\", %i)", path, flags);
        fd = real_open(path, flags);
        if (fd < 0)
            return fd;
        /* store info */
        strncpy_s(mock_devs[fd].pathname, sizeof(mock_devs[fd].pathname), path, MAX_STRLEN - 1);
        mock_devs[fd].objtype = FPGA_ACCELERATOR;
        mock_devs[fd].valid = 1;

	/* Write the AFU header to offset 0, where the mmap call for CSR space 0 will point. */
        header.afu_dfh  = 0x1000000000001070ULL;
        header.afu_id_l = 0xf89e433683f9040bULL;
        header.afu_id_h = 0xd8424dc4a4a3c413ULL;

        lseek(fd, 0, SEEK_SET);

        sz = 0;
        do
        {
            res = write(fd, &header+sz, sizeof(header)-sz);
            if (res < 0)
                break;
            sz += res;
        } while((size_t)sz < sizeof(header));

        lseek(fd, 0, SEEK_SET);

    } else if (strncmp(SYSFS_FPGA_CLASS_PATH, pathname, strlen(SYSFS_FPGA_CLASS_PATH)) == 0 ) {

        /* rewrite path */
        rewrite_sysfs_path(pathname, path, MAX_STRLEN);
        /* call real open */
        FPGA_DBG("-> open(\"%s\", %i)", path, flags);
        fd = real_open(path, flags);

    } else {
        FPGA_DBG("-> open(\"%s\", %i)", pathname, flags);
        if (flags & O_CREAT){
            va_start(argp, flags);
            mode_t arg = va_arg(argp, mode_t);
            fd = ((open_mode_func)real_open)(pathname, flags, arg);
            va_end(argp);
        }else{
            fd = real_open(pathname, flags);
        }
    }

    return fd;
}

int close(int fd)
{
	int retval;
	char *err;

	dlerror(); /* clear errors */
	close_func real_close = (close_func)dlsym(RTLD_NEXT, "close");
	assert(real_close);
	err = dlerror();
	if (err) {
		FPGA_ERR("dlsym() failed: %s", err);
		errno = EINVAL;
		return -1;
	}

	FPGA_DBG("close(%i)", fd);
	retval = real_close(fd);
	if (retval >= 0 && fd < MAX_FD && mock_devs[fd].valid) {
		/* drop mock device */
		mock_devs[fd].valid = 0;
	}
	return retval;
}

DIR *opendir(const char *name)
{
	char *err;
	char s[MAX_STRLEN];

	dlerror(); /* clear errors */
	opendir_func real_opendir = (opendir_func)dlsym(RTLD_NEXT, "opendir");
	assert(real_opendir);
	err = dlerror();
	if (err) {
		FPGA_ERR("dlsym() failed: %s", err);
		errno = EINVAL;
		return NULL;
	}

	FPGA_DBG("opendir(%s)", name);
	rewrite_sysfs_path(name, s, MAX_STRLEN);
	FPGA_DBG("-> opendir(%s)", s);
	return real_opendir(s);
}

ssize_t readlink(const char *pathname, char *buf, size_t bufsiz)
{
	char *err;
	char s[MAX_STRLEN];

	dlerror(); /* clear errors */
	readlink_func real_readlink = (readlink_func)dlsym(RTLD_NEXT, "readlink");
	assert(real_readlink);
	err = dlerror();
	if (err) {
		FPGA_ERR("dlsym() failed: %s", err);
		errno = EINVAL;
		return -1;
	}

	FPGA_DBG("readlink(%s)", pathname);
	rewrite_sysfs_path(pathname, s, MAX_STRLEN);
	FPGA_DBG("-> readlink(%s)", s);
	return real_readlink(s, buf, bufsiz);
}

/* stat() redirects to __xstat() */
int __xstat(int ver, const char *pathname, struct stat *buf)
{
	char *err;
	char s[MAX_STRLEN];

	dlerror(); /* clear errors */
	__xstat_func real_xstat = (__xstat_func)dlsym(RTLD_NEXT, "__xstat");
	assert(real_xstat);
	err = dlerror();
	if (err) {
		FPGA_ERR("dlsym() failed: %s", err);
		errno = EINVAL;
		return -1;
	}

	FPGA_DBG("stat(%s)", pathname);
	rewrite_sysfs_path(pathname, s, MAX_STRLEN);
	FPGA_DBG("-> stat(%s)", s);
	return real_xstat(ver, s, buf);
}

/* lstat() redirects to __lxstat() */
int __lxstat(int ver, const char *pathname, struct stat *buf)
{
	char *err;
	char s[MAX_STRLEN];

	dlerror(); /* clear errors */
	__xstat_func real_lxstat = (__xstat_func)dlsym(RTLD_NEXT, "__lxstat");
	assert(real_lxstat);
	err = dlerror();
	if (err) {
		FPGA_ERR("dlsym() failed: %s", err);
		errno = EINVAL;
		return -1;
	}

	FPGA_DBG("lstat(%s)", pathname);
	rewrite_sysfs_path(pathname, s, MAX_STRLEN);
	FPGA_DBG("-> lstat(%s)", s);
	return real_lxstat(ver, s, buf);
}

fpga_result fpgaReconfigureSlot(fpga_handle fpga,
				uint32_t slot,
				const uint8_t *bitstream,
				size_t bitstream_len,
				int flags)
{
	(void)flags;  /* silence unused-parameter warning */

	if (!fpga ||
		(((struct _fpga_handle *)fpga)->magic != FPGA_HANDLE_MAGIC) ||
		(((struct _fpga_handle *)fpga)->fddev < 0)) {
		FPGA_MSG("Invalid handle object");
		return FPGA_INVALID_PARAM;
	}

	if (slot > 2) {
		FPGA_MSG("Invalid slot: %d", slot);
		return FPGA_INVALID_PARAM;
	}

	if (!bitstream) {
		FPGA_MSG("NULL bitstream pointer");
		return FPGA_INVALID_PARAM;
	}

	if (!bitstream_len) {
		FPGA_MSG("bitstream length is 0");
		return FPGA_INVALID_PARAM;
	}

    uint32_t hash = stupid_hash((uint32_t*)bitstream, bitstream_len / 4);

    char* hashfilename  = "/tmp/intel-fpga-fme.0.gbshash";
    FILE* hashfile = fopen(hashfilename, "w");
    if (hashfile) {
        fwrite(&hash, sizeof(hash), 1, hashfile);
        fclose(hashfile);
    }

    return FPGA_OK;
}
