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

#include <opae/properties.h>

#include "xfpga.h"
#include "common_int.h"
#include "props.h"
#include "error_int.h"
#include "opae_drv.h"


fpga_result __XFPGA_API__
xfpga_fpgaGetPropertiesFromHandle(fpga_handle handle, fpga_properties *prop)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	result = xfpga_fpgaGetProperties(_handle->token, prop);

	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		OPAE_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
	}

	return result;
}

fpga_result __XFPGA_API__ xfpga_fpgaGetProperties(fpga_token token,
						 fpga_properties *prop)
{
	struct _fpga_properties *_prop = NULL;
	fpga_result result = FPGA_OK;

	ASSERT_NOT_NULL(prop);

	result = fpgaGetProperties(NULL, (fpga_properties *)&_prop);

	ASSERT_RESULT(result);

	if (token) {
		result = xfpga_fpgaUpdateProperties(token, _prop);
		if (result != FPGA_OK)
			goto out_free;
	}

	*prop = (fpga_properties)_prop;
	return result;

out_free:
	free(_prop);
	return result;
}

fpga_result __XFPGA_API__ xfpga_fpgaUpdateProperties(fpga_token token,
						    fpga_properties prop)
{
	struct _fpga_token *_token = (struct _fpga_token *)token;
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;

	struct _fpga_properties _iprop;

	char spath[SYSFS_PATH_MAX] = { 0, };
	char idpath[SYSFS_PATH_MAX] = { 0, };
	char *p;
	int s, b, d, f;
	int res;
	int err = 0;
	int resval = 0;
	uint64_t value = 0;
	uint32_t x = 0;
	size_t len;

	pthread_mutex_t lock;

	fpga_result result = FPGA_INVALID_PARAM;

	ASSERT_NOT_NULL(token);
	if (_token->magic != FPGA_TOKEN_MAGIC) {
		OPAE_MSG("Invalid token");
		return FPGA_INVALID_PARAM;
	}

	ASSERT_NOT_NULL(_prop);
	if (_prop->magic != FPGA_PROPERTY_MAGIC) {
		OPAE_MSG("Invalid properties object");
		return FPGA_INVALID_PARAM;
	}

	// clear fpga_properties buffer
	memset(&_iprop, 0, sizeof(struct _fpga_properties));
	_iprop.magic = FPGA_PROPERTY_MAGIC;

	// read the vendor and device ID from the 'device' path
	if (snprintf(idpath, sizeof(idpath),
		     "%s/../device/vendor", _token->sysfspath) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

	x = 0;
	result = sysfs_read_u32(idpath, &x);
	if (result != FPGA_OK)
		return result;
	_iprop.vendor_id = (uint16_t)x;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_VENDORID);

	if (snprintf(idpath, sizeof(idpath),
		     "%s/../device/device", _token->sysfspath) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

	x = 0;
	result = sysfs_read_u32(idpath, &x);
	if (result != FPGA_OK)
		return result;
	_iprop.device_id = (uint16_t)x;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_DEVICEID);

	// The input token is either for an FME or an AFU.
	// Go one level back to get to the dev.

	len = strnlen(_token->sysfspath, sizeof(spath) - 1);
	memcpy(spath, _token->sysfspath, len);
	spath[len] = '\0';

	p = strrchr(spath, '/');
	ASSERT_NOT_NULL_MSG(p, "Invalid token sysfs path");

	*p = 0;

	p = strstr(_token->sysfspath, FPGA_SYSFS_AFU);
	if (NULL != p) {
		// AFU
		result = sysfs_get_guid(_token, FPGA_SYSFS_AFU_GUID,
			 _iprop.guid);
		if (FPGA_OK != result)
			return result;
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_GUID);

		_iprop.parent = (fpga_token)token_get_parent(_token);
		if (NULL != _iprop.parent)
			SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_PARENT);

		_iprop.objtype = FPGA_ACCELERATOR;
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_OBJTYPE);

		_iprop.u.accelerator.num_mmio = 0;
		_iprop.u.accelerator.num_interrupts = 0;

		res = open(_token->devpath, O_RDWR);
		if (-1 == res) {
			_iprop.u.accelerator.state = FPGA_ACCELERATOR_ASSIGNED;
		} else {
			opae_port_info info = { 0, 0, 0, 0, 0 };

			if (opae_get_port_info(res, &info) == FPGA_OK) {
				_iprop.u.accelerator.num_mmio = info.num_regions;
				SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_NUM_MMIO);

				if (info.capability & OPAE_PORT_CAP_UAFU_IRQS) {
					_iprop.u.accelerator.num_interrupts =
						info.num_uafu_irqs;
					SET_FIELD_VALID(&_iprop,
						FPGA_PROPERTY_NUM_INTERRUPTS);
				}
			}

			close(res);
			_iprop.u.accelerator.state =
				FPGA_ACCELERATOR_UNASSIGNED;
		}
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_ACCELERATOR_STATE);
	}

	p = strstr(_token->sysfspath, FPGA_SYSFS_FME);
	if (NULL != p) {
		// FME
		_iprop.objtype = FPGA_DEVICE;
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_OBJTYPE);
		// get bitstream id
		result = sysfs_get_interface_id(_token, _iprop.guid);
		if (FPGA_OK != result)
			return result;
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_GUID);

		resval = sysfs_parse_attribute64(_token->sysfspath,
			FPGA_SYSFS_NUM_SLOTS, &value);
		if (resval != 0) {
			return FPGA_NOT_FOUND;
		}
		_iprop.u.fpga.num_slots = (uint32_t)value;
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_NUM_SLOTS);

		resval = sysfs_parse_attribute64(_token->sysfspath,
			FPGA_SYSFS_BITSTREAM_ID, &_iprop.u.fpga.bbs_id);
		if (resval != 0) {
			return FPGA_NOT_FOUND;
		}
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_BBSID);

		_iprop.u.fpga.bbs_version.major =
				FPGA_BBS_VER_MAJOR(_iprop.u.fpga.bbs_id);
		_iprop.u.fpga.bbs_version.minor =
				FPGA_BBS_VER_MINOR(_iprop.u.fpga.bbs_id);
		_iprop.u.fpga.bbs_version.patch =
				FPGA_BBS_VER_PATCH(_iprop.u.fpga.bbs_id);
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_BBSVERSION);
	}

	result = sysfs_sbdf_from_path(spath, &s, &b, &d, &f);
	if (result)
		return result;

	_iprop.segment = (uint16_t)s;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_SEGMENT);

	_iprop.bus = (uint8_t)b;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_BUS);

	_iprop.device = (uint8_t)d;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_DEVICE);

	_iprop.function = (uint8_t)f;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_FUNCTION);

	// only set socket id if we have it on sysfs
	if (sysfs_get_fme_path(_token->sysfspath, spath) == FPGA_OK) {
		resval = sysfs_parse_attribute64(spath,
			FPGA_SYSFS_SOCKET_ID, &value);

		if (0 == resval) {
			_iprop.socket_id = (uint8_t)value;
			SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_SOCKETID);
		}
	}

	result = sysfs_objectid_from_path(_token->sysfspath, &_iprop.object_id);
	if (0 == result)
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_OBJECTID);

	char errpath[SYSFS_PATH_MAX] = { 0, };

	if (snprintf(errpath, sizeof(errpath),
		     "%s/errors", _token->sysfspath) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

	_iprop.num_errors = count_error_files(errpath);
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_NUM_ERRORS);

	if (pthread_mutex_lock(&_prop->lock)) {
		OPAE_MSG("Failed to lock properties mutex");
		return FPGA_EXCEPTION;
	}

	lock = _prop->lock;
	*_prop = _iprop;
	_prop->lock = lock;

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));

	return FPGA_OK;
}
