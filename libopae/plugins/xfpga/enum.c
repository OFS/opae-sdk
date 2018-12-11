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

#include "types_int.h"
#include "safe_string/safe_string.h"
#include "xfpga.h"
#include "common_int.h"
#include "error_int.h"
#include "props.h"

/* mutex to protect global data structures */
extern pthread_mutex_t global_lock;

struct dev_list {
	char sysfspath[SYSFS_PATH_MAX];
	char devpath[DEV_PATH_MAX];
	fpga_objtype objtype;
	fpga_guid guid;
	uint16_t segment;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	uint8_t socket_id;
	uint16_t vendor_id;
	uint16_t device_id;

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
		FPGA_MSG("Failed to lock filter mutex");
		return false;
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_PARENT)) {
		struct _fpga_token *_parent_tok =
			(struct _fpga_token *)_filter->parent;
		char spath[SYSFS_PATH_MAX];
		char *p;
		int subdev_instance;
		int device_instance;

		if (FPGA_ACCELERATOR != attr->objtype) {
			res = false; // Only accelerator can have a parent
			goto out_unlock;
		}

		if (NULL == _parent_tok) {
			res = false; // Reject search based on NULL parent token
			goto out_unlock;
		}

		// Find the FME/Port sub-device instance.
		p = strrchr(attr->sysfspath, '.');

		if (NULL == p) {
			res = false;
			goto out_unlock;
		}

		subdev_instance = (int)strtoul(p + 1, NULL, 10);

		// Find the device instance.
		p = strchr(attr->sysfspath, '.');

		if (NULL == p) {
			res = false;
			goto out_unlock;
		}

		device_instance = (int)strtoul(p + 1, NULL, 10);

		snprintf_s_ii(spath, SYSFS_PATH_MAX,
				get_fpga_fme_sysfs_path(),
			      device_instance, subdev_instance);

		if (strcmp(spath, _parent_tok->sysfspath)) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJTYPE)) {
		if (_filter->objtype != attr->objtype) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_SEGMENT)) {
		if (_filter->segment != attr->segment) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_BUS)) {
		if (_filter->bus != attr->bus) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_DEVICE)) {
		if (_filter->device != attr->device) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_FUNCTION)) {
		if (_filter->function != attr->function) {
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
		if (0 != memcmp(attr->guid, _filter->guid, sizeof(fpga_guid))) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJECTID)) {
		uint64_t objid;
		fpga_result result;
		result = sysfs_objectid_from_path(attr->sysfspath, &objid);
		if (result != FPGA_OK || _filter->object_id != objid) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_VENDORID)) {
		if (_filter->vendor_id != attr->vendor_id) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_DEVICEID)) {
		if (_filter->device_id != attr->device_id) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_ERRORS)) {
		uint32_t errors;
		char errpath[SYSFS_PATH_MAX];

		snprintf_s_s(errpath, SYSFS_PATH_MAX, "%s/errors",
			     attr->sysfspath);
		errors = count_error_files(errpath);
		if (errors != _filter->num_errors) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJTYPE)
	    && (FPGA_DEVICE == _filter->objtype)) {

		if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_SLOTS)) {
			if ((FPGA_DEVICE != attr->objtype)
			    || (attr->fpga_num_slots
				!= _filter->u.fpga.num_slots)) {
				res = false;
				goto out_unlock;
			}
		}

		if (FIELD_VALID(_filter, FPGA_PROPERTY_BBSID)) {
			if ((FPGA_DEVICE != attr->objtype)
			    || (attr->fpga_bitstream_id
				!= _filter->u.fpga.bbs_id)) {
				res = false;
				goto out_unlock;
			}
		}

		if (FIELD_VALID(_filter, FPGA_PROPERTY_BBSVERSION)) {
			if ((FPGA_DEVICE != attr->objtype)
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
			if ((FPGA_ACCELERATOR != attr->objtype)
			    || (attr->accelerator_state
				!= _filter->u.accelerator.state)) {
				res = false;
				goto out_unlock;
			}
		}

		if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_MMIO)) {
			if ((FPGA_ACCELERATOR != attr->objtype)
			    || (attr->accelerator_num_mmios
				!= _filter->u.accelerator.num_mmio)) {
				res = false;
				goto out_unlock;
			}
		}

		if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_INTERRUPTS)) {
			if ((FPGA_ACCELERATOR != attr->objtype)
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
		FPGA_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
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
	errno_t e;

	pdev = (struct dev_list *)malloc(sizeof(*pdev));
	if (NULL == pdev)
		return NULL;

	e = strncpy_s(pdev->sysfspath, sizeof(pdev->sysfspath), sysfspath,
		      SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;

	e = strncpy_s(pdev->devpath, sizeof(pdev->devpath), devpath,
		      DEV_PATH_MAX);
	if (EOK != e)
		goto out_free;

	pdev->next = parent->next;
	parent->next = pdev;

	pdev->parent = parent;

	return pdev;

out_free:
	free(pdev);
	return NULL;
}

STATIC fpga_result enum_fme(const char *sysfspath, const char *name,
		     struct dev_list *parent)
{
	fpga_result result;
	struct stat stats;
	struct dev_list *pdev;
	char spath[SYSFS_PATH_MAX] = { 0 };
	char dpath[DEV_PATH_MAX];

	// Make sure it's a directory.
	if (stat(sysfspath, &stats) != 0) {
		FPGA_MSG("stat failed: %s", strerror(errno));
		return FPGA_NOT_FOUND;
	}

	if (!S_ISDIR(stats.st_mode))
		return FPGA_OK;
	int socket_id = 0;

	snprintf_s_s(dpath, sizeof(dpath), FPGA_DEV_PATH "/%s", name);

	pdev = add_dev(sysfspath, dpath, parent);
	if (!pdev) {
		FPGA_MSG("Failed to allocate device");
		return FPGA_NO_MEMORY;
	}

	pdev->objtype = FPGA_DEVICE;

	pdev->segment = parent->segment;
	pdev->bus = parent->bus;
	pdev->device = parent->device;
	pdev->function = parent->function;
	pdev->vendor_id = parent->vendor_id;
	pdev->device_id = parent->device_id;

	// Discover the FME GUID from sysfs (pr/interface_id)
	//snprintf_s_s(spath, sizeof(spath), "%s/" FPGA_SYSFS_FME_INTERFACE_ID,
	//	     sysfspath);

	if (get_fpga_pr_interfaceid_sysfs_path(sysfspath, spath) != 0) {
		return FPGA_EXCEPTION;
	}

	result = sysfs_read_guid(spath, pdev->guid);
	if (FPGA_OK != result)
		return result;

	// Discover the socket id from the FME's sysfs entry.
	snprintf_s_s(spath, sizeof(spath), "%s/" FPGA_SYSFS_SOCKET_ID,
		sysfspath);
	result = sysfs_read_int(spath, &socket_id);
	if (FPGA_OK != result)
		return result;

	snprintf_s_s(spath, sizeof(spath), "%s/" FPGA_SYSFS_NUM_SLOTS,
		sysfspath);
	result = sysfs_read_u32(spath, &pdev->fpga_num_slots);
	if (FPGA_OK != result)
		return result;

	snprintf_s_s(spath, sizeof(spath), "%s/" FPGA_SYSFS_BITSTREAM_ID,
		sysfspath);
	result = sysfs_read_u64(spath, &pdev->fpga_bitstream_id);
	if (FPGA_OK != result)
		return result;

	pdev->fpga_bbs_version.major =
		FPGA_BBS_VER_MAJOR(pdev->fpga_bitstream_id);
	pdev->fpga_bbs_version.minor =
		FPGA_BBS_VER_MINOR(pdev->fpga_bitstream_id);
	pdev->fpga_bbs_version.patch =
		FPGA_BBS_VER_PATCH(pdev->fpga_bitstream_id);

	parent->socket_id = socket_id;
	parent->fme = pdev;
	return FPGA_OK;
}

STATIC fpga_result enum_afu(const char *sysfspath, const char *name,
		     struct dev_list *parent)
{
	fpga_result result;
	struct stat stats;
	struct dev_list *pdev;
	char spath[SYSFS_PATH_MAX];
	char dpath[DEV_PATH_MAX];

	// Make sure it's a directory.
	if (stat(sysfspath, &stats) != 0) {
		FPGA_MSG("stat failed: %s", strerror(errno));
		return FPGA_NOT_FOUND;
	}

	if (!S_ISDIR(stats.st_mode))
		return FPGA_OK;
	int res;

	snprintf_s_s(dpath, sizeof(dpath), FPGA_DEV_PATH "/%s", name);

	pdev = add_dev(sysfspath, dpath, parent);
	if (!pdev) {
		FPGA_MSG("Failed to allocate device");
		return FPGA_NO_MEMORY;
	}

	pdev->objtype = FPGA_ACCELERATOR;

	pdev->segment = parent->segment;
	pdev->bus = parent->bus;
	pdev->device = parent->device;
	pdev->function = parent->function;
	pdev->vendor_id = parent->vendor_id;
	pdev->device_id = parent->device_id;

	res = open(pdev->devpath, O_RDWR);
	if (-1 == res) {
		pdev->accelerator_state = FPGA_ACCELERATOR_ASSIGNED;
	} else {
		close(res);
		pdev->accelerator_state = FPGA_ACCELERATOR_UNASSIGNED;
	}

	// FIXME: not to rely on hard-coded constants.
	pdev->accelerator_num_mmios = 2;
	pdev->accelerator_num_irqs = 0;

	// Discover the AFU GUID from sysfs.
	snprintf_s_s(spath, sizeof(spath), "%s/" FPGA_SYSFS_AFU_GUID,
		sysfspath);

	result = sysfs_read_guid(spath, pdev->guid);
	/* if we can't read the afu_id, remove device from list */
	if (FPGA_OK != result) {
		FPGA_MSG("Could not read afu_id from '%s', ignoring", spath);
		parent->next = pdev->next;
		free(pdev);
	}

	return FPGA_OK;
}

STATIC fpga_result enum_top_dev(const char *sysfspath, struct dev_list *list,
			 bool include_port)
{
	fpga_result result = FPGA_NOT_FOUND;
	struct stat stats;

	struct dev_list *pdev;

	DIR *dir;
	struct dirent *dirent;
	char spath[SYSFS_PATH_MAX];
	int res;
	char *p;
	int f;
	unsigned s, b, d;

	// Make sure it's a directory.
	if (stat(sysfspath, &stats) != 0) {
		FPGA_MSG("stat failed: %s", strerror(errno));
		return FPGA_NO_DRIVER;
	}

	if (!S_ISDIR(stats.st_mode))
		return FPGA_OK;

	res = readlink(sysfspath, spath, sizeof(spath));
	if (-1 == res) {
		FPGA_MSG("Can't read link");
		return FPGA_NO_DRIVER;
	}

	pdev = add_dev(sysfspath, "", list);
	if (!pdev) {
		FPGA_MSG("Failed to allocate device");
		return FPGA_NO_MEMORY;
	}

	// Find the BDF from the link path.
	spath[res] = 0;
	p = strrchr(spath, '/');
	if (!p) {
		FPGA_MSG("Invalid link");
		return FPGA_NO_DRIVER;
	}
	*p = 0;
	p = strrchr(spath, '/');
	if (!p) {
		FPGA_MSG("Invalid link");
		return FPGA_NO_DRIVER;
	}
	*p = 0;
	p = strrchr(spath, '/');
	if (!p) {
		FPGA_MSG("Invalid link");
		return FPGA_NO_DRIVER;
	}
	++p;

	//           11
	// 012345678901
	// ssss:bb:dd.f
	f = 0;
	sscanf_s_i(p + 11, "%d", &f);

	pdev->function = (uint8_t)f;
	*(p + 10) = 0;

	d = 0;
	sscanf_s_u(p + 8, "%x", &d);

	pdev->device = (uint8_t)d;
	*(p + 7) = 0;

	b = 0;
	sscanf_s_u(p + 5, "%x", &b);

	pdev->bus = (uint8_t)b;
	*(p + 4) = 0;

	s = 0;
	sscanf_s_u(p, "%x", &s);
	pdev->segment = (uint16_t)s;

	// read the vendor and device ID from the 'device' path
	uint32_t x = 0;
	char vendorpath[SYSFS_PATH_MAX];
	snprintf_s_s(vendorpath, SYSFS_PATH_MAX, "%s/device/vendor", sysfspath);
	result = sysfs_read_u32(vendorpath, &x);
	if (result != FPGA_OK)
		return result;
	pdev->vendor_id = (uint16_t)x;

	char devicepath[SYSFS_PATH_MAX];
	snprintf_s_s(devicepath, SYSFS_PATH_MAX, "%s/device/device", sysfspath);
	result = sysfs_read_u32(devicepath, &x);
	if (result != FPGA_OK)
		return result;
	pdev->device_id = (uint16_t)x;

	// Find the FME and AFU devices.
	dir = opendir(sysfspath);
	if (NULL == dir) {
		FPGA_MSG("Can't open directory: %s", sysfspath);
		return FPGA_NO_DRIVER;
	}

	while ((dirent = readdir(dir)) != NULL) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;

		snprintf_s_ss(spath, sizeof(spath), "%s/%s", sysfspath,
			      dirent->d_name);
		if (strstr(dirent->d_name, FPGA_SYSFS_FME)) {
			result = enum_fme(spath, dirent->d_name, pdev);
			if (result != FPGA_OK)
				break;
		} else if (include_port
			   && strstr(dirent->d_name, FPGA_SYSFS_AFU)) {
			result = enum_afu(spath, dirent->d_name, pdev);
			if (result != FPGA_OK)
				break;
		}
	}

	closedir(dir);

	return result;
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

fpga_result __FPGA_API__ xfpga_fpgaEnumerate(const fpga_properties *filters,
				       uint32_t num_filters, fpga_token *tokens,
				       uint32_t max_tokens,
				       uint32_t *num_matches)
{
	fpga_result result = FPGA_NOT_FOUND;

	DIR *dir = NULL;
	struct dirent *dirent = NULL;
	char sysfspath[SYSFS_PATH_MAX];
	struct dev_list head;
	struct dev_list *lptr;
	int count = 0;

	if (NULL == num_matches) {
		FPGA_MSG("num_matches is NULL");
		return FPGA_INVALID_PARAM;
	}

	/* requiring a max number of tokens, but not providing a pointer to
	 * return them through is invalid */
	if ((max_tokens > 0) && (NULL == tokens)) {
		FPGA_MSG("max_tokens > 0 with NULL tokens");
		return FPGA_INVALID_PARAM;
	}

	if ((num_filters > 0) && (NULL == filters)) {
		FPGA_MSG("num_filters > 0 with NULL filters");
		return FPGA_INVALID_PARAM;
	}

	if (!num_filters && (NULL != filters)) {
		FPGA_MSG("num_filters == 0 with non-NULL filters");
		return FPGA_INVALID_PARAM;
	}

	*num_matches = 0;

	memset_s(&head, sizeof(head), 0);

	// Find the top-level FPGA devices.
	dir = opendir(get_fpga_class_sysfs_path());
	if (NULL == dir) {
		FPGA_MSG("can't find %s (no driver?)", get_fpga_class_sysfs_path());
		return FPGA_NO_DRIVER;
	}

	while ((dirent = readdir(dir)) != NULL) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;

		snprintf_s_ss(sysfspath, sizeof(sysfspath), "%s/%s",
			get_fpga_class_sysfs_path(), dirent->d_name);

		result = enum_top_dev(sysfspath, &head,
				      include_afu(filters, num_filters));
		if (result == FPGA_OK) {
			count++;
			continue;
		}
		if (count > 0)
			result = FPGA_OK;
	}

	closedir(dir);

	if (result != FPGA_OK) {
		FPGA_MSG("No FPGA resources found");
		return result;
	}

	printf("---------FOUND FPGA  fpga count =%d \n", count);

	/* create and populate token data structures */
	for (lptr = head.next; NULL != lptr; lptr = lptr->next) {
		struct _fpga_token *_tok;

		if (!strnlen_s(lptr->devpath, sizeof(lptr->devpath)))
			continue;

		// propagate the socket_id field.
		lptr->socket_id = lptr->parent->socket_id;
		lptr->fme = lptr->parent->fme;

		/* FIXME: do we need to keep a global list of tokens? */
		/* For now we do becaue it is used in xfpga_fpgaUpdateProperties
		 * to lookup a parent from the global list of tokens...*/
		_tok = token_add(lptr->sysfspath, lptr->devpath);

		if (NULL == _tok) {
			FPGA_MSG("Failed to allocate memory for token");
			result = FPGA_NO_MEMORY;
			goto out_free_trash;
		}

		// FIXME: should check contents of filter for token magic
		if (matches_filters(lptr, filters, num_filters)) {
			if (*num_matches < max_tokens) {
				if (xfpga_fpgaCloneToken(_tok, &tokens[*num_matches])
				    != FPGA_OK) {
					// FIXME: should we error out here?
					FPGA_MSG("Error cloning token");
				}
			}
			++(*num_matches);
		}
	}

out_free_trash:
	/* FIXME: should this live in a separate function? */
	for (lptr = head.next; NULL != lptr;) {
		struct dev_list *trash = lptr;
		lptr = lptr->next;
		free(trash);
	}

	return result;
}

fpga_result __FPGA_API__ xfpga_fpgaCloneToken(fpga_token src, fpga_token *dst)
{
	struct _fpga_token *_src = (struct _fpga_token *)src;
	struct _fpga_token *_dst;
	fpga_result result;
	errno_t e;

	if (NULL == src || NULL == dst) {
		FPGA_MSG("src or dst in NULL");
		return FPGA_INVALID_PARAM;
	}

	if (_src->magic != FPGA_TOKEN_MAGIC) {
		FPGA_MSG("Invalid src");
		return FPGA_INVALID_PARAM;
	}

	_dst = malloc(sizeof(struct _fpga_token));
	if (NULL == _dst) {
		FPGA_MSG("Failed to allocate memory for token");
		return FPGA_NO_MEMORY;
	}

	_dst->magic = FPGA_TOKEN_MAGIC;
	_dst->device_instance = _src->device_instance;
	_dst->subdev_instance = _src->subdev_instance;

	e = strncpy_s(_dst->sysfspath, sizeof(_dst->sysfspath), _src->sysfspath,
		      sizeof(_src->sysfspath));
	if (EOK != e) {
		FPGA_MSG("strncpy_s failed");
		result = FPGA_EXCEPTION;
		goto out_free;
	}

	e = strncpy_s(_dst->devpath, sizeof(_dst->devpath), _src->devpath,
		      sizeof(_src->devpath));
	if (EOK != e) {
		FPGA_MSG("strncpy_s failed");
		result = FPGA_EXCEPTION;
		goto out_free;
	}

	// driver version
	switch (get_fpga_drv_devl_ver()) {

	case FPGA_LATEST_DRV_VER:
		_dst->drv_devl_ver = FPGA_LATEST_DRV_VER;
		break;
	case FPGA_LINUX_UPS_DRV_VER:
		_dst->drv_devl_ver = FPGA_LINUX_UPS_DRV_VER;
		break;
	default:
		_dst->drv_devl_ver = FPGA_UNKNOWN_DRV_VER;
		break;
	}

	// shallow-copy error list
	_dst->errors = _src->errors;

	*dst = _dst;
	return FPGA_OK;

out_free:
	free(_dst);
	return result;
}

fpga_result __FPGA_API__ xfpga_fpgaDestroyToken(fpga_token *token)
{
	fpga_result result = FPGA_OK;
	int err = 0;

	if (NULL == token || NULL == *token) {
		FPGA_MSG("Invalid token pointer");
		return FPGA_INVALID_PARAM;
	}

	struct _fpga_token *_token = (struct _fpga_token *)*token;

	if (pthread_mutex_lock(&global_lock)) {
		FPGA_MSG("Failed to lock global mutex");
		return FPGA_EXCEPTION;
	}

	if (_token->magic != FPGA_TOKEN_MAGIC) {
		FPGA_MSG("Invalid token");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	// invalidate magic (just in case)
	_token->magic = FPGA_INVALID_MAGIC;

	free(*token);
	*token = NULL;

out_unlock:
	err = pthread_mutex_unlock(&global_lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
	}
	return result;
}
