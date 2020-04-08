// Copyright(c) 2018-2020, Intel Corporation
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

#ifndef _WIN32
#include <unistd.h>
#include <uuid/uuid.h>
#else
#include <io.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <wchar.h>
#include "bmcinfo.h"
#include "bmcdata.h"
#include <opae/fpga.h>

static wchar_t *base_units[] = {L"unspecified",
				L"\x00b0\x0043", // degrees C
				L"\x00b0\x0046", // degrees F
				L"\x00b0\x004b", // degrees K
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

static size_t max_base_units = (sizeof(base_units) / sizeof(base_units[0]));

Values *bmc_build_values(sensor_reading *reading, sdr_header *header,
			 sdr_key *key, sdr_body *body)
{
	Values *val = (Values *)calloc(1, sizeof(Values));

	(void)header;

	if (NULL == val)
		return NULL;

	val->is_valid = true;

	if (!reading->sensor_validity.sensor_state.sensor_scanning_disabled) {
		val->annotation_1 = "scanning enabled";
		// val->is_valid = false;
	}
	if (reading->sensor_validity.sensor_state.reading_state_unavailable) {
		val->annotation_2 = "reading state unavailable";
		val->is_valid = false;
	}
	if (!reading->sensor_validity.sensor_state.event_messages_disabled) {
		val->annotation_3 = "event messages enabled";
	}

	if (body->id_string_type_length_code.bits.format == ASCII_8) {
		uint8_t len =
			body->id_string_type_length_code.bits.len_in_characters;
		if ((len == 0x1f) || (len == 0)) {
			val->name = strdup("**INVALID**");
			val->is_valid = false;
		} else {
			val->name = strdup((char *)&body->string_bytes[0]);
		}
	} else {
		val->name = strdup("**String type unimplemented**");
		DBG_PRINT("String type other than ASCII8\n");
	}

	val->sensor_number = key->sensor_number;
	val->sensor_type =
		SDR_SENSOR_IS_TEMP(body)
			? BMC_THERMAL
			: SDR_SENSOR_IS_POWER(body) ? BMC_POWER : BMC_ALL;

	switch (body->sensor_units_1.bits.analog_data_format) {
	case 0x0: // unsigned
	case 0x1: // 1's compliment (signed)
	case 0x2: // 2's complement (signed)
		break;
	case 0x3: // Does not return a reading
		val->is_valid = false;
		break;
	}

	if (body->sensor_units_2 < max_base_units) {
		val->units = base_units[body->sensor_units_2];
	} else {
		val->units = L"*OUT OF RANGE*";
	}

	calc_params(body, val);

	val->raw_value = (uint64_t)reading->sens_reading;
	val->val_type = SENSOR_FLOAT;
	val->value.f_val = getvalue(val, val->raw_value);

	return val;
}
