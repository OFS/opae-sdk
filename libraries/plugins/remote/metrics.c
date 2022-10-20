// Copyright(c) 2022, Intel Corporation
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <opae/types.h>
#include <opae/log.h>

#include "mock/opae_std.h"

#include "remote.h"

fpga_result __REMOTE_API__
remote_fpgaGetNumMetrics(fpga_handle handle, uint64_t *num_metrics)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) num_metrics;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaGetMetricsInfo(fpga_handle handle,
			  fpga_metric_info *metric_info,
			  uint64_t *num_metrics)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) metric_info;
(void) num_metrics;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaGetMetricsByIndex(fpga_handle handle,
			     uint64_t *metric_num,
			     uint64_t num_metric_indexes,
			     fpga_metric *metrics)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) metric_num;
(void) num_metric_indexes;
(void) metrics;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaGetMetricsByName(fpga_handle handle,
			    char **metrics_names,
			    uint64_t num_metric_names,
			    fpga_metric *metrics)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) metrics_names;
(void) num_metric_names;
(void) metrics;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaGetMetricsThresholdInfo(fpga_handle handle,
				   metric_threshold *metric_thresholds,
				   uint32_t *num_thresholds)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) metric_thresholds;
(void) num_thresholds;


	return result;
}
