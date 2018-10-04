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

#ifndef __PACD_BMC_THERMAL_H__
#define __PACD_BMC_THERMAL_H__

#include <semaphore.h>
#include "config_int.h"

#define MAKE_MASK(num) ((1 << ((num) % 8)))
#define MAKE_INDEX(num) ((num) / 8)
#define BIT_SET(p, num) (((p)[MAKE_INDEX((num))] & MAKE_MASK((num))) != 0)
#define SET_BIT(p, num)                                                        \
	do {                                                                   \
		(p)[MAKE_INDEX((num))] |= MAKE_MASK((num));                    \
	} while (0)
#define CLEAR_BIT(p, num)                                                      \
	do {                                                                   \
		(p)[MAKE_INDEX((num))] &= ~(MAKE_MASK((num)));                 \
	} while (0)

struct bmc_thermal_context {
	struct config *config;
	int PAC_index;
	uint32_t has_been_PRd;
	fpga_token fme_token;

	uint16_t segment;
	uint8_t bus;
	uint8_t device;
	uint8_t function;

	uint32_t num_thresholds;
	int32_t sensor_number[MAX_SENSORS_TO_MONITOR];
	double upper_trigger_value[MAX_SENSORS_TO_MONITOR];
	double upper_reset_value[MAX_SENSORS_TO_MONITOR];
	double lower_trigger_value[MAX_SENSORS_TO_MONITOR];
	double lower_reset_value[MAX_SENSORS_TO_MONITOR];
	uint32_t invalid_count[MAX_SENSORS_TO_MONITOR];
};

void *bmc_thermal_thread(void *thread_context);
fpga_result sysfs_write_1(fpga_token token, const char *path);

#endif // __PACD_BMC_THERMAL_H__
