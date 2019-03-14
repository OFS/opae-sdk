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
* @file metrics.h
* @brief Functions for Discover/ Enumerates metrics and retrieves values
*
*
*
*
*/

#ifndef __FPGA_METRICS_H__
#define __FPGA_METRICS_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enumerates number of metrics
 *
 * @param[in] handle Handle to previously opened fpga resource
 * @param[inout] num_metrics Number of metrics are discovered in
 * fpga resource
 *
 * @returns FPGA_OK on success. FPGA_NOT_FOUND if the Metrics are not
 * discovered
 *
 */
fpga_result fpgaGetNumMetrics(fpga_handle handle,
				uint64_t *num_metrics);

/**
 * Retrieve metrics information
 *
 * @param[in] handle Handle to previously opened fpga resource
 * @param[inout] metric_info Pointer to array of metric info struct
 * user allocates metrics info array
 *
 * @param[inout] num_metrics Size of metric info array
 *
 * @returns FPGA_OK on success. FPGA_NOT_FOUND if the Metrics are not
 * found. FPGA_NO_MEMORY if there was not enough memory to enumerates
 * metrics.
 *
 */
fpga_result fpgaGetMetricsInfo(fpga_handle handle,
				fpga_metric_info *metric_info,
				uint64_t *num_metrics);

/**
 * Retrieve metrics values by index
 *
 * @param[in] handle Handle to previously opened fpga resource
 * @param[inout] metric_num Pointer to array of metric index
 * user allocates metric array
 * @param[inout] num_metric_indexes Size of metric array
 * @param[inout] metrics pointer to array of metric struct
 *
 * @returns FPGA_OK on success. FPGA_NOT_FOUND if the Metrics are not
 * found. FPGA_NO_MEMORY if there was not enough memory to enumerates
 * metrics.
 *
 */
fpga_result fpgaGetMetricsByIndex(fpga_handle handle,
				uint64_t *metric_num,
				uint64_t num_metric_indexes,
				fpga_metric *metrics);

/**
 * Retrieve metric values by names
 *
 * @param[in] handle Handle to previously opened fpga resource
 * @param[inout] metrics_names Pointer to array of metrics name
 * user allocates metrics name array
 * @param[inout] num_metric_names Size of metric name array
 * @param[inout] metrics Pointer to array of metric struct
 *
 * @returns FPGA_OK on success. FPGA_NOT_FOUND if the Metrics are not
 * found
 *
 */
fpga_result fpgaGetMetricsByName(fpga_handle handle,
				char **metrics_names,
				uint64_t num_metric_names,
				fpga_metric *metrics);


/**
 * Retrieve metrics / sendor threshold information and values
 *
 * @param[in] handle Handle to previously opened fpga resource
 * @param[inout] metrics_threshold pointer to array of metric thresholds
 * user allocates threshold array memory
 * Number of thresholds returns enumerated threholds if user pass
 * NULL metrics_thresholds
 * @param[inout] num_thresholds number of thresholds
 *
 *
 * @returns FPGA_OK on success. FPGA_NOT_FOUND if the Metrics are not
 * found. FPGA_NO_MEMORY if there was not enough memory to enumerates
 * metrics.
 *
 */
fpga_result fpgaGetMetricsThresholdInfo(fpga_handle handle,
				struct metric_threshold *metric_thresholds,
				uint32_t *num_thresholds);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_METRICS_H__
