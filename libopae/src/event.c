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

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/eventfd.h>
#include <errno.h>

#include "safe_string/safe_string.h"

#include "opae/access.h"
#include "types_int.h"
#include "common_int.h"
#include "intel-fpga.h"

#define EVENT_SOCKET_NAME     "/tmp/fpga_event_socket"
#define EVENT_SOCKET_NAME_LEN 23
#define MAX_PATH_LEN 256

enum request_type {
	REGISTER_EVENT = 0,
	UNREGISTER_EVENT = 1
};

struct event_request {
	enum request_type type;
	fpga_event_type event;
	char device[MAX_PATH_LEN];
};

fpga_result send_event_request(int conn_socket, int fd, struct event_request *req)
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
	memset(buf, 0x0, sizeof(buf));
	cmh = (struct cmsghdr *)buf;
	cmh->cmsg_len = CMSG_LEN(sizeof(int));
	cmh->cmsg_level = SOL_SOCKET;
	cmh->cmsg_type = SCM_RIGHTS;
	mh.msg_name = NULL;
	mh.msg_namelen = 0;
	mh.msg_iov = iov;
	mh.msg_iovlen = sizeof(iov) / sizeof(iov[0]);
	mh.msg_control = cmh;
	mh.msg_controllen = CMSG_LEN(sizeof(int));
	mh.msg_flags = 0;
	fd_ptr = (int *)CMSG_DATA((struct cmsghdr *)buf);
	*fd_ptr = fd;

	/* send ancillary data */
	n = sendmsg(conn_socket, &mh, 0);
	if (n < 0) {
		FPGA_ERR("sendmsg failed: %s", strerror(errno));
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;

}

static fpga_result send_fme_event_request(fpga_handle handle,
	fpga_event_handle event_handle, int fme_operation)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct fpga_fme_info fme_info  = {.argsz = sizeof(fme_info),
						.flags = 0 };
	struct fpga_fme_err_irq_set fme_irq = {.argsz = sizeof(fme_irq),
						.flags = 0};

	if (fme_operation != FPGA_IRQ_ASSIGN && fme_operation != FPGA_IRQ_DEASSIGN) {
		FPGA_ERR("Invalid FME operation requested");
		return FPGA_INVALID_PARAM;
	}

	if (ioctl(_handle->fddev, FPGA_FME_GET_INFO, &fme_info) != 0) {
		FPGA_ERR("Could not get FME info: %s", strerror(errno));
		return FPGA_EXCEPTION;
	}

	/*irq_capability field is set to 1 if the platform supports interrupts*/
	if (fme_info.irq_capability & FPGA_FME_CAP_ERR_IRQ) {
		fme_irq.evtfd = event_handle;
		fme_irq.flags = fme_operation;

		if (ioctl(_handle->fddev, FPGA_FME_ERR_SET_IRQ, &fme_irq) != 0) {
			FPGA_ERR("Could not set eventfd %s", strerror(errno));
			return FPGA_EXCEPTION;
		}
	} else {
		FPGA_ERR("FME interrupts not supported in hw");
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

static fpga_result get_handle_objtype(fpga_handle handle, fpga_objtype *objtype)
{
	fpga_result res = FPGA_OK;
	fpga_result destroy_res = FPGA_OK;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct _fpga_token *_token;
	fpga_properties prop = NULL;

	/*_handle->lock mutex is not locked since it will be locked
	  by the calling functions*/
	_token = (struct _fpga_token *)_handle->token;

	res = fpgaGetProperties(_token, &prop);
	if (res != FPGA_OK) {
		FPGA_MSG("Could not get FPGA properties");
		return res;
	}

	res = fpgaPropertiesGetObjectType(prop, &objtype);
	if (res != FPGA_OK)
		FPGA_MSG("Could not determine FPGA object type");

	destroy_res = fpgaDestroyProperties(&prop);
	if (destroy_res != FPGA_OK)
		FPGA_MSG("Could not destroy FPGA properties");

	return res;
}

static fpga_result check_interrupts_supported(fpga_handle handle)
{
	fpga_result res = FPGA_OK;
	fpga_result destroy_res = FPGA_OK;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct _fpga_token *_token;
	fpga_properties prop = NULL;
	fpga_objtype objtype;
	struct fpga_fme_info fme_info  = {.argsz = sizeof(fme_info),
					.flags = 0 };
	struct fpga_port_info port_info  = {.argsz = sizeof(port_info),
					.flags = 0 };

	/*_handle->lock mutex is not locked since it will be locked
	  by the calling functions*/
	_token = (struct _fpga_token *)_handle->token;

	res = fpgaGetProperties(_token, &prop);
	if (res != FPGA_OK) {
		FPGA_MSG("Could not get FPGA properties");
		return res;
	}

	res = fpgaPropertiesGetObjectType(prop, &objtype);
	if (res != FPGA_OK) {
		FPGA_MSG("Could not determine FPGA object type");
		goto destroy_prop;
	}

	if (objtype == FPGA_DEVICE) {
		if (ioctl(_handle->fddev, FPGA_FME_GET_INFO, &fme_info) != 0) {
			FPGA_ERR("Could not get FME info: %s", strerror(errno));
			res = FPGA_EXCEPTION;
			goto destroy_prop;
		}

		if (fme_info.irq_capability & FPGA_FME_CAP_ERR_IRQ) {
			res = FPGA_OK;
		} else {
			FPGA_ERR("Interrupts not supported in hw");
			res = FPGA_NOT_SUPPORTED;
		}
	} else if (objtype == FPGA_ACCELERATOR) {
		if (ioctl(_handle->fddev, FPGA_PORT_GET_INFO, &port_info) != 0) {
			FPGA_ERR("Could not get PORT info: %s", strerror(errno));
			res = FPGA_EXCEPTION;
			goto destroy_prop;
		}

		if (port_info.irq_capability & FPGA_PORT_CAP_ERR_IRQ) {
			res = FPGA_OK;
		} else {
			FPGA_ERR("Interrupts not supported in hw");
			res = FPGA_NOT_SUPPORTED;
		}
	}

destroy_prop:
	destroy_res = fpgaDestroyProperties(&prop);
	if (destroy_res != FPGA_OK) {
		FPGA_MSG("Could not destroy FPGA properties");
		return destroy_res;
	}

	return res;
}

static fpga_result driver_register_event(fpga_handle handle,
	fpga_event_type event_type,
	fpga_event_handle event_handle)
{
	fpga_objtype objtype;
	fpga_result res = FPGA_OK;

	res = check_interrupts_supported(handle);
	if (res != FPGA_OK) {
		FPGA_ERR("Could not determine whether interrupts are supported");
		return FPGA_NOT_SUPPORTED;
	}

	switch (event_type) {
	case FPGA_EVENT_ERROR:
		res = get_handle_objtype(handle, &objtype);
		if (res != FPGA_OK) {
			FPGA_MSG("Could not determine FPGA object type");
			return res;
		}

		if (objtype == FPGA_DEVICE) {
			return send_fme_event_request(handle, event_handle, FPGA_IRQ_ASSIGN);
		} else if (objtype == FPGA_ACCELERATOR) {
			FPGA_MSG("Port event type not supported");
			return FPGA_NOT_SUPPORTED;
		}
	case FPGA_EVENT_INTERRUPT:
		FPGA_MSG("User AFU event type not supported");
		return FPGA_NOT_SUPPORTED;
	case FPGA_EVENT_POWER_THERMAL:
		FPGA_MSG("Thermal interrupts not supported");
		return FPGA_NOT_SUPPORTED;
	default:
		FPGA_ERR("Invalid event type");
		return FPGA_EXCEPTION;
	}
}

static fpga_result driver_unregister_event(fpga_handle handle,
	fpga_event_type event_type, fpga_event_handle event_handle)
{
	fpga_objtype objtype;
	fpga_result res = FPGA_OK;

	res = check_interrupts_supported(handle);
	if (res != FPGA_OK) {
		FPGA_ERR("Could not determine whether interrupts are supported");
		return FPGA_NOT_SUPPORTED;
	}

	switch (event_type) {
	case FPGA_EVENT_ERROR:
		res = get_handle_objtype(handle, &objtype);
		if (res != FPGA_OK) {
			FPGA_ERR("Could not determine FPGA object type");
			return res;
		}

		if (objtype == FPGA_DEVICE) {
			return send_fme_event_request(handle, event_handle, FPGA_IRQ_DEASSIGN);
		} else if (objtype == FPGA_ACCELERATOR) {
			FPGA_MSG("Port event type not supported");
			return FPGA_NOT_SUPPORTED;
		}
	case FPGA_EVENT_INTERRUPT:
		FPGA_MSG("User AFU event type not supported");
		return FPGA_NOT_SUPPORTED;
	case FPGA_EVENT_POWER_THERMAL:
		FPGA_MSG("Thermal interrupts not supported");
		return FPGA_NOT_SUPPORTED;
	default:
		FPGA_ERR("Invalid event type");
		return FPGA_EXCEPTION;
	}
}

static fpga_result daemon_register_event(fpga_handle handle,
					 fpga_event_type event_type,
					 fpga_event_handle event_handle,
					 uint32_t flags)
{
	int fd = event_handle;
	fpga_result result = FPGA_OK;
	struct sockaddr_un addr;
	struct event_request req;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct _fpga_token *_token = (struct _fpga_token *)_handle->token;
	errno_t e;

	if (_handle->fdfpgad < 0) {

		/* connect to event socket */
		_handle->fdfpgad = socket(AF_UNIX, SOCK_STREAM, 0);
		if (_handle->fdfpgad < 0) {
			FPGA_ERR("socket: %s", strerror(errno));
			return FPGA_EXCEPTION;
		}

		addr.sun_family = AF_UNIX;
		e = strncpy_s(addr.sun_path, sizeof(addr.sun_path),
				EVENT_SOCKET_NAME, EVENT_SOCKET_NAME_LEN);
		if (EOK != e) {
			FPGA_ERR("strncpy_s failed");
			return FPGA_EXCEPTION;
		}

		if (connect(_handle->fdfpgad, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			FPGA_DBG("connect: %s", strerror(errno));
			result = FPGA_NO_DAEMON;
			goto out_close_conn;
		}

	}

	/* create event registration request */
	req.type = REGISTER_EVENT;
	req.event = event_type;

	e = strncpy_s(req.device, sizeof(req.device),
			_token->sysfspath, sizeof(_token->sysfspath));
	if (EOK != e) {
		FPGA_ERR("strncpy_s failed");
		result = FPGA_EXCEPTION;
		goto out_close_conn;
	}

	req.device[sizeof(req.device)-1] = '\0';

	/* send event packet */
	result = send_event_request(_handle->fdfpgad, fd, &req);
	if (result != FPGA_OK) {
		FPGA_ERR("send_event_request failed");
		goto out_close_conn;
	}

	return result;

out_close_conn:
	close(_handle->fdfpgad);
	_handle->fdfpgad = -1;
	return result;
}

static fpga_result daemon_unregister_event(fpga_handle handle,
					   fpga_event_type event_type)
{
	fpga_result result = FPGA_OK;

	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct _fpga_token *_token = (struct _fpga_token *)_handle->token;

	struct event_request req;
	ssize_t n;
	errno_t e;

	if (_handle->fdfpgad < 0) {
		FPGA_MSG("No fpgad connection");
		return FPGA_INVALID_PARAM;
	}

	req.type = UNREGISTER_EVENT;
	req.event = event_type;

	e = strncpy_s(req.device, sizeof(req.device),
			_token->sysfspath, sizeof(_token->sysfspath));
	if (EOK != e) {
		FPGA_ERR("strncpy_s failed");
		result = FPGA_EXCEPTION;
		goto out_close_conn;
	}

	req.device[sizeof(req.device)-1] = '\0';

	n = send(_handle->fdfpgad, &req, sizeof(req), 0);
	if (n < 0) {
		FPGA_ERR("send : %s", strerror(errno));
		result = FPGA_EXCEPTION;
		goto out_close_conn;
	}

	return result;

out_close_conn:
	close(_handle->fdfpgad);
	_handle->fdfpgad = -1;
	return result;
}

fpga_result __FPGA_API__ fpgaCreateEventHandle(fpga_event_handle *event_handle)
{
	int fd;

	ASSERT_NOT_NULL(event_handle);

	/* create eventfd */
	fd = eventfd(0, 0);
	if (fd < 0) {
		FPGA_ERR("eventfd : %s", strerror(errno));
		return FPGA_NOT_SUPPORTED;
	}

	*event_handle = fd;
	return FPGA_OK;
}

fpga_result __FPGA_API__ fpgaDestroyEventHandle(fpga_event_handle *event_handle)
{
	ASSERT_NOT_NULL(event_handle);

	if (close(*event_handle) < 0) {
		FPGA_ERR("eventfd : %s", strerror(errno));
		if (errno == EBADF)
			return FPGA_INVALID_PARAM;
		else
			return FPGA_EXCEPTION;
	}
	return FPGA_OK;
}

fpga_result __FPGA_API__ fpgaRegisterEvent(fpga_handle handle,
					   fpga_event_type event_type,
					   fpga_event_handle event_handle,
					   uint32_t flags)
{
	fpga_result result = FPGA_OK;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct _fpga_token *_token;
	int err;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	_token = (struct _fpga_token *)_handle->token;

	if (_token->magic != FPGA_TOKEN_MAGIC) {
		FPGA_MSG("Invalid token found in handle");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	switch (event_type) {
	case FPGA_EVENT_INTERRUPT:
		if (!strstr(_token->devpath, "port")) {
			FPGA_MSG("Handle does not refer to accelerator object");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}
		break;
	}

	/* TODO: reject unknown flags */

	/* try driver first */
	result = driver_register_event(handle, event_type, event_handle);
	if (result == FPGA_NOT_SUPPORTED) {
		result = daemon_register_event(handle, event_type,
					       event_handle, flags);
	}

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __FPGA_API__ fpgaUnregisterEvent(fpga_handle handle,
					     fpga_event_type event_type,
					     fpga_event_handle event_handle)
{
	fpga_result result = FPGA_OK;
	int err;

	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	struct _fpga_token *_token;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	_token = (struct _fpga_token *)_handle->token;

	if (_token->magic != FPGA_TOKEN_MAGIC) {
		FPGA_MSG("Invalid token found in handle");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	switch (event_type) {
	case FPGA_EVENT_INTERRUPT:
		if (!strstr(_token->devpath, "port")) {
			FPGA_MSG("Handle does not refer to accelerator object");
			result = FPGA_INVALID_PARAM;
			goto out_unlock;
		}
		break;
	}

	/* try driver first */
	result = driver_unregister_event(handle, event_type, event_handle);
	if (result == FPGA_NOT_SUPPORTED) {
		result = daemon_unregister_event(handle, event_type);
	}

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));

	return result;
}

