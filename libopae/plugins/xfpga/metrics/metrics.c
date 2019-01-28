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

/**
* \file metrics.c
* \brief xfpgs fpga metrics API
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H


#include "safe_string/safe_string.h"
#include "opae/access.h"
#include "opae/utils.h"
#include "common_int.h"
#include "types_int.h"
#include "opae/metrics.h"
#include "metrics/vector.h"
#include "metrics/metrics_int.h"

//Wrong search string invalid array index
#define METRIC_ARRAY_INVALID_INDEX     0xFFFFFF

fpga_result __FPGA_API__ xfpga_fpgaGetNumMetrics(fpga_handle handle,
					uint64_t *num_metrics)
{
	fpga_result result               = FPGA_OK;
	struct _fpga_handle *_handle     = (struct _fpga_handle *)handle;
	int err                          = 0;
	uint64_t num_enun_metrics        = 0;

	if (_handle == NULL) {
		FPGA_ERR("NULL fpga handle");
		return FPGA_INVALID_PARAM;
	}

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (num_metrics == NULL) {
		FPGA_ERR("Invlaid Input parameters");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	result = enum_fpga_metrics(handle);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to Discover Metrics");
		result = FPGA_NOT_FOUND;
		goto out_unlock;
	}

	fpga_vector_total(&(_handle->fpga_enum_metric_vector), &num_enun_metrics);

	if (num_enun_metrics == 0)
		result = FPGA_NOT_FOUND;

	*num_metrics = num_enun_metrics;

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}

	return result;
}

fpga_result __FPGA_API__ xfpga_fpgaGetMetricsInfo(fpga_handle handle,
					fpga_metric_info *metric_info,
					uint64_t *num_metrics)
{

	fpga_result result                              = FPGA_OK;
	struct _fpga_handle *_handle                    = (struct _fpga_handle *)handle;
	int err                                         = 0;
	uint64_t i                                      = 0;
	uint64_t num_enun_metrics                       = 0;
	struct _fpga_enum_metric	*fpga_enum_metric   = NULL;

	if (_handle == NULL) {
		FPGA_ERR("NULL fpga handle");
		return FPGA_INVALID_PARAM;
	}

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (metric_info == NULL ||
		num_metrics == NULL) {
		FPGA_ERR("Invlaid Input parameters");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	result = enum_fpga_metrics(handle);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to enum Metrics");
		result = FPGA_NOT_FOUND;
		goto out_unlock;
	}

	fpga_vector_total(&(_handle->fpga_enum_metric_vector), &num_enun_metrics);

	// get metric info
	for (i = 0; i < *num_metrics; i++) {

		if (*num_metrics <= num_enun_metrics) {

			fpga_enum_metric = (struct _fpga_enum_metric *)	fpga_vector_get(&(_handle->fpga_enum_metric_vector), i);
			result = add_metric_info(fpga_enum_metric, &metric_info[i]);
			if (result != FPGA_OK) {
				FPGA_MSG("Failed to add metric info");
				continue;
			}

		}
	}

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}

	return result;
}


fpga_result __FPGA_API__ xfpga_fpgaGetMetricsByIndex(fpga_handle handle,
						uint64_t *metric_num,
						uint64_t num_metric_indexes,
						fpga_metric *metrics)
{
	fpga_result result                      = FPGA_OK;
	uint64_t found                          = 0;
	struct _fpga_handle *_handle            = (struct _fpga_handle *)handle;
	int err                                 = 0;
	uint64_t i                              = 0;
	fpga_objtype objtype;

	if (_handle == NULL) {
		FPGA_ERR("NULL fpga handle");
		return FPGA_INVALID_PARAM;
	}

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (metrics == NULL ||
		metric_num == NULL) {
		FPGA_ERR("Invlaid Input parameters");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	result = enum_fpga_metrics(handle);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to Discover Metrics");
		result = FPGA_NOT_FOUND;
		goto out_unlock;
	}

	result = get_fpga_object_type(handle, &objtype);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to init vector");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (objtype == FPGA_ACCELERATOR) {
		// get AFU metrics
		for (i = 0; i < num_metric_indexes; i++) {

			result = get_afu_metric_value(handle,
						&(_handle->fpga_enum_metric_vector),
						metric_num[i],
						&metrics[i]);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to get metric value  at Index = %ld", metric_num[i]);
				metrics[i].metric_num = metric_num[i];
				continue;
			} else {
				// found metrics num
				found++;
			}
		}

		// API returns not found if doesnot found any metric
		if (found == 0 || num_metric_indexes == 0) {
			result = FPGA_NOT_FOUND;
		} else {
			result = FPGA_OK;
		}

	} else if (objtype == FPGA_DEVICE) {
		// get FME metrics
		for (i = 0; i < num_metric_indexes; i++) {

			result = get_fme_metric_value(handle,
							&(_handle->fpga_enum_metric_vector),
							metric_num[i],
							&metrics[i]);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to get metric value  at Index = %ld", metric_num[i]);
				metrics[i].metric_num = metric_num[i];
				continue;
			} else {
				// found metrics num
				found++;
			}
		}

		// API returns not found if doesnot found any metric
		if (found == 0 || num_metric_indexes == 0) {
			result = FPGA_NOT_FOUND;
		} else {
			result = FPGA_OK;
		}

	}

out_unlock:

	clear_cached_values(_handle);
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}

	return result;
}

fpga_result __FPGA_API__ xfpga_fpgaGetMetricsByName(fpga_handle handle,
						char **metrics_names,
						uint64_t num_metric_names,
						fpga_metric *metrics)
{
	fpga_result result                     = FPGA_OK;
	uint64_t found                         = 0;
	struct _fpga_handle *_handle           = (struct _fpga_handle *)handle;
	int err                                = 0;
	uint64_t i                             = 0;
	uint64_t metric_num                    = 0;
	fpga_objtype objtype;

	if (_handle == NULL) {
		FPGA_ERR("NULL fpga handle");
		return FPGA_INVALID_PARAM;
	}

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (metrics_names == NULL ||
		metrics == NULL) {
		FPGA_ERR("Invlaid Input parameters");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (num_metric_names == 0) {
		FPGA_ERR("Invlaid Input parameters");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	result = enum_fpga_metrics(handle);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to Discover Metrics");
		result = FPGA_NOT_FOUND;
		goto out_unlock;
	}


	result = get_fpga_object_type(handle, &objtype);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to init vector");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (objtype == FPGA_ACCELERATOR) {
		// get AFU metrics
		for (i = 0; i < num_metric_names; i++) {
			result = parse_metric_num_name(metrics_names[i],
							&(_handle->fpga_enum_metric_vector),
							&metric_num);
			if (result != FPGA_OK) {
				FPGA_ERR("Invalid input metrics string= %s", metrics_names[i]);
				metrics[i].metric_num = METRIC_ARRAY_INVALID_INDEX;
				continue;
			}

			result = get_afu_metric_value(handle, &(_handle->fpga_enum_metric_vector),
				metric_num,
				&metrics[i]);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to get metric value  for metric = %s", metrics_names[i]);
				metrics[i].metric_num = METRIC_ARRAY_INVALID_INDEX;
				continue;
			} else {
				// found metrics num
				found++;
			}
		}

		// API returns not found if doesnot found any metric
		if (found == 0 || num_metric_names == 0) {
			result = FPGA_NOT_FOUND;
		} else {
			result = FPGA_OK;
		}
	} else	if (objtype == FPGA_DEVICE) {
		// get FME metrics
		for (i = 0; i < num_metric_names; i++) {

			result = parse_metric_num_name(metrics_names[i],
							&(_handle->fpga_enum_metric_vector),
							&metric_num);
			if (result != FPGA_OK) {
				FPGA_ERR("Invalid input metrics string= %s", metrics_names[i]);
				metrics[i].metric_num = METRIC_ARRAY_INVALID_INDEX;
				continue;
			}

			result = get_fme_metric_value(handle,
							&(_handle->fpga_enum_metric_vector),
							metric_num,
							&metrics[i]);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to get metric value  for metric = %s \n", metrics_names[i]);
				metrics[i].metric_num = METRIC_ARRAY_INVALID_INDEX;
				continue;
			} else {
				// found metrics num
				found++;
			}
		}

		// API returns not found if doesnot found any metric
		if (found == 0 || num_metric_names == 0) {
			result = FPGA_NOT_FOUND;
		} else {
			result = FPGA_OK;
		}
	}

out_unlock:

	clear_cached_values(_handle);

	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}