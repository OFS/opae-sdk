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
/*
 * @file fmeinfo.h
 *
 * @brief
 */
#ifndef BMCINFO_H
#define BMCINFO_H

#include <opae/fpga.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

// sysfs file names for power and temperature
#define SYSFS_SDR_FILE "avmmi-bmc.*.auto/bmc_info/sdr"
#define SYSFS_SENSOR_FILE "avmmi-bmc.*.auto/bmc_info/sensors"
#define SYSFS_DEVID_FILE "avmmi-bmc.*.auto/bmc_info/device_id"
#define SYSFS_RESET_FILE "avmmi-bmc.*.auto/bmc_info/reset_cause"
#define SYSFS_PWRDN_FILE "avmmi-bmc.*.auto/bmc_info/power_down_cause"
#define SYSFS_THERMAL_FILE "thermal_mgmt/temperature"

typedef enum { BMC_THERMAL, BMC_POWER, BMC_SENSORS } BMC_TYPE;

#pragma pack(push, 1)

// Structures used to read and decode Sensor Data Records (SDR)
typedef struct _sdr_header {
	uint16_t record_id;
	uint8_t sdr_version;
	uint8_t record_type;
	uint8_t record_length;
} sdr_header;

typedef struct _sdr_key {
	uint8_t sensor_owner_id;
	uint8_t sensor_owner_lun;
	uint8_t sensor_number;
} sdr_key;

typedef struct _sdr_body {
	uint8_t entity_id;

	union _entity_instance {
		struct {
			uint8_t instance_number : 7;
			uint8_t physical_logical : 1;
		} bits;
		uint8_t _value;
	} entity_instance;

	union _sensor_initialization {
		struct {
			uint8_t scanning_enabled : 1;
			uint8_t events_enabled : 1;
			uint8_t init_sensor_type : 1;
			uint8_t init_hysteresis : 1;
			uint8_t init_thresholds : 1;
			uint8_t init_events : 1;
			uint8_t init_scanning : 1;
			uint8_t settable_sensor : 1;
		} bits;
		uint8_t _value;
	} sensor_initialization;

	union _sensor_capabilities {
		struct {
			uint8_t msg_control_support : 2;
			uint8_t threshold_access_support : 2;
			uint8_t hysteresis_support : 2;
			uint8_t auto_rearm : 1;
			uint8_t ignore_sensor : 1;
		} bits;
		uint8_t _value;
	} sensor_capabilities;

	uint8_t sensor_type;
#define SDR_SENSOR_IS_TEMP(psdr) ((psdr)->sensor_type == 0x1)
#define SDR_SENSOR_IS_POWER(psdr)                                              \
	(((psdr)->sensor_type == 0x2) || ((psdr)->sensor_type == 0x3)          \
	 || ((psdr)->sensor_type == 0x8))

	uint8_t event_reading_type_code;

	union _assertion_event_lower_threshold_mask {
		struct {
			uint16_t event_offset_0 : 1;
			uint16_t event_offset_1 : 1;
			uint16_t event_offset_2 : 1;
			uint16_t event_offset_3 : 1;
			uint16_t event_offset_4 : 1;
			uint16_t event_offset_5 : 1;
			uint16_t event_offset_6 : 1;
			uint16_t event_offset_7 : 1;
			uint16_t event_offset_8 : 1;
			uint16_t event_offset_9 : 1;
			uint16_t event_offset_10 : 1;
			uint16_t event_offset_11 : 1;
			uint16_t event_offset_12 : 1;
			uint16_t event_offset_13 : 1;
			uint16_t event_offset_14 : 1;
			uint16_t _reserved : 1;
		} assertion_event_mask;
		struct {
			uint16_t _unused : 12;
			uint16_t lower_nc_thresh_comparison : 1;
			uint16_t lower_c_thresh_comparison : 1;
			uint16_t lower_nr_thresh_comparison : 1;
			uint16_t _reserved : 1;
		} lower_threshold_mask;
		struct {
			uint16_t
				assertion_event_lower_nc_going_low_supported : 1;
			uint16_t
				assertion_event_lower_nc_going_high_supported : 1;
			uint16_t
				assertion_event_lower_c_going_low_supported : 1;
			uint16_t
				assertion_event_lower_c_going_high_supported : 1;
			uint16_t
				assertion_event_lower_nr_going_low_supported : 1;
			uint16_t
				assertion_event_lower_nr_going_high_supported : 1;
			uint16_t
				assertion_event_upper_nc_going_low_supported : 1;
			uint16_t
				assertion_event_upper_nc_going_high_supported : 1;
			uint16_t
				assertion_event_upper_c_going_low_supported : 1;
			uint16_t
				assertion_event_upper_c_going_high_supported : 1;
			uint16_t
				assertion_event_upper_nr_going_low_supported : 1;
			uint16_t
				assertion_event_upper_nr_going_high_supported : 1;
			uint16_t _unused : 4;
		} threshold_assertion_event_mask;
		uint16_t _value;
	} assertion_event_lower_threshold_mask;

	union _deassertion_event_upper_threshold_mask {
		struct {
			uint16_t event_offset_0 : 1;
			uint16_t event_offset_1 : 1;
			uint16_t event_offset_2 : 1;
			uint16_t event_offset_3 : 1;
			uint16_t event_offset_4 : 1;
			uint16_t event_offset_5 : 1;
			uint16_t event_offset_6 : 1;
			uint16_t event_offset_7 : 1;
			uint16_t event_offset_8 : 1;
			uint16_t event_offset_9 : 1;
			uint16_t event_offset_10 : 1;
			uint16_t event_offset_11 : 1;
			uint16_t event_offset_12 : 1;
			uint16_t event_offset_13 : 1;
			uint16_t event_offset_14 : 1;
			uint16_t _reserved : 1;
		} deassertion_event_mask;
		struct {
			uint16_t _unused : 12;
			uint16_t upper_nc_thresh_comparison : 1;
			uint16_t upper_c_thresh_comparison : 1;
			uint16_t upper_nr_thresh_comparison : 1;
			uint16_t _reserved : 1;
		} upper_threshold_mask;
		struct {
			uint16_t
				deassertion_event_lower_nc_going_low_supported : 1;
			uint16_t
				deassertion_event_lower_nc_going_high_supported : 1;
			uint16_t
				deassertion_event_lower_c_going_low_supported : 1;
			uint16_t
				deassertion_event_lower_c_going_high_supported : 1;
			uint16_t
				deassertion_event_lower_nr_going_low_supported : 1;
			uint16_t
				deassertion_event_lower_nr_going_high_supported : 1;
			uint16_t
				deassertion_event_upper_nc_going_low_supported : 1;
			uint16_t
				deassertion_event_upper_nc_going_high_supported : 1;
			uint16_t
				deassertion_event_upper_c_going_low_supported : 1;
			uint16_t
				deassertion_event_upper_c_going_high_supported : 1;
			uint16_t
				deassertion_event_upper_nr_going_low_supported : 1;
			uint16_t
				deassertion_event_upper_nr_going_high_supported : 1;
			uint16_t _unused : 4;
		} threshold_deassertion_event_mask;
		uint16_t _value;
	} deassertion_event_upper_threshold_mask;

	union _discrete_settable_readable_threshold_mask {
		struct {
			uint16_t discrete_state_enable_0 : 1;
			uint16_t discrete_state_enable_1 : 1;
			uint16_t discrete_state_enable_2 : 1;
			uint16_t discrete_state_enable_3 : 1;
			uint16_t discrete_state_enable_4 : 1;
			uint16_t discrete_state_enable_5 : 1;
			uint16_t discrete_state_enable_6 : 1;
			uint16_t discrete_state_enable_7 : 1;
			uint16_t discrete_state_enable_8 : 1;
			uint16_t discrete_state_enable_9 : 1;
			uint16_t discrete_state_enable_10 : 1;
			uint16_t discrete_state_enable_11 : 1;
			uint16_t discrete_state_enable_12 : 1;
			uint16_t discrete_state_enable_13 : 1;
			uint16_t discrete_state_enable_14 : 1;
			uint16_t _reserved : 1;
		} discrete_reading_mask;
		struct {
			uint16_t _unused : 8;
			uint16_t lower_nc_thresh_settable : 1;
			uint16_t lower_c_thresh_settable : 1;
			uint16_t lower_nr_thresh_settable : 1;
			uint16_t upper_nc_thresh_settable : 1;
			uint16_t upper_c_thresh_settable : 1;
			uint16_t upper_nr_thresh_settable : 1;
			uint16_t _reserved : 2;
		} settable_threshold_mask;
		struct {
			uint16_t lower_nc_thresh_readable : 1;
			uint16_t lower_c_thresh_readable : 1;
			uint16_t lower_nr_thresh_readable : 1;
			uint16_t upper_nc_thresh_readable : 1;
			uint16_t upper_c_thresh_readable : 1;
			uint16_t upper_nr_thresh_readable : 1;
			uint16_t _unused : 10;
		} readable_threshold_mask;
		uint16_t _value;
	} discrete_settable_readable_threshold_mask;

	union _sensor_units_1 {
		struct {
			uint8_t percentage : 1;
			uint8_t modifier_unit : 2;
			uint8_t rate_unit : 3;
			uint8_t analog_data_format : 2;
		} bits;
		uint8_t _value;
	} sensor_units_1;

	uint8_t sensor_units_2;
	uint8_t sensor_units_3;

	union _linearization {
		struct {
			uint8_t linearity_enum : 7;
			uint8_t _reserved : 1;
		} bits;
		uint8_t _value;
	} linearization;

	uint8_t M_8_lsb;

	union _M_tolerance {
		struct {
			uint8_t tolerance : 6;
			uint8_t M_2_msb : 2;
		} bits;
		uint8_t _value;
	} M_tolerance;

	uint8_t B_8_lsb;

	union _B_accuracy {
		struct {
			uint8_t accuracy_6_lsb : 6;
			uint8_t B_2_msb : 2;
		} bits;
		uint8_t _value;
	} B_accuracy;

	union _accuracy_accexp_sensor_direction {
		struct {
			uint8_t sensor_direction : 2;
			uint8_t accuracy_exp : 2;
			uint8_t accuracy_4_msb : 4;
		} bits;
		uint8_t _value;
	} accuracy_accexp_sensor_direction;

	union _R_exp_B_exp {
		struct {
			uint8_t B_exp : 4;
			uint8_t R_exp : 4;
		} bits;
		uint8_t _value;
	} R_exp_B_exp;

	union _analog_characteristic_flags {
		struct {
			uint8_t nominal_reading_specified : 1;
			uint8_t normal_max_specified : 1;
			uint8_t normal_min_specified : 1;
			uint8_t _reserved : 5;
		} bits;
		uint8_t _value;
	} analog_characteristic_flags;

	uint8_t nominal_reading;
	uint8_t normal_maximum;
	uint8_t normal_minimum;
	uint8_t sensor_maximum_reading;
	uint8_t sensor_minimum_reading;
	uint8_t upper_nr_threshold;
	uint8_t upper_c_threshold;
	uint8_t upper_nc_threshold;
	uint8_t lower_nr_threshold;
	uint8_t lower_c_threshold;
	uint8_t lower_nc_threshold;
	uint8_t pos_going_threshold_hysteresis_val;
	uint8_t neg_going_threshold_hysteresis_val;

	uint8_t _reserved0;
	uint8_t _reserved1;
	uint8_t oem;

	union _id_string_type_length_code {
		struct {
			uint8_t len_in_characters : 5; // 11111b reserved,
						       // 00000b means none
						       // following
			uint8_t _reserved : 1;
			uint8_t format : 2; // using TLC_FORMAT enum
		} bits;
		uint8_t _value;
	} id_string_type_length_code;

	uint8_t string_bytes[30]; // Interpreted based on type/length code
} sdr_body;

typedef enum _TLC_FORMAT {
	unicode = 0x0,
	BCD_plus = 0x1,
	ASCII_6 = 0x2, // packed
	ASCII_8 = 0x3
} TLC_FORMAT;

extern uint8_t bcd_plus[];

extern uint8_t ASCII_6_bit_translation[];

typedef struct _sensor_reading {
	uint8_t _header[3]; // Ignored
	uint8_t completion_code;
	uint8_t sensor_reading;
	union {
		struct {
			uint8_t _unused : 5;
			uint8_t reading_state_unavailable : 1;
			uint8_t sensor_scanning_disabled : 1;
			uint8_t event_messages_disabled : 1;
		} sensor_state;
		uint8_t _value;
	} sensor_validity;
	union {
		struct {
			uint8_t at_or_below_lower_nc_threshold : 1;
			uint8_t at_or_below_lower_c_threshold : 1;
			uint8_t at_or_below_lower_nr_threshold : 1;
			uint8_t at_or_above_upper_nc_threshold : 1;
			uint8_t at_or_above_upper_c_threshold : 1;
			uint8_t at_or_above_upper_nr_threshold : 1;
			uint8_t _unused : 2;
		} threshold_sensors;
		struct {
			uint8_t state_asserted_0 : 1;
			uint8_t state_asserted_1 : 1;
			uint8_t state_asserted_2 : 1;
			uint8_t state_asserted_3 : 1;
			uint8_t state_asserted_4 : 1;
			uint8_t state_asserted_5 : 1;
			uint8_t state_asserted_6 : 1;
			uint8_t state_asserted_7 : 1;
		} discrete_sensors;
		uint8_t _value;
	} threshold_events;
} sensor_reading;

typedef struct _device_id {
	uint8_t _header[3]; // Ignored
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

typedef struct _powerdown_cause {
	uint8_t _header[3]; // Ignored
	uint8_t completion_code;
	uint8_t iana[3];
	uint8_t count;
	uint8_t message[40];
} powerdown_cause;

typedef struct _reset_cause {
	uint8_t _header[3]; // Ignored
	uint8_t completion_code;
	uint8_t iana[3];
	uint8_t reset_cause; // * TODO: Not sure about this
} reset_cause;

#pragma pack(pop)

typedef enum { SENSOR_INT, SENSOR_FLOAT } sensor_value_type;

typedef struct _Values {
	struct _Values *next;
	char *name;
	wchar_t *units;
	char *annotation_1;
	char *annotation_2;
	char *annotation_3;
	uint8_t raw_value;
	uint8_t is_valid;
	uint32_t tolerance;
	double accuracy;
	double M;
	double B;
	int32_t A_exp;
	int32_t result_exp;
	union {
		double f_val;
		uint64_t i_val;
	} value;
	uint8_t sensor_number;
	uint8_t sensor_type;
	sensor_value_type val_type;
} Values;

fpga_result bmc_print_values(const char *sysfs_path, BMC_TYPE type);
int get_bmc_path(const char *in_path, const char *key_str, char *out_path,
				 int size);
void get_sysfs_attr(const char *attr_path, char *buf, int size);
void print_sensor_info(const char *sysfspath, BMC_TYPE type);
fpga_result bmc_filter(fpga_properties *filter, int argc, char *argv[]);
void print_bmc_info(const char *sysfspath);
fpga_result bmc_command(fpga_token *tokens, int num_tokens, int argc,
			 char *argv[]);
void bmc_help(void);

#ifdef __cplusplus
}
#endif

#endif /* !BMCINFO_H */
