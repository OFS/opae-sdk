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
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/**
* @file metrifcs.h
* @brief Functions for reading and clearing errors in resources
*
* Many FPGA resources have the ability to track the occurrence of errors.
* This file provides functions to retrieve information about errors within
* resources.
*/

#ifndef __FPGA_METRICS_H__
#define __FPGA_METRICS_H__

#include <opae/types.h>

#define FPGA_METRICS_STR_SIZE   256




/**
* OPAE C API Enumurate FPGA Metrics
*
*/


fpga_result  fpgaGetNumMetrics(fpga_handle handle,
							uint64_t *num_metrics);


fpga_result  fpgaGetMetricsInfo(fpga_handle handle,
								struct fpga_metric_t  *metric_info,
								uint64_t num_metrics);


fpga_result  fpgaGetMetricsByIds(fpga_handle handle,
								uint64_t * metric_id,
								uint64_t  num_metric_ids,
								struct fpga_metric_t  *metrics);

fpga_result  fpgaGetMetricsByStrings(fpga_handle handle,
									char ** metrics_serach_string,
									uint64_t  serach_string_size,
									struct fpga_metric_t  *metrics);


//////////////////



#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_METRICS_H__