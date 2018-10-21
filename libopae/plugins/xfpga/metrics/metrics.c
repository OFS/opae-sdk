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

/**
* \file metrics.c
* \brief fpga metrics API
*/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>


#include "safe_string/safe_string.h"
#include "opae/access.h"
#include "opae/utils.h"
#include "common_int.h"
#include "metrics_int.h"
#include "types_int.h"
#include "opae/metrics.h"
#include "metrics/vector.h"
#include "metrics/metrics_int.h"



fpga_result  __FPGA_API__ xfpga_fpgaGetNumMetrics(fpga_handle handle,
											uint64_t *num_metrics)
{
	fpga_result result = FPGA_OK;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	int err = 0;

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

	*num_metrics = fpga_vector_total(&(_handle->fpga_enum_metric_vector));

	printf(" ----------------------- num_enun_metrics =%d\n", *num_metrics);

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}

	return result;


}


fpga_result __FPGA_API__  xfpga_fpgaGetMetricsInfo(fpga_handle handle,
								struct fpga_metric_t  *fpga_metric,
								uint64_t num_metrics)
{

	fpga_result result = FPGA_OK;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	int err = 0;
	uint64_t i = 0;
	uint64_t num_enun_metrics;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (fpga_metric == NULL) {
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

	num_enun_metrics = fpga_vector_total(&(_handle->fpga_enum_metric_vector));



	for (i = 0; i < num_metrics; i++) {

		if (num_metrics <= num_enun_metrics) {

			struct _fpga_enum_metric* fpga_enum_metric = NULL;
			fpga_enum_metric = (struct _fpga_enum_metric*)	fpga_vector_get(&(_handle->fpga_enum_metric_vector), i);
			 
			add_metric_info(fpga_enum_metric, &fpga_metric[i]);
			
		}
	}

out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}

	return result;

}



fpga_result __FPGA_API__ xfpga_fpgaGetMetricsByIds(fpga_handle handle,
												uint64_t * metric_id,
												uint64_t  num_metric_ids,
													struct fpga_metric_t  *metrics)
{
	fpga_result result = FPGA_OK;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	int err = 0;
	uint64_t i = 0;
	fpga_objtype objtype;
	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (metrics == NULL ||
		metric_id == NULL) {
		FPGA_ERR("Invlaid Input parameters");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}
	/*
	result = enum_fpga_metrics(handle);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to Discover Metrics");
		result = FPGA_NOT_FOUND;
		goto out_unlock;
	}
	*/

	result = get_fpga_object_type(handle, &objtype);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to init vector");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (objtype == FPGA_ACCELERATOR) {


		// enum AFU
		for (i = 0; i < num_metric_ids; i++) {

			result = get_afu_metric_value(handle, &(_handle->fpga_enum_metric_vector),
				metric_id[i],
				&metrics[i]);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to get metric value");
				continue;
			}

	
		}
	}
	else	if (objtype == FPGA_DEVICE) {

		for (i = 0; i < num_metric_ids; i++) {

			result = get_metric_value_byid(_handle->token, &(_handle->fpga_enum_metric_vector),
				metric_id[i],
				&metrics[i]);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to get metric value");
				continue;
			}
		}
	}
	else {
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	







out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}

	return result;

}

fpga_result __FPGA_API__ xfpga_fpgaGetMetricsByStrings(fpga_handle handle,
										char ** metrics_serach_string,
										uint64_t  serach_string_size,
										struct fpga_metric_t  *metrics)
{

	fpga_result result = FPGA_OK;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	int err = 0;
	uint64_t i = 0;
	uint64_t metric_id;
	fpga_objtype objtype;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (metrics_serach_string == NULL  ||
		metrics == NULL) {
		FPGA_ERR("Invlaid Input parameters");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (serach_string_size == 0) {
		FPGA_ERR("Invlaid Input parameters");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	result = get_fpga_object_type(handle, &objtype);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to init vector");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (objtype == FPGA_ACCELERATOR) {


		// enum AFU

		for (i = 0; i < serach_string_size; i++) {

			result = get_metricid_from_serach_string(metrics_serach_string[i], &(_handle->fpga_enum_metric_vector), &metric_id);
			if (result != FPGA_OK) {
				FPGA_ERR("Invalid Input Metrics string");
				continue;
			}

			result = get_afu_metric_value(handle, &(_handle->fpga_enum_metric_vector),
				metric_id,
				&metrics[i]);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to get metric value");
				continue;
			}

		}
	}
	else	if (objtype == FPGA_DEVICE) {


		for (i = 0; i < serach_string_size; i++) {

			result = get_metricid_from_serach_string(metrics_serach_string[i], &(_handle->fpga_enum_metric_vector), &metric_id);
			if (result != FPGA_OK) {
				FPGA_ERR("Invalid Input Metrics string");
				continue;
			}

			result = get_metric_value_byid(_handle->token, &(_handle->fpga_enum_metric_vector),
				metric_id,
				&metrics[i]);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to get metric value");
				goto out_unlock;
			}

		}
	}
	else {
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}





out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;
}

