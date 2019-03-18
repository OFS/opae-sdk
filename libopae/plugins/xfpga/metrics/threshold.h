// Copyright(c) 2018-2019, Intel Corporation
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
#ifndef FPGA_THRESHOLD_H
#define FPGA_THRESHOLD_H

#include <opae/fpga.h>
#include "bmc/bmc_types.h"


#define  UPPER_NR_THRESHOLD                     "Upper Non-Recoverable Threshold"
#define  UPPER_C_THRESHOLD                      "Upper Critical Threshold"
#define  UPPER_NC_THRESHOLD                     "Upper Non-Critical Threshold"

#define  LOWER_NR_THRESHOLD                     "Lower Non-Recoverable Threshold"
#define  LOWER_C_THRESHOLD                      "Lower Critical Threshold"
#define  LOWER_NC_THRESHOLD                     "Lower Non-Critical Threshold"

#define  HYSTERESIS                             "Hysteresis"

#define  SYSFS_HIGH_FATAL                       "high_fatal"
#define  SYSFS_HIGH_WARN                        "high_warn"
#define  SYSFS_HYSTERESIS                       "hysteresis"
#define  SYSFS_LOW_FATAL                        "low_fatal"
#define  SYSFS_LOW_WARN                         "low_warn"


fpga_result get_bmc_threshold_info(fpga_handle handle,
	metric_threshold *metric_thresholds,
	uint32_t *num_thresholds);

fpga_result get_max10_threshold_info(fpga_handle handle,
	metric_threshold *metric_thresholds,
	uint32_t *num_thresholds);

#endif /* !FPGA_THRESHOLD_H */
