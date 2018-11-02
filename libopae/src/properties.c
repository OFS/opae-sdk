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

#include "common_int.h"
#include "opae/properties.h"
#include "opae/enum.h"
#include "opae/utils.h"
#include "properties_int.h"
#include "error_int.h"

#include "safe_string/safe_string.h"


fpga_result __FPGA_API__ fpgaGetPropertiesFromHandle(fpga_handle handle, fpga_properties *prop)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	result = fpgaGetProperties(_handle->token, prop);

	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
	}

	return result;
}


fpga_result __FPGA_API__ fpgaGetProperties(fpga_token token, fpga_properties *prop)
{
	struct _fpga_properties *_prop;
	fpga_result result = FPGA_OK;
	pthread_mutexattr_t mattr;
	int err = 0;

	ASSERT_NOT_NULL(prop);

	_prop = malloc(sizeof(struct _fpga_properties));
	if (NULL == _prop) {
		FPGA_MSG("Failed to allocate memory for properties");
		return FPGA_NO_MEMORY;
	}
	memset_s(_prop, sizeof(struct _fpga_properties), 0);
	// mark data structure as valid
	_prop->magic = FPGA_PROPERTY_MAGIC;

	if (pthread_mutexattr_init(&mattr)) {
		FPGA_MSG("Failed to initialized property mutex attributes");
		result = FPGA_EXCEPTION;
		goto out_free;
	}

	if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE)) {
		FPGA_MSG("Failed to initialize property mutex attributes");
		result = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}

	if (pthread_mutex_init(&_prop->lock, &mattr)) {
		FPGA_MSG("Failed to initialize property mutex");
		result = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}

	if (token) {
		result = fpgaUpdateProperties(token, _prop);
		if (result != FPGA_OK)
			goto out_mutex_destroy;
	}

	pthread_mutexattr_destroy(&mattr);
	*prop = (fpga_properties)_prop;
	return result;

out_mutex_destroy:
	err = pthread_mutex_destroy(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_destroy() failed: %s", strerror(err));

out_attr_destroy:
	err = pthread_mutexattr_destroy(&mattr);
	if (err)
		FPGA_ERR("pthread_mutexatr_destroy() failed: %s", strerror(err));

out_free:
	free(_prop);
	return result;
}

fpga_result __FPGA_API__ fpgaClearProperties(fpga_properties prop)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	_prop->valid_fields = 0;

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__ fpgaCloneProperties(fpga_properties src,
					     fpga_properties *dst)
{
	struct _fpga_properties *_src = (struct _fpga_properties *)src;
	struct _fpga_properties *_dst;
	int err = 0;
	pthread_mutexattr_t mattr;
	fpga_result result;

	ASSERT_NOT_NULL(dst);
	result = prop_check_and_lock(_src);
	if (result)
		return result;

	_dst = malloc(sizeof(struct _fpga_properties));
	if (NULL == _dst) {
		FPGA_MSG("Failed to allocate memory for properties");
		err = pthread_mutex_unlock(&_src->lock);
		if (err)
			FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
		return FPGA_NO_MEMORY;
	}

	*_dst = *_src;

	err = pthread_mutex_unlock(&_src->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));

	/* we just copied a locked mutex, so reinitialize it */
	if (pthread_mutexattr_init(&mattr)) {
		FPGA_MSG("Failed to initialize dst mutex attributes");
		result = FPGA_EXCEPTION;
		goto out_free;
	}

	if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE) ||
	    pthread_mutex_init(&_dst->lock, &mattr)) {
		FPGA_MSG("Failed to initialize dst mutex");
		result = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}

	pthread_mutexattr_destroy(&mattr);

	*dst = _dst;
	return FPGA_OK;

out_attr_destroy:
	pthread_mutexattr_destroy(&mattr);

out_free:
	free(_dst);
	return result;
}

fpga_result __FPGA_API__ fpgaDestroyProperties(fpga_properties *prop)
{
	struct _fpga_properties *_prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(prop);

	_prop = (struct _fpga_properties *)*prop;
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	// invalidate magic (just in case)
	_prop->magic = FPGA_INVALID_MAGIC;

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	err = pthread_mutex_destroy(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_destroy() failed: %s", strerror(err));

	free(*prop);
	*prop = NULL;

	return result;
}

fpga_result __FPGA_API__
fpgaUpdateProperties(fpga_token token, fpga_properties prop)
{
	struct _fpga_token *_token = (struct _fpga_token *)token;
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;

	struct _fpga_properties _iprop;

	char spath[SYSFS_PATH_MAX];
	char *p;
	int s, b, d, f;
	int res;
	errno_t e;
	int err = 0;

	pthread_mutex_t lock;

	fpga_result result = FPGA_INVALID_PARAM;

	ASSERT_NOT_NULL(token);
	if (_token->magic != FPGA_TOKEN_MAGIC) {
		FPGA_MSG("Invalid token");
		return FPGA_INVALID_PARAM;
	}

	ASSERT_NOT_NULL(_prop);
	if (_prop->magic != FPGA_PROPERTY_MAGIC) {
		FPGA_MSG("Invalid properties object");
		return FPGA_INVALID_PARAM;
	}

	//clear fpga_properties buffer
	memset_s(&_iprop, sizeof(struct _fpga_properties), 0);
	_iprop.magic = FPGA_PROPERTY_MAGIC;

	// The input token is either for an FME or an AFU.
	// Go one level back to get to the dev.

	e = strncpy_s(spath, sizeof(spath),
			_token->sysfspath, sizeof(_token->sysfspath));
	if (EOK != e) {
		FPGA_ERR("strncpy_s failed");
		return FPGA_EXCEPTION;
	}

	p = strrchr(spath, '/');
	ASSERT_NOT_NULL_MSG(p, "Invalid token sysfs path");

	*p = 0;

	p = strstr(_token->sysfspath, FPGA_SYSFS_AFU);
	if (NULL != p) {
		// AFU
		result = sysfs_get_afu_id(_token->device_instance,
								  _token->subdev_instance,
								  _iprop.guid);
		if (FPGA_OK != result)
			return result;
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_GUID);

		_iprop.parent = (fpga_token) token_get_parent(_token);
		if (NULL != _iprop.parent)
			SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_PARENT);

		_iprop.objtype = FPGA_ACCELERATOR;
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_OBJTYPE);

		res = open(_token->devpath, O_RDWR);
		if (-1 == res) {
			_iprop.u.accelerator.state = FPGA_ACCELERATOR_ASSIGNED;
		} else {
			close(res);
			_iprop.u.accelerator.state = FPGA_ACCELERATOR_UNASSIGNED;
		}
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_ACCELERATOR_STATE);

		_iprop.u.accelerator.num_mmio = 2;
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_NUM_MMIO);

		_iprop.u.accelerator.num_interrupts = 0;
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_NUM_INTERRUPTS);
	}

	p = strstr(_token->sysfspath, FPGA_SYSFS_FME);
	if (NULL != p) {
		// FME
		_iprop.objtype = FPGA_DEVICE;
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_OBJTYPE);
		// get bitstream id
		result = sysfs_get_pr_id(_token->device_instance,
								 _token->subdev_instance,
								 _iprop.guid);
		if (FPGA_OK != result)
			return result;
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_GUID);

		result = sysfs_get_slots(_token->device_instance,
								 _token->subdev_instance,
								 &_iprop.u.fpga.num_slots);
		if (FPGA_OK != result)
			return result;
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_NUM_SLOTS);

		result = sysfs_get_bitstream_id(_token->device_instance,
										_token->subdev_instance,
										&_iprop.u.fpga.bbs_id);
		if (FPGA_OK != result)
			return result;
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

	_iprop.segment = (uint16_t) s;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_SEGMENT);

	_iprop.bus = (uint8_t) b;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_BUS);

	_iprop.device = (uint8_t) d;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_DEVICE);

	_iprop.function = (uint8_t) f;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_FUNCTION);

	// only set socket id if we have it on sysfs
	result = sysfs_get_socket_id(_token->device_instance,
								 _token->subdev_instance,
								 &_iprop.socket_id);
	if (0 == result)
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_SOCKETID);

	result = sysfs_objectid_from_path(_token->sysfspath, &_iprop.object_id);
	if (0 == result)
		SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_OBJECTID);

	// read the vendor and device ID from the 'device' path
	uint32_t x = 0;
	char vendorpath[SYSFS_PATH_MAX];
	snprintf_s_s(vendorpath, SYSFS_PATH_MAX, "%s/../device/vendor", _token->sysfspath);
	result = sysfs_read_u32(vendorpath, &x);
	if (result != FPGA_OK)
		return result;
	_iprop.vendor_id = (uint16_t)x;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_VENDORID);

	char devicepath[SYSFS_PATH_MAX];
	snprintf_s_s(devicepath, SYSFS_PATH_MAX, "%s/../device/device", _token->sysfspath);
	result = sysfs_read_u32(devicepath, &x);
	if (result != FPGA_OK)
		return result;
	_iprop.device_id = (uint16_t)x;
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_DEVICEID);

	char errpath[SYSFS_PATH_MAX];
	snprintf_s_s(errpath, SYSFS_PATH_MAX, "%s/errors", _token->sysfspath);
	_iprop.num_errors = count_error_files(errpath);
	SET_FIELD_VALID(&_iprop, FPGA_PROPERTY_NUM_ERRORS);

	if (pthread_mutex_lock(&_prop->lock)) {
		FPGA_MSG("Failed to lock properties mutex");
		return FPGA_EXCEPTION;
	}

	lock = _prop->lock;
	*_prop = _iprop;
	_prop->lock = lock;

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));

	return FPGA_OK;
}

fpga_result __FPGA_API__
fpgaPropertiesGetParent(const fpga_properties prop, fpga_token *parent)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(parent);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_PARENT)) {
		result = fpgaCloneToken(_prop->parent, parent);
		if (FPGA_OK != result)
			FPGA_MSG("Error cloning token from property");
	} else {
		FPGA_MSG("No parent");
		result = FPGA_NOT_FOUND;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesSetParent(fpga_properties prop, fpga_token parent)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	_prop->parent = parent;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesGetObjectType(const fpga_properties prop, fpga_objtype *objtype)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(objtype);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)) {
		*objtype = _prop->objtype;
	} else {
		FPGA_MSG("No object type");
		result = FPGA_NOT_FOUND;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesSetObjectType(fpga_properties prop, fpga_objtype objtype)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	_prop->objtype = objtype;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__ fpgaPropertiesGetSegment(const fpga_properties prop, uint16_t *segment)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(segment);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_SEGMENT)) {
		*segment = _prop->segment;
	} else {
		FPGA_MSG("No segment");
		result = FPGA_NOT_FOUND;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__ fpgaPropertiesSetSegment(fpga_properties prop, uint16_t segment)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	_prop->segment = segment;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_SEGMENT);

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__ fpgaPropertiesGetBus(const fpga_properties prop, uint8_t *bus)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(bus);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_BUS)) {
		*bus = _prop->bus;
	} else {
		FPGA_MSG("No bus");
		result = FPGA_NOT_FOUND;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}


fpga_result __FPGA_API__ fpgaPropertiesSetBus(fpga_properties prop, uint8_t bus)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	_prop->bus = bus;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_BUS);

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}


fpga_result __FPGA_API__
fpgaPropertiesGetDevice(const fpga_properties prop, uint8_t *device)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(device);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_DEVICE)) {
		*device = _prop->device;
	} else {
		FPGA_MSG("No device");
		result = FPGA_NOT_FOUND;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}


fpga_result __FPGA_API__ fpgaPropertiesSetDevice(fpga_properties prop, uint8_t device)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	// PCIe supports 32 devices per bus.
	if (device > 31) {
		FPGA_MSG("Invalid device number");
		return FPGA_INVALID_PARAM;
	}

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	_prop->device = device;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_DEVICE);

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}


fpga_result __FPGA_API__
fpgaPropertiesGetFunction(const fpga_properties prop, uint8_t *function)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(function);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_FUNCTION)) {
		*function = _prop->function;
	} else {
		FPGA_MSG("No function");
		result = FPGA_NOT_FOUND;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}


fpga_result __FPGA_API__
fpgaPropertiesSetFunction(fpga_properties prop, uint8_t function)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	// PCIe supports 8 functions per device.
	if (function > 7) {
		FPGA_MSG("Invalid function number");
		return FPGA_INVALID_PARAM;
	}

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	_prop->function = function;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_FUNCTION);

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesGetSocketID(const fpga_properties prop, uint8_t *socket_id)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(socket_id);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_SOCKETID)) {
		*socket_id = _prop->socket_id;
	} else {
		FPGA_MSG("No socket ID");
		result = FPGA_NOT_FOUND;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesSetSocketID(fpga_properties prop, uint8_t socket_id)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	_prop->socket_id = socket_id;
	SET_FIELD_VALID(_prop, FPGA_PROPERTY_SOCKETID);

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesGetDeviceID(const fpga_properties prop, uint16_t *device_id)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *) prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(device_id);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_DEVICEID)) {
		*device_id = _prop->device_id;
		result = FPGA_OK;
	} else {
		result = FPGA_NOT_FOUND;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesSetDeviceID(fpga_properties prop, uint16_t device_id)
{
	int err = 0;
	fpga_result result = FPGA_OK;

	struct _fpga_properties *_prop = (struct _fpga_properties *) prop;
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	SET_FIELD_VALID(_prop, FPGA_PROPERTY_DEVICEID);
	_prop->device_id = device_id;

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesGetNumSlots(const fpga_properties prop, uint32_t *num_slots)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(num_slots);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_DEVICE == _prop->objtype) {
		if (FIELD_VALID(_prop, FPGA_PROPERTY_NUM_SLOTS)) {
			*num_slots = _prop->u.fpga.num_slots;
		} else {
			FPGA_MSG("No number of slots");
			result = FPGA_NOT_FOUND;
		}
	} else {
		FPGA_ERR
		    ("Attempting to get num_slots from invalid object type: %d",
		     _prop->objtype);
		result = FPGA_INVALID_PARAM;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesSetNumSlots(fpga_properties prop, uint32_t num_slots)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_DEVICE == _prop->objtype) {
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_SLOTS);
		_prop->u.fpga.num_slots = num_slots;
	} else {
		FPGA_ERR
		    ("Attempting to set num_slots from invalid object type: %d",
		     _prop->objtype);
		result = FPGA_INVALID_PARAM;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesGetBBSID(const fpga_properties prop, uint64_t *bbs_id)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(bbs_id);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_DEVICE == _prop->objtype) {
		if (FIELD_VALID(_prop, FPGA_PROPERTY_BBSID)) {
			*bbs_id = _prop->u.fpga.bbs_id;
		} else {
			FPGA_MSG("No BBS ID");
			result = FPGA_NOT_FOUND;
		}
	} else {
		FPGA_ERR
		    ("Attempting to get BBS ID from invalid object type: %d",
		     _prop->objtype);
		result = FPGA_INVALID_PARAM;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesSetBBSID(fpga_properties prop, uint64_t bbs_id)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_DEVICE == _prop->objtype) {
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_BBSID);
		_prop->u.fpga.bbs_id = bbs_id;
	} else {
		FPGA_ERR
		    ("Attempting to set BBS ID from invalid object type: %d",
		     _prop->objtype);
		result = FPGA_INVALID_PARAM;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}


fpga_result __FPGA_API__
fpgaPropertiesGetBBSVersion(const fpga_properties prop,
			    fpga_version *bbs_version)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(bbs_version);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_DEVICE == _prop->objtype) {
		if (FIELD_VALID(_prop, FPGA_PROPERTY_BBSVERSION)) {
			*bbs_version = _prop->u.fpga.bbs_version;
		} else {
			FPGA_MSG("No BBS version");
			result = FPGA_NOT_FOUND;
		}
	} else {
		FPGA_ERR
		    ("Attempting to get BBS version from invalid object type: %d",
		     _prop->objtype);
		result = FPGA_INVALID_PARAM;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesSetBBSVersion(fpga_properties prop,
			    fpga_version bbs_version)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_DEVICE == _prop->objtype) {
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_BBSVERSION);
		_prop->u.fpga.bbs_version = bbs_version;
	} else {
		FPGA_ERR
		    ("Attempting to set BBS version from invalid object type: %d",
		     _prop->objtype);
		result = FPGA_INVALID_PARAM;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesGetVendorID(const fpga_properties prop, uint16_t *vendor_id)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *) prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(vendor_id);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_VENDORID)) {
		*vendor_id = _prop->vendor_id;
		result = FPGA_OK;
	} else {
		result = FPGA_NOT_FOUND;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesSetVendorID(fpga_properties prop, uint16_t vendor_id)
{
	int err = 0;
	fpga_result result = FPGA_OK;

	struct _fpga_properties *_prop = (struct _fpga_properties *) prop;
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	SET_FIELD_VALID(_prop, FPGA_PROPERTY_VENDORID);
	_prop->vendor_id = vendor_id;

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesGetModel(const fpga_properties prop, char *model)
{
	UNUSED_PARAM(prop);
	UNUSED_PARAM(model);
	FPGA_MSG("Model not supported");
	return FPGA_NOT_SUPPORTED;
}

fpga_result __FPGA_API__
fpgaPropertiesSetModel(fpga_properties prop, char *model)
{
	UNUSED_PARAM(prop);
	UNUSED_PARAM(model);
	FPGA_MSG("Model not supported");
	return FPGA_NOT_SUPPORTED;
}

fpga_result __FPGA_API__
fpgaPropertiesGetLocalMemorySize(const fpga_properties prop,
				 uint64_t *local_memory_size)
{
	UNUSED_PARAM(prop);
	UNUSED_PARAM(local_memory_size);
	FPGA_MSG("Local memory not supported");
	return FPGA_NOT_SUPPORTED;
}

fpga_result __FPGA_API__
fpgaPropertiesSetLocalMemorySize(fpga_properties prop,
				 uint64_t local_memory_size)
{
	UNUSED_PARAM(prop);
	UNUSED_PARAM(local_memory_size);
	FPGA_MSG("Local memory not supported");
	return FPGA_NOT_SUPPORTED;
}

fpga_result __FPGA_API__
fpgaPropertiesGetCapabilities(const fpga_properties prop,
			      uint64_t *capabilities)
{
	UNUSED_PARAM(prop);
	UNUSED_PARAM(capabilities);
	FPGA_MSG("Capabilities not supported");
	return FPGA_NOT_SUPPORTED;
}

fpga_result __FPGA_API__
fpgaPropertiesSetCapabilities(fpga_properties prop,
			      uint64_t capabilities)
{
	UNUSED_PARAM(prop);
	UNUSED_PARAM(capabilities);
	FPGA_MSG("Capabilities not supported");
	return FPGA_NOT_SUPPORTED;
}

fpga_result __FPGA_API__ fpgaPropertiesGetGUID(const fpga_properties prop, fpga_guid *guid)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(guid);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_GUID)) {
		errno_t e;
		e = memcpy_s(*guid, sizeof(fpga_guid),
				_prop->guid, sizeof(fpga_guid));
		if (EOK != e) {
			FPGA_ERR("memcpy_s failed");
			result = FPGA_EXCEPTION;
		} else {
			result = FPGA_OK;
		}
	} else {
		FPGA_MSG("No GUID");
		result = FPGA_NOT_FOUND;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__ fpgaPropertiesSetGUID(fpga_properties prop, fpga_guid guid)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	errno_t e;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	SET_FIELD_VALID(_prop, FPGA_PROPERTY_GUID);
	e = memcpy_s(_prop->guid, sizeof(fpga_guid),
			guid, sizeof(fpga_guid));
	if (EOK != e) {
		FPGA_ERR("memcpy_s failed");
		result = FPGA_EXCEPTION;
	} else {
		result = FPGA_OK;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesGetNumMMIO(const fpga_properties prop, uint32_t *mmio_spaces)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(mmio_spaces);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_ACCELERATOR == _prop->objtype) {
		if (FIELD_VALID(_prop, FPGA_PROPERTY_NUM_MMIO)) {
			*mmio_spaces = _prop->u.accelerator.num_mmio;
		} else {
			FPGA_MSG("No MMIO spaces");
			result = FPGA_NOT_FOUND;
		}
	} else {
		FPGA_ERR
		    ("Attempting to get number of MMIO spaces from invalid object type: %d",
		     _prop->objtype);
		result = FPGA_INVALID_PARAM;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesSetNumMMIO(fpga_properties prop, uint32_t mmio_spaces)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_ACCELERATOR == _prop->objtype) {
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_MMIO);
		_prop->u.accelerator.num_mmio = mmio_spaces;
	} else {
		FPGA_ERR
		    ("Attempting to set number of MMIO spaces on invalid object type: %d",
		     _prop->objtype);
		result = FPGA_INVALID_PARAM;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesGetNumInterrupts(const fpga_properties prop,
			       uint32_t *num_interrupts)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(num_interrupts);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_ACCELERATOR == _prop->objtype) {
		if (FIELD_VALID(_prop, FPGA_PROPERTY_NUM_INTERRUPTS)) {
			*num_interrupts = _prop->u.accelerator.num_interrupts;
		} else {
			FPGA_MSG("No interrupts");
			result = FPGA_NOT_FOUND;
		}
	} else {
		FPGA_ERR
		    ("Attempting to get number of interrupts from invalid object type: %d",
		     _prop->objtype);
		result = FPGA_INVALID_PARAM;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesSetNumInterrupts(fpga_properties prop,
			       uint32_t num_interrupts)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_ACCELERATOR == _prop->objtype) {
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_INTERRUPTS);
		_prop->u.accelerator.num_interrupts = num_interrupts;
	} else {
		FPGA_ERR
		    ("Attempting to set number of interrupts from invalid object type: %d",
		     _prop->objtype);
		result = FPGA_INVALID_PARAM;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesGetAcceleratorState(const fpga_properties prop, fpga_accelerator_state *state)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(state);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_ACCELERATOR == _prop->objtype) {
		if (FIELD_VALID(_prop, FPGA_PROPERTY_ACCELERATOR_STATE)) {
			*state = _prop->u.accelerator.state;
		} else {
			FPGA_MSG("No accelerator state");
			result = FPGA_NOT_FOUND;
		}
	} else {
		FPGA_ERR
		    ("Attempting to get state from invalid object type: %d",
		     _prop->objtype);
		result = FPGA_INVALID_PARAM;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesSetAcceleratorState(fpga_properties prop, fpga_accelerator_state state)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_ACCELERATOR == _prop->objtype) {
		SET_FIELD_VALID(_prop, FPGA_PROPERTY_ACCELERATOR_STATE);
		_prop->u.accelerator.state = state;
	} else {
		FPGA_ERR
		    ("Attempting to set state from invalid object type: %d",
		     _prop->objtype);
		result = FPGA_INVALID_PARAM;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesGetObjectID(const fpga_properties prop, uint64_t *object_id)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(object_id);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_OBJECTID)) {
		*object_id = _prop->object_id;
	} else {
		FPGA_MSG("No object_id");
		result = FPGA_NOT_FOUND;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesSetObjectID(fpga_properties prop, uint64_t object_id)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJECTID);
	_prop->object_id = object_id;

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesGetNumErrors(const fpga_properties prop,
			   uint32_t *num_errors)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	ASSERT_NOT_NULL(num_errors);
	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	if (FIELD_VALID(_prop, FPGA_PROPERTY_NUM_ERRORS)) {
		*num_errors = _prop->num_errors;
	} else {
		FPGA_MSG("No num_errors");
		result = FPGA_NOT_FOUND;
	}

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__
fpgaPropertiesSetNumErrors(const fpga_properties prop,
			   uint32_t num_errors)
{
	struct _fpga_properties *_prop = (struct _fpga_properties *)prop;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = prop_check_and_lock(_prop);
	if (result)
		return result;

	SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_ERRORS);
	_prop->num_errors = num_errors;

	err = pthread_mutex_unlock(&_prop->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}
