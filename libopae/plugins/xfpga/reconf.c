// Copyright(c) 2017-2018, Intel Corporation
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

#include "xfpga.h"
#include "bitstream_int.h"
#include "common_int.h"
#include "opae_drv.h"
#include "usrclk/user_clk_pgm_uclock.h"

#include "reconf_int.h"
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


STATIC fpga_result validate_bitstream(fpga_handle handle,
			const uint8_t *bitstream, size_t bitstream_len,
			int *header_len)
{
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
		return FPGA_INVALID_PARAM;
	}
}


// open child accelerator exclusively - it not, it's busy!
STATIC fpga_result open_accel(fpga_handle handle, fpga_handle *accel)
{
	fpga_result result                = FPGA_OK;
	fpga_result destroy_result        = FPGA_OK;
	struct _fpga_handle *_handle      = (struct _fpga_handle *)handle;
	fpga_token token = NULL;
	fpga_properties props;
	uint32_t matches = 0;

	if (_handle == NULL) {
		FPGA_ERR("Invalid handle");
		return FPGA_INVALID_PARAM;
	}

	if (_handle->token == NULL) {
		FPGA_ERR("Invalid token within handle");
		return FPGA_INVALID_PARAM;
	}

	result = xfpga_fpgaGetProperties(NULL, &props);
	if (result != FPGA_OK)
		return result;

	result = fpgaPropertiesSetParent(props, _handle->token);
	if (result != FPGA_OK) {
		FPGA_ERR("Error setting parent in properties.");
		goto free_props;
	}

	// TODO: Use slot number as part of filter
	//       We only want to query for accelerators for the
	//       slot being reconfigured
	result = xfpga_fpgaEnumerate(&props, 1, &token, 1, &matches);
	if (result != FPGA_OK) {
		FPGA_ERR("Error enumerating for accelerator to reconfigure");
		goto free_props;
	}

	if (matches == 0) {
		FPGA_ERR("No accelerator found to reconfigure");
		result = FPGA_BUSY;
		goto destroy_token;
	}

	result = xfpga_fpgaOpen(token, accel, 0);
	if (result != FPGA_OK) {
		FPGA_ERR("Could not open accelerator for given slot");
		goto destroy_token;
	}

destroy_token:
	destroy_result = xfpga_fpgaDestroyToken(&token);
	if (destroy_result != FPGA_OK)
		FPGA_ERR("Error destroying a token");

free_props:
	destroy_result = fpgaDestroyProperties(&props);
	if (destroy_result != FPGA_OK)
		FPGA_ERR("Error destroying properties");

	if (result != FPGA_OK || destroy_result != FPGA_OK)
		return result != FPGA_OK ? result : destroy_result;

	return FPGA_OK;
}


// clears port errors
STATIC fpga_result clear_port_errors(fpga_handle handle)
{
	char sysfs_path[SYSFS_PATH_MAX]    = {0};
	char sysfs_errpath[SYSFS_PATH_MAX] = {0};
	fpga_result result                = FPGA_OK;
	uint64_t error                    = 0 ;

	result = get_port_sysfs(handle, sysfs_path);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get port syfs path");
		return result;
	}

	snprintf_s_ss(sysfs_errpath, sizeof(sysfs_errpath), "%s/%s", sysfs_path, PORT_SYSFS_ERRORS);
	// Read port error.
	result = sysfs_read_u64(sysfs_errpath, &error);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get port errors");
		return result;
	}

	snprintf_s_ss(sysfs_errpath, sizeof(sysfs_errpath), "%s/%s", sysfs_path, PORT_SYSFS_ERR_CLEAR);
	// Clear port error.
	result = sysfs_write_u64(sysfs_errpath, error);
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
	char sysfs_path[SYSFS_PATH_MAX]    = {0};
	fpga_result result                = FPGA_OK;
	uint64_t userclk_high             = 0;
	uint64_t userclk_low              = 0;

	// Read port sysfs path
	result = get_port_sysfs(handle, sysfs_path);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get port syfs path");
		return result;
	}

	// set user clock
	result = set_userclock(sysfs_path, usrlclock_high, usrlclock_low);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to set user clock");
		return result;
	}

	// read user clock
	result = get_userclock(sysfs_path, &userclk_high, &userclk_low);
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

	// Set max power if not specified by gbs
	if (gbs_power == 0) {
		gbs_power = FPGA_GBS_MAX_POWER;
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
	snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s",  _token->sysfspath, PWRMGMT_THRESHOLD1);
	FPGA_DBG(" FPGA Threshold1             :%ld watts\n", fpga_power);

	result = sysfs_write_u64(sysfs_path, fpga_power);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to write power threshold 1");
		return result;
	}

	return result;
}

fpga_result __FPGA_API__ xfpga_fpgaReconfigureSlot(fpga_handle fpga,
						uint32_t slot,
						const uint8_t *bitstream,
						size_t bitstream_len,
						int flags)
{
	struct _fpga_handle *_handle    = (struct _fpga_handle *)fpga;
	fpga_result result              = FPGA_OK;
	struct reconf_error  error      = { {0} };
	struct gbs_metadata  metadata;
	int bitstream_header_len        = 0;
	int err                         = 0;
	fpga_handle accel               = NULL;

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

	// error out if "force" flag is NOT indicated
	// and the resource is in use
	if (!(flags & FPGA_RECONF_FORCE)) {
		result = open_accel(fpga, &accel);
		if (result != FPGA_OK) {
			FPGA_ERR("Accelerator in use or not found");
			goto out_unlock;
		}
	}

	// Clear port errors
	result = clear_port_errors(fpga);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to clear port errors.");
	}

	if (get_bitstream_json_len(bitstream) > 0) {
		enum fpga_hw_type hw_type = FPGA_HW_UNKNOWN;

		// Read GBS json metadata
		memset_s(&metadata, sizeof(metadata), 0);
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
		if (metadata.afu_image.clock_frequency_high > 0 || metadata.afu_image.clock_frequency_low > 0) {
			result = set_afu_userclock(fpga, metadata.afu_image.clock_frequency_high, metadata.afu_image.clock_frequency_low);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to set user clock");
				goto out_unlock;
			}
		}

		// get fpga device id.
		result = get_fpga_hw_type(fpga, &hw_type);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed to discover hardware type.");
			goto out_unlock;
		}

		// Set power threshold for integrated fpga.
		if (hw_type == FPGA_HW_MCP) {

			result = set_fpga_pwr_threshold(fpga, metadata.afu_image.power);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to set threshold.");
				goto out_unlock;
			}

		} // device id

	}

	result = opae_fme_port_pr(
		_handle->fddev, 0, slot, bitstream_len - bitstream_header_len,
		(uint64_t)bitstream + bitstream_header_len, &error.csr);
	if (result != 0) {
		FPGA_ERR("Failed to reconfigure bitstream: %s",
			  strerror(errno));

		if ((errno == EINVAL) || (errno == EFAULT)) {
			result = FPGA_INVALID_PARAM;
		} else {
			result = FPGA_EXCEPTION;
		}
	}

	if (error.reconf_operation_error == 0x1) {
		FPGA_ERR("PR operation error detected");
		result = FPGA_RECONF_ERROR;
	}

	if (error.reconf_CRC_error == 0x1) {
		FPGA_ERR("PR CRC error detected");
		result = FPGA_RECONF_ERROR;
	}

	if (error.reconf_incompatible_bitstream_error == 0x1) {
		FPGA_ERR("PR incompatible bitstream error detected");
		result = FPGA_RECONF_ERROR;
	}

	if (error.reconf_IP_protocol_error == 0x1) {
		FPGA_ERR("PR IP protocol error detected");
		result = FPGA_RECONF_ERROR;
	}

	if (error.reconf_FIFO_overflow_error == 0x1) {
		FPGA_ERR("PR FIFO overflow error detected");
		result = FPGA_RECONF_ERROR;
	}

	if (error.reconf_timeout_error == 0x1) {
		FPGA_ERR("PR timeout error detected");
		result = FPGA_RECONF_ERROR;
	}

	if (error.reconf_secure_load_error == 0x1) {
		FPGA_ERR("PR secure load error detected");
		result = FPGA_RECONF_ERROR;
	}

out_unlock:
	// close the accelerator opened during `open_accel`
	if (accel && xfpga_fpgaClose(accel) != FPGA_OK) {
		FPGA_ERR("Error closing accelerator after reconfiguration");
		result = FPGA_RECONF_ERROR;
	}

	err = pthread_mutex_unlock(&_handle->lock);
	if (err)
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	return result;
}
