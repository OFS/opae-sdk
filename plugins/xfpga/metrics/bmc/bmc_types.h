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
#ifndef BMC_TYPES_H
#define BMC_TYPES_H

#include <opae/fpga.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

// Order and values important - see IPMI spec 35.14
#define BMC_UPPER_NON_RECOVERABLE 0x20
#define BMC_UPPER_CRITICAL 0x10
#define BMC_UPPER_NON_CRITICAL 0x08
#define BMC_LOWER_NON_RECOVERABLE 0x04
#define BMC_LOWER_CRITICAL 0x02
#define BMC_LOWER_NON_CRITICAL 0x01
#define BMC_THRESHOLD_EVENT_MASK                                               \
	(BMC_UPPER_NON_RECOVERABLE | BMC_UPPER_CRITICAL                        \
	 | BMC_UPPER_NON_CRITICAL | BMC_LOWER_NON_RECOVERABLE                  \
	 | BMC_LOWER_CRITICAL | BMC_LOWER_NON_CRITICAL)

#define BMC_UPPER_NR_TRIPPED(t)                                                \
	(((t).which_thresholds & BMC_UPPER_NON_RECOVERABLE) != 0)
#define BMC_UPPER_C_TRIPPED(t)                                                 \
	(((t).which_thresholds & BMC_UPPER_CRITICAL) != 0)
#define BMC_UPPER_NC_TRIPPED(t)                                                \
	(((t).which_thresholds & BMC_UPPER_NON_CRITICAL) != 0)
#define BMC_LOWER_NR_TRIPPED(t)                                                \
	(((t).which_thresholds & BMC_LOWER_NON_RECOVERABLE) != 0)
#define BMC_LOWER_C_TRIPPED(t)                                                 \
	(((t).which_thresholds & BMC_LOWER_CRITICAL) != 0)
#define BMC_LOWER_NC_TRIPPED(t)                                                \
	(((t).which_thresholds & BMC_LOWER_NON_CRITICAL) != 0)

typedef void *bmc_sdr_handle;
typedef void *bmc_values_handle;

typedef enum {
	BMC_THERMAL,
	BMC_POWER,
	BMC_ALL,
} BMC_SENSOR_TYPE;

typedef struct {
	uint32_t sensor_number;
	BMC_SENSOR_TYPE type;
	uint32_t which_thresholds; // bit vector
} tripped_thresholds;

typedef struct _per_threshold {
	uint32_t is_valid;
	double value;
} per_thresh;

typedef struct _thresholds {
	per_thresh upper_nr_thresh;
	per_thresh upper_c_thresh;
	per_thresh upper_nc_thresh;
	per_thresh lower_nr_thresh;
	per_thresh lower_c_thresh;
	per_thresh lower_nc_thresh;
} threshold_list;

typedef struct _sdr_details {
	uint32_t sensor_number;
	BMC_SENSOR_TYPE type;
	char *name;
	wchar_t *units;
	double M;
	double B;
	double accuracy;
	uint32_t tolerance;
	int32_t result_exp;
	threshold_list thresholds;
} sdr_details;

#ifdef __cplusplus
}
#endif

#endif /* !BMC_TYPES_H */
