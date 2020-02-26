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

#include "fpgainfo.h"
#include "bmcdata.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

fpga_result get_metrics(fpga_token token,
			metrics_inquiry inquiry,
			fpga_metric_info *metrics_info,
			uint64_t *num_metrics_info,
			fpga_metric *metrics,
			uint64_t *num_metrics)
{
	if (!metrics_info || !metrics || !num_metrics || !num_metrics_info) {
	    return FPGA_INVALID_PARAM;
	}

	fpga_result res = FPGA_OK;
	fpga_result ret = FPGA_OK;
	fpga_handle handle;

	/* open FPGA */
	res = fpgaOpen(token, &handle, FPGA_OPEN_SHARED);
	ON_FPGAINFO_ERR_GOTO(res, out_close, "opening FPGA");

	res = fpgaGetNumMetrics(handle, num_metrics_info);
	ON_FPGAINFO_ERR_GOTO(res, out_close,
			     "getting number of metrics");

	res = fpgaGetMetricsInfo(handle, metrics_info, num_metrics_info);
	ON_FPGAINFO_ERR_GOTO(res, out_close,
			     "getting metrics info");

	/* get metrics */
	uint64_t id_array[METRICS_MAX_NUM];
	uint64_t i = 0;
	uint64_t j = 0;
	switch (inquiry) {
	case FPGA_ALL:
	    for (i = 0; i < *num_metrics_info; ++i) {
		id_array[j++] = i;
	    }
	    break;
	case FPGA_POWER:
	    for (i = 0; i < *num_metrics_info; ++i) {
		if (metrics_info[i].metric_type == FPGA_METRIC_TYPE_POWER) {
			id_array[j++] = i;
		}
	    }
	    break;
	case FPGA_THERMAL:
	    for (i = 0; i < *num_metrics_info; ++i) {
		if (metrics_info[i].metric_type == FPGA_METRIC_TYPE_THERMAL) {
			id_array[j++] = i;
		}
	    }
	    break;
	case FPGA_PERF:
	    for (i = 0; i < *num_metrics_info; ++i) {
		if (metrics_info[i].metric_type == FPGA_METRIC_TYPE_PERFORMANCE_CTR) {
		    id_array[j++] = i;
		}
	    }
	    break;
	}

	*num_metrics = j;

	if (*num_metrics == 0) {
		goto out_close;
	}

	res = fpgaGetMetricsByIndex(handle, id_array, *num_metrics, metrics);
	ON_FPGAINFO_ERR_GOTO(res, out_close, "getting metrics");

out_close:
	/* close FPGA */
	ret = (res != FPGA_OK) ? res : ret;
	res = fpgaClose(handle);
	ON_FPGAINFO_ERR_GOTO(res, out_exit, "closing FPGA");

out_exit:
	ret = (res != FPGA_OK) ? res : ret;
	return ret;
}

void print_metrics(const fpga_metric_info *metrics_info,
		   uint64_t num_metrics_info,
		   const fpga_metric *metrics, uint64_t num_metrics)
{
	uint64_t i = 0;
	for (i = 0; i < num_metrics; ++i) {
		uint64_t idx = metrics[i].metric_num;

		if (metrics[i].isvalid) {

			if (idx < num_metrics_info) {
				printf("(%2ld) %-27s : ", i + 1, metrics_info[idx].metric_name);

				switch (metrics_info[idx].metric_datatype) {
				case FPGA_METRIC_DATATYPE_INT:
					printf("%" PRId64 "", metrics[i].value.ivalue);
					break;
				case FPGA_METRIC_DATATYPE_DOUBLE: /* FALLTHROUGH */
				case FPGA_METRIC_DATATYPE_FLOAT:
					printf("%0.2f", metrics[i].value.dvalue);
					break;
				case FPGA_METRIC_DATATYPE_BOOL:
					printf("%d", metrics[i].value.bvalue);
					break;
				default:
					OPAE_ERR("Metrics Invalid datatype");
					break;
				}

				printf(" %s\n", metrics_info[idx].metric_units);
			}
		} else {
			// Failed to read metric value
			fprintf(stdout, "(%2ld) %-27s : %s\n", i + 1, metrics_info[idx].metric_name, "N/A");
		}

	}
}

