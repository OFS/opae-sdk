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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libudev.h>
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
#include <opae/dfl.h>

/* mutex to protect global data structures */
extern pthread_mutex_t global_lock;

STATIC bool matches_filter(dfl_device *dev, const fpga_properties filter)
{
	struct _fpga_properties *_filter = (struct _fpga_properties *)filter;
	bool res = true;
	int err = 0;
	char buffer[PATH_MAX] = {0};
	uint64_t value64 = 0;

	if (pthread_mutex_lock(&_filter->lock)) {
		OPAE_MSG("Failed to lock filter mutex");
		return false;
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_PARENT)) {
		struct _fpga_token *_parent_tok =
			(struct _fpga_token *)_filter->parent;
		char spath[PATH_MAX] = {0};

		if (FPGA_ACCELERATOR != dev->type) {
			res = false; // Only accelerator can have a parent
			goto out_unlock;
		}

		if (NULL == _parent_tok) {
			res = false; // Reject search based on NULL parent token
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJTYPE)) {
		if (_filter->objtype != dev->type) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_SEGMENT)) {
		if (_filter->segment != dev->segment) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_BUS)) {
		if (_filter->bus != dev->bus) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_DEVICE)) {
		if (_filter->device != dev->device) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_FUNCTION)) {
		if (_filter->function != dev->function) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_SOCKETID)) {
		if (dev->numa_node != _filter->socket_id) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_GUID)) {
		if (dfl_device_guid_match(dev, _filter->guid)) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJECTID)) {
		if (dev->object_id != _filter->object_id) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_VENDORID)) {
		if (dev->vendor_id != _filter->vendor_id) {
			res = false;
			goto out_unlock;
		}
	}

	if (FIELD_VALID(_filter, FPGA_PROPERTY_DEVICEID)) {
		if (dev->device_id != _filter->device_id) {
			res = false;
			goto out_unlock;
		}
	}


	//if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_ERRORS)) {
	//	uint32_t errors;
	//	char errpath[SYSFS_PATH_MAX] = { 0, };

	//	if (snprintf(errpath, sizeof(errpath),
	//		     "%s/errors", dev->sysfspath) < 0) {
	//		OPAE_ERR("snprintf buffer overflow");
	//		res = false;
	//		goto out_unlock;
	//	}

	//	errors = count_error_files(errpath);
	//	if (errors != _filter->num_errors) {
	//		res = false;
	//		goto out_unlock;
	//	}
	//}

	//if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJTYPE)
	//    && (FPGA_DEVICE == _filter->objtype)) {

	//	if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_SLOTS)) {
	//		if ((FPGA_DEVICE != dev->objtype)
	//		    || (dev->fpga_num_slots
	//			!= _filter->u.fpga.num_slots)) {
	//			res = false;
	//			goto out_unlock;
	//		}
	//	}

	//	if (FIELD_VALID(_filter, FPGA_PROPERTY_BBSID)) {
	//		if ((FPGA_DEVICE != dev->objtype)
	//		    || (dev->fpga_bitstream_id
	//			!= _filter->u.fpga.bbs_id)) {
	//			res = false;
	//			goto out_unlock;
	//		}
	//	}

	//	if (FIELD_VALID(_filter, FPGA_PROPERTY_BBSVERSION)) {
	//		if ((FPGA_DEVICE != dev->objtype)
	//		    || (dev->fpga_bbs_version.major
	//			!= _filter->u.fpga.bbs_version.major)
	//		    || (dev->fpga_bbs_version.minor
	//			!= _filter->u.fpga.bbs_version.minor)
	//		    || (dev->fpga_bbs_version.patch
	//			!= _filter->u.fpga.bbs_version.patch)) {
	//			res = false;
	//			goto out_unlock;
	//		}
	//	}

	//} else if (FIELD_VALID(_filter, FPGA_PROPERTY_OBJTYPE)
	//	   && (FPGA_ACCELERATOR == _filter->objtype)) {

	//	if (FIELD_VALID(_filter, FPGA_PROPERTY_ACCELERATOR_STATE)) {
	//		if ((FPGA_ACCELERATOR != dev->objtype)
	//		    || (dev->accelerator_state
	//			!= _filter->u.accelerator.state)) {
	//			res = false;
	//			goto out_unlock;
	//		}
	//	}

	//	if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_MMIO)) {
	//		if ((FPGA_ACCELERATOR != dev->objtype)
	//		    || (dev->accelerator_num_mmios
	//			!= _filter->u.accelerator.num_mmio)) {
	//			res = false;
	//			goto out_unlock;
	//		}
	//	}

	//	if (FIELD_VALID(_filter, FPGA_PROPERTY_NUM_INTERRUPTS)) {
	//		if ((FPGA_ACCELERATOR != dev->objtype)
	//		    || (dev->accelerator_num_irqs
	//			!= _filter->u.accelerator.num_interrupts)) {
	//			res = false;
	//			goto out_unlock;
	//		}
	//	}
	//}

out_unlock:
	err = pthread_mutex_unlock(&_filter->lock);
	if (err) {
		OPAE_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
	}
	return res;
}

STATIC bool matches_filters(dfl_device *dev, const fpga_properties *filter,
		     uint32_t num_filter)
{
	uint32_t i;

	if (!num_filter) // no filter == match everything
		return true;

	for (i = 0; i < num_filter; ++i) {
		if (matches_filter(dev, filter[i])) {
			return true;
		}
	}
	return false;
}


fpga_token fpga_token_new(dfl_device *dfl, bool clone)
{
	struct _fpga_token *t = (struct _fpga_token*)malloc(sizeof(struct _fpga_token));
	t->magic = FPGA_TOKEN_MAGIC;
	t->dev = clone ? dfl_device_clone(dfl) : dfl;
	t->sysfspath = udev_device_get_syspath(t->dev->dev);
	t->devpath = udev_device_get_devnode(t->dev->dev);
	return t;
}


fpga_result __XFPGA_API__ xfpga_fpgaEnumerate(const fpga_properties *filters,
				       uint32_t num_filters, fpga_token *tokens,
				       uint32_t max_tokens,
				       uint32_t *num_matches)
{
	dfl_device *e = dfl_device_enum();
	if (!e) {
		return FPGA_NOT_FOUND;
	}
	dfl_device *dev = e;
	while(dev) {
		if (matches_filters(dev, filters, num_filters)) {
			if (*num_matches < max_tokens) {
				tokens[*num_matches] = fpga_token_new(dev, true);
			}
			++(*num_matches);
		}
		dev = dev->next;
		
	}
	dfl_device_destroy(e);
	return FPGA_OK;
}

fpga_result __XFPGA_API__ xfpga_fpgaCloneToken(fpga_token src, fpga_token *dst)
{
	struct _fpga_token *_src = (struct _fpga_token *)src;
	*dst = fpga_token_new(_src->dev, true);
	return FPGA_OK;
}

fpga_result __XFPGA_API__ xfpga_fpgaDestroyToken(fpga_token *token)
{
	fpga_result result = FPGA_OK;
	int err = 0;

	if (NULL == token || NULL == *token) {
		OPAE_MSG("Invalid token pointer");
		return FPGA_INVALID_PARAM;
	}

	struct _fpga_token *_token = (struct _fpga_token *)*token;

	if (pthread_mutex_lock(&global_lock)) {
		OPAE_MSG("Failed to lock global mutex");
		return FPGA_EXCEPTION;
	}

	if (_token->magic != FPGA_TOKEN_MAGIC) {
		OPAE_MSG("Invalid token");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	// invalidate magic (just in case)
	_token->magic = FPGA_INVALID_MAGIC;
	dfl_device_destroy(_token->dev);
	free(*token);
	*token = NULL;

out_unlock:
	err = pthread_mutex_unlock(&global_lock);
	if (err) {
		OPAE_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
	}
	return result;
}
