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
#include "fpgainfo.h"
#include "bmcdata.h"
#include "safe_string/safe_string.h"
#include <opae/fpga.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include "sysinfo.h"
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <wchar.h>

#ifdef DEBUG
#define DBG_PRINT(...) printf(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

typedef enum {
	CHIP_RESET_CAUSE_POR = 0x01,
	CHIP_RESET_CAUSE_EXTRST = 0x02,
	CHIP_RESET_CAUSE_BOD_IO = 0x04,
	CHIP_RESET_CAUSE_WDT = 0x08,
	CHIP_RESET_CAUSE_OCD = 0x10,
	CHIP_RESET_CAUSE_SOFT = 0x20,
	CHIP_RESET_CAUSE_SPIKE = 0x40,
} ResetCauses;

uint8_t bcd_plus[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
			'8', '9', ' ', '-', '.', ':', ',', '_'};

uint8_t ASCII_6_bit_translation[64] = {
	' ', '!', '\"', '#', '$', '%', '&', '\'', '(',  ')', '*', '+', ',',
	'-', '.', '/',  '0', '1', '2', '3', '4',  '5',  '6', '7', '8', '9',
	':', ';', '<',  '=', '>', '?', '@', 'A',  'B',  'C', 'D', 'E', 'F',
	'G', 'H', 'I',  'J', 'K', 'L', 'M', 'N',  'O',  'P', 'Q', 'R', 'S',
	'T', 'U', 'V',  'W', 'X', 'Y', 'Z', '[',  '\\', ']', '^', '_'};

wchar_t *base_units[] = {L"unspecified",
			 L"Celsius", // degrees C
			 L"Fahrenheit", // degrees F
			 L"Kelvin", // degrees K
			 L"Volts",
			 L"Amps",
			 L"Watts",
			 L"Joules",
			 L"Coulombs",
			 L"VA",
			 L"Nits",
			 L"limen",
			 L"lux",
			 L"Candela",
			 L"kPa",
			 L"PSI",
			 L"Newton",
			 L"CFM",
			 L"RPM",
			 L"Hz",
			 L"microsecond",
			 L"millisecond",
			 L"second",
			 L"minute",
			 L"hour",
			 L"day",
			 L"week",
			 L"mil",
			 L"inches",
			 L"feet",
			 L"in\x00b3",
			 L"ft\x00b3",
			 L"mm",
			 L"cm",
			 L"m",
			 L"cm\x00b3",
			 L"m\x00b3",
			 L"liters",
			 L"fluid ounce",
			 L"radians",
			 L"steradians",
			 L"revolutions",
			 L"cycles",
			 L"gravities",
			 L"ounce",
			 L"pound",
			 L"ft-lb",
			 L"oz-in",
			 L"gauss",
			 L"gilberts",
			 L"henry",
			 L"millihenry",
			 L"farad",
			 L"microfarad",
			 L"ohms",
			 L"siemens",
			 L"mole",
			 L"becquerel",
			 L"PPM",
			 L"reserved",
			 L"Decibels",
			 L"DbA",
			 L"DbC",
			 L"gray",
			 L"sievert",
			 L"color temp \x00b0\x004b", // degrees K
			 L"bit",
			 L"kilobit",
			 L"megabit",
			 L"gigabit",
			 L"byte",
			 L"kilobyte",
			 L"megabyte",
			 L"gigabyte",
			 L"word",
			 L"dword",
			 L"qword",
			 L"line",
			 L"hit",
			 L"miss",
			 L"retry",
			 L"reset",
			 L"overrun / overflow",
			 L"underrun",
			 L"collision",
			 L"packets",
			 L"messages",
			 L"characters",
			 L"error",
			 L"correctable error",
			 L"uncorrectable error",
			 L"fatal error",
			 L"grams"};

size_t max_base_units = (sizeof(base_units) / sizeof(base_units[0]));

char *sensor_type_codes[] = {"reserved",
			     "Temperature",
			     "Voltage",
			     "Current",
			     "Fan",
			     "Physical Security",
			     "Platform Security",
			     "Processor",
			     "Power Supply/VRM/Converter",
			     "Power Unit",
			     "Cooling Device",
			     "Other",
			     "Memory",
			     "Drive Slot",
			     "POST Memory Resize",
			     "System Firmware Progress",
			     "Event Logging Disabled",
			     "Watchdog 1",
			     "System Event",
			     "Critical Interrupt",
			     "Button / Switch",
			     "Module / Board",
			     "Microcontroller / Coprocessor",
			     "Add-in Card",
			     "Chassis",
			     "Chip Set",
			     "Other FRU",
			     "Cable / Interconnect",
			     "Terminator",
			     "System Boot / Restart Initiated",
			     "Boot Error",
			     "Base OS Boot / Installation Status",
			     "OS Stop / Shutdown",
			     "Slot / Connector",
			     "System ACPI Power State",
			     "Watchdog 2",
			     "Platform Alert",
			     "Entity Presence",
			     "Monitor ASIC / IC",
			     "LAN",
			     "Management Subsystem Health",
			     "Battery",
			     "Session Audit",
			     "Version Change",
			     "FRU State"};

size_t max_sensor_type_code =
	(sizeof(sensor_type_codes) / sizeof(sensor_type_codes[0]));
#define IS_SENSOR_TYPE_RESERVED(x) ((x) >= max_sensor_type_code)

char *event_reading_type_codes[] = {"unspecified",
				    "Threshold (01h)",
				    "Discrete (02h)",
				    "'digital' Discrete (03h)",
				    "'digital' Discrete (04h)",
				    "'digital' Discrete (05h)",
				    "'digital' Discrete (06h)",
				    "Discrete (07h)",
				    "'digital' Discrete (08h)",
				    "'digital' Discrete (09h)",
				    "Discrete (0Ah)",
				    "Discrete (0Bh)",
				    "Discrete (0Ch)"};


size_t max_event_reading_type_code = (sizeof(event_reading_type_codes)
				      / sizeof(event_reading_type_codes[0]));

char *entity_id_codes[0x43] = {"Unspecified",
			       "Other",
			       "Unknown",
			       "Processor",
			       "Disk or Disk Bay",
			       "Peripheral Bay",
			       "System Management Module",
			       "System Board",
			       "Memory Module",
			       "Processor Module",
			       "Power Supply",
			       "Add-in Card",
			       "Front Panel Board",
			       "Back Panel Board",
			       "Power System Board",
			       "Drive Backplane",
			       "System Internal Expansion Board",
			       "Other System Board",
			       "Processor Board",
			       "Power Unit / Power Domain",
			       "Power Module / Converter",
			       "Power Management / Distribution Board",
			       "Chassis Back Panel Board",
			       "System Chassis",
			       "Sub-Chassis",
			       "Other Chassis Board",
			       "Disk Drive Bay",
			       "Peripheral Bay",
			       "Device Bay",
			       "Fan / Cooling Device",
			       "Cooling Unit / Domain",
			       "Cable / Interconnect",
			       "Memory Device",
			       "System Management Software",
			       "System Firmware",
			       "Operating System",
			       "System Bus",
			       "Group",
			       "Remote Management Communication Device",
			       "External Environment",
			       "Battery",
			       "Processing Blade",
			       "Connectivity Switch",
			       "Processor / Memory Module",
			       "I/O Module",
			       "Processor I/O Module",
			       "Management Controller Firmware",
			       "IPMI Channel",
			       "PCI Bus",
			       "PCI Express Bus",
			       "SCSI Bus",
			       "SATA / SAS Bus",
			       "Processor / Front Side Bus",
			       "Real-Time Clock",
			       "Reserved",
			       "Air Inlet",
			       "Reserved",
			       "Air Inlet",
			       "Processor / CPU",
			       "Baseboard / Main System Board"};

size_t max_entity_id_code =
	(sizeof(entity_id_codes) / sizeof(entity_id_codes[0]));

static char *str_format[] = {"UNICODE", "BCD+", "6-bit packed ASCII",
			     "8-bit ASCII"};

#define IS_ENTITY_CHASSIS_SPECIFIC(x) (((x) >= 0x90) && ((x) <= 0xaf))
#define IS_ENTITY_BOARDSET_SPECIFIC(x) (((x) >= 0xb0) && ((x) <= 0xcf))
#define IS_ENTITY_OEM_DEFINED(x) ((x) >= 0xd0)
#define IS_ENTITY_RESERVED(x) (((x) >= 0x43) && ((x) <= 0x8f))

static char *linearity[] = {"linear", "ln",  "log10", "log2", "e",    "exp10",
			    "exp2",   "1/x", "x^2",   "x^3",  "sqrt", "1/x^3"};

static char *evt_strs[] = {
	"upper non-recoverable going high", "upper non-recoverable going low",
	"upper critical going high",	"upper critical going low",
	"upper non-critical going high",    "upper non-critical going low",
	"lower non-recoverable going high", "lower non-recoverable going low",
	"lower critical going high",	"lower critical going low",
	"lower non-critical going high",    "lower non-critical going low",
};

size_t max_linearity = sizeof(linearity) / sizeof(linearity[0]);

// Global for verbose mode
int bmcdata_verbose = 1;

#define PRINT(level, ...)                                                      \
	do {                                                                   \
		if (bmcdata_verbose) {                                         \
			printf("%.*s", (level), "\t\t\t\t\t\t\t\t\t\t\t");     \
			printf(__VA_ARGS__);                                   \
			printf("\n");                                          \
			fflush(stdout);                                        \
			fflush(stderr);                                        \
		}                                                              \
	} while (0)

static void print_entity(sdr_body *body, int level);
static void print_body(sdr_body *body, int level);
static void print_reading(sdr_body *body, sensor_reading *reading, int level);

void bmc_print_detail(sensor_reading *reading, sdr_header *header, sdr_key *key,
		      sdr_body *body)
{
	(void)header;

	PRINT(0, "Sensor number %d:", key->sensor_number);
	print_entity(body, 1);
	print_body(body, 1);
	print_reading(body, reading, 1);
}

void print_assertion_mask(int level, uint32_t mask, char *str)
{
	int i;
	for (i = 11; i >= 0; i--) {
		PRINT(level, "%s event for %s%s supported", str,
		      evt_strs[11 - i], (mask & (1 << i)) ? "" : " not");
	}
}


static void print_entity(sdr_body *body, int level)
{
	uint8_t entity_id = body->entity_id;
	uint8_t is_logical = body->entity_instance.bits.physical_logical;
	uint8_t instance_num = body->entity_instance.bits.instance_number;
	char *entity_type = NULL;
	char *relativity = NULL;

	if (entity_id < max_entity_id_code) {
		entity_type = entity_id_codes[entity_id];
	} else if (IS_ENTITY_CHASSIS_SPECIFIC(entity_id)) {
		entity_type = "Chassis-Specific";
	} else if (IS_ENTITY_BOARDSET_SPECIFIC(entity_id)) {
		entity_type = "Board-Set-Specific";
	} else if (IS_ENTITY_OEM_DEFINED(entity_id)) {
		entity_type = "OEM Defined";
	} else if (IS_ENTITY_RESERVED(entity_id)) {
		entity_type = "Reserved";
	}

	if (instance_num <= 0x5f) {
		relativity = "System-Relative";
	} else if (instance_num <= 0x7f) {
		relativity = "Device-Relative";
	}

	PRINT(level, "Entity is %s (%d)", entity_type, entity_id);
	PRINT(level + 1, "Entity is %s, and is %s.",
	      is_logical ? "Logical" : "Physical", relativity);
}

static char *data_format[] = {"unsigned", "1's complement signed",
			      "2's complement (signed)", "NO READING"};

static char *rate_unit[] = {"none",       "per uS",   "per ms",  "per s",
			    "per minute", "per hour", "per day", "reserved"};

double getvalue(Values *val, uint8_t raw)
{
	int i;
	double res = val->M * raw + val->B;
	if (val->result_exp >= 0) {
		for (i = 0; i < val->result_exp; i++) {
			res *= 10.0;
		}
	} else {
		for (i = val->result_exp; i; i++) {
			res /= 10.0;
		}
	}

	return res;
}

static char *thresholds[] = {"Lower non-critical",    "Lower critical",
			     "Lower non-recoverable", "Upper non-critical",
			     "Upper critical",	"Upper non-recoverable"};

void print_mask(int level, uint8_t val, const char *str)
{
	int i;
	for (i = 5; i >= 0; i--) {
		PRINT(level, "%s threshold is %s%s", thresholds[i],
		      (val & (1 << i)) ? "" : "not ", str);
	}
}

static void print_body(sdr_body *body, int level)
{
	char *str = NULL;
	char *str2 = NULL;
	char *str3 = NULL;
	wchar_t *wstr = NULL;
	wchar_t *wstr2 = NULL;
	int i;
	uint8_t settable = 0;
	uint8_t readable = 0;

	if (body->sensor_type < max_sensor_type_code) {
		str = sensor_type_codes[body->sensor_type];
	} else {
		str = "** UNKNOWN **";
	}

	uint8_t event_type_code = body->event_reading_type_code;
#define THRESHOLD_TYPE 1

	if (event_type_code < max_event_reading_type_code) {
		str2 = event_reading_type_codes[event_type_code];
	} else {
		str2 = "** UNKNOWN **";
	}

	PRINT(level, "Sensor type '%s' (0x%x): %s", str, body->sensor_type,
	      str2);

	Values val = {0};
	calc_params(body, &val);

	PRINT(level, "Sensor name:");
	PRINT(level + 1, "Length is %d bytes",
	      body->id_string_type_length_code.bits.len_in_characters != 0x1f
		      ? body->id_string_type_length_code.bits.len_in_characters
		      : -1);
	PRINT(level + 1, "String type is %s",
	      str_format[body->id_string_type_length_code.bits.format]);
	switch (body->id_string_type_length_code.bits.format) {
	case ASCII_8:
		PRINT(level + 1, "Name is '%s'", body->string_bytes);
		break;

		// TODO: Implement string decoding for other types
	case ASCII_6:
	case BCD_plus:
	case unicode:
		PRINT(level + 1, "*UNSUPPORTED");
		break;
	default:
		PRINT(level + 1, "*INVALID*");
	}

	if (0 != body->sensor_units_1._value) {
		PRINT(level, "Sensor Units: (0x%x)",
		      body->sensor_units_1._value);
		level++;
		PRINT(level, "Data format is %s",
		      data_format[body->sensor_units_1.bits
					  .analog_data_format]);
		PRINT(level, "Rate unit is %s",
		      rate_unit[body->sensor_units_1.bits.rate_unit]);
		if (body->sensor_units_1.bits.modifier_unit) {
			uint8_t unit = body->sensor_units_1.bits.modifier_unit;
			if (unit != 0x3) {
				if ((body->sensor_units_2 < max_base_units)
				    && (body->sensor_units_3
					< max_base_units)) {
					wstr = base_units[body->sensor_units_2];
					wstr2 = base_units
						[body->sensor_units_3];
				}
				str3 = unit == 1 ? "/" : "*";
			}

                        if (str3) {
                            PRINT(level, "Modifier unit is %ls %s %ls ", wstr, str3,
                                  wstr2);
                        }
		}
		PRINT(level, "Is%s a percentage",
		      body->sensor_units_1.bits.percentage ? "" : " not");
		level--;
	} else {
		PRINT(level, "No threshold or analog readings provided");
	}
	if (body->sensor_units_2 < max_base_units) {
		PRINT(level, "Sensor base units (0x%x): '%ls'",
		      body->sensor_units_2, base_units[body->sensor_units_2]);
	}

	PRINT(level, "Raw value conversion parameters:");
	level++;
	PRINT(level, "M(x) + B is %f(x) + %f", val.M, val.B);
	PRINT(level, "Result scale is 10^%d", val.result_exp);
	PRINT(level, "Accuracy is %f%%", val.accuracy / 100.0);
	PRINT(level, "Tolerance (in +/- 0.5 raw counts) is %d", val.tolerance);
	level--;

	switch (body->accuracy_accexp_sensor_direction.bits.sensor_direction) {
	case 0x00:
		PRINT(level, "Sensor direction unspecified or not applicable");
		break;
	case 0x01:
	case 0x02:
		PRINT(level, "Sensor is an %s sensor",
		      body->accuracy_accexp_sensor_direction.bits
					      .sensor_direction
				      == 0x1
			      ? "input"
			      : "output");
		break;
	case 0x03:
		PRINT(level, "Sensor direction *RESERVED*");
		break;
	}

	PRINT(level, "Analog characteristic flags: (0x%x)",
	      body->analog_characteristic_flags._value);
	if (0 == body->analog_characteristic_flags._value) {
		PRINT(level + 1, "None specified");
	} else {
		level++;
		if (body->analog_characteristic_flags.bits
			    .nominal_reading_specified) {
			PRINT(level, "Nominal reading specified");
			PRINT(level + 1, "Value: %f",
			      getvalue(&val, body->nominal_reading));
		}
		if (body->analog_characteristic_flags.bits
			    .normal_max_specified) {
			PRINT(level, "Normal maximum specified");
			PRINT(level + 1, "Value: %f",
			      getvalue(&val, body->normal_maximum));
		}
		if (body->analog_characteristic_flags.bits
			    .normal_min_specified) {
			PRINT(level, "Normal minimum specified");
			PRINT(level + 1, "Value: %f",
			      getvalue(&val, body->normal_minimum));
		}
		level--;
	}

	settable = (body->discrete_settable_readable_threshold_mask._value
		    & 0x3f00)
		   >> 8;
	readable =
		body->discrete_settable_readable_threshold_mask._value & 0x003f;

	PRINT(level, "Sensor maximum reading: %f",
	      getvalue(&val, body->sensor_maximum_reading));
	PRINT(level, "Sensor minimum reading: %f",
	      getvalue(&val, body->sensor_minimum_reading));

	PRINT(level, "Initialization: (0x%x)",
	      body->sensor_initialization._value);
	level++;
	PRINT(level, "Sensor is%s settable",
	      body->sensor_initialization.bits.settable_sensor ? "" : " not");
	PRINT(level, "Sensor can%s enable/disable scanning",
	      body->sensor_initialization.bits.init_scanning ? "" : "not");
	PRINT(level, "Sensor Events are %sabled",
	      body->sensor_initialization.bits.init_events ? "en" : "dis");
	PRINT(level, "Sensor thresholds are%s initialized",
	      body->sensor_initialization.bits.init_thresholds ? "" : " not");
	PRINT(level, "Sensor hysteresis is%s initialized",
	      body->sensor_initialization.bits.init_hysteresis ? "" : " not");
	PRINT(level, "Sensor Type and Event Reading Type are%s initialized",
	      body->sensor_initialization.bits.init_sensor_type ? "" : "not");
	PRINT(level, "Sensor event generation is %sabled",
	      body->sensor_initialization.bits.events_enabled ? "en" : "dis");
	PRINT(level, "Sensor scanning is %sabled",
	      body->sensor_initialization.bits.scanning_enabled ? "en" : "dis");

	PRINT(level - 1, "Capabilities: (0x%x)",
	      body->sensor_capabilities._value);
	PRINT(level, "%sgnore sensor if Entity not present or disabled",
	      body->sensor_capabilities.bits.ignore_sensor ? "I" : "Don't i");
	PRINT(level, "Auto re-arm is %s",
	      body->sensor_capabilities.bits.auto_rearm ? "auto" : "manual");

	switch (body->sensor_capabilities.bits.hysteresis_support) {
	case 0x0:
		str = "No hysteresis, or built-in but not specified";
		break;
	case 0x1:
		str = "Hysteresis is readable";
		break;
	case 0x2:
		str = "Hysteresis is readable and settable";
		break;
	case 0x3:
		str = "Fixed, unreadable Hysteresis";
		break;
	}
	PRINT(level, "%s", str);
	if (body->sensor_capabilities.bits.hysteresis_support) {
		PRINT(level + 1,
		      "Positive-going Threshold Hysteresis value is %f",
		      getvalue(&val, body->pos_going_threshold_hysteresis_val));
		PRINT(level + 1,
		      "Negative-going Threshold Hysteresis value is %f",
		      getvalue(&val, body->neg_going_threshold_hysteresis_val));
	}

	switch (body->sensor_capabilities.bits.threshold_access_support) {
	case 0x0:
		str = "No thresholds";
		break;
	case 0x1:
		str = "Thresholds are readable per reading mask";
		break;
	case 0x2:
		str = "Thresholds are readable and settable per reading/settable masks";
		break;
	case 0x3:
		str = "Fixed, unreadable thresholds";
		break;
	}
	PRINT(level, "%s", str);

	switch (body->sensor_capabilities.bits.msg_control_support) {
	case 0x0:
		str = "Event messages on per threshold event enable/disable control";
		break;
	case 0x1:
		str = "Event messages on entire sensor only";
		break;
	case 0x2:
		str = "Event messages global disable only";
		break;
	case 0x3:
		str = "No events from sensor";
		break;
	}
	PRINT(level, "%s", str);

	if (event_type_code == THRESHOLD_TYPE) {
		PRINT(level - 1, "Lower Threshold Reading Mask (0x%x)",
		      (body->assertion_event_lower_threshold_mask._value
		       & 0x7000)
			      >> 12);
		PRINT(level,
		      "Lower non-recoverable threshold comparison is%s returned",
		      body->assertion_event_lower_threshold_mask
				      .lower_threshold_mask
				      .lower_nr_thresh_comparison
			      ? ""
			      : " not");
		PRINT(level,
		      "Lower critical threshold comparison is%s returned",
		      body->assertion_event_lower_threshold_mask
				      .lower_threshold_mask
				      .lower_c_thresh_comparison
			      ? ""
			      : " not");
		PRINT(level,
		      "Lower non-critical threshold comparison is%s returned",
		      body->assertion_event_lower_threshold_mask
				      .lower_threshold_mask
				      .lower_nc_thresh_comparison
			      ? ""
			      : " not");

		PRINT(level - 1, "Upper Threshold Reading Mask (0x%x)",
		      (body->deassertion_event_upper_threshold_mask._value
		       & 0x7000)
			      >> 12);
		PRINT(level,
		      "Upper non-recoverable threshold comparison is%s returned",
		      body->deassertion_event_upper_threshold_mask
				      .upper_threshold_mask
				      .upper_nr_thresh_comparison
			      ? ""
			      : " not");
		PRINT(level,
		      "Upper critical threshold comparison is%s returned",
		      body->deassertion_event_upper_threshold_mask
				      .upper_threshold_mask
				      .upper_c_thresh_comparison
			      ? ""
			      : " not");
		PRINT(level,
		      "Upper non-critical threshold comparison is%s returned",
		      body->deassertion_event_upper_threshold_mask
				      .upper_threshold_mask
				      .upper_nc_thresh_comparison
			      ? ""
			      : " not");

		PRINT(level - 1, "Settable / Readable Threshold Masks (0x%x)",
		      body->discrete_settable_readable_threshold_mask._value);

		PRINT(level, "Settable Mask: (0x%x)", settable);
		print_mask(level + 1, settable, "settable");
		PRINT(level, "Readable Mask: (0x%x)", readable);
		print_mask(level + 1, readable, "readable");
		uint32_t assert_mask =
			body->assertion_event_lower_threshold_mask._value
			& 0x0fff;
		PRINT(level - 1, "Threshold Assertion Event Mask: (0x%x)",
		      assert_mask);
		print_assertion_mask(level, assert_mask, "Assertion");
		uint32_t deassert_mask =
			body->deassertion_event_upper_threshold_mask._value
			& 0x0fff;
		PRINT(level - 1, "Threshold Dessertion Event Mask: (0x%x)",
		      deassert_mask);
		print_assertion_mask(level, deassert_mask, "Deassertion");
	} else {
		settable = 0;
		readable = 0;
		PRINT(level - 1, "No Assertion Event Mask - Discrete sensor");
	}

	level--;

	size_t lin_val = body->linearization.bits.linearity_enum;
	PRINT(level, "Linearization: (0x%x) '%s'", (uint32_t)lin_val,
	      lin_val < max_linearity
		      ? linearity[lin_val]
		      : (lin_val > 0x70 ? "OEM non-linear" : "non-linear"));

	PRINT(level, "Threshold values:");

	for (i = 5; i >= 0; i--) {
		uint8_t t_val =
			((uint8_t *)body)[offsetof(sdr_body, upper_nr_threshold)
					  + 5 - i];
		if (settable & (1 << i)) {
			PRINT(level + 1, "%s threshold value is %f",
			      thresholds[i], getvalue(&val, t_val));
		} else {
			PRINT(level + 1,
			      "%s threshold value is IGNORED (raw val is 0x%x)",
			      thresholds[i], t_val);
		}
	}

	PRINT(level, "OEM field is 0x%02x", body->oem);

	PRINT(level, "%s", "");
}

void calc_params(sdr_body *body, Values *val)
{
	int32_t i;
	int32_t M_val = 0;
	int32_t B_val = 0;
	uint32_t A_val = 0;
	uint32_t T_val = 0;
	uint32_t A_exp = 0;
	int32_t R_exp = 0;
	int32_t B_exp = 0;

#define SIGN_EXT(val, bitpos) (((val) ^ (1 << (bitpos))) - (1 << (bitpos)))

	B_val = (body->B_accuracy.bits.B_2_msb << 8) | body->B_8_lsb;
	B_val = SIGN_EXT(B_val, 9);

	M_val = (body->M_tolerance.bits.M_2_msb << 8) | body->M_8_lsb;
	M_val = SIGN_EXT(M_val, 9);

	A_val = (body->accuracy_accexp_sensor_direction.bits.accuracy_4_msb
		 << 6)
		| body->B_accuracy.bits.accuracy_6_lsb;

	T_val = body->M_tolerance.bits.tolerance;

	A_exp = body->accuracy_accexp_sensor_direction.bits.accuracy_exp;
	R_exp = body->R_exp_B_exp.bits.R_exp;
	R_exp = SIGN_EXT(R_exp, 3);
	B_exp = body->R_exp_B_exp.bits.B_exp;
	B_exp = SIGN_EXT(B_exp, 3);

	// PRINT(4, "M=%d, B=%d * 10^%d, T=%d, A=%d * 10 ^ %d, Rexp=%d", M_val,
	//      B_val, B_exp, T_val, A_val, A_exp, R_exp);

	val->M = (double)M_val;
	val->tolerance = (double)T_val;
	val->B = (double)B_val;
	val->result_exp = R_exp;
	val->A_exp = A_exp;
	if (B_exp >= 0) {
		for (i = 0; i < B_exp; i++) {
			val->B *= 10.0;
		}
	} else {
		for (i = B_exp; i; i++) {
			val->B /= 10.0;
		}
	}

	val->accuracy = (double)A_val;
	for (i = 0; i < (int32_t)A_exp; i++) {
		val->accuracy *= 10.0;
	}
}

static void print_reading(sdr_body *body, sensor_reading *reading, int level)
{
	if (0 != reading->completion_code) {
		PRINT(level,
		      "Sensor reading returned non-zero completion code: 0x%02x",
		      reading->completion_code);
		return;
	}

	if (0
	    != reading->sensor_validity.sensor_state
		       .reading_state_unavailable) {
		PRINT(level, "Sensor reading unavailable");
		return;
	}

	Values val = {0};
	calc_params(body, &val);

	PRINT(level, "Sensor reading:");
	level++;
	PRINT(level, "Raw value: 0x%02x", reading->sensor_reading);
	PRINT(level + 1, "Scaled value is %f", getvalue(&val, reading->sensor_reading));

	if (0
	    != reading->sensor_validity.sensor_state.sensor_scanning_disabled) {
		PRINT(level, "Sensor scanning disabled");
	}

	if (0
	    != reading->sensor_validity.sensor_state.event_messages_disabled) {
		PRINT(level, "Event messages disabled");
	}

	if (body->event_reading_type_code == THRESHOLD_TYPE) {
		if (0 == reading->threshold_events._value) {
			PRINT(level, "No thresholds tripped");
		} else {
			if (reading->threshold_events.threshold_sensors
				    .at_or_above_upper_nr_threshold) {
				PRINT(level,
				      "At or above (>=) upper non-recoverable threshold");
			}
			if (reading->threshold_events.threshold_sensors
				    .at_or_above_upper_c_threshold) {
				PRINT(level,
				      "At or above (>=) upper critical threshold");
			}
			if (reading->threshold_events.threshold_sensors
				    .at_or_above_upper_nc_threshold) {
				PRINT(level,
				      "At or above (>=) upper non-critical threshold");
			}
			if (reading->threshold_events.threshold_sensors
				    .at_or_below_lower_nr_threshold) {
				PRINT(level,
				      "At or below (<=) lower non-recoverable threshold");
			}
			if (reading->threshold_events.threshold_sensors
				    .at_or_below_lower_c_threshold) {
				PRINT(level,
				      "At or below (<=) lower critical threshold");
			}
			if (reading->threshold_events.threshold_sensors
				    .at_or_below_lower_nc_threshold) {
				PRINT(level,
				      "At or below (<=) lower non-critical threshold");
			}
		}
	} else {
		PRINT(level, "Discrete sensor not implemented");
	}
}

void print_reset_cause(reset_cause *cause)
{
	if (cause->completion_code != 0) {
		printf("unavailable (cc = %d)\n", cause->completion_code);
		return;
	}

	if (0 == cause->reset_cause) {
		printf("None\n");
		return;
	}

	if (cause->reset_cause & CHIP_RESET_CAUSE_EXTRST)
		printf("External reset\n");

	if (cause->reset_cause & CHIP_RESET_CAUSE_BOD_IO)
		printf("Brown-out detected\n");

	if (cause->reset_cause & CHIP_RESET_CAUSE_OCD)
		printf("On-chip debug system\n");

	if (cause->reset_cause & CHIP_RESET_CAUSE_POR)
		printf("Power-on-reset\n");

	if (cause->reset_cause & CHIP_RESET_CAUSE_SOFT)
		printf("Software reset\n");

	if (cause->reset_cause & CHIP_RESET_CAUSE_SPIKE)
		printf("Spike detected\n");

	if (cause->reset_cause & CHIP_RESET_CAUSE_WDT)
		printf("Watchdog timeout\n");

	return;
}
