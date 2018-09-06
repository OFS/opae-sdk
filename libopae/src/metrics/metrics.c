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
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <dirent.h>

#include "safe_string/safe_string.h"
#include "opae/access.h"
#include "opae/utils.h"
#include "common_int.h"
#include "metrics_int.h"
#include "types_int.h"
#include "opae/metrics.h"
#include "metrics/vector.h"

#include "bmcinfo.h"
#include "bmcdata.h"



fpga_result __FPGA_API__ CreateMetricsFilter(fpga_handle handle,
											metrics_filter *filter)
{
	fpga_result result						= FPGA_OK;
	struct _fpga_handle *_handle			= NULL;
	struct _metrics_filter *_filter			= NULL;
	struct _fpga_token *_token				= NULL;

	if (NULL == filter) {
		FPGA_ERR("Metrics filter is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (NULL == handle) {
		FPGA_ERR("Handle is NULL");
		return FPGA_INVALID_PARAM;
	}


	_handle = (struct _fpga_handle *)handle;
	if (_handle->magic != FPGA_HANDLE_MAGIC) {
		FPGA_MSG("Invalid handle Maginc number");
		return FPGA_INVALID_PARAM;
	}

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		FPGA_ERR("Invalid token within handle");
		return FPGA_INVALID_PARAM;
	}

	_filter = malloc(sizeof(struct _metrics_filter));
	if (NULL == _filter) {
		FPGA_MSG("Failed to allocate memory for filter");
		return FPGA_NO_MEMORY;
	}

	memset_s(_filter, sizeof(*_filter), 0);

	// mark data structure as valid
	_filter->magic = FPGA_METRICS_FILTER_MAGIC;

	// set handle return value
	*filter = (void *) _filter;


	result = fpga_vector_init(&(_filter->fpga_filter_metrics_vector));
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to init vector");
		goto out_free;
	}

	_filter->fpga_handle = _handle;

	return result;

out_free:

	if (_filter)
		free(_filter);

	return result;
}


fpga_result __FPGA_API__ DestroyMetricsFilter(metrics_filter filter)
{
	fpga_result result					= FPGA_OK;
	uint64_t i							= 0;
	struct _metrics_filter *_filter		= NULL;

	if (NULL == filter) {
		FPGA_MSG("Metrics Filter is NULL");
		return FPGA_INVALID_PARAM;
	}

	_filter = (struct _metrics_filter *)filter;
	if (_filter->magic != FPGA_METRICS_FILTER_MAGIC) {
		FPGA_MSG("Invalid handle");
		return FPGA_INVALID_PARAM;
	}

	for (i = 0; i < fpga_vector_total(&(_filter->fpga_filter_metrics_vector)); i++) {
		fpga_vector_delete(&(_filter->fpga_filter_metrics_vector), i);
	}

	fpga_vector_free(&(_filter->fpga_filter_metrics_vector));

	delete_fpga_metrics_vector(&(_filter->fpga_metrics_values));

	return result;
}


fpga_result __FPGA_API__ AddMetricsFilter(metrics_filter filter, char *metrics_string)
{

	fpga_result result							= FPGA_OK;
	char group_name[METRICS_NAME_SIZE]			= { 0 };
	char metrics_name[METRICS_NAME_SIZE]		= { 0 };
		struct _metrics_filter *_filter			= NULL;

	_filter = (struct _metrics_filter *)filter;
	if (NULL == _filter) {
		FPGA_MSG("Metrics filter is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (NULL == metrics_name) {
		FPGA_MSG("metrics name is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (_filter->magic != FPGA_METRICS_FILTER_MAGIC) {
		FPGA_MSG("Invalid handle");
		return FPGA_INVALID_PARAM;
	}

	result = ParseMetricsSearchString(metrics_string, group_name, metrics_name);
	if (result != FPGA_OK) {
		FPGA_MSG("Invalid input parameters");
		return result;
	}

	//printf("AddMetricsFilter \n");
	//printf("group_name = %s \n", group_name);
	//printf("metrics_name = %s \n", metrics_name);

	result = EnumMetricsByString(group_name, metrics_name, &(_filter->fpga_handle->fpga_enum_metrics_vector), &(_filter->fpga_filter_metrics_vector));
	if (result != FPGA_OK) {
		FPGA_MSG("Invalid input parameters");
		return result;
	}

	return result;

}

fpga_result __FPGA_API__ RemoveMetricsFilter(metrics_filter filter, char *metrics_string)
{
	fpga_result result							= FPGA_OK;
	char group_name[METRICS_NAME_SIZE]			= { 0 };
	char metrics_name[METRICS_NAME_SIZE]		= { 0 };
	struct _metrics_filter *_filter				= NULL;

	_filter = (struct _metrics_filter *)filter;
	if (NULL == _filter) {
		FPGA_MSG("Metrics filter is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (NULL == metrics_name) {
		FPGA_MSG("metrics name is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (_filter->magic != FPGA_METRICS_FILTER_MAGIC) {
		FPGA_MSG("Invalid handle");
		return FPGA_INVALID_PARAM;
	}

	result = ParseMetricsSearchString(metrics_string, group_name, metrics_name);
	if (result != FPGA_OK) {
		FPGA_MSG("Invalid input parameters");
		return result;
	}

	result = RemovieMetricsByString(group_name, metrics_name, &(_filter->fpga_filter_metrics_vector));
	if (result != FPGA_OK) {
		FPGA_MSG("Failed to remove metrics string ");
		return result;
	}

	return result;
}



fpga_result  __FPGA_API__ fpgaGetMetricsValues(fpga_handle handle,
												metrics_filter filter,
												struct fpga_metrics **fpga_metics)

{
	fpga_result result							= FPGA_OK;
	struct _metrics_filter *_filter				= NULL;
	uint64_t deviceid							= 0;
	int err										= 0;

	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;

	result = handle_check_and_lock(_handle);
	if (result)
		return result;

	if (_handle->fddev < 0) {
		FPGA_ERR("Invalid handle file descriptor");
		result = FPGA_INVALID_PARAM;
		goto out_unlock;
	}

	if (NULL == fpga_metics) {
		FPGA_MSG("Invalid input");
		goto out_unlock;
	}

	_filter = (struct _metrics_filter *)filter;
	if (NULL == _filter) {
		FPGA_MSG("Metrics filter is NULL");
		goto out_unlock;
	}

	if (_filter->magic != FPGA_METRICS_FILTER_MAGIC) {
		FPGA_MSG("Invalid handle");
		goto out_unlock;
	}

	// get fpga device id.
	result = get_fpga_deviceid(handle, &deviceid);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to read device id.");
		goto out_unlock;
	}


	delete_fpga_metrics_vector(&(_filter->fpga_metrics_values));


	result = fpga_metrics_vector_init(&(_filter->fpga_metrics_values));
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to init vector");
		goto out_unlock;
	}

	if (fpga_vector_total(&(_handle->fpga_enum_metrics_vector)) == 0)  {
		FPGA_ERR("FPGA Metrics doesn't supported ");
		result = FPGA_NOT_SUPPORTED;
		goto out_unlock;

	}

	// Get metrics values
	result = getMetricsValuesByString(&(_filter->fpga_filter_metrics_vector), &(_filter->fpga_metrics_values));
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get Metrics values");
		goto out_unlock;
	}

	/*

	for (uint64_t i = 0; i < fpga_metrics_vector_total(&(_handle->fpga_metrics_values)); i++) {
		metrics_info_val = (fpga_metrics_values*) fpga_metrics_vector_get(&(_handle->fpga_metrics_values), i);

		printf("metrics_class_name : %s\n ", metrics_info_val->group_name);
		printf("metrics_name : %s\n ", metrics_info_val->metrics_name);
		printf("metrics_sysfs : %ld\n ", metrics_info_val->fpga_value.ivalue);
		printf("\n");
		}
	*/


	*fpga_metics = (fpga_metrics *) &(_filter->fpga_metrics_values);


	printf("--------------- fpgaGetMetricsValues STRING  EXIT --------- API \n ");


out_unlock:
	err = pthread_mutex_unlock(&_handle->lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %s", strerror(err));
	}
	return result;

}