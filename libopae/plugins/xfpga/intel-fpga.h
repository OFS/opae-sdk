/*
 * Header File for Intel FPGA User API
 *
 * Copyright 2016 Intel Corporation, Inc.
 *
 * Authors:
 *   Kang Luwei <luwei.kang@intel.com>
 *   Zhang Yi <yi.z.zhang@intel.com>
 *   Wu Hao <hao.wu@linux.intel.com>
 *   Xiao Guangrong <guangrong.xiao@linux.intel.com>
 *
 * This work is licensed under the terms of the GNU GPL version 2. See
 * the COPYING file in the top-level directory.
 *
 */

#ifndef _UAPI_INTEL_FPGA_H
#define _UAPI_INTEL_FPGA_H

#include <linux/types.h>

#define FPGA_API_VERSION 0

/*
 * The IOCTL interface for Intel FPGA is designed for extensibility by
 * embedding the structure length (argsz) and flags into structures passed
 * between kernel and userspace. This design referenced the VFIO IOCTL
 * interface (include/uapi/linux/vfio.h).
 */

#define FPGA_MAGIC 0xB5

#define FPGA_BASE 0
#define PORT_BASE 0x40
#define FME_BASE 0x80

/* Common IOCTLs for both FME and AFU file descriptor */

/**
 * FPGA_GET_API_VERSION - _IO(FPGA_MAGIC, FPGA_BASE + 0)
 *
 * Report the version of the driver API.
 * Return: Driver API Version.
 */

#define FPGA_GET_API_VERSION	_IO(FPGA_MAGIC, FPGA_BASE + 0)

/**
 * FPGA_CHECK_EXTENSION - _IO(FPGA_MAGIC, FPGA_BASE + 1)
 *
 * Check whether an extension is supported.
 * Return: 0 if not supported, otherwise the extension is supported.
 */

#define FPGA_CHECK_EXTENSION	_IO(FPGA_MAGIC, FPGA_BASE + 1)

/* IOCTLs for AFU file descriptor */

/**
 * FPGA_PORT_RESET - _IO(FPGA_MAGIC, PORT_BASE + 0)
 *
 * Reset the FPGA AFU Port. No parameters are supported.
 * Return: 0 on success, -errno of failure
 */

#define FPGA_PORT_RESET		_IO(FPGA_MAGIC, PORT_BASE + 0)

/**
 * FPGA_PORT_GET_INFO - _IOR(FPGA_MAGIC, PORT_BASE + 1, struct fpga_port_info)
 *
 * Retrieve information about the fpga port.
 * Driver fills the info in provided struct fpga_port_info.
 * Return: 0 on success, -errno on failure.
 */
struct fpga_port_info {
	/* Input */
	__u32 argsz;		/* Structure length */
	/* Output */
	__u32 flags;		/* Zero for now */
	__u32 capability;	/* The capability of port device */
#define FPGA_PORT_CAP_ERR_IRQ	(1 << 0) /* Support port error interrupt */
#define FPGA_PORT_CAP_UAFU_IRQ	(1 << 1) /* Support uafu error interrupt */
	__u32 num_regions;	/* The number of supported regions */
	__u32 num_umsgs;	/* The number of allocated umsgs */
	__u32 num_uafu_irqs;    /* The number of uafu interrupts */
};

#define FPGA_PORT_GET_INFO	_IO(FPGA_MAGIC, PORT_BASE + 1)

/**
 * FPGA_PORT_GET_REGION_INFO - _IOWR(FPGA_MAGIC, PORT_BASE + 2,
 *						struct fpga_port_region_info)
 *
 * Retrieve information about a device region.
 * Caller provides struct fpga_port_region_info with index value set.
 * Driver returns the region info in other fields.
 * Return: 0 on success, -errno on failure.
 */
struct fpga_port_region_info {
	/* input */
	__u32 argsz;		/* Structure length */
	/* Output */
	__u32 flags;		/* Access permission */
#define FPGA_REGION_READ	(1 << 0)	/* Region is readable */
#define FPGA_REGION_WRITE	(1 << 1)	/* Region is writable */
#define FPGA_REGION_MMAP	(1 << 2)	/* Can be mmaped to userspace */
	/* Input */
	__u32 index;		/* Region index */
#define FPGA_PORT_INDEX_UAFU	0		/* User AFU */
#define FPGA_PORT_INDEX_STP	1		/* Signal Tap */
	__u32 padding;
	/* Output */
	__u64 size;		/* Region size (bytes) */
	__u64 offset;		/* Region offset from start of device fd */
};

#define FPGA_PORT_GET_REGION_INFO	_IO(FPGA_MAGIC, PORT_BASE + 2)

/**
 * FPGA_PORT_DMA_MAP - _IOWR(FPGA_MAGIC, PORT_BASE + 3,
 *						struct fpga_port_dma_map)
 *
 * Map the dma memory per user_addr and length which are provided by caller.
 * Driver fills the iova in provided struct afu_port_dma_map.
 * This interface only accepts page-size aligned user memory for dma mapping.
 * Return: 0 on success, -errno on failure.
 */
struct fpga_port_dma_map {
	/* Input */
	__u32 argsz;		/* Structure length */
	__u32 flags;		/* Zero for now */
	__u64 user_addr;        /* Process virtual address */
	__u64 length;           /* Length of mapping (bytes)*/
	/* Output */
	__u64 iova;             /* IO virtual address */
};

#define FPGA_PORT_DMA_MAP	_IO(FPGA_MAGIC, PORT_BASE + 3)

/**
 * FPGA_PORT_DMA_UNMAP - _IOW(FPGA_MAGIC, PORT_BASE + 4,
 *						struct fpga_port_dma_unmap)
 *
 * Unmap the dma memory per iova provided by caller.
 * Return: 0 on success, -errno on failure.
 */
struct fpga_port_dma_unmap {
	/* Input */
	__u32 argsz;		/* Structure length */
	__u32 flags;		/* Zero for now */
	__u64 iova;		/* IO virtual address */
};

#define FPGA_PORT_DMA_UNMAP	_IO(FPGA_MAGIC, PORT_BASE + 4)

/**
 * FPGA_PORT_UMSG_ENABLE - _IO(FPGA_MAGIC, PORT_BASE + 5)
 * FPGA_PORT_UMSG_DISABLE - _IO(FPGA_MAGIC, PORT_BASE + 6)
 *
 * Interfaces to control UMSG function. No parameters are supported.
 * Return: 0 on success, -errno on failure.
 */

#define FPGA_PORT_UMSG_ENABLE	_IO(FPGA_MAGIC, PORT_BASE + 5)
#define FPGA_PORT_UMSG_DISABLE	_IO(FPGA_MAGIC, PORT_BASE + 6)

/**
 * FPGA_PORT_UMSG_SET_MODE - _IOW(FPGA_MAGIC, PORT_BASE + 7,
 *						struct fpga_port_umsg_cfg)
 *
 * Set Hint Mode per bitmap provided by caller. One bit for each page
 * in hint_bitmap. 0 - Disable and 1 - Enable Hint Mode.
 * Return: 0 on success, -errno on failure.
 */
struct fpga_port_umsg_cfg {
	/* Input */
	__u32 argsz;		/* Structure length */
	__u32 flags;		/* Zero for now */
	__u32 hint_bitmap;	/* UMSG Hint Mode Bitmap */
};

#define FPGA_PORT_UMSG_SET_MODE		_IO(FPGA_MAGIC, PORT_BASE + 7)

/**
 * FPGA_PORT_UMSG_SET_BASE_ADDR - _IOW(FPGA_MAGIC, PORT_BASE + 8,
 *						struct afu_port_umsg_base_addr)
 *
 * Set UMSG base address per iova provided by caller. Driver configures the
 * UMSG base address with the iova, but only accept iova which get from the
 * DMA_MAP IOCTL interface and the DMA region is big enough for all UMSGs
 * (num_umsg * PAGE_SIZE)
 * Return: 0 on success, -errno on failure.
 */
struct fpga_port_umsg_base_addr {
	/* Input */
	__u32 argsz;		/* Structure length */
	__u32 flags;		/* Zero for now */
	__u64 iova;		/* IO virtual address */
};

#define FPGA_PORT_UMSG_SET_BASE_ADDR	_IO(FPGA_MAGIC, PORT_BASE + 8)

/**
 * FPGA_PORT_ERR_SET_IRQ - _IOW(FPGA_MAGIC, PORT_BASE + 9,
 *                                             struct fpga_port_err_irq_set)
 *
 * Set fpga port global error interrupt eventfd
 * Return: 0 on success, -errno on failure.
 */
struct fpga_port_err_irq_set {
	/* Input */
	__u32 argsz;		/* Structure length */
	__u32 flags;		/* Zero for now */
	__s32 evtfd;		/* Eventfd handler */
};

#define FPGA_PORT_ERR_SET_IRQ		_IO(FPGA_MAGIC, PORT_BASE + 9)

/**
 * FPGA_PORT_UAFU_SET_IRQ - _IOW(FPGA_MAGIC, PORT_BASE + 10,
 *                                             struct fpga_port_uafu_irq_set)
 *
 * Set fpga UAFU interrupt eventfd
 * Return: 0 on success, -errno on failure.
 */
struct fpga_port_uafu_irq_set {
	/* Input */
	__u32 argsz;		/* Structure length */
	__u32 flags;		/* Zero for now */
	__u32 start;		/* First irq number */
	__u32 count;		/* The number of eventfd handler */
	__s32 evtfd[];		/* Eventfd handler */
};

#define FPGA_PORT_UAFU_SET_IRQ		_IO(FPGA_MAGIC, PORT_BASE + 10)

/* IOCTLs for FME file descriptor */

/**
 * FPGA_FME_PORT_PR - _IOWR(FPGA_MAGIC, FME_BASE + 0, struct fpga_fme_port_pr)
 *
 * Driver does Partial Reconfiguration based on Port ID and Buffer (Image)
 * provided by caller.
 * Return: 0 on success, -errno on failure.
 * If FPGA_FME_PORT_PR returns -EIO, that indicates the HW has detected
 * some errors during PR, under this case, the user can fetch HW error code
 * from fpga_fme_port_pr.status. Each bit on the error code is used as the
 * index for the array created by DEFINE_FPGA_PR_ERR_MSG().
 * Otherwise, it is always zero.
 */

#define DEFINE_FPGA_PR_ERR_MSG(_name_)			\
static const char * const _name_[] = {			\
	"PR operation error detected",			\
	"PR CRC error detected",			\
	"PR incompatiable bitstream error detected",	\
	"PR IP protocol error detected",		\
	"PR FIFO overflow error detected",		\
	"PR timeout error detected",			\
	"PR secure load error detected",		\
}

#define PR_MAX_ERR_NUM	7

struct fpga_fme_port_pr {
	/* Input */
	__u32 argsz;		/* Structure length */
	__u32 flags;		/* Zero for now */
	__u32 port_id;
	__u32 buffer_size;
	__u64 buffer_address;	/* Userspace address to the buffer for PR */
	/* Output */
	__u64 status;		/* HW error code if ioctl returns -EIO */
};

#define FPGA_FME_PORT_PR	_IO(FPGA_MAGIC, FME_BASE + 0)

/**
 * FPGA_FME_PORT_RELEASE - _IOW(FPGA_MAGIC, FME_BASE + 1,
 *						struct fpga_fme_port_release)
 *
 * Driver releases the port per Port ID provided by caller.
 * Return: 0 on success, -errno on failure.
 */
struct fpga_fme_port_release {
	/* Input */
	__u32 argsz;		/* Structure length */
	__u32 flags;		/* Zero for now */
	__u32 port_id;
};

#define FPGA_FME_PORT_RELEASE	_IO(FPGA_MAGIC, FME_BASE + 1)

/**
 * FPGA_FME_PORT_ASSIGN - _IOW(FPGA_MAGIC, FME_BASE + 2,
 *						struct fpga_fme_port_assign)
 *
 * Driver assigns the port per Port ID provided by caller.
 * Return: 0 on success, -errno on failure.
 */
struct fpga_fme_port_assign {
	/* Input */
	__u32 argsz;		/* Structure length */
	__u32 flags;		/* Zero for now */
	__u32 port_id;
};

#define FPGA_FME_PORT_ASSIGN	_IO(FPGA_MAGIC, FME_BASE + 2)

/**
 * FPGA_FME_GET_INFO - _IOR(FPGA_MAGIC, FME_BASE + 3, struct fpga_fme_info)
 *
 * Retrieve information about the fpga fme.
 * Driver fills the info in provided struct fpga_fme_info.
 * Return: 0 on success, -errno on failure.
 */
struct fpga_fme_info {
	/* Input */
	__u32 argsz;		/* Structure length */
	/* Output */
	__u32 flags;		/* Zero for now */
	__u32 capability;	/* The capability of FME device */
#define FPGA_FME_CAP_ERR_IRQ	(1 << 0) /* Support fme error interrupt */
};

#define FPGA_FME_GET_INFO      _IO(FPGA_MAGIC, FME_BASE + 3)

/**
 * FPGA_FME_ERR_SET_IRQ - _IOW(FPGA_MAGIC, FME_BASE + 4,
 *                                             struct fpga_fme_err_irq_set)
 *
 * Set fpga fme global error interrupt eventfd
 * Return: 0 on success, -errno on failure.
 */
struct fpga_fme_err_irq_set {
	/* Input */
	__u32 argsz;		/* Structure length */
	__u32 flags;		/* Zero for now */
	__s32 evtfd;		/* Eventfd handler */
};

#define FPGA_FME_ERR_SET_IRQ	_IO(FPGA_MAGIC, FME_BASE + 4)

#endif /* _UAPI_INTEL_FPGA_H */
