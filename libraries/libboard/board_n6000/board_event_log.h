// Copyright(c) 2021-2022, Intel Corporation
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

#ifndef __FPGA_BOARD_BEL_H__
#define __FPGA_BOARD_BEL_H__

#include <opae/types.h>

#define BEL_SENSOR_COUNT 83

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct bel_header {
	uint32_t magic;
	uint32_t timestamp_low;
	uint32_t timespamp_high;
} __attribute__((__packed__));

struct bel_power_on_status {
	struct bel_header header;
	uint32_t status;
	uint32_t fpga_status;
	uint32_t fpga_config_status;
	uint32_t sequencer_status_1;
	uint32_t sequencer_status_2;
	uint32_t power_good_status;
	uint32_t reserved[2];
} __attribute__((__packed__));

struct bel_power_off_status {
	struct bel_header header;
	uint32_t fpga_status;
	uint32_t fpga_config_status;
	uint32_t record_1;
	uint32_t record_2;
	uint32_t general_purpose_input_status;
	uint32_t sensor_failed;
	uint32_t sensor_alert_1;
	uint32_t sensor_alert_2;
	uint32_t sensor_alert_3;
	uint32_t reserved[2];
} __attribute__((__packed__));

struct bel_sensor_state {
	uint32_t id;
	uint32_t reading;
} __attribute__((__packed__));

struct bel_sensors_state {
	struct bel_header header;
	struct bel_sensor_state sensor_state[BEL_SENSOR_COUNT];
} __attribute__((__packed__));

struct bel_ext_status {
	union {
		struct {
			uint32_t word;
			uint32_t vout;
			uint32_t iout;
			uint32_t input;
			uint32_t temp;
			uint32_t cml;
		};
		uint32_t data[6];
	};
} __attribute__((__packed__));

struct bel_sensors_status {
	struct bel_header header;
	uint32_t ina3221_1_mask_enable;
	uint32_t ina3221_2_mask_enable;
	uint32_t ina3221_3_mask_enable;
	struct bel_ext_status ir38062;
	struct bel_ext_status ir38063;
	struct bel_ext_status isl68220;
	uint32_t ed8401_status;
} __attribute__((__packed__));

struct bel_timeoff_day {
	struct bel_header header;
	uint32_t timeofday_low;
	uint32_t timeofday_high;
} __attribute__((__packed__));

struct bel_max10_seu {
	struct bel_header header;
	uint32_t max10_seu;
} __attribute__((__packed__));

struct bel_fpga_seu {
	struct bel_header header;
	uint32_t fpga_seu;
} __attribute__((__packed__));

struct bel_pci_error_status {
	struct bel_header header;
	uint32_t pcie_link_status;
	uint32_t pcie_uncorr_err;
	uint32_t reserved[7];
} __attribute__((__packed__));

struct bel_event {
	union {
		struct {
			struct bel_power_on_status power_on_status;
			struct bel_timeoff_day timeoff_day;
			struct bel_max10_seu max10_seu;
			struct bel_fpga_seu fpga_seu;
			struct bel_pci_error_status pci_error_status;
			struct bel_power_off_status power_off_status;
			struct bel_sensors_state sensors_state;
			uint32_t reserved[8];
			struct bel_sensors_status sensors_status;
		};
		uint32_t data[1];
	};
} __attribute__((__packed__));

/**
 * Print human readable event info
 *
 * @param[in] event          Event structure to print
 * @param[in] print_sensors  Flag to enable printing of the many sensors
 * @param[in] print_sensors  Flag to enable printing of the many field bits
 */
void bel_print(struct bel_event *event, bool print_sensors, bool print_bits);

/**
 * Print event time span
 *
 * @param[in] event  Event structure to print time info for
 * @param[in] idx    Boot counter for the passed event
 */
void bel_timespan(struct bel_event *event, uint32_t idx);

/**
 * Test if an event is empty
 *
 * @param[in] event  Event structure to test
 *
 * @return True if event magic is not all 0xff's
 */
bool bel_empty(struct bel_event *event);

/**
 * Get number of events in log on flash
 *
 * @return Number of events
 */
uint32_t bel_ptr_count(void);

/**
 * Increment index to events while handling wrap-around
 *
 * @param[in] ptr  Offset in log to increment from
 *
 * @return Offset to next event in log on flash
 */
uint32_t bel_ptr_next(uint32_t ptr);

/**
 * Read index of latest event in log on flash
 *
 * @param[in] fpga_object  Sysfs node to read from
 * @param[out] ptr         Offset in log to read from
 *
 * @return FPGA_OK on success
 */
fpga_result bel_ptr(fpga_object fpga_object, uint32_t *ptr);

/**
 * Read event entry from log on flash
 *
 * @param[in] fpga_object  Sysfs node to read from
 * @param[in] ptr          Offset in log to read from
 * @param[out] event       Event structure to read into
 *
 * @return FPGA_OK on success
 */
fpga_result bel_read(fpga_object fpga_object, uint32_t ptr, struct bel_event *event);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FPGA_BOARD_BEL_H__ */
