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

#include "opae/access.h"
#include "opae/utils.h"
#include "opae/umsg.h"
#include "common_int.h"
#include "intel-fpga.h"

#include "usrclk/user_clk_pgm_uclock.h"

fpga_result __XFPGA_API__ xfpga_fpgaSetUserClock(fpga_handle handle,
						uint64_t high_clk,
						uint64_t low_clk,
						int flags)
{
	struct _fpga_handle  *_handle = (struct _fpga_handle *)handle;
	fpga_result result            = FPGA_OK;
	int err                       = 0;
	struct _fpga_token  *_token;
	char *p                       = 0;

	UNUSED_PARAM(flags);

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		OPAE_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		OPAE_ERR("Token not found");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	p = strstr(_token->sysfspath, FPGA_SYSFS_AFU);
	if (NULL == p) {
		OPAE_ERR("Invalid sysfspath in token");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}
	p = strrchr(_token->sysfspath, '.');
	if (NULL == p) {
		OPAE_ERR("Invalid sysfspath in token");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	result = set_userclock(_token->sysfspath, high_clk, low_clk);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to set user clock");
	}

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}

fpga_result __XFPGA_API__ xfpga_fpgaGetUserClock(fpga_handle handle,
						uint64_t *high_clk,
						uint64_t *low_clk,
						int flags)
{
	struct _fpga_handle  *_handle = (struct _fpga_handle *)handle;
	fpga_result result            = FPGA_OK;
	int err                       = 0;
	struct _fpga_token  *_token;
	char *p                       = 0;

	UNUSED_PARAM(flags);

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		OPAE_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		OPAE_ERR("Token not found");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	p = strstr(_token->sysfspath, FPGA_SYSFS_AFU);
	if (NULL == p) {
		OPAE_ERR("Invalid sysfspath in token");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	p = strrchr(_token->sysfspath, '.');
	if (NULL == p) {
		OPAE_ERR("Invalid sysfspath in token");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	result = get_userclock(_token->sysfspath, high_clk, low_clk);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get user clock");
	}

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		OPAE_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}
