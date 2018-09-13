// Copyright(c) 2018, Intel Corporation
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
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "safe_string/safe_string.h"

#include <opae/enum.h>

#include "props.h"

struct _fpga_properties *opae_properties_create(void)
{
	struct _fpga_properties *props;
	pthread_mutexattr_t mattr;
	int err;

	props = (struct _fpga_properties *)calloc(
		1, sizeof(struct _fpga_properties));

	if (!props)
		return NULL;

	props->magic = FPGA_PROPERTY_MAGIC;

	if (pthread_mutexattr_init(&mattr)) {
		OPAE_ERR("pthread_mutexattr_init() failed");
		goto out_free;
	}

	if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE)) {
		OPAE_ERR("pthread_mutexattr_settype() failed");
		goto out_destroy_attr;
	}

	if (pthread_mutex_init(&props->lock, &mattr)) {
		OPAE_ERR("pthread_mutex_init() failed");
		goto out_destroy_attr;
	}

	pthread_mutexattr_destroy(&mattr);

	return props;

out_destroy_attr:
	err = pthread_mutexattr_destroy(&mattr);
	if (err)
		OPAE_ERR("pthread_mutexattr_destroy() failed: %s",
			 strerror(err));
out_free:
	free(props);
	return NULL;
}

void opae_properties_destroy(struct _fpga_properties *props)
{
	int err;

	if (!props)
		return;

	opae_mutex_lock(err, &props->lock);
	props->magic = 0;
	opae_mutex_unlock(err, &props->lock);

	err = pthread_mutex_destroy(&props->lock);
	if (err)
		OPAE_ERR("pthread_mutex_destroy() failed: %s", strerror(err));

	free(props);
}

struct _fpga_properties *opae_properties_clone(fpga_properties props)
{
	int err;
	struct _fpga_properties *clone;
	pthread_mutex_t save_lock;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	if (!p)
		return NULL;

	clone = opae_properties_create();
	if (!clone) {
		opae_mutex_unlock(err, &p->lock);
		return NULL;
	}

	save_lock = clone->lock;

	*clone = *p;
	clone->lock = save_lock;

	opae_mutex_unlock(err, &p->lock);

	return clone;
}

fpga_result opae_properties_clear(fpga_properties props)
{
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	p->valid_fields = 0;

	opae_mutex_unlock(err, &p->lock);

	return FPGA_OK;
}

fpga_result opae_properties_get_parent(fpga_properties props,
				       fpga_token *parent)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(parent);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_PARENT)) {
		*parent = p->parent;
	} else {
		OPAE_MSG("No parent");
		res = FPGA_NOT_FOUND;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_parent(fpga_properties props, fpga_token parent)
{
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(parent);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	p->parent = parent;
	SET_FIELD_VALID(p, FPGA_PROPERTY_PARENT);

	opae_mutex_unlock(err, &p->lock);

	return FPGA_OK;
}

fpga_result opae_properties_get_object_type(fpga_properties props,
					    fpga_objtype *objtype)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(objtype);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)) {
		*objtype = p->objtype;
	} else {
		OPAE_MSG("No object type");
		res = FPGA_NOT_FOUND;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_object_type(fpga_properties props,
					    fpga_objtype objtype)
{
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	p->objtype = objtype;
	SET_FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE);

	opae_mutex_unlock(err, &p->lock);

	return FPGA_OK;
}

fpga_result opae_properties_get_segment(fpga_properties props,
					uint16_t *segment)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(segment);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_SEGMENT)) {
		*segment = p->segment;
	} else {
		OPAE_MSG("No segment");
		res = FPGA_NOT_FOUND;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_segment(fpga_properties props, uint16_t segment)
{
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	SET_FIELD_VALID(p, FPGA_PROPERTY_SEGMENT);
	p->segment = segment;

	opae_mutex_unlock(err, &p->lock);

	return FPGA_OK;
}

fpga_result opae_properties_get_bus(fpga_properties props, uint8_t *bus)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(bus);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_BUS)) {
		*bus = p->bus;
	} else {
		OPAE_MSG("No bus");
		res = FPGA_NOT_FOUND;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_bus(fpga_properties props, uint8_t bus)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	SET_FIELD_VALID(p, FPGA_PROPERTY_BUS);
	p->bus = bus;

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_device(fpga_properties props, uint8_t *device)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(device);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_DEVICE)) {
		*device = p->device;
	} else {
		OPAE_MSG("No device");
		res = FPGA_NOT_FOUND;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_device(fpga_properties props, uint8_t device)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	SET_FIELD_VALID(p, FPGA_PROPERTY_DEVICE);
	p->device = device;

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_function(fpga_properties props,
					 uint8_t *function)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(function);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_FUNCTION)) {
		*function = p->function;
	} else {
		OPAE_MSG("No function");
		res = FPGA_NOT_FOUND;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_function(fpga_properties props,
					 uint8_t function)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	SET_FIELD_VALID(p, FPGA_PROPERTY_FUNCTION);
	p->function = function;

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_socket_id(fpga_properties props,
					  uint8_t *socket_id)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(socket_id);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_SOCKETID)) {
		*socket_id = p->socket_id;
	} else {
		OPAE_MSG("No socket ID");
		res = FPGA_NOT_FOUND;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_socket_id(fpga_properties props,
					  uint8_t socket_id)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	SET_FIELD_VALID(p, FPGA_PROPERTY_SOCKETID);
	p->socket_id = socket_id;

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_device_id(fpga_properties props,
					  uint16_t *device_id)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(device_id);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_DEVICEID)) {
		*device_id = p->device_id;
	} else {
		OPAE_MSG("No device ID");
		res = FPGA_NOT_FOUND;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_device_id(fpga_properties props,
					  uint16_t device_id)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	SET_FIELD_VALID(p, FPGA_PROPERTY_DEVICEID);
	p->device_id = device_id;

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_num_slots(fpga_properties props,
					  uint32_t *num_slots)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(num_slots);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_DEVICE == p->objtype) {
		if (FIELD_VALID(p, FPGA_PROPERTY_NUM_SLOTS)) {
			*num_slots = p->u.fpga.num_slots;
		} else {
			OPAE_MSG("No number of slots");
			res = FPGA_NOT_FOUND;
		}
	} else {
		OPAE_ERR(
			"Attempting to get num_slots from invalid object type: %d",
			p->objtype);
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_num_slots(fpga_properties props,
					  uint32_t num_slots)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_DEVICE == p->objtype) {
		SET_FIELD_VALID(p, FPGA_PROPERTY_NUM_SLOTS);
		p->u.fpga.num_slots = num_slots;
	} else {
		OPAE_ERR(
			"Attempting to set num slots on invalid object type: %d",
			p->objtype);
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_bbs_id(fpga_properties props, uint64_t *bbs_id)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(bbs_id);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_DEVICE == p->objtype) {
		if (FIELD_VALID(p, FPGA_PROPERTY_BBSID)) {
			*bbs_id = p->u.fpga.bbs_id;
		} else {
			OPAE_MSG("No BBS ID");
			res = FPGA_NOT_FOUND;
		}
	} else {
		OPAE_ERR(
			"Attempting to get BBS ID from invalid object type: %d",
			p->objtype);
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_bbs_id(fpga_properties props, uint64_t bbs_id)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_DEVICE == p->objtype) {
		SET_FIELD_VALID(p, FPGA_PROPERTY_BBSID);
		p->u.fpga.bbs_id = bbs_id;
	} else {
		OPAE_ERR("Attempting to set BBS ID on invalid object type: %d",
			 p->objtype);
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_bbs_version(fpga_properties props,
					    fpga_version *bbs_version)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(bbs_version);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_DEVICE == p->objtype) {
		if (FIELD_VALID(p, FPGA_PROPERTY_BBSVERSION)) {
			*bbs_version = p->u.fpga.bbs_version;
		} else {
			OPAE_MSG("No BBS version");
			res = FPGA_NOT_FOUND;
		}
	} else {
		OPAE_ERR(
			"Attempting to get BBS version from invalid object type: %d",
			p->objtype);
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_bbs_version(fpga_properties props,
					    fpga_version bbs_version)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_DEVICE == p->objtype) {
		SET_FIELD_VALID(p, FPGA_PROPERTY_BBSVERSION);
		p->u.fpga.bbs_version = bbs_version;
	} else {
		OPAE_ERR(
			"Attempting to set BBS version on invalid object type: %d",
			p->objtype);
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_vendor_id(fpga_properties props,
					  uint16_t *vendor_id)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(vendor_id);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_VENDORID)) {
		*vendor_id = p->vendor_id;
	} else {
		OPAE_MSG("No vendor ID");
		res = FPGA_NOT_FOUND;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_vendor_id(fpga_properties props,
					  uint16_t vendor_id)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	SET_FIELD_VALID(p, FPGA_PROPERTY_VENDORID);
	p->vendor_id = vendor_id;

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_model(fpga_properties props, char *model)
{
	UNUSED_PARAM(props);
	UNUSED_PARAM(model);
	OPAE_MSG("Model not supported");
	return FPGA_NOT_SUPPORTED;
}

fpga_result opae_properties_set_model(fpga_properties props, char *model)
{
	UNUSED_PARAM(props);
	UNUSED_PARAM(model);
	OPAE_MSG("Model not supported");
	return FPGA_NOT_SUPPORTED;
}

fpga_result opae_properties_get_local_memory_size(fpga_properties props,
						  uint64_t *lms)
{
	UNUSED_PARAM(props);
	UNUSED_PARAM(lms);
	OPAE_MSG("Local memory not supported");
	return FPGA_NOT_SUPPORTED;
}

fpga_result opae_properties_set_local_memory_size(fpga_properties props,
						  uint64_t lms)
{
	UNUSED_PARAM(props);
	UNUSED_PARAM(lms);
	OPAE_MSG("Local memory not supported");
	return FPGA_NOT_SUPPORTED;
}

fpga_result opae_properties_get_capabilities(fpga_properties props,
					     uint64_t *capabilities)
{
	UNUSED_PARAM(props);
	UNUSED_PARAM(capabilities);
	OPAE_MSG("Capabilities not supported");
	return FPGA_NOT_SUPPORTED;
}

fpga_result opae_properties_set_capabilities(fpga_properties props,
					     uint64_t capabilities)
{
	UNUSED_PARAM(props);
	UNUSED_PARAM(capabilities);
	OPAE_MSG("Capabilities not supported");
	return FPGA_NOT_SUPPORTED;
}

fpga_result opae_properties_get_guid(fpga_properties props, fpga_guid *guid)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(guid);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_GUID)) {
		errno_t e;
		e = memcpy_s(*guid, sizeof(fpga_guid), p->guid,
			     sizeof(fpga_guid));
		if (EOK != e) {
			OPAE_ERR("memcpy_s failed");
			res = FPGA_EXCEPTION;
		}
	} else {
		OPAE_MSG("No GUID");
		res = FPGA_NOT_FOUND;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_guid(fpga_properties props, fpga_guid guid)
{
	fpga_result res = FPGA_OK;
	int err;
	errno_t e;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	SET_FIELD_VALID(p, FPGA_PROPERTY_GUID);
	e = memcpy_s(p->guid, sizeof(fpga_guid), guid, sizeof(fpga_guid));
	if (EOK != e) {
		OPAE_ERR("memcpy_s failed");
		res = FPGA_EXCEPTION;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_num_mmio(fpga_properties props,
					 uint32_t *mmio_spaces)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(mmio_spaces);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_ACCELERATOR == p->objtype) {
		if (FIELD_VALID(p, FPGA_PROPERTY_NUM_MMIO)) {
			*mmio_spaces = p->u.accelerator.num_mmio;
		} else {
			OPAE_MSG("No MMIO spaces");
			res = FPGA_NOT_FOUND;
		}
	} else {
		OPAE_ERR(
			"Attempting to get number of MMIO spaces from invalid object type: %d",
			p->objtype);
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_num_mmio(fpga_properties props,
					 uint32_t mmio_spaces)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_ACCELERATOR == p->objtype) {
		SET_FIELD_VALID(p, FPGA_PROPERTY_NUM_MMIO);
		p->u.accelerator.num_mmio = mmio_spaces;
	} else {
		OPAE_ERR(
			"Attempting to set number of MMIO spaces on invalid object type: %d",
			p->objtype);
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_num_interrupts(fpga_properties props,
					       uint32_t *num_interrupts)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(num_interrupts);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_ACCELERATOR == p->objtype) {
		if (FIELD_VALID(p, FPGA_PROPERTY_NUM_INTERRUPTS)) {
			*num_interrupts = p->u.accelerator.num_interrupts;
		} else {
			OPAE_MSG("No interrupts");
			res = FPGA_NOT_FOUND;
		}
	} else {
		OPAE_ERR(
			"Attempting to get number of interrupts from invalid object type: %d",
			p->objtype);
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_num_interrupts(fpga_properties props,
					       uint32_t num_interrupts)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_ACCELERATOR == p->objtype) {
		SET_FIELD_VALID(p, FPGA_PROPERTY_NUM_INTERRUPTS);
		p->u.accelerator.num_interrupts = num_interrupts;
	} else {
		OPAE_ERR(
			"Attempting to set number of interrupts on invalid object type: %d",
			p->objtype);
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_accelerator_state(fpga_properties props,
						  fpga_accelerator_state *state)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(state);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_ACCELERATOR == p->objtype) {
		if (FIELD_VALID(p, FPGA_PROPERTY_ACCELERATOR_STATE)) {
			*state = p->u.accelerator.state;
		} else {
			OPAE_MSG("No accelerator state");
			res = FPGA_NOT_FOUND;
		}
	} else {
		OPAE_ERR("Attempting to get state from invalid object type: %d",
			 p->objtype);
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_accelerator_state(fpga_properties props,
						  fpga_accelerator_state state)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJTYPE)
	    && FPGA_ACCELERATOR == p->objtype) {
		SET_FIELD_VALID(p, FPGA_PROPERTY_ACCELERATOR_STATE);
		p->u.accelerator.state = state;
	} else {
		OPAE_ERR("Attempting to set state from invalid object type: %d",
			 p->objtype);
		res = FPGA_INVALID_PARAM;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_object_id(fpga_properties props,
					  uint64_t *object_id)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(object_id);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_OBJECTID)) {
		*object_id = p->object_id;
	} else {
		OPAE_MSG("No object ID");
		res = FPGA_NOT_FOUND;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_object_id(fpga_properties props,
					  uint64_t object_id)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	SET_FIELD_VALID(p, FPGA_PROPERTY_OBJECTID);
	p->object_id = object_id;

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_get_num_errors(fpga_properties props,
					   uint32_t *num_errors)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p;

	ASSERT_NOT_NULL(num_errors);

	p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_NUM_ERRORS)) {
		*num_errors = p->num_errors;
	} else {
		OPAE_MSG("No num errors");
		res = FPGA_NOT_FOUND;
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result opae_properties_set_num_errors(fpga_properties props,
					   uint32_t num_errors)
{
	fpga_result res = FPGA_OK;
	int err;
	struct _fpga_properties *p = opae_validate_and_lock_properties(props);

	ASSERT_NOT_NULL(p);

	SET_FIELD_VALID(p, FPGA_PROPERTY_NUM_ERRORS);
	p->num_errors = num_errors;

	opae_mutex_unlock(err, &p->lock);

	return res;
}
