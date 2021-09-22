// Copyright(c) 2017-2021, Intel Corporation
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include "xfpga.h"
#include "common_int.h"
#include "error_int.h"
#include "props.h"
#include "opae_drv.h"


struct dev_list {
	fpga_token_header hdr;

	char sysfspath[SYSFS_PATH_MAX];
	char devpath[DEV_PATH_MAX];
	uint8_t socket_id;

	uint32_t fpga_num_slots;
	uint64_t fpga_bitstream_id;
	fpga_version fpga_bbs_version;

	fpga_accelerator_state accelerator_state;
	uint32_t accelerator_num_mmios;
	uint32_t accelerator_num_irqs;
	struct dev_list *next;
	struct dev_list *parent;
	struct dev_list *fme;
};

STATIC bool matches_filter(const struct dev_list *attr, const fpga_properties filter)
{
	struct _fpga_properties *_filter = (struct _fpga_properties *)filter;
	bool res = true;
	int err = 0;

	if (pthread_mutex_lock(&_filter->lock)) {
		OPAE_MSG("Failed to lock filter mutex");
		return false;
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_PARENT)) {
		fpga_token_header *parent_hdr =
			(fpga_token_header *)_filter->parent;

		if (!parent_hdr) {
			res = false; // Reject search based on NULL parent token
			goto out_unlock;
		}

		if (!fpga_is_parent_child(parent_hdr,
					  &attr->hdr)) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJTYPE)) {
		if (_filter->objtype != attr->hdr.objtype) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_SEGMENT)) {
		if (_filter->segment != attr->hdr.segment) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_BUS)) {
		if (_filter->bus != attr->hdr.bus) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_DEVICE)) {
		if (_filter->device != attr->hdr.device) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_FUNCTION)) {
		if (_filter->function != attr->hdr.function) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_SOCKETID)) {
		if (_filter->socket_id != attr->socket_id) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_GUID)) {
		if (0 != memcmp(attr->hdr.guid, _filter->guid, sizeof(fpga_guid))) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJECTID)) {
		if (_filter->object_id != attr->hdr.object_id) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_VENDORID)) {
		if (_filter->vendor_id != attr->hdr.vendor_id) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_DEVICEID)) {
		if (_filter->device_id != attr->hdr.device_id) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_ERRORS)) {
		uint32_t errors;
		char errpath[SYSFS_PATH_MAX] = { 0, };

		if (snprintf(errpath, sizeof(errpath),
			     "%s/errors", attr->sysfspath) < 0) {
			OPAE_ERR("snprintf buffer overflow");
			res = false;
			goto out_unlock;
		}

		errors = count_error_files(errpath);
		if (errors != _filter->num_errors) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_INTERFACE)) {
		if (_filter->interface != attr->hdr.interface) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJTYPE)
	    && (FPGA_DEVICE == _filter->objtype)) {

		if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_SLOTS)) {
			if ((FPGA_DEVICE != attr->hdr.objtype)
			    || (attr->fpga_num_slots
				!= _filter->u.fpga.num_slots)) {
				res = false;
				goto out_unlock;
			}
		}

		if (FIELD_VALID(_filter, FPGA_PROPERTY_BBSID)) {
			if ((FPGA_DEVICE != attr->hdr.objtype)
			    || (attr->fpga_bitstream_id
				!= _filter->u.fpga.bbs_id)) {
				res = false;
				goto out_unlock;
			}
		}

		if (FIELD_VALID(_filter, FPGA_PROPERTY_BBSVERSION)) {
			if ((FPGA_DEVICE != attr->hdr.objtype)
			    || (attr->fpga_bbs_version.major
				!= _filter->u.fpga.bbs_version.major)
			    || (attr->fpga_bbs_version.minor
				!= _filter->u.fpga.bbs_version.minor)
			    || (attr->fpga_bbs_version.patch
				!= _filter->u.fpga.bbs_version.patch)) {
				res = false;
				goto out_unlock;
			}
		}

	} else if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJTYPE)
		   && (FPGA_ACCELERATOR == _filter->objtype)) {

		if (FIELD_VALID(_filter, FPGA_PROPERTY_ACCELERATOR_STATE)) {
			if ((FPGA_ACCELERATOR != attr->hdr.objtype)
			    || (attr->accelerator_state
				!= _filter->u.accelerator.state)) {
				res = false;
				goto out_unlock;
			}
		}

		if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_MMIO)) {
			if ((FPGA_ACCELERATOR != attr->hdr.objtype)
			    || (attr->accelerator_num_mmios
				!= _filter->u.accelerator.num_mmio)) {
				res = false;
				goto out_unlock;
			}
		}

		if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_INTERRUPTS)) {
			if ((FPGA_ACCELERATOR != attr->hdr.objtype)
			    || (attr->accelerator_num_irqs
				!= _filter->u.accelerator.num_interrupts)) {
				res = false;
				goto out_unlock;
			}
		}
	}

out_unlock:
	err = pthread_mutex_unlock(&_filter->lock);
	if (err) {
		OPAE_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
	}
	return res;
}

STATIC bool matches_filters(const struct dev_list *attr, const fpga_properties *filter,
		     uint32_t num_filter)
{
	uint32_t i;

	if (!num_filter) // no filter == match everything
		return true;

	for (i = 0; i < num_filter; ++i) {
		if (matches_filter(attr, filter[i])) {
			return true;
		}
	}
	return false;
}

STATIC struct dev_list *add_dev(const char *sysfspath, const char *devpath,
				struct dev_list *parent)
{
	struct dev_list *pdev;
	size_t len;

	pdev = (struct dev_list *)calloc(1, sizeof(*pdev));
	if (NULL == pdev)
		return NULL;

	len = strnlen(sysfspath, sizeof(pdev->sysfspath) - 1);
	memcpy(pdev->sysfspath, sysfspath, len);
	pdev->sysfspath[len] = '\0';

	len = strnlen(devpath, sizeof(pdev->devpath) - 1);
	memcpy(pdev->devpath, devpath, len);
	pdev->devpath[len] = '\0';

	pdev->next = parent->next;
	parent->next = pdev;

	pdev->parent = parent;

	return pdev;
}

STATIC fpga_result enum_fme(const char *sysfspath, const char *name,
			    struct dev_list *parent)
{
	char devpath[DEV_PATH_MAX];
	struct dev_list *pdev;

	if (snprintf(devpath, sizeof(devpath),
		     FPGA_DEV_PATH "/%s", name) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

	pdev = add_dev(sysfspath, devpath, parent);
	if (!pdev) {
		OPAE_MSG("Failed to allocate device");
		return FPGA_NO_MEMORY;
	}

	pdev->hdr.magic = FPGA_TOKEN_MAGIC;
	pdev->hdr.vendor_id = parent->hdr.vendor_id;
	pdev->hdr.device_id = parent->hdr.device_id;
	pdev->hdr.segment = parent->hdr.segment;
	pdev->hdr.bus = parent->hdr.bus;
	pdev->hdr.device = parent->hdr.device;
	pdev->hdr.function = parent->hdr.function;
	pdev->hdr.interface = FPGA_IFC_DFL;
	pdev->hdr.objtype = FPGA_DEVICE;

	parent->fme = pdev->fme = pdev;

	return FPGA_OK;
}

STATIC fpga_result sync_fme(struct dev_list *fme)
{
	struct stat stats;
	uint64_t value = 0;

	// The sysfspath must be a directory.
	if (stat(fme->sysfspath, &stats) ||
	    !S_ISDIR(stats.st_mode)) {
		OPAE_DBG("stat(%s) failed: %s",
			 fme->sysfspath, strerror(errno));
		return FPGA_NOT_FOUND;
	}

	// The device path must be a char device.
	if (stat(fme->devpath, &stats) ||
	    !S_ISCHR(stats.st_mode)) {
		OPAE_DBG("stat(%s) failed: %s",
			 fme->devpath, strerror(errno));
		return FPGA_NOT_FOUND;
	}

	if (sysfs_objectid_from_path(fme->sysfspath,
				     &fme->hdr.object_id) != FPGA_OK) {
		OPAE_DBG("Could not determine object_id from '%s'", fme->sysfspath);
		return FPGA_EXCEPTION;
	}

	// PR interface ID is the FME GUID.
	if (sysfs_get_fme_pr_interface_id(fme->sysfspath, fme->hdr.guid)) {
		OPAE_DBG("failed to read PR interface ID.");
	}

	value = 0;
	if (sysfs_path_is_valid(fme->sysfspath,
				FPGA_SYSFS_SOCKET_ID) == FPGA_OK) {
		sysfs_parse_attribute64(fme->sysfspath, FPGA_SYSFS_SOCKET_ID, &value);
	} else {
		OPAE_DBG("failed to read socket ID.");
	}
	fme->socket_id = fme->parent->socket_id = (uint8_t)value;

	value = 0;
	if (sysfs_path_is_valid(fme->sysfspath,
				FPGA_SYSFS_NUM_SLOTS) == FPGA_OK) {
		sysfs_parse_attribute64(fme->sysfspath, FPGA_SYSFS_NUM_SLOTS, &value);

	} else {
		OPAE_DBG("failed to read number of slots.");
	}
	fme->fpga_num_slots = (uint32_t)value;

	if (sysfs_path_is_valid(fme->sysfspath,
				FPGA_SYSFS_BITSTREAM_ID) == FPGA_OK) {
		sysfs_parse_attribute64(fme->sysfspath,
					FPGA_SYSFS_BITSTREAM_ID,
					&fme->fpga_bitstream_id);

		fme->fpga_bbs_version.major =
				FPGA_BBS_VER_MAJOR(fme->fpga_bitstream_id);
		fme->fpga_bbs_version.minor =
				FPGA_BBS_VER_MINOR(fme->fpga_bitstream_id);
		fme->fpga_bbs_version.patch =
				FPGA_BBS_VER_PATCH(fme->fpga_bitstream_id);
	} else {
		OPAE_DBG("failed to read bitstream ID.");
	}

	return FPGA_OK;
}

STATIC fpga_result sync_afu(struct dev_list *afu)
{
	struct stat stats;
	int res;
	char sysfspath[SYSFS_PATH_MAX];

	// The sysfspath must be a directory.
	if (stat(afu->sysfspath, &stats) ||
	    !S_ISDIR(stats.st_mode)) {
		OPAE_DBG("stat(%s) failed: %s",
			 afu->sysfspath, strerror(errno));
		return FPGA_NOT_FOUND;
	}

	// The device path must be a char device.
	if (stat(afu->devpath, &stats) ||
	    !S_ISCHR(stats.st_mode)) {
		OPAE_DBG("stat(%s) failed: %s",
			 afu->devpath, strerror(errno));
		return FPGA_NOT_FOUND;
	}

	if (afu->fme)
		afu->socket_id = afu->fme->socket_id;

	afu->accelerator_num_mmios = 0;
	afu->accelerator_num_irqs = 0;

	res = open(afu->devpath, O_RDWR);
	if (-1 == res) {
		afu->accelerator_state = FPGA_ACCELERATOR_ASSIGNED;
	} else {
		opae_port_info info = { 0, 0, 0, 0, 0 };

		if (opae_get_port_info(res, &info) == FPGA_OK) {
			afu->accelerator_num_mmios = info.num_regions;
			if (info.capability & OPAE_PORT_CAP_UAFU_IRQS)
				afu->accelerator_num_irqs = info.num_uafu_irqs;
		}

		close(res);

		afu->accelerator_state = FPGA_ACCELERATOR_UNASSIGNED;
	}

	if (sysfs_objectid_from_path(afu->sysfspath,
				     &afu->hdr.object_id) != FPGA_OK) {
		OPAE_DBG("Could not determine object_id from '%s'", afu->sysfspath);
		return FPGA_EXCEPTION;
	}

	// Discover the AFU GUID.
	if (snprintf(sysfspath, sizeof(sysfspath),
		     "%s/" FPGA_SYSFS_AFU_GUID, afu->sysfspath) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

	// If we can't read the afu_id, don't return a token.
	if (sysfs_read_guid(sysfspath, afu->hdr.guid) != FPGA_OK) {
	// TODO: undo this hack. It was put in place to deal with the lack of
	// afu_id in dfl-port.X during OFS Rel1.
#if 0
		OPAE_MSG("Could not read AFU ID from '%s', ignoring", sysfspath);
		return FPGA_EXCEPTION;
#endif
	}

	return FPGA_OK;
}

STATIC fpga_result enum_afu(const char *sysfspath, const char *name,
			    struct dev_list *parent)
{
	struct dev_list *pdev;
	char devpath[DEV_PATH_MAX];

	if (snprintf(devpath, sizeof(devpath),
		     FPGA_DEV_PATH "/%s", name) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

	pdev = add_dev(sysfspath, devpath, parent);
	if (!pdev) {
		OPAE_ERR("Failed to allocate device");
		return FPGA_NO_MEMORY;
	}

	pdev->hdr.magic = FPGA_TOKEN_MAGIC;
	pdev->hdr.vendor_id = parent->hdr.vendor_id;
	pdev->hdr.device_id = parent->hdr.device_id;
	pdev->hdr.segment = parent->hdr.segment;
	pdev->hdr.bus = parent->hdr.bus;
	pdev->hdr.device = parent->hdr.device;
	pdev->hdr.function = parent->hdr.function;
	pdev->hdr.interface = FPGA_IFC_DFL;
	pdev->hdr.objtype = FPGA_ACCELERATOR;

	pdev->fme = parent->fme;

	return FPGA_OK;
}

typedef struct _enum_region_ctx{
	struct dev_list *list;
	bool include_port;
} enum_region_ctx;

STATIC fpga_result enum_regions(const sysfs_fpga_device *device, void *context)
{
	enum_region_ctx *ctx = (enum_region_ctx *)context;
	fpga_result result = FPGA_OK;
	struct dev_list *pdev;

	pdev = ctx->list;
	while (pdev->next)
		pdev = pdev->next;

	pdev = add_dev(device->sysfs_path, "", pdev);
	if (!pdev) {
		OPAE_MSG("Failed to allocate device");
		return FPGA_NO_MEMORY;
	}

	pdev->hdr.magic = FPGA_TOKEN_MAGIC;
	pdev->hdr.vendor_id = device->vendor_id;
	pdev->hdr.device_id = device->device_id;
	pdev->hdr.segment = device->segment;
	pdev->hdr.bus = device->bus;
	pdev->hdr.device = device->device;
	pdev->hdr.function = device->function;

	// Enum fme
	if (device->fme) {
		result = enum_fme(device->fme->sysfs_path,
				  device->fme->sysfs_name, pdev);
		if (result != FPGA_OK) {
			OPAE_ERR("Failed to enum FME");
			return result;
		}
	}

	while (pdev->next)
		pdev = pdev->next;

	// Enum port
	if (device->port && ctx->include_port) {
		result = enum_afu(device->port->sysfs_path,
				  device->port->sysfs_name, pdev);
		if (result != FPGA_OK) {
			OPAE_ERR("Failed to enum PORT");
			return result;
		}
	}

	return FPGA_OK;
}

STATIC fpga_result enum_fpga_region_resources(struct dev_list *list,
				bool include_port)
{
	enum_region_ctx ctx = {.list = list, .include_port = include_port};

	return sysfs_foreach_device(enum_regions, &ctx);
}

/// Determine if filters require reading AFUs
///
/// Return true if any of the following conditions are met:
/// * The number of filters is zero
/// * At least one filter specifies FPGA_ACCELERATOR as object type
/// * At least one filter does NOT specify an object type
/// Return false otherwise
bool include_afu(const fpga_properties *filters, uint32_t num_filters)
{
	size_t i = 0;
	if (!num_filters)
		return true;
	for (i = 0; i < num_filters; ++i) {
		struct _fpga_properties *_filter =
			(struct _fpga_properties *)filters[i];
		if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJTYPE)) {
			if (_filter->objtype == FPGA_ACCELERATOR) {
				return true;
			}
		} else {
			return true;
		}
	}
	return false;
}

struct _fpga_token *token_add(struct dev_list *dev)
{
	struct _fpga_token *_tok = NULL;
	uint32_t device_instance;
	uint32_t subdev_instance;
	char *endptr = NULL;
	const char *ptr;
	size_t len;
	char errpath[SYSFS_PATH_MAX] = { 0, };

	//                                    11111111112
	//                          012345678901234567890
	//                          /fpga_region/regionDD/
	endptr = strstr(dev->sysfspath, "/fpga_region/");

	if (endptr) {
		// dfl driver
		ptr = (const char *)endptr + 18;

	} else {
		/* get the device instance id */
		ptr = strchr(dev->sysfspath, '.');
		if (!ptr) {
			OPAE_ERR("sysfspath does not meet expected format");
			return NULL;
		}
	}

	endptr = NULL;
	device_instance = strtoul(++ptr, &endptr, 10);
	/* no digits in path */
	if (!endptr || *endptr != '/') {
		OPAE_ERR("sysfspath does not meet expected format");
		return NULL;
	}

	/* get the sub-device (FME/Port) instance id */
	ptr = strrchr(dev->sysfspath, '.');
	if (!ptr) {
		OPAE_ERR("sysfspath does not meet expected format");
		return NULL;
	}

	endptr = NULL;
	subdev_instance = strtoul(++ptr, &endptr, 10);
	/* no digits in path */
	if (endptr != ptr + strlen(ptr)) {
		OPAE_ERR("sysfspath does not meet expected format");
		return NULL;
	}

	_tok = (struct _fpga_token *)malloc(sizeof(struct _fpga_token));
	if (!_tok) {
		OPAE_ERR("malloc failed");
		return NULL;
	}

	if (snprintf(errpath, sizeof(errpath),
		     "%s/errors", dev->sysfspath) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		free(_tok);
		return NULL;
	}

	_tok->errors = NULL;
	build_error_list(errpath, &_tok->errors);

	/* mark data structure as valid/populate header fields */
	_tok->hdr = dev->hdr;

	/* assign the instances num from above */
	_tok->device_instance = device_instance;
	_tok->subdev_instance = subdev_instance;

	/* deep copy token data */
	len = strnlen(dev->sysfspath, SYSFS_PATH_MAX - 1);
	memcpy(_tok->sysfspath, dev->sysfspath, len);
	_tok->sysfspath[len] = '\0';

	len = strnlen(dev->devpath, DEV_PATH_MAX - 1);
	memcpy(_tok->devpath, dev->devpath, len);
	_tok->devpath[len] = '\0';

	return _tok;
}

fpga_result __XFPGA_API__ xfpga_fpgaEnumerate(const fpga_properties *filters,
				       uint32_t num_filters, fpga_token *tokens,
				       uint32_t max_tokens,
				       uint32_t *num_matches)
{
	fpga_result result = FPGA_NOT_FOUND;

	struct dev_list head;
	struct dev_list *lptr;

	if (NULL == num_matches) {
		OPAE_MSG("num_matches is NULL");
		return FPGA_INVALID_PARAM;
	}

	/* requiring a max number of tokens, but not providing a pointer to
	 * return them through is invalid */
	if ((max_tokens > 0) && (NULL == tokens)) {
		OPAE_MSG("max_tokens > 0 with NULL tokens");
		return FPGA_INVALID_PARAM;
	}

	if ((num_filters > 0) && (NULL == filters)) {
		OPAE_MSG("num_filters > 0 with NULL filters");
		return FPGA_INVALID_PARAM;
	}

	if (!num_filters && (NULL != filters)) {
		OPAE_MSG("num_filters == 0 with non-NULL filters");
		return FPGA_INVALID_PARAM;
	}

	*num_matches = 0;

	memset(&head, 0, sizeof(head));

	// enum FPGA regions & resources
	result = enum_fpga_region_resources(&head,
				include_afu(filters, num_filters));

	if (result != FPGA_OK) {
		OPAE_MSG("No FPGA resources found");
		return result;
	}

	/* create and populate token data structures */
	for (lptr = head.next; NULL != lptr; lptr = lptr->next) {
		// Skip the "container" device list nodes.
		if (!lptr->devpath[0])
			continue;

		if (lptr->hdr.objtype == FPGA_DEVICE &&
		    sync_fme(lptr) != FPGA_OK) {
			continue;
		} else if (lptr->hdr.objtype == FPGA_ACCELERATOR &&
			   sync_afu(lptr) != FPGA_OK) {
			continue;
		}

		if (matches_filters(lptr, filters, num_filters)) {
			if (*num_matches < max_tokens) {

				tokens[*num_matches] = token_add(lptr);

				if (!tokens[*num_matches]) {
					uint32_t i;
					OPAE_ERR("Failed to allocate memory for token");
					result = FPGA_NO_MEMORY;

					for (i = 0 ; i < *num_matches ; ++i)
						free(tokens[i]);
					*num_matches = 0;

					goto out_free_trash;
				}

			}
			++(*num_matches);
		}
	}

out_free_trash:
	for (lptr = head.next; NULL != lptr;) {
		struct dev_list *trash = lptr;
		lptr = lptr->next;
		free(trash);
	}

	return result;
}

fpga_result __XFPGA_API__ xfpga_fpgaCloneToken(fpga_token src, fpga_token *dst)
{
	struct _fpga_token *_src = (struct _fpga_token *)src;
	struct _fpga_token *_dst;
	size_t len;

	if (NULL == src || NULL == dst) {
		OPAE_MSG("src or dst in NULL");
		return FPGA_INVALID_PARAM;
	}

	if (_src->hdr.magic != FPGA_TOKEN_MAGIC) {
		OPAE_MSG("Invalid src");
		return FPGA_INVALID_PARAM;
	}

	_dst = calloc(1, sizeof(struct _fpga_token));
	if (NULL == _dst) {
		OPAE_MSG("Failed to allocate memory for token");
		return FPGA_NO_MEMORY;
	}

	_dst->hdr = _src->hdr;

	_dst->device_instance = _src->device_instance;
	_dst->subdev_instance = _src->subdev_instance;

	len = strnlen(_src->sysfspath, sizeof(_src->sysfspath) - 1);
	strncpy(_dst->sysfspath, _src->sysfspath, len + 1);

	len = strnlen(_src->devpath, sizeof(_src->devpath) - 1);
	strncpy(_dst->devpath, _src->devpath, len + 1);

	_dst->errors = clone_error_list(_src->errors);

	*dst = _dst;

	return FPGA_OK;
}

fpga_result __XFPGA_API__ xfpga_fpgaDestroyToken(fpga_token *token)
{
	struct error_list *err;
	struct _fpga_token *_token;

	if (NULL == token || NULL == *token) {
		OPAE_MSG("Invalid token pointer");
		return FPGA_INVALID_PARAM;
	}

	_token = (struct _fpga_token *)*token;

	if (_token->hdr.magic != FPGA_TOKEN_MAGIC) {
		OPAE_MSG("Invalid token");
		return FPGA_INVALID_PARAM;
	}

	err = _token->errors;
	while (err) {
		struct error_list *trash = err;
		err = err->next;
		free(trash);
	}

	// invalidate token header (just in case)
	memset(&_token->hdr, 0, sizeof(_token->hdr));

	free(*token);
	*token = NULL;

	return FPGA_OK;
}
