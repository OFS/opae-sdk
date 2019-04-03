// Copyright(c) 2019, Intel Corporation
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

#ifndef __FPGA_BOARD_RC_H__
#define __FPGA_BOARD_RC_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define SDR_HEADER_LEN    3
#define SDR_MSG_LEN       4

typedef struct _powerdown_cause {
	uint8_t _header[SDR_HEADER_LEN];
	uint8_t completion_code;
	uint8_t iana[SDR_HEADER_LEN];
	uint8_t count;
	uint8_t message[SDR_MSG_LEN];
} powerdown_cause;

typedef struct _reset_cause {
	uint8_t _header[SDR_HEADER_LEN];
	uint8_t completion_code;
	uint8_t iana[SDR_HEADER_LEN];
	uint8_t reset_cause;
} reset_cause;


typedef enum {
	CHIP_RESET_CAUSE_POR = 0x01,
	CHIP_RESET_CAUSE_EXTRST = 0x02,
	CHIP_RESET_CAUSE_BOD_IO = 0x04,
	CHIP_RESET_CAUSE_WDT = 0x08,
	CHIP_RESET_CAUSE_OCD = 0x10,
	CHIP_RESET_CAUSE_SOFT = 0x20,
	CHIP_RESET_CAUSE_SPIKE = 0x40,
} ResetCauses;

typedef struct _device_id {
	uint8_t _header[SDR_HEADER_LEN];
	uint8_t completion_code;
	uint8_t device_id;
	union {
		struct {
			uint8_t device_revision : 3;
			uint8_t _unused : 3;
			uint8_t provides_sdrs : 2;
		} bits;
		uint8_t _value;
	} device_revision;
	union {
		struct {
			uint8_t device_available : 7;
			uint8_t major_fw_revision : 1;
		} bits;
		uint8_t _value;
	} firmware_revision_1;
	uint8_t firmware_revision_2;
	uint8_t ipmi_version;
	union {
		struct {
			uint8_t sensor_device : 1;
			uint8_t sdr_repository_device : 1;
			uint8_t sel_device : 1;
			uint8_t fru_inventory_device : 1;
			uint8_t ipmb_event_receiver : 1;
			uint8_t ipmb_event_generator : 1;
			uint8_t bridge : 1;
			uint8_t chassis_device : 1;
		} bits;
		uint8_t _value;
	} additional_device_support;
	uint8_t manufacturer_id_0_7;
	uint8_t manufacturer_id_8_15;
	uint8_t manufacturer_id_16_23;
	uint8_t product_id_0_7;
	uint8_t product_id_8_15;
	uint8_t aux_fw_rev_0_7;
	uint8_t aux_fw_rev_8_15;
	uint8_t aux_fw_rev_16_23;
	uint8_t aux_fw_rev_24_31;
} device_id;

 /**
 * Get Baseboard Management Controller version.
 *
 * @param[in] token           fpga_token object for device (FPGA_DEVICE type)
 * @param[inout] version      pointer to BMC version
* @returns FPGA_OK on success. FPGA_NOT_FOUND if BMC sysfs not found.
* FPGA_INVALID_PARAM if invalid parameters were provide
  *
  */
fpga_result read_bmc_version(fpga_token token, int *version);

/**
* Get BMC power down root cause
*
* @param[in] token                    fpga_token object for device (FPGA_DEVICE type)
* @param[inout] pwr_down_cause        pointer to power down root cause string.
*                                     user allocates memory and fee input string
* @returns FPGA_OK on success. FPGA_NOT_FOUND if BMC sysfs not found.
* FPGA_INVALID_PARAM if invalid parameters were provided
 *
 */
fpga_result read_bmc_pwr_down_cause(fpga_token token, char *pwr_down_cause);

/**
* Get BMC last reset root cause
*
* @param[in] token                    fpga_token object for device (FPGA_DEVICE type)
* @param[inout] reset_causee          pointer to reset root cause string.
*                                     user allocates memory and fee input string
* @returns FPGA_OK on success. FPGA_NOT_FOUND if BMC sysfs not found.
* FPGA_INVALID_PARAM if invalid parameters were provided
 *
 */
fpga_result read_bmc_reset_cause(fpga_token token, char *reset_causee);

/**
* Prints BMC version, Power down cause and Reset cause
*
* @param[in] token              fpga_token object for device (FPGA_DEVICE type)
* @returns FPGA_OK on success. FPGA_NOT_FOUND if BMC sysfs not found.
* FPGA_INVALID_PARAM if invalid parameters were provided
 *
 */
fpga_result print_borad_info(fpga_token token);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FPGA_BOARD_RC_H__ */