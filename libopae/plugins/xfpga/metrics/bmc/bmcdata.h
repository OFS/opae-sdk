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
 * @file bmcdata.h
 *
 * @brief
 */
#ifndef BMCDATA_H
#define BMCDATA_H

#include <opae/fpga.h>
#include <wchar.h>
#include "bmcinfo.h"

//#ifdef __cplusplus
//extern "C" {
//#endif

typedef enum {
	CHIP_RESET_CAUSE_POR = 0x01,
	CHIP_RESET_CAUSE_EXTRST = 0x02,
	CHIP_RESET_CAUSE_BOD_IO = 0x04,
	CHIP_RESET_CAUSE_WDT = 0x08,
	CHIP_RESET_CAUSE_OCD = 0x10,
	CHIP_RESET_CAUSE_SOFT = 0x20,
	CHIP_RESET_CAUSE_SPIKE = 0x40,
} ResetCauses;

//extern uint8_t bcd_plus[];
//extern uint8_t ASCII_6_bit_translation[];
//extern wchar_t *base_units[];
//extern size_t max_base_units;
//extern char *sensor_type_codes[];
//extern size_t max_sensor_type_code;
//extern char *event_reading_type_codes[];
//extern size_t max_event_reading_type_code;
//extern char *entity_id_codes[];
//extern size_t max_entity_id_code;
//extern int bmcdata_verbose;

//void bmc_print_detail(sensor_reading *reading, sdr_header *header, sdr_key *key,
//		      sdr_body *body);
void calc_params(sdr_body *body, Values *val);
double getvalue(Values *val, uint8_t raw);
//void print_reset_cause(reset_cause *cause);

//#ifdef __cplusplus
//}
//#endif

#endif /* !BMCDATA_H */
