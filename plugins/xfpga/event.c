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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/eventfd.h>
#include <errno.h>

#include "safe_string/safe_string.h"

#include <opae/properties.h>
#include "xfpga.h"
#include "common_int.h"
#include "opae_drv.h"
#include "types_int.h"
#include "intel-fpga.h"

#define EVENT_SOCKET_NAME "/tmp/fpga_event_socket"
#define EVENT_SOCKET_NAME_LEN 23

enum request_type { REGISTER_EVENT = 0, UNREGISTER_EVENT = 1 };

struct event_request {
	enum request_type type;
	fpga_event_type event;
	uint64_t object_id;
};

fpga_result send_event_request(int conn_socket, int fd,
			       struct event_request *req)
{
	struct msghdr mh;
	struct cmsghdr *cmh;
	struct iovec iov[1];
	char buf[CMSG_SPACE(sizeof(int))];
	ssize_t n;
	int *fd_ptr;

	/* set up ancillary data message header */
	iov[0].iov_base = req;
	iov[0].iov_len = sizeof(*req);
	memset_s(buf, sizeof(buf), 0);
	mh.msg_name = NULL;
	mh.msg_namelen = 0;
	mh.msg_iov = iov;
	mh.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
	mh.msg_control = buf;
	mh.msg_controllen = CMSG_LEN(sizeof(int));
	mh.msg_flags = 0;
	cmh = CMSG_FIRSTHDR(&mh);
	cmh->cmsg_len = CMSG_LEN(sizeof(int));
	cmh->cmsg_level = SOL_SOCKET;
	cmh->cmsg_type = SCM_RIGHTS;
	fd_ptr = (int *)CMSG_DATA(cmh);
	*fd_ptr = fd;
	/* send ancillary data */
	n = sendmsg(conn_socket, &mh, 0);
	if (n < 0) {
		OPAE_ERR("sendmsg failed: %s", strerror(errno));
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

STATIC fpga_result send_fme_event_request(fpga_handle handle,
					  fpga_event_handle event_handle,
					  int fme_operation)
{
	int fd = FILE_DESCRIPTOR(event_handle);
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	fpga_result res = FPGA_OK;
	opae_fme_info fme_info = { 0 };

	if (fme_operation != FPGA_IRQ_ASSIGN
	    && fme_operation != FPGA_IRQ_DEASSIGN) {
		OPAE_ERR("Invalid FME operation requested");
		return FPGA_INVALID_PARAM;
	}

	res = opae_get_fme_info(_handle->fddev, &fme_info);
	if (res) {
		return res;
	}

	/*capability field is set to 1 if the platform supports interrupts*/
	if (fme_info.capability & FPGA_FME_CAP_ERR_IRQ) {
		res = opae_fme_set_err_irq(_handle->fddev, 0, fme_operation == FPGA_IRQ_ASSIGN ? fd : -1);
		if (res) {
			OPAE_ERR("Could not set eventfd %s", strerror(errno));
		}
	} else {
		OPAE_ERR("FME interrupts not supported in hw");
		res = FPGA_NOT_SUPPORTED;
	}

	return res;
}

STATIC fpga_result send_port_event_request(fpga_handle handle,
					   fpga_event_handle event_handle,
					   int port_operation)
{
	fpga_result res = FPGA_OK;
	int fd = FILE_DESCRIPTOR(event_handle);
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	opae_port_info port_info = { 0 };
	if (port_operation != FPGA_IRQ_ASSIGN
	    && port_operation != FPGA_IRQ_DEASSIGN) {
		OPAE_ERR("Invalid PORT operation requested");
		return FPGA_INVALID_PARAM;
	}

	res = opae_get_port_info(_handle->fddev, &port_info);
	if (res) {
		return res;
	}

	/*capability field is set to 1 if the platform supports interrupts*/
	if (port_info.capability & FPGA_PORT_CAP_ERR_IRQ) {
		res = opae_port_set_err_irq(_handle->fddev, 0, port_operation == FPGA_IRQ_ASSIGN ? fd : -1);
		if (res) {
			OPAE_ERR("Could not set eventfd");
		}
	} else {
		OPAE_ERR("PORT interrupts not supported in hw");
		res = FPGA_NOT_SUPPORTED;
	}

	return res;
}

STATIC fpga_result send_uafu_event_request(fpga_handle handle,
					   fpga_event_handle event_handle,
					   uint32_t flags, int uafu_operation)
{
	int res = FPGA_OK;
	int fd = FILE_DESCRIPTOR(event_handle);
	struct _fpga_event_handle *_eh =
		(struct _fpga_event_handle *)event_handle;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	opae_port_info port_info = { 0 };
	int32_t neg = -1;

	if (uafu_operation != FPGA_IRQ_ASSIGN
	    && uafu_operation != FPGA_IRQ_DEASSIGN) {
		OPAE_ERR("Invalid UAFU operation requested");
		return FPGA_INVALID_PARAM;
	}

	res = opae_get_port_info(_handle->fddev, &port_info);
	if (res) {
		return res;
	}

	/*capability field is set to 1 if the platform supports interrupts*/
	if (port_info.capability & FPGA_PORT_CAP_UAFU_IRQ) {
		if (flags >= port_info.num_uafu_irqs) {
			OPAE_ERR("Invalid User Interrupt vector id");
			return FPGA_INVALID_PARAM;
		}

		if (uafu_operation == FPGA_IRQ_ASSIGN) {
			res = opae_port_set_user_irq(_handle->fddev, 0, flags, 1, &fd);
			_eh->flags = flags;
		} else {
			res = opae_port_set_user_irq(_handle->fddev, 0, _eh->flags, 1, &neg);
		}

		if (res) {
			OPAE_ERR("Could not set eventfd");
			res = FPGA_EXCEPTION;
		}
	} else {
		OPAE_ERR("UAFU interrupts not supported in hw");
		res = FPGA_NOT_SUPPORTED;
	}

	return res;
}

/*
 * Uses driver ioctls to determine whether the driver supports interrupts
 * on this platform. objtype is an output parameter.
 */
STATIC fpga_result check_interrupts_supported(fpga_handle handle,
					      fpga_objtype *objtype)
{
	fpga_result res = FPGA_OK;
	fpga_result destroy_res = FPGA_OK;
	fpga_properties prop = NULL;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	opae_fme_info fme_info = { 0 };
	opae_port_info port_info = { 0 };

	res = xfpga_fpgaGetPropertiesFromHandle(handle, &prop);
	if (res != FPGA_OK) {
		OPAE_MSG("Could not get FPGA properties from handle");
		return res;
	}

	res = fpgaPropertiesGetObjectType(prop, objtype);
	if (res != FPGA_OK) {
		OPAE_MSG("Could not determine FPGA object type");
		goto destroy_prop;
	}

	if (*objtype == FPGA_DEVICE) {
		res = opae_get_fme_info(_handle->fddev, &fme_info);
		if (res) {
			res = FPGA_EXCEPTION;
			goto destroy_prop;
		}

		if (fme_info.capability & FPGA_FME_CAP_ERR_IRQ) {
			res = FPGA_OK;
		} else {
			OPAE_MSG("Interrupts not supported in hw");
			res = FPGA_NOT_SUPPORTED;
		}
	} else if (*objtype == FPGA_ACCELERATOR) {
		res = opae_get_port_info(_handle->fddev, &port_info);
		if (res) {
			OPAE_ERR("Could not get PORT info: %s",
				 strerror(errno));
			goto destroy_prop;
		}

		if (port_info.capability & FPGA_PORT_CAP_ERR_IRQ) {
			res = FPGA_OK;
		} else {
			OPAE_MSG("Interrupts not supported in hw");
			res = FPGA_NOT_SUPPORTED;
		}
	}

destroy_prop:
	destroy_res = fpgaDestroyProperties(&prop);
	if (destroy_res != FPGA_OK) {
		OPAE_MSG("Could not destroy FPGA properties");
		return destroy_res;
	}

	return res;
}

STATIC fpga_result driver_register_event(fpga_handle handle,
					 fpga_event_type event_type,
					 fpga_event_handle event_handle,
					 uint32_t flags)
{
	fpga_objtype objtype;
	fpga_result res = FPGA_OK;

	res = check_interrupts_supported(handle, &objtype);
	if (res != FPGA_OK) {
		OPAE_MSG(
			"Could not determine whether interrupts are supported");
		return FPGA_NOT_SUPPORTED;
	}

	switch (event_type) {
	case FPGA_EVENT_ERROR:
		if (objtype == FPGA_DEVICE) {
			return send_fme_event_request(handle, event_handle,
						      FPGA_IRQ_ASSIGN);
		} else if (objtype == FPGA_ACCELERATOR) {
			return send_port_event_request(handle, event_handle,
						       FPGA_IRQ_ASSIGN);
		}
		OPAE_ERR("Invalid objtype: %d", objtype);
		return FPGA_EXCEPTION;
	case FPGA_EVENT_INTERRUPT:
		if (objtype != FPGA_ACCELERATOR) {
			OPAE_MSG("User events need an accelerator object");
			return FPGA_INVALID_PARAM;
		}

		return send_uafu_event_request(handle, event_handle, flags,
					       FPGA_IRQ_ASSIGN);
	case FPGA_EVENT_POWER_THERMAL:
		OPAE_MSG("Thermal interrupts not supported");
		return FPGA_NOT_SUPPORTED;
	default:
		OPAE_ERR("Invalid event type");
		return FPGA_EXCEPTION;
	}
}

STATIC fpga_result driver_unregister_event(fpga_handle handle,
					   fpga_event_type event_type,
					   fpga_event_handle event_handle)
{
	fpga_objtype objtype;
	fpga_result res = FPGA_OK;

	res = check_interrupts_supported(handle, &objtype);
	if (res != FPGA_OK) {
		OPAE_MSG(
			"Could not determine whether interrupts are supported");
		return FPGA_NOT_SUPPORTED;
	}

	switch (event_type) {
	case FPGA_EVENT_ERROR:
		if (objtype == FPGA_DEVICE) {
			return send_fme_event_request(handle, event_handle,
						      FPGA_IRQ_DEASSIGN);
		} else if (objtype == FPGA_ACCELERATOR) {
			return send_port_event_request(handle, event_handle,
						       FPGA_IRQ_DEASSIGN);
		}
		OPAE_ERR("Invalid objtype: %d", objtype);
		return FPGA_EXCEPTION;
	case FPGA_EVENT_INTERRUPT:
		if (objtype != FPGA_ACCELERATOR) {
			OPAE_MSG("User events need an Accelerator object");
			return FPGA_INVALID_PARAM;
		}

		return send_uafu_event_request(handle, event_handle, 0,
					       FPGA_IRQ_DEASSIGN);
	case FPGA_EVENT_POWER_THERMAL:
		OPAE_MSG("Thermal interrupts not supported");
		return FPGA_NOT_SUPPORTED;
	default:
		OPAE_ERR("Invalid event type");
		return FPGA_EXCEPTION;
	}
}

STATIC fpga_result daemon_register_event(fpga_handle handle,
					 fpga_event_type event_type,
					 fpga_event_handle event_handle,
					 uint32_t flags)
{
	int fd = FILE_DESCRIPTOR(event_handle);
	fpga_result result = FPGA_OK;
	struct sockaddr_un addr;
	struct event_request req;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	fpga_properties prop = NULL;
	uint64_t object_id = (uint64_t) -1;

	UNUSED_PARAM(flags);

	if (_handle->fdfpgad < 0) {
		errno_t e;

		/* connect to event socket */
		_handle->fdfpgad = socket(AF_UNIX, SOCK_STREAM, 0);
		if (_handle->fdfpgad < 0) {
			OPAE_ERR("socket: %s", strerror(errno));
			return FPGA_EXCEPTION;
		}

		addr.sun_family = AF_UNIX;
		e = strncpy_s(addr.sun_path, sizeof(addr.sun_path),
			      EVENT_SOCKET_NAME, EVENT_SOCKET_NAME_LEN);
		if (EOK != e) {
			OPAE_ERR("strncpy_s failed");
			return FPGA_EXCEPTION;
		}

		if (connect(_handle->fdfpgad, (struct sockaddr *)&addr,
			    sizeof(addr))
		    < 0) {
			FPGA_DBG("connect: %s", strerror(errno));
			result = FPGA_NO_DAEMON;
			goto out_close_conn;
		}
	}

	/* get the requestor's object ID */
	result = xfpga_fpgaGetPropertiesFromHandle(handle, &prop);
	if (result != FPGA_OK) {
		OPAE_ERR("failed to get props");
		goto out_close_conn;
	}

	result = fpgaPropertiesGetObjectID(prop, &object_id);
	if (result != FPGA_OK) {
		fpgaDestroyProperties(&prop);
		OPAE_ERR("failed to get object ID");
		goto out_close_conn;
	}

	result = fpgaDestroyProperties(&prop);
	if (result != FPGA_OK) {
		OPAE_ERR("failed to destroy props");
		goto out_close_conn;
	}

	/* create event registration request */
	req.type = REGISTER_EVENT;
	req.event = event_type;
	req.object_id = object_id;

	/* send event packet */
	result = send_event_request(_handle->fdfpgad, fd, &req);
	if (result != FPGA_OK) {
		OPAE_ERR("send_event_request failed");
		goto out_close_conn;
	}

	return result;

out_close_conn:
	close(_handle->fdfpgad);
	_handle->fdfpgad = -1;
	return result;
}

STATIC fpga_result daemon_unregister_event(fpga_handle handle,
					   fpga_event_type event_type)
{
	fpga_result result = FPGA_OK;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct event_request req;
	ssize_t n;
	fpga_properties prop = NULL;
	uint64_t object_id = (uint64_t) -1;

	if (_handle->fdfpgad < 0) {
		OPAE_MSG("No fpgad connection");
		return FPGA_INVALID_PARAM;
	}

	/* get the requestor's object ID */
	result = xfpga_fpgaGetPropertiesFromHandle(handle, &prop);
	if (result != FPGA_OK) {
		OPAE_ERR("failed to get properties");
		goto out_close_conn;
	}

	result = fpgaPropertiesGetObjectID(prop, &object_id);
	if (result != FPGA_OK) {
		fpgaDestroyProperties(&prop);
		OPAE_ERR("failed to get object ID");
		goto out_close_conn;
	}

	result = fpgaDestroyProperties(&prop);
	if (result != FPGA_OK) {
		OPAE_ERR("failed to destroy properties");
		goto out_close_conn;
	}

	req.type = UNREGISTER_EVENT;
	req.event = event_type;
	req.object_id = object_id;

	n = send(_handle->fdfpgad, &req, sizeof(req), 0);
	if (n < 0) {
		OPAE_ERR("send : %s", strerror(errno));
		result = FPGA_EXCEPTION;
		goto out_close_conn;
	}

	return result;

out_close_conn:
	close(_handle->fdfpgad);
	_handle->fdfpgad = -1;
	return result;
}

fpga_result __XFPGA_API__
xfpga_fpgaCreateEventHandle(fpga_event_handle *event_handle)
{
	struct _fpga_event_handle *_eh;
	fpga_result result = FPGA_OK;
	pthread_mutexattr_t mattr;
	int err = 0;

	ASSERT_NOT_NULL(event_handle);

	_eh = malloc(sizeof(struct _fpga_event_handle));
	if (NULL == _eh) {
		OPAE_ERR("Could not allocate memory for event handle");
		return FPGA_NO_MEMORY;
	}

	_eh->magic = FPGA_EVENT_HANDLE_MAGIC;

	/* create eventfd */
	_eh->fd = eventfd(0, 0);
	if (_eh->fd < 0) {
		OPAE_ERR("eventfd : %s", strerror(errno));
		result = FPGA_EXCEPTION;
		goto out_free;
	}

	if (pthread_mutexattr_init(&mattr)) {
		OPAE_MSG("Failed to initialized event handle mutex attributes");
		result = FPGA_EXCEPTION;
		goto out_free;
	}

	if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE)) {
		OPAE_MSG("Failed to initialize event handle mutex attributes");
		result = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}

	if (pthread_mutex_init(&_eh->lock, &mattr)) {
		OPAE_MSG("Failed to initialize event handle mutex");
		result = FPGA_EXCEPTION;
		goto out_attr_destroy;
	}

	pthread_mutexattr_destroy(&mattr);

	*event_handle = (fpga_event_handle)_eh;
	return FPGA_OK;

out_attr_destroy:
	err = pthread_mutexattr_destroy(&mattr);
	if (err)
		OPAE_ERR("pthread_mutexatr_destroy() failed: %s",
			 strerror(err));

out_free:
	free(_eh);
	return result;
}

fpga_result __XFPGA_API__
xfpga_fpgaDestroyEventHandle(fpga_event_handle *event_handle)
{
	struct _fpga_event_handle *_eh;
	fpga_result result = FPGA_OK;
	int err = 0;

	// sanity check
	if (!event_handle) {
		return FPGA_INVALID_PARAM;
	}

	_eh = (struct _fpga_event_handle *)*event_handle;

	result = event_handle_check_and_lock(_eh);
	if (result)
		return result;

	if (close(_eh->fd) < 0) {
		OPAE_ERR("eventfd : %s", strerror(errno));
		err = pthread_mutex_unlock(&_eh->lock);
		if (err)
			OPAE_ERR("pthread_mutex_unlock() failed: %S",
				 strerror(err));

		if (errno == EBADF)
			return FPGA_INVALID_PARAM;
		else
			return FPGA_EXCEPTION;
	}

	_eh->magic = FPGA_INVALID_MAGIC;

	err = pthread_mutex_unlock(&_eh->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %S", strerror(err));

	err = pthread_mutex_destroy(&_eh->lock);
	if (err)
		OPAE_ERR("pthread_mutex_destroy() failed: %S", strerror(err));

	free(*event_handle);
	*event_handle = NULL;
	return FPGA_OK;
}

fpga_result __XFPGA_API__
xfpga_fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh, int *fd)
{
	struct _fpga_event_handle *_eh = (struct _fpga_event_handle *)eh;
	fpga_result result = FPGA_OK;
	int err = 0;

	result = event_handle_check_and_lock(_eh);
	if (result)
		return result;

	*fd = _eh->fd;

	err = pthread_mutex_unlock(&_eh->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));

	return FPGA_OK;
}

fpga_result __XFPGA_API__ xfpga_fpgaRegisterEvent(fpga_handle handle,
						 fpga_event_type event_type,
						 fpga_event_handle event_handle,
						 uint32_t flags)
{
	fpga_result result = FPGA_OK;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct _fpga_event_handle *_eh =
		(struct _fpga_event_handle *)event_handle;
	struct _fpga_token *_token;
	int err;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	result = event_handle_check_and_lock(_eh);
	if (result)
		goto out_unlock_handle;

	_token = (struct _fpga_token *)_handle->token;

	if (_token->magic != FPGA_TOKEN_MAGIC) {
		OPAE_MSG("Invalid token found in handle");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	switch (event_type) {
	case FPGA_EVENT_INTERRUPT:
		if (!strstr(_token->devpath, "port")) {
			OPAE_MSG("Handle does not refer to accelerator object");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}
		break;
	case FPGA_EVENT_ERROR: /* fall through */
	case FPGA_EVENT_POWER_THERMAL:
		break;
	}

	/* TODO: reject unknown flags */

	/* try driver first */
	result = driver_register_event(handle, event_type, event_handle, flags);
	if (result == FPGA_NOT_SUPPORTED) {
		result = daemon_register_event(handle, event_type, event_handle,
					       flags);
	}

out_unlock:
	err = pthread_mutex_unlock(&_eh->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));

out_unlock_handle:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));

	return result;
}

fpga_result __XFPGA_API__
xfpga_fpgaUnregisterEvent(fpga_handle handle, fpga_event_type event_type,
			  fpga_event_handle event_handle)
{
	fpga_result result = FPGA_OK;
	int err;

	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct _fpga_event_handle *_eh =
		(struct _fpga_event_handle *)event_handle;
	struct _fpga_token *_token;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	result = event_handle_check_and_lock(_eh);
	if (result)
		goto out_unlock_handle;

	_token = (struct _fpga_token *)_handle->token;

	if (_token->magic != FPGA_TOKEN_MAGIC) {
		OPAE_MSG("Invalid token found in handle");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	switch (event_type) {
	case FPGA_EVENT_INTERRUPT:
		if (!strstr(_token->devpath, "port")) {
			OPAE_MSG("Handle does not refer to accelerator object");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}
		break;
	case FPGA_EVENT_ERROR: /* fall through */
	case FPGA_EVENT_POWER_THERMAL:
		break;
	}

	/* try driver first */
	result = driver_unregister_event(handle, event_type, event_handle);
	if (result == FPGA_NOT_SUPPORTED) {
		result = daemon_unregister_event(handle, event_type);
	}

out_unlock:
	err = pthread_mutex_unlock(&_eh->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));

out_unlock_handle:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));

	return result;
}
