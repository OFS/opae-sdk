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
#include <uuid/uuid.h>

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
	int err = 0;

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

	_iprop.vendor_id = _token->dev->vendor_id;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_VENDORID);

	_iprop.device_id = _token->dev->device_id;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_DEVICEID);

	_iprop.objtype = _token->dev->type;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_OBJTYPE);

	_iprop.segment = _token->dev->segment;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_SEGMENT);

	_iprop.bus = _token->dev->bus;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_BUS);

	_iprop.device = _token->dev->device;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_DEVICE);

	_iprop.function = _token->dev->function;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_FUNCTION);
	
	_iprop.object_id = _token->dev->object_id;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_OBJECTID);

	_iprop.socket_id = _token->dev->numa_node;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_SOCKETID);

	_iprop.num_errors = _token->dev->num_errors;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_NUM_ERRORS);


	if (_token->dev->type == FPGA_ACCELERATOR) {
		// AFU
		const char *afu_id = dfl_device_get_attr(_token->dev,
							 FPGA_SYSFS_AFU_GUID);
		if (afu_id && dfl_parse_guid(afu_id, _iprop.guid)) {
			SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_GUID);
		} else {
			OPAE_MSG("error reading/parsing afu_id: '%s'", afu_id);
		}

		dfl_device *parent = dfl_device_get_parent(_token->dev);
		if (parent) {
			_iprop.parent = fpga_token_new(parent, false);
			SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_PARENT);
		}


	} else if (_token->dev->type == FPGA_DEVICE) {
		// FME
		// get bitstream id
		if (!dfl_device_get_compat_id(_token->dev, _iprop.guid)) {
			SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_GUID);
		}

		if (!dfl_device_get_ports_num(_token->dev, &_iprop.u.fpga.num_slots)) {
			SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_NUM_SLOTS);
		}

		if (!dfl_device_get_bbs_id(_token->dev, &_iprop.u.fpga.bbs_id)) {
			SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_BBSID);
			_iprop.u.fpga.bbs_version.major =
					FPGA_BBS_VER_MAJOR(_iprop.u.fpga.bbs_id);
			_iprop.u.fpga.bbs_version.minor =
					FPGA_BBS_VER_MINOR(_iprop.u.fpga.bbs_id);
			_iprop.u.fpga.bbs_version.patch =
					FPGA_BBS_VER_PATCH(_iprop.u.fpga.bbs_id);
			SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_BBSVERSION);
		}
	}

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
