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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>

#include "safe_string/safe_string.h"

#include "opae/access.h"
#include "opae/utils.h"
#include "opae/manage.h"
#include "opae/manage.h"
#include "bitstream_int.h"
#include "common_int.h"
#include "intel-fpga.h"
#include "usrclk/user_clk_pgm_uclock.h"

// sysfs attributes
#define PORT_SYSFS_ERRORS     "errors/errors"
#define PORT_SYSFS_ERR_CLEAR  "errors/clear"
#define PWRMGMT_THRESHOLD1    "power_mgmt/threshold1"
#define PWRMGMT_THRESHOLD2    "power_mgmt/threshold2"

// Max power values
#define FPGA_BBS_IDLE_POWER   30            // watts
#define FPGA_MAX_POWER        90            // watts
#define FPGA_GBS_MAX_POWER    60            // watts
#define FPGA_THRESHOLD2(x)    ((x*10)/100)  // threshold1 + 10%

#pragma pack(push, 1)
// GBS Header
struct bitstream_header {
	uint32_t magic;
	uint64_t ifid_l;
	uint64_t ifid_h;
};
#pragma pack(pop)

// Reconnfigure Error CSR
struct reconf_error {
	union {
		uint64_t csr;
		struct {
			uint64_t  reconf_operation_error:1;                   /* PR operation error detected */
			uint64_t  reconf_CRC_error:1;                         /* PR CRC error detected*/
			uint64_t  reconf_incompatible_bitstream_error:1;      /* PR incompatible bitstream error detected */
			uint64_t  reconf_IP_protocol_error:1;                 /* PR IP protocol error detected */
			uint64_t  reconf_FIFO_overflow_error:1;               /* PR FIFO overflow error detected */
			uint64_t  reconf_timeout_error:1;                     /* PR timeout error detected */
			uint64_t  reconf_secure_load_error:1;                 /* PR secure load error detected */
			uint64_t  rsvd:57;                                    /* Reserved */
		};
	};
};


static fpga_result validate_bitstream(fpga_handle handle,
			const uint8_t *bitstream, size_t bitstream_len,
			int *header_len)
{
	struct bitstream_header bts_hdr = {0};

	if (bitstream == NULL) {
		FPGA_MSG("Bitstream is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (bitstream_len <= 0 ||
		bitstream_len <= sizeof(struct bitstream_header)) {
		FPGA_MSG("Invalid bitstream size");
		return FPGA_INVALID_PARAM;
	}

	if (check_bitstream_guid(bitstream) == FPGA_OK) {
		*header_len = get_bitstream_header_len(bitstream);

		if (*header_len < 0) {
			FPGA_MSG("Invalid bitstream header length");
			return FPGA_EXCEPTION;
		}

		if (validate_bitstream_metadata(handle, bitstream) != FPGA_OK) {
			FPGA_MSG("Invalid JSON data");
			return FPGA_EXCEPTION;
		}

		return FPGA_OK;
	} else {
		errno_t e;
		// TODO: This is needed for legacy bitstreams since they
		// do not have new metadata with GUID. Remove once
		// all bitstreams conform to new metadata format.
		*header_len = sizeof(struct bitstream_header);
		e = memcpy_s(&bts_hdr, sizeof(struct bitstream_header),
				bitstream, sizeof(struct bitstream_header));
		if (EOK != e) {
			FPGA_ERR("memcpy_s failed");
			return FPGA_EXCEPTION;
		}

		return check_interface_id(handle, bts_hdr.magic, bts_hdr.ifid_l,
						bts_hdr.ifid_h);
	}
}


// clears port errors
static fpga_result clear_port_errors(fpga_handle handle)
{
	char syfs_path[SYSFS_PATH_MAX]    = {0};
	char syfs_errpath[SYSFS_PATH_MAX] = {0};
	fpga_result result                = FPGA_OK;
	uint64_t error                    = 0 ;

	result = get_port_sysfs(handle, syfs_path);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get port syfs path");
		return result;
	}

	snprintf(syfs_errpath, sizeof(syfs_errpath), "%s/%s", syfs_path, PORT_SYSFS_ERRORS);
	// Read port error.
	result = sysfs_read_u64(syfs_errpath, &error);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get port errors");
		return result;
	}

	snprintf(syfs_errpath, sizeof(syfs_errpath), "%s/%s", syfs_path, PORT_SYSFS_ERR_CLEAR);
	// Clear port error.
	result = sysfs_write_u64(syfs_errpath, error);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to clear port errors");
		return result;
	}

	return result;
}

// set afu user clock
fpga_result set_afu_userclock(fpga_handle handle,
				uint64_t usrlclock_high,
				uint64_t usrlclock_low)
{
	char syfs_path[SYSFS_PATH_MAX]    = {0};
	fpga_result result                = FPGA_OK;
	uint64_t userclk_high             = 0;
	uint64_t userclk_low              = 0;

	// Read port sysfs path
	result = get_port_sysfs(handle, syfs_path);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get port syfs path");
		return result;
	}

	// set user clock
	result = set_userclock(syfs_path, usrlclock_high, usrlclock_low);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to set user clock");
		return result;
	}

	// read user clock
	result = get_userclock(syfs_path, &userclk_high, &userclk_low);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get user clock");
		return result;
	}

	return result;
}

// Sets FPGA threshold power values
fpga_result set_fpga_pwr_threshold(fpga_handle handle,
					uint64_t gbs_power)
{
	char sysfs_path[SYSFS_PATH_MAX]   = {0};
	fpga_result result                = FPGA_OK;
	uint64_t fpga_power               = 0;
	struct _fpga_token  *_token       = NULL;
	struct _fpga_handle *_handle      = (struct _fpga_handle *)handle;

	if (_handle == NULL) {
		FPGA_ERR("Invalid handle");
		return FPGA_INVALID_PARAM;
	}

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		FPGA_ERR("Invalid token within handle");
		return FPGA_INVALID_PARAM;
	}

	if (gbs_power == 0) {
		FPGA_ERR("GBS power value is 0");
		return FPGA_INVALID_PARAM;
	}

	// verify gbs power limits
	if (gbs_power > FPGA_GBS_MAX_POWER) {
		FPGA_ERR("Invalid GBS power value");
		result = FPGA_NOT_SUPPORTED;
		return result;
	}

	// FPGA threshold1 = BBS Idle power + GBS power
	fpga_power = gbs_power + FPGA_BBS_IDLE_POWER;
	if (fpga_power > FPGA_MAX_POWER) {
		FPGA_ERR("Total power requirements exceed FPGA maximum");
		result = FPGA_NOT_SUPPORTED;
		return result;
	}

	// set fpga threshold 1
	snprintf(sysfs_path, sizeof(sysfs_path), "%s/%s",  _token->sysfspath, PWRMGMT_THRESHOLD1);
	FPGA_DBG(" FPGA Threshold1             :%ld watts\n", fpga_power);

	result = sysfs_write_u64(sysfs_path, fpga_power);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to write power threshold 1");
		return result;
	}

	// FIXME Fix threshold2 calculation.
	// FPGA threshold2 =  110% (FPGA threshold1)
	fpga_power = fpga_power + FPGA_THRESHOLD2(fpga_power);
	if (fpga_power > FPGA_MAX_POWER) {
		FPGA_ERR("Invalid power threshold 2");
		result = FPGA_NOT_SUPPORTED;
		return result;
	}

	// set fpga threshold 2
	snprintf(sysfs_path, sizeof(sysfs_path), "%s/%s",  _token->sysfspath, PWRMGMT_THRESHOLD2);
	FPGA_DBG(" FPGA Threshold2             :%ld watts\n", fpga_power);

	result = sysfs_write_u64(sysfs_path, fpga_power);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to write power threshold 2");
		return result;
	}

	return result;
}

fpga_result __FPGA_API__ fpgaReconfigureSlot(fpga_handle fpga,
						uint32_t slot,
						const uint8_t *bitstream,
						size_t bitstream_len,
						int flags)
{
	struct _fpga_handle *_handle    = (struct _fpga_handle *)fpga;
	fpga_result result              = FPGA_OK;
	struct fpga_fme_port_pr port_pr = {0};
	struct reconf_error  error      = {0};
	struct gbs_metadata  metadata   = {0};
	int bitstream_header_len        = 0;
	uint64_t deviceid               = 0;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (validate_bitstream(fpga, bitstream, bitstream_len,
				&bitstream_header_len) != FPGA_OK) {
		FPGA_MSG("Invalid bitstream");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	// Clear port errors
	result = clear_port_errors(fpga);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to clear port errors.");
	}

	if (get_bitstream_json_len(bitstream) > 0) {

		// Read GBS json metadata
		result = read_gbs_metadata(bitstream, &metadata);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed to read metadata");
			goto out_unlock;
		}

		FPGA_DBG(" Version                  :%f\n", metadata.version);
		FPGA_DBG(" Magic Num                :%ld\n",
			 metadata.afu_image.magic_num);
		FPGA_DBG(" Interface Id             :%s\n",
			 metadata.afu_image.interface_uuid);
		FPGA_DBG(" Clock_frequency_high     :%d\n",
			 metadata.afu_image.clock_frequency_high);
		FPGA_DBG(" Clock_frequency_low      :%d\n",
			 metadata.afu_image.clock_frequency_low);
		FPGA_DBG(" Power                    :%d\n",
			 metadata.afu_image.power);
		FPGA_DBG(" Name                     :%s\n",
			 metadata.afu_image.afu_clusters.name);
		FPGA_DBG(" Total_contexts           :%d\n",
			 metadata.afu_image.afu_clusters.total_contexts);
		FPGA_DBG(" AFU_uuid                 :%s\n",
			 metadata.afu_image.afu_clusters.afu_uuid);

		// Set AFU user clock
		if (metadata.afu_image.clock_frequency_high > 0 && metadata.afu_image.clock_frequency_low > 0) {
			result = set_afu_userclock(fpga, metadata.afu_image.clock_frequency_high, metadata.afu_image.clock_frequency_low);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to set user clock");
				goto out_unlock;
			}
		}

		// get fpga device id.
		result = get_fpga_deviceid(fpga, &deviceid);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed to read device id.");
			goto out_unlock;
		}

		// Set power threshold for integrated fpga.
		if (deviceid == FPGA_INTEGRATED_DEVICEID) {

			result = set_fpga_pwr_threshold(fpga, metadata.afu_image.power);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to set threshold.");
				goto out_unlock;
			}

		} // device id

	}

	port_pr.flags                 = 0;
	port_pr.argsz                 = sizeof(struct fpga_fme_port_pr);
	port_pr.buffer_address        = (__u64)bitstream + bitstream_header_len;
	port_pr.buffer_size           = (__u32) bitstream_len - bitstream_header_len;
	port_pr.port_id               = slot;

	result = ioctl(_handle->fddev, FPGA_FME_PORT_PR, &port_pr);
	if (result != 0) {
		FPGA_MSG("Failed to reconfigure bitstream");

		if ((errno == EINVAL) ||
		(errno == EFAULT)) {
			result = FPGA_INVALID_PARAM;
		} else {
			result = FPGA_EXCEPTION;
		}
		goto out_unlock;
	}

	// PR error
	error.csr = port_pr.status;

	if (error.reconf_operation_error == 0x1) {
		FPGA_MSG("PR operation error detected");
		result = FPGA_RECONF_ERROR;
	}

	if (error.reconf_CRC_error == 0x1) {
		FPGA_MSG("PR CRC error detected");
		result = FPGA_RECONF_ERROR;
	}

	if (error.reconf_incompatible_bitstream_error == 0x1) {
		FPGA_MSG("PR incompatible bitstream error detected");
		result = FPGA_RECONF_ERROR;
	}

	if (error.reconf_IP_protocol_error == 0x1) {
		FPGA_MSG("PR IP protocol error detected");
		result = FPGA_RECONF_ERROR;
	}

	if (error.reconf_FIFO_overflow_error == 0x1) {
		FPGA_MSG("PR FIFO overflow error detected");
		result = FPGA_RECONF_ERROR;
	}

	if (error.reconf_timeout_error == 0x1) {
		FPGA_MSG("PR timeout error detected");
		result = FPGA_RECONF_ERROR;
	}

	if (error.reconf_secure_load_error == 0x1) {
		FPGA_MSG("PR secure load error detected");
		result = FPGA_RECONF_ERROR;
	}

out_unlock:
	pthread_mutex_unlock(&_handle->lock);
	return result;
}
