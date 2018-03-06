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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include "safe_string/safe_string.h"

#include "common_int.h"
#include "opae/enum.h"
#include "opae/properties.h"
#include "opae/utils.h"
#include "properties_int.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

/* mutex to protect global data structures */
extern pthread_mutex_t global_lock;

struct dev_list {
	char sysfspath[SYSFS_PATH_MAX];
	char devpath[DEV_PATH_MAX];
	fpga_objtype objtype;
	fpga_guid guid;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
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

	static bool
matches_filter(const struct dev_list *attr, const fpga_properties filter)
{
	struct _fpga_properties *_filter = (struct _fpga_properties *)filter;
	bool res = true;
	int err = 0;

	if (pthread_mutex_lock(&_filter->lock)) {
		FPGA_MSG("Failed to lock global mutex");
		return false;
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_PARENT)) {
		struct _fpga_token *_tok =
			(struct _fpga_token *) _filter->parent;
		char spath[SYSFS_PATH_MAX];
		char *p;
		int device_instance;

		if (FPGA_ACCELERATOR != attr->objtype) {
			res = false; // Only accelerator can have a parent
			goto out_unlock;
		}

		if (NULL == _tok) {
			res = false; // Reject search based on NULL parent token
			goto out_unlock;
		}

		p = strrchr(attr->sysfspath, '.');

		if (NULL == p) {
			res = false;
			goto out_unlock;
		}

		device_instance = (int) strtoul(p+1, NULL, 10);

		snprintf_s_ii(spath, SYSFS_PATH_MAX,
				SYSFS_FPGA_CLASS_PATH
				SYSFS_FME_PATH_FMT,
				device_instance, device_instance);

		if (strcmp(spath, ((struct _fpga_token *)
						_filter->parent)->sysfspath)) {
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

	// FIXME
	// if (FIELD_VALID(_filter, FPGA_PROPERTY_DEVICEID)) {
	//
	// }

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

	if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJTYPE) &&
			(FPGA_DEVICE == _filter->objtype)) {

		if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_SLOTS)) {
			if ((FPGA_DEVICE != attr->objtype) ||
					(attr->fpga_num_slots !=
					 _filter->u.fpga.num_slots)) {
				res = false;
				goto out_unlock;
			}
		}

		if (FIELD_VALID(_filter, FPGA_PROPERTY_BBSID)) {
			if ((FPGA_DEVICE != attr->objtype) ||
					(attr->fpga_bitstream_id !=
					 _filter->u.fpga.bbs_id)) {
				res = false;
				goto out_unlock;
			}
		}

		if (FIELD_VALID(_filter, FPGA_PROPERTY_BBSVERSION)) {
			if ((FPGA_DEVICE != attr->objtype) ||
					(attr->fpga_bbs_version.major !=
					 _filter->u.fpga.bbs_version.major) ||
					(attr->fpga_bbs_version.minor !=
					 _filter->u.fpga.bbs_version.minor) ||
					(attr->fpga_bbs_version.patch !=
					 _filter->u.fpga.bbs_version.patch)) {
				res = false;
				goto out_unlock;
			}
		}

		// FIXME
		// if (FIELD_VALID(_filter, FPGA_PROPERTY_VENDORID)) {
		//	if (FPGA_DEVICE != attr->objtype)
		//		return false;
		//
		// }

		// FIXME
		// if (FIELD_VALID(_filter, FPGA_PROPERTY_MODEL)) {
		//	if (FPGA_DEVICE != attr->objtype)
		//		return false;
		//
		// }

		// FIXME
		// if (FIELD_VALID(_filter, FPGA_PROPERTY_LOCAL_MEMORY)) {
		//	if (FPGA_DEVICE != attr->objtype)
		//		return false;
		//
		// }

		// FIXME
		// if (FIELD_VALID(_filter, FPGA_PROPERTY_CAPABILITIES)) {
		//	if (FPGA_DEVICE != attr->objtype)
		//		return false;
		//
		// }

	} else if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJTYPE) &&
			(FPGA_ACCELERATOR == _filter->objtype)) {

		if (FIELD_VALID(_filter, FPGA_PROPERTY_ACCELERATOR_STATE)) {
			if ((FPGA_ACCELERATOR != attr->objtype) ||
					(attr->accelerator_state != _filter->u.accelerator.state)) {
				res = false;
				goto out_unlock;
			}
		}

		if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_MMIO)) {
			if ((FPGA_ACCELERATOR != attr->objtype) ||
					(attr->accelerator_num_mmios != _filter->u.accelerator.num_mmio)) {
				res = false;
				goto out_unlock;
			}
		}

		if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_INTERRUPTS)) {
			if ((FPGA_ACCELERATOR != attr->objtype) ||
					(attr->accelerator_num_irqs !=
					 _filter->u.accelerator.num_interrupts)) {
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

	static bool
matches_filters(const struct dev_list *attr,
		const fpga_properties *filter, uint32_t num_filter)
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

	static struct dev_list *
add_dev(const char *sysfspath, const char *devpath, struct dev_list *parent)
{
	struct dev_list *pdev;
	errno_t e;

	pdev = (struct dev_list *) malloc(sizeof(*pdev));
	if (NULL == pdev)
		return NULL;

	e = strncpy_s(pdev->sysfspath, sizeof(pdev->sysfspath),
			sysfspath, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;

	e = strncpy_s(pdev->devpath, sizeof(pdev->devpath),
			devpath, DEV_PATH_MAX);
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

bool del_dev(struct dev_list *pdev, struct dev_list *parent)
{
	if (!parent || !pdev)
		return false;

	parent->next = pdev->next;
	free(pdev);

	return true;
}

//static const fpga_guid FPGA_FME_GUID = {
//	0xbf, 0xaf, 0x2a, 0xe9, 0x4a, 0x52, 0x46, 0xe3,
//	0x82, 0xfe, 0x38, 0xf0, 0xf9, 0xe1, 0x77, 0x64
//};

	static fpga_result
enum_fme_afu(const char *sysfspath, const char *name, struct dev_list *parent)
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

	if (strstr(name, FPGA_SYSFS_FME)) {
		int socket_id = 0;

		snprintf_s_s(dpath, sizeof(dpath), FPGA_DEV_PATH "/%s", name);

		pdev = add_dev(sysfspath, dpath, parent);
		if (!pdev) {
			FPGA_MSG("Failed to allocate device");
			return FPGA_NO_MEMORY;
		}

		pdev->objtype  = FPGA_DEVICE;

		pdev->bus      = parent->bus;
		pdev->device   = parent->device;
		pdev->function = parent->function;

		// Hard-coding the FME guid for now. Leave the below code in case this changes.

		//memcpy(pdev->guid, FPGA_FME_GUID, sizeof(fpga_guid));
		// populate from pr/interface_id

		// Discover the FME GUID from sysfs (pr/interface_id)
		snprintf_s_s(spath, sizeof(spath), "%s/"
				FPGA_SYSFS_FME_INTERFACE_ID, sysfspath);

		result = sysfs_read_guid(spath, pdev->guid);
		if (FPGA_OK != result)
			return result;

		// Discover the socket id from the FME's sysfs entry.
		snprintf_s_s(spath, sizeof(spath), "%s/"
				FPGA_SYSFS_SOCKET_ID, sysfspath);

		result = sysfs_read_int(spath, &socket_id);
		if (FPGA_OK != result)
			return result;

		snprintf_s_s(spath, sizeof(spath), "%s/"
				FPGA_SYSFS_NUM_SLOTS, sysfspath);
		result = sysfs_read_u32(spath, &pdev->fpga_num_slots);
		if (FPGA_OK != result)
			return result;

		snprintf_s_s(spath, sizeof(spath), "%s/"
				FPGA_SYSFS_BITSTREAM_ID, sysfspath);
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
	}

	if (strstr(name, FPGA_SYSFS_AFU)) {
		int res;

		snprintf_s_s(dpath, sizeof(dpath), FPGA_DEV_PATH "/%s", name);

		pdev = add_dev(sysfspath, dpath, parent);
		if (!pdev) {
			FPGA_MSG("Failed to allocate device");
			return FPGA_NO_MEMORY;
		}

		pdev->objtype  = FPGA_ACCELERATOR;

		pdev->bus      = parent->bus;
		pdev->device   = parent->device;
		pdev->function = parent->function;

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
			FPGA_MSG("Could not read afu_id from '%s', ignoring",
				 spath);
			if (!del_dev(pdev, parent)) {
				FPGA_ERR("del_dev() failed");
				return FPGA_EXCEPTION;
			}
		}
	}

	return FPGA_OK;
}

	static fpga_result
enum_top_dev(const char *sysfspath, struct dev_list *list)
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
	unsigned b, d;

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
	p += 6;

	// 0123456
	// bb:dd.f
	f = 0;
	sscanf_s_i(p+6, "%d", &f);

	pdev->function = (uint8_t) f;
	*(p + 5) = 0;

	d = 0;
	sscanf_s_u(p+3, "%x", &d);

	pdev->device = (uint8_t) d;
	*(p + 2) = 0;

	b = 0;
	sscanf_s_u(p, "%x", &b);

	pdev->bus = (uint8_t) b;

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

		result = enum_fme_afu(spath, dirent->d_name, pdev);
		if (result != FPGA_OK)
			break;
	}

	closedir(dir);

	return result;
}


	fpga_result __FPGA_API__
fpgaEnumerate(const fpga_properties *filters, uint32_t num_filters,
		fpga_token *tokens, uint32_t max_tokens, uint32_t *num_matches)
{
	fpga_result result = FPGA_NOT_FOUND;

	DIR *dir = NULL;
	struct dirent *dirent = NULL;
	char sysfspath[SYSFS_PATH_MAX];
	struct dev_list head;
	struct dev_list *lptr;

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

	memset(&head, 0, sizeof(head));

	// Find the top-level FPGA devices.
	dir = opendir(SYSFS_FPGA_CLASS_PATH);
	if (NULL == dir) {
		FPGA_MSG("can't find %s (no driver?)", SYSFS_FPGA_CLASS_PATH);
		return FPGA_NO_DRIVER;
	}

	while ((dirent = readdir(dir)) != NULL) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;

		snprintf_s_ss(sysfspath, sizeof(sysfspath), "%s/%s",
				SYSFS_FPGA_CLASS_PATH,	dirent->d_name);

		result = enum_top_dev(sysfspath, &head);
		if (result != FPGA_OK)
			break;
	}

	closedir(dir);

	if (result != FPGA_OK) {
		FPGA_MSG("No FPGA resources found");
		return result;
	}

	/* create and populate token data structures */
	for (lptr = head.next ; NULL != lptr ; lptr = lptr->next) {
		struct _fpga_token *_tok;

		if (!strnlen_s(lptr->devpath, sizeof(lptr->devpath)))
			continue;

		// propagate the socket_id field.
		lptr->socket_id = lptr->parent->socket_id;
		lptr->fme = lptr->parent->fme;

		/* FIXME: do we need to keep a global list of tokens? */
		/* For now we do becaue it is used in fpgaUpdateProperties
		 * to lookup a parent from the global list of tokens...*/
		_tok = token_add(lptr->sysfspath,
				lptr->devpath);

		if (NULL == _tok) {
			FPGA_MSG("Failed to allocate memory for token");
			result = FPGA_NO_MEMORY;
			goto out_free_trash;
		}

		// FIXME: should check contents of filter for token magic
		if (matches_filters(lptr, filters, num_filters)) {
			if (*num_matches < max_tokens) {
				if (fpgaCloneToken(_tok, &tokens[*num_matches])
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
	for (lptr = head.next ; NULL != lptr;) {
		struct dev_list *trash = lptr;
		lptr = lptr->next;
		free(trash);
	}

	return result;
}

fpga_result __FPGA_API__ fpgaCloneToken(fpga_token src,
		fpga_token *dst)
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

	e = strncpy_s(_dst->sysfspath, sizeof(_dst->sysfspath),
			_src->sysfspath, sizeof(_src->sysfspath));
	if (EOK != e) {
		FPGA_MSG("strncpy_s failed");
		result = FPGA_EXCEPTION;
		goto out_free;
	}

	e = strncpy_s(_dst->devpath, sizeof(_dst->devpath),
			_src->devpath, sizeof(_src->devpath));
	if (EOK != e) {
		FPGA_MSG("strncpy_s failed");
		result = FPGA_EXCEPTION;
		goto out_free;
	}

	*dst = _dst;
	return FPGA_OK;

out_free:
	free(_dst);
	return result;
}

fpga_result __FPGA_API__ fpgaDestroyToken(fpga_token *token)
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

