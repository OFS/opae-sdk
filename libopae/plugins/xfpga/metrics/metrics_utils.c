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

/**
* \file metrics_utils.c
* \brief fpga metrics utils functions
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <uuid/uuid.h>
#include <dlfcn.h>

#include "common_int.h"
#include "metrics_int.h"
#include "types_int.h"
#include "opae/metrics.h"
#include "metrics/vector.h"
#include "xfpga.h"
#include "metrics/bmc/bmc.h"
#include "safe_string/safe_string.h"
#include "metrics/metrics_metadata.h"
#include "mcp_metadata.h"
#include "metrics_max10.h"

fpga_result metric_sysfs_path_is_dir(const char *path)
{
	struct stat astats;

	if (path == NULL) {
		return FPGA_INVALID_PARAM;
	}

	if ((stat(path, &astats)) != 0) {
		return FPGA_NOT_FOUND;
	}

	if (S_ISDIR(astats.st_mode)) {
		return FPGA_OK;
	}

	return FPGA_NOT_FOUND;
}

fpga_result metric_sysfs_path_is_file(const char *path)
{
	struct stat astats;

	if (path == NULL) {
		return FPGA_INVALID_PARAM;
	}

	if ((stat(path, &astats)) != 0) {
		return FPGA_NOT_FOUND;
	}

	if (S_ISREG(astats.st_mode)) {
		return FPGA_OK;
	}

	return FPGA_NOT_FOUND;
}

// Adds Metrics info to vector
fpga_result add_metric_vector(fpga_metric_vector *vector,
				uint64_t metric_num,
				const char *qualifier_name,
				const char *group_name,
				const char *group_sysfs,
				const char *metric_name,
				const char *metric_sysfs,
				const char *metric_units,
				enum fpga_metric_datatype  metric_datatype,
				enum fpga_metric_type	metric_type,
				enum fpga_hw_type	hw_type,
				uint64_t mmio_offset)
{

	fpga_result result                           = FPGA_OK;
	struct _fpga_enum_metric *fpga_enum_metric   = NULL;
	errno_t e                                    = 0;

	if (vector == NULL ||
		group_name == NULL ||
		group_sysfs == NULL ||
		metric_name == NULL ||
		metric_sysfs == NULL ||
		qualifier_name == NULL ||
		metric_units == NULL) {
		FPGA_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	fpga_enum_metric = (struct _fpga_enum_metric *)malloc(sizeof(struct _fpga_enum_metric));
	if (fpga_enum_metric == NULL) {
		FPGA_ERR("Failed to allocate memory");
		return FPGA_NO_MEMORY;
	}

	e = strncpy_s(fpga_enum_metric->group_name, sizeof(fpga_enum_metric->group_name),
		group_name, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;

	e = strncpy_s(fpga_enum_metric->group_sysfs, sizeof(fpga_enum_metric->group_sysfs),
		group_sysfs, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;

	e = strncpy_s(fpga_enum_metric->metric_name, sizeof(fpga_enum_metric->metric_name),
		metric_name, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;

	e = strncpy_s(fpga_enum_metric->metric_sysfs, sizeof(fpga_enum_metric->metric_sysfs),
		metric_sysfs, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;

	e = strncpy_s(fpga_enum_metric->qualifier_name, sizeof(fpga_enum_metric->qualifier_name),
		qualifier_name, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;

	e = strncpy_s(fpga_enum_metric->metric_units, sizeof(fpga_enum_metric->metric_units),
		metric_units, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out_free;


	fpga_enum_metric->metric_type = metric_type;
	fpga_enum_metric->metric_datatype = metric_datatype;
	fpga_enum_metric->hw_type = hw_type;
	fpga_enum_metric->metric_num = metric_num;
	fpga_enum_metric->mmio_offset = mmio_offset;

	fpga_vector_push(vector, fpga_enum_metric);

	return result;

out_free:
	free(fpga_enum_metric);
	return FPGA_INVALID_PARAM;
}

fpga_result get_metric_data_info(const char *group_name,
				const char *metric_name,
				fpga_metric_metadata *metric_data_serach,
				uint64_t size,
				fpga_metric_metadata *metric_data)
{
	fpga_result result       = FPGA_OK;
	uint64_t i               = 0;
	int group_indicator      = 0;
	int metric_indicator     = 0;

	if (group_name == NULL ||
		metric_name == NULL ||
		metric_data_serach == NULL ||
		metric_data == NULL) {
		FPGA_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	for (i = 0; i < size; i++) {

		strcasecmp_s(metric_data_serach[i].group_name, sizeof(metric_data_serach[i].group_name),
			group_name, &group_indicator);

		strcasecmp_s(metric_data_serach[i].metric_name, sizeof(metric_data_serach[i].metric_name),
			metric_name, &metric_indicator);

		if (group_indicator == 0 &&
			metric_indicator == 0) {
			*metric_data = (struct fpga_metric_metadata)metric_data_serach[i];
			return result;
		}

	}

	return FPGA_NOT_SUPPORTED;
}

// enumerates thermal metrics info
fpga_result enum_thermalmgmt_metrics(fpga_metric_vector *vector,
					uint64_t *metric_num,
					const char *sysfspath,
					enum fpga_hw_type	hw_type)
{
	fpga_result result                      = FPGA_OK;
	fpga_metric_metadata metric_data;
	size_t i = 0;
	glob_t pglob;

	memset_s(&metric_data, sizeof(metric_data), 0);

	if (vector == NULL ||
		sysfspath == NULL ||
		metric_num == NULL) {
		FPGA_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	int gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
	if (gres) {
		FPGA_ERR("Failed pattern match %s: %s", sysfspath, strerror(errno));
		//TODO refactor to common function
		switch (gres) {
		case GLOB_NOSPACE:
			result = FPGA_NO_MEMORY;
			break;
		case GLOB_NOMATCH:
			result = FPGA_NOT_FOUND;
			break;
		default:
			result = FPGA_EXCEPTION;
		}

		if (pglob.gl_pathv) {
			globfree(&pglob);
		}
		return result;
	}

	for (i = 0; i < pglob.gl_pathc; i++) {

		char *dir_name = strrchr(pglob.gl_pathv[i], '/');

		if (!dir_name)
			continue;

		if (!strcmp((dir_name + 1), REVISION))
			continue;

		result = get_metric_data_info(THERLGMT, (dir_name + 1), mcp_metric_metadata, MCP_MDATA_SIZE, &metric_data);
		if (result != FPGA_OK) {
			FPGA_MSG("Failed to get metric metadata ");
		}

		result = add_metric_vector(vector, *metric_num, THERLGMT, THERLGMT, sysfspath, (dir_name + 1), pglob.gl_pathv[i], metric_data.metric_units,
								FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_THERMAL, hw_type, 0);
		if (result != FPGA_OK) {
			FPGA_MSG("Failed to add metrics");
			if (pglob.gl_pathv) {
				globfree(&pglob);
			}
			return result;
		}
		*metric_num = *metric_num + 1;
	}

	if (pglob.gl_pathv) {
		globfree(&pglob);
	}
	return result;
}

// enumerates power metrics info
fpga_result enum_powermgmt_metrics(fpga_metric_vector *vector,
					uint64_t *metric_num,
					const char *sysfspath,
					enum fpga_hw_type hw_type)
{
	fpga_result result                  = FPGA_OK;
	size_t i = 0;
	fpga_metric_metadata metric_data;
	glob_t pglob;

	memset_s(&metric_data, sizeof(metric_data), 0);

	if (vector == NULL ||
		sysfspath == NULL ||
		metric_num == NULL) {
		FPGA_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	int gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
	if (gres) {
		FPGA_ERR("Failed pattern match %s: %s", sysfspath, strerror(errno));
		//TODO refactor to common function
		switch (gres) {
		case GLOB_NOSPACE:
			result = FPGA_NO_MEMORY;
			break;
		case GLOB_NOMATCH:
			result = FPGA_NOT_FOUND;
			break;
		default:
			result = FPGA_EXCEPTION;
		}

		if (pglob.gl_pathv) {
			globfree(&pglob);
		}
		return result;
	}

	for (i = 0; i < pglob.gl_pathc; i++) {

		char *dir_name = strrchr(pglob.gl_pathv[i], '/');

		if (!dir_name)
			continue;

		if (!strcmp((dir_name + 1), REVISION))
			continue;

		result = get_metric_data_info(PWRMGMT, (dir_name + 1), mcp_metric_metadata, MCP_MDATA_SIZE, &metric_data);
		if (result != FPGA_OK) {
			FPGA_MSG("Failed to get metric metadata ");
		}

		result = add_metric_vector(vector, *metric_num, PWRMGMT, PWRMGMT, sysfspath, (dir_name + 1), pglob.gl_pathv[i], metric_data.metric_units,
								FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, hw_type, 0);
		if (result != FPGA_OK) {
			FPGA_MSG("Failed to add metrics");
			if (pglob.gl_pathv) {
				globfree(&pglob);
			}
			return result;
		}
		*metric_num = *metric_num + 1;
	}

	if (pglob.gl_pathv) {
		globfree(&pglob);
	}

	return result;
}

// enumerates performance counters metrics info
fpga_result enum_perf_counter_items(fpga_metric_vector *vector,
				uint64_t *metric_num,
				const char *qualifier_name,
				const char *sysfspath,
				const char *sysfs_name,
				enum fpga_metric_type metric_type,
				enum fpga_hw_type  hw_type)
{
	fpga_result result                  = FPGA_OK;
	DIR *dir                            = NULL;
	struct dirent *dirent               = NULL;
	char sysfs_path[SYSFS_PATH_MAX]     = { 0 };
	char metric_sysfs[SYSFS_PATH_MAX]   = { 0 };
	char qname[SYSFS_PATH_MAX]          = { 0 };

	if (vector == NULL ||
		sysfspath == NULL ||
		sysfs_name == NULL ||
		qualifier_name == NULL ||
		metric_num == NULL) {
		FPGA_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}


	snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", sysfspath, sysfs_name);

	dir = opendir(sysfs_path);
	if (NULL == dir) {
		FPGA_MSG("can't find dir %s ", strerror(errno));
		return FPGA_NOT_FOUND;
	}

	while ((dirent = readdir(dir)) != NULL) {
		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;
		if (!strcmp(dirent->d_name, PERF_ENABLE))
			continue;

		if (!strcmp(dirent->d_name, PERF_FREEZE))
			continue;

		if (dirent->d_type == DT_DIR) {

			snprintf_s_ss(qname, sizeof(qname), "%s:%s", qualifier_name, dirent->d_name);

			result = enum_perf_counter_items(vector, metric_num, qname, sysfs_path, dirent->d_name, metric_type, hw_type);
			if (result != FPGA_OK) {
				FPGA_MSG("Failed to add metrics");
			}
			continue;

		}

		snprintf_s_ss(metric_sysfs, sizeof(metric_sysfs), "%s/%s", sysfs_path, dirent->d_name);

		result = add_metric_vector(vector, *metric_num, qualifier_name, "performance", sysfs_path, dirent->d_name,
			metric_sysfs, "", FPGA_METRIC_DATATYPE_INT, metric_type, hw_type, 0);
		if (result != FPGA_OK) {
			FPGA_MSG("Failed to add metrics");
			closedir(dir);
			return result;
		}

		*metric_num = *metric_num + 1;
	}
	closedir(dir);
	return result;

}

// enumerates performance counters metrics info
fpga_result enum_perf_counter_metrics(fpga_metric_vector *vector,
					uint64_t *metric_num,
					const char *sysfspath,
					enum fpga_hw_type  hw_type)
{
	fpga_result result                  = FPGA_OK;
	DIR *dir                            = NULL;
	struct dirent *dirent               = NULL;
	char sysfs_path[SYSFS_PATH_MAX]     = { 0 };
	char qualifier_name[SYSFS_PATH_MAX] = { 0 };
	glob_t pglob;

	if (vector == NULL ||
		sysfspath == NULL ||
		metric_num == NULL) {
		FPGA_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	int gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
	if (gres) {
		FPGA_ERR("Failed pattern match %s: %s", sysfspath, strerror(errno));
		if (pglob.gl_pathv) {
			globfree(&pglob);
		}
		return FPGA_NOT_FOUND;
	}

	snprintf_s_s(sysfs_path, sizeof(sysfs_path), "%s", pglob.gl_pathv[0]);
	globfree(&pglob);

	dir = opendir(sysfs_path);
	if (NULL == dir) {
		FPGA_MSG("can't find dirt %s ", strerror(errno));
		return FPGA_NOT_FOUND;
	}

	while ((dirent = readdir(dir)) != NULL) {

		if (!strcmp(dirent->d_name, "."))
			continue;
		if (!strcmp(dirent->d_name, ".."))
			continue;
		if (!strcmp(dirent->d_name, REVISION))
			continue;


		if (strcmp(dirent->d_name, PERF_CACHE) == 0) {
			snprintf_s_ss(qualifier_name, sizeof(qualifier_name), "%s:%s", PERFORMANCE, PERF_CACHE);
			result = enum_perf_counter_items(vector, metric_num, qualifier_name, sysfs_path, dirent->d_name, FPGA_METRIC_TYPE_PERFORMANCE_CTR, hw_type);
			if (result != FPGA_OK) {
				FPGA_MSG("Failed to add metrics");
			}

		}

		if (strcmp(dirent->d_name, PERF_FABRIC) == 0) {
			snprintf_s_ss(qualifier_name, sizeof(qualifier_name), "%s:%s", PERFORMANCE, PERF_FABRIC);
			result = enum_perf_counter_items(vector, metric_num, qualifier_name, sysfs_path, dirent->d_name, FPGA_METRIC_TYPE_PERFORMANCE_CTR, hw_type);
			if (result != FPGA_OK) {
				FPGA_MSG("Failed to add metrics");
			}

		}

		if (strcmp(dirent->d_name, PERF_IOMMU) == 0) {
			snprintf_s_ss(qualifier_name, sizeof(qualifier_name), "%s:%s", PERFORMANCE, PERF_IOMMU);
			result = enum_perf_counter_items(vector, metric_num, qualifier_name, sysfs_path, dirent->d_name, FPGA_METRIC_TYPE_PERFORMANCE_CTR, hw_type);
			if (result != FPGA_OK) {
				FPGA_MSG("Failed to add metrics");
			}

		}

	}
	closedir(dir);
	return result;
}

fpga_result xfpga_bmcLoadSDRs(struct _fpga_handle *_handle,
		bmc_sdr_handle *records,
		uint32_t *num_sensors)
{
	fpga_result result = FPGA_NOT_FOUND;
	fpga_result(*bmcLoadSDRs)(fpga_token token, bmc_sdr_handle *records,
		uint32_t *num_sensors);
	if (_handle->bmc_handle != NULL) {

		bmcLoadSDRs = dlsym(_handle->bmc_handle, "bmcLoadSDRs");
		if (bmcLoadSDRs)
			result = bmcLoadSDRs(_handle->token, records, num_sensors);
		else
			result = FPGA_EXCEPTION;

	}
	return result;
}

fpga_result xfpga_bmcDestroySDRs(struct _fpga_handle *_handle,
		bmc_sdr_handle *records)
{
	fpga_result result = FPGA_NOT_FOUND;
	fpga_result(*bmcDestroySDRs)(bmc_sdr_handle *records);

	if (_handle->bmc_handle != NULL) {

		bmcDestroySDRs = dlsym(_handle->bmc_handle, "bmcDestroySDRs");
		if (bmcDestroySDRs)
			result = bmcDestroySDRs(records);
		else
			result = FPGA_EXCEPTION;

	}
	return result;
}


fpga_result xfpga_bmcReadSensorValues(struct _fpga_handle *_handle,
		bmc_sdr_handle records,
		bmc_values_handle *values,
		uint32_t *num_values)
{
	fpga_result result = FPGA_NOT_FOUND;
	fpga_result(*bmcReadSensorValues)(bmc_sdr_handle records, bmc_values_handle *values, uint32_t *num_values);

	if (_handle->bmc_handle != NULL) {

		bmcReadSensorValues = dlsym(_handle->bmc_handle, "bmcReadSensorValues");
		if (bmcReadSensorValues)
			result = bmcReadSensorValues(records, values, num_values);
		else
			result = FPGA_EXCEPTION;

	}
	return result;
}


fpga_result xfpga_bmcDestroySensorValues(struct _fpga_handle *_handle,
		bmc_values_handle *values)
{
	fpga_result result = FPGA_NOT_FOUND;
	fpga_result(*bmcDestroySensorValues)(bmc_values_handle *values);

	if (_handle->bmc_handle != NULL) {

		bmcDestroySensorValues = dlsym(_handle->bmc_handle, "bmcDestroySensorValues");
		if (bmcDestroySensorValues)
			result = bmcDestroySensorValues(values);
		else
			result = FPGA_EXCEPTION;

	}
	return result;
}

fpga_result xfpga_bmcGetSensorReading(struct _fpga_handle *_handle,
		bmc_values_handle values,
		uint32_t sensor_number,
		uint32_t *is_valid,
		double *value)
{
	fpga_result result = FPGA_NOT_FOUND;
	fpga_result(*bmcGetSensorReading)(bmc_values_handle values,
		uint32_t sensor_number, uint32_t *is_valid,
		double *value);

	if (_handle->bmc_handle != NULL) {

		bmcGetSensorReading = dlsym(_handle->bmc_handle, "bmcGetSensorReading");
		if (bmcGetSensorReading)
			result = bmcGetSensorReading(values, sensor_number, is_valid, value);
		else
			result = FPGA_EXCEPTION;

	}
	return result;
}

fpga_result xfpga_bmcGetSDRDetails(struct _fpga_handle *_handle,
		bmc_values_handle values,
		uint32_t sensor_number,
		sdr_details *details)
{
	fpga_result result = FPGA_NOT_FOUND;
	fpga_result(*bmcGetSDRDetails)(bmc_values_handle values, uint32_t sensor_number,
		sdr_details *details);

	if (_handle->bmc_handle != NULL) {

		bmcGetSDRDetails = dlsym(_handle->bmc_handle, "bmcGetSDRDetails");
		if (bmcGetSDRDetails)
			result = bmcGetSDRDetails(values, sensor_number, details);
		else
			result = FPGA_EXCEPTION;

	}
	return result;
}


// enumerates bmc power & theraml metrics info
fpga_result  enum_bmc_metrics_info(struct _fpga_handle *_handle,
				fpga_metric_vector *vector,
				uint64_t *metric_num,
				enum fpga_hw_type  hw_type)
{
	fpga_result result                      = FPGA_OK;
	uint32_t x                              = 0;
	uint32_t num_sensors                    = 0;
	uint32_t num_values                     = 0;
	enum fpga_metric_type metric_type       = FPGA_METRIC_TYPE_POWER;
	char group_name[SYSFS_PATH_MAX]         = { 0 };
	char qualifier_name[SYSFS_PATH_MAX]     = { 0 };
	char units[SYSFS_PATH_MAX]              = { 0 };
	sdr_details details;
	bmc_sdr_handle records;
	bmc_values_handle values;

	if (vector == NULL ||
		metric_num == NULL) {
		FPGA_ERR("Invalid input");
		return result;
	}
	result = xfpga_bmcLoadSDRs(_handle, &records, &num_sensors);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to load BMC SDR.");
		return result;
	}

	result = xfpga_bmcReadSensorValues(_handle, records, &values, &num_values);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to read BMC sensor values.");
		return result;
	}

	for (x = 0; x < num_sensors; x++) {
		result = xfpga_bmcGetSDRDetails(_handle, values, x, &details);


		if (details.type == BMC_THERMAL) {

			metric_type = FPGA_METRIC_TYPE_THERMAL;
			snprintf_s_s(group_name, sizeof(group_name), "%s", THERLGMT);
			snprintf_s_s(units, sizeof(units), "%s", TEMP);
			snprintf_s_ss(qualifier_name, sizeof(qualifier_name), "%s:%s", THERLGMT, details.name);

		} else if (details.type == BMC_POWER) {

			metric_type = FPGA_METRIC_TYPE_POWER;
			snprintf_s_s(group_name, sizeof(group_name), "%s", PWRMGMT);
			snprintf_s_ss(qualifier_name, sizeof(qualifier_name), "%s:%s", PWRMGMT, details.name);
			snprintf(units, sizeof(units), "%ls", details.units);
		} else {
				continue;
		}

		result = add_metric_vector(vector, *metric_num, qualifier_name, group_name, "", details.name, "", units, FPGA_METRIC_DATATYPE_DOUBLE, metric_type, hw_type, 0);
		if (result != FPGA_OK) {
			FPGA_MSG("Failed to add metrics");
			return result;
		}

		*metric_num = *metric_num + 1;
	}

	result = xfpga_bmcDestroySensorValues(_handle, &values);
	if (result != FPGA_OK) {
		FPGA_MSG("Failed to Destroy Sensor value.");
	}

	result = xfpga_bmcDestroySDRs(_handle, &records);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to Destroy SDR.");
		return result;
	}

	return result;
}

// frees metrics info vector
fpga_result free_fpga_enum_metrics_vector(struct _fpga_handle *_handle)
{
	fpga_result result        = FPGA_OK;
	uint64_t i                = 0;
	uint64_t num_enun_metrics = 0;

	if (_handle == NULL) {
		FPGA_ERR("Invalid handle ");
		return FPGA_INVALID_PARAM;
	}

	if (_handle->magic != FPGA_HANDLE_MAGIC) {
		FPGA_MSG("Invalid handle");
		return FPGA_INVALID_PARAM;
	}

	result = fpga_vector_total(&(_handle->fpga_enum_metric_vector), &num_enun_metrics);
	if (result != FPGA_OK) {
		FPGA_MSG("Failed to get metric total");
		return FPGA_INVALID_PARAM;
	}

	for (i = 0; i < num_enun_metrics; i++) {
		fpga_vector_delete(&(_handle->fpga_enum_metric_vector), i);
	}

	fpga_vector_free(&(_handle->fpga_enum_metric_vector));

	if (_handle->bmc_handle) {
		dlclose(_handle->bmc_handle);
		_handle->bmc_handle = NULL;
	}

	clear_cached_values(_handle);
	_handle->metric_enum_status = false;

	return result;
}

// retrives fpga object type
fpga_result get_fpga_object_type(fpga_handle handle,
		fpga_objtype *objtype)
{
	fpga_result result     = FPGA_OK;
	fpga_result resval     = FPGA_OK;
	fpga_properties prop;

	result = xfpga_fpgaGetPropertiesFromHandle(handle, &prop);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get properties");
		return result;
	}

	result = fpgaPropertiesGetObjectType(prop, objtype);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to object type.");
	}

	resval = (result != FPGA_OK) ? result : resval;
	result = fpgaDestroyProperties(&prop);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to destroy properties");
	}

	resval = (result != FPGA_OK) ? result : resval;

	return resval;
}

// enumerates FME & AFU metrics info
fpga_result enum_fpga_metrics(fpga_handle handle)
{
	fpga_result result              = FPGA_OK;
	struct _fpga_token *_token      = NULL;
	enum fpga_hw_type hw_type	= FPGA_HW_UNKNOWN;
	uint64_t mmio_offset            = 0;
	uint64_t metric_num             = 0;
	char metrics_path[SYSFS_PATH_MAX] = { 0 };

	fpga_objtype objtype;

	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;

	if (_handle == NULL) {
		FPGA_ERR("Invalid handle ");
		return FPGA_INVALID_PARAM;
	}

	if (_handle->metric_enum_status)
		return FPGA_OK;

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		FPGA_ERR("Invalid token within handle");
		return FPGA_INVALID_PARAM;
	}

	result = get_fpga_object_type(handle, &objtype);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to init vector");
		return result;
	}

	// Init vector
	result = fpga_vector_init(&(_handle->fpga_enum_metric_vector));
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to init vector");
		return result;
	}

	if (objtype == FPGA_ACCELERATOR) {
		// enum AFU
		result = discover_afu_metrics_feature(handle, &mmio_offset);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed to discover AFU Metrics BBB");
			return result;
		}


		result = enum_afu_metrics(handle,
			&(_handle->fpga_enum_metric_vector),
			&metric_num,
			mmio_offset);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to enum AFU metrics BBB");
				return result;
			}


	} else	if (objtype == FPGA_DEVICE) {
		// enum FME

		// get fpga hw type.
		result = get_fpga_hw_type(_handle, &hw_type);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed to discover hardware type.");
			return result;
		}


		switch (hw_type) {
			// MCP
		case FPGA_HW_MCP: {

			memset_s(metrics_path, SYSFS_PATH_MAX, 0);

			if (sysfs_get_fme_pwr_path(_token, metrics_path) == FPGA_OK) {
				result = enum_powermgmt_metrics(&(_handle->fpga_enum_metric_vector), &metric_num, metrics_path, FPGA_HW_MCP);
				if (result != FPGA_OK) {
					FPGA_ERR("Failed to Enum Power metrics.");
				}
			}

			memset_s(metrics_path, SYSFS_PATH_MAX, 0);
			if (sysfs_get_fme_temp_path(_token, metrics_path) == FPGA_OK) {
				result = enum_thermalmgmt_metrics(&(_handle->fpga_enum_metric_vector), &metric_num, metrics_path, FPGA_HW_MCP);
				if (result != FPGA_OK) {
					FPGA_ERR("Failed to Enum Thermal metrics.");
				}
			}

			memset_s(metrics_path, SYSFS_PATH_MAX, 0);
			if (sysfs_get_fme_perf_path(_token, metrics_path) == FPGA_OK) {
				result = enum_perf_counter_metrics(&(_handle->fpga_enum_metric_vector), &metric_num, metrics_path, FPGA_HW_MCP);
				if (result != FPGA_OK) {
					FPGA_ERR("Failed to Enum Performance metrics.");
				}
			}

		}
		break;

		 // DCP RC
		case FPGA_HW_DCP_RC: {

			memset_s(metrics_path, SYSFS_PATH_MAX, 0);
			if (sysfs_get_fme_perf_path(_token, metrics_path) == FPGA_OK) {

				result = enum_perf_counter_metrics(&(_handle->fpga_enum_metric_vector), &metric_num, metrics_path, FPGA_HW_DCP_RC);
				if (result != FPGA_OK) {
					FPGA_ERR("Failed to Enum Performance metrics.");
				}
			}

			memset_s(metrics_path, SYSFS_PATH_MAX, 0);
			if (sysfs_get_bmc_path(_token, metrics_path) == FPGA_OK) {

				if (_handle->bmc_handle == NULL)
					_handle->bmc_handle = dlopen(BMC_LIB, RTLD_LAZY | RTLD_LOCAL);

				if (_handle->bmc_handle) {
					result = enum_bmc_metrics_info(_handle, &(_handle->fpga_enum_metric_vector), &metric_num, FPGA_HW_DCP_RC);
					if (result != FPGA_OK) {
						FPGA_ERR("Failed to enumerate BMC metrics.");
					}

				}
			}

		}
		break;

		// DCP VC DC
		case FPGA_HW_DCP_DC:
		case FPGA_HW_DCP_VC: {

			memset_s(metrics_path, SYSFS_PATH_MAX, 0);
			if (sysfs_get_max10_path(_token, metrics_path) == FPGA_OK) {

				// Max10 Power & Thermal
				result = enum_max10_metrics_info(_handle,
					&(_handle->fpga_enum_metric_vector),
					&metric_num,
					hw_type);
				if (result != FPGA_OK) {
					FPGA_ERR("Failed to Enum Power and Thermal metrics.");
				}
			}

			memset_s(metrics_path, SYSFS_PATH_MAX, 0);
			if (sysfs_get_fme_perf_path(_token, metrics_path) == FPGA_OK) {

				// Perf Counters
				result = enum_perf_counter_metrics(&(_handle->fpga_enum_metric_vector), &metric_num, _token->sysfspath, hw_type);
				if (result != FPGA_OK) {
					FPGA_ERR("Failed to Enum Performance metrics.");
				}
			}
		}
		break;

		default:
			FPGA_MSG("Unknown hardware type.");
			result = FPGA_EXCEPTION;
		}

	} // if Object type

	if (result != FPGA_OK)
		free_fpga_enum_metrics_vector(_handle);

	_handle->metric_enum_status = true;

	return result;
}


fpga_result add_metric_info(struct _fpga_enum_metric *_enum_metrics,
			struct fpga_metric_info *fpga_metric_info)
{

	fpga_result result   = FPGA_OK;
	errno_t e            = 0;

	if (_enum_metrics == NULL ||
		fpga_metric_info == NULL) {

		FPGA_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	e = strncpy_s(fpga_metric_info->group_name, sizeof(fpga_metric_info->group_name),
					_enum_metrics->group_name, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out;

	e = strncpy_s(fpga_metric_info->metric_name, sizeof(fpga_metric_info->metric_name),
					_enum_metrics->metric_name, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out;

	e = strncpy_s(fpga_metric_info->qualifier_name, sizeof(fpga_metric_info->qualifier_name),
					_enum_metrics->qualifier_name, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out;

	e = strncpy_s(fpga_metric_info->metric_units, sizeof(fpga_metric_info->metric_units),
				_enum_metrics->metric_units, SYSFS_PATH_MAX);
	if (EOK != e)
		goto out;


	fpga_metric_info->metric_num = _enum_metrics->metric_num;
	fpga_metric_info->metric_type = _enum_metrics->metric_type;
	fpga_metric_info->metric_datatype = _enum_metrics->metric_datatype;

	return result;
out:
	return FPGA_INVALID_PARAM;
}




// Reads bmc metric value
fpga_result get_bmc_metrics_values(fpga_handle handle,
				struct _fpga_enum_metric *_fpga_enum_metric,
				struct fpga_metric *fpga_metric)
{
	fpga_result result                  = FPGA_OK;
	uint32_t num_sensors                = 0;
	uint32_t num_values                 = 0;
	uint32_t x                          = 0;
	uint32_t is_valid                   = 0;
	double tmp                          = 0;
	int metric_indicator                = 0;
	bmc_sdr_handle records;
	bmc_values_handle values;
	sdr_details details;

	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;

	if (_handle->_bmc_metric_cache_value) {

		for (x = 0; x < _handle->num_bmc_metric; x++) {

			strcasecmp_s(_handle->_bmc_metric_cache_value[x].metric_name, sizeof(_handle->_bmc_metric_cache_value[x].metric_name),
				_fpga_enum_metric->metric_name, &metric_indicator);

			if (metric_indicator == 0) {
				fpga_metric->value.dvalue = _handle->_bmc_metric_cache_value[x].fpga_metric.value.dvalue;
				return result;
			}
		}
		return FPGA_NOT_FOUND;
	}

	result = xfpga_bmcLoadSDRs(_handle, &records, &num_sensors);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to load BMC SDR.");
		return result;
	}

	if (_handle->_bmc_metric_cache_value == NULL) {
		_handle->_bmc_metric_cache_value = calloc(sizeof(struct _fpga_bmc_metric), num_sensors);
		if (_handle->_bmc_metric_cache_value == NULL) {
			FPGA_ERR("Failed to allocate memory");
			result = FPGA_NO_MEMORY;
			goto out_destroy;
		}
		_handle->num_bmc_metric = num_sensors;
	}

	result = xfpga_bmcReadSensorValues(_handle, records, &values, &num_values);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to read BMC sensor values.");
		goto out_destroy;
	}

	for (x = 0; x < num_sensors; x++) {

		result = xfpga_bmcGetSDRDetails(_handle, values, x, &details);
		if (result != FPGA_OK) {
			FPGA_MSG("Failed to get SDR details.");
		}

		result = xfpga_bmcGetSensorReading(_handle, values, x, &is_valid, &tmp);
		if (result != FPGA_OK) {
			FPGA_MSG("Failed to read sensor readings.");
			continue;
		}

		if (!is_valid) {
			continue;
		}

		snprintf_s_s(_handle->_bmc_metric_cache_value[x].metric_name, sizeof(_handle->_bmc_metric_cache_value[x].metric_name), "%s", details.name);
		_handle->_bmc_metric_cache_value[x].fpga_metric.value.dvalue = tmp;

		strcasecmp_s(details.name, strnlen_s(details.name, FPGA_METRIC_STR_SIZE), _fpga_enum_metric->metric_name, &metric_indicator);
		if (metric_indicator == 0) {
			fpga_metric->value.dvalue = tmp;
		}

	}


	result = xfpga_bmcDestroySensorValues(_handle, &values);
	if (result != FPGA_OK) {
		FPGA_MSG("Failed to Destroy Sensor value.");
	}

out_destroy:
	result = xfpga_bmcDestroySDRs(_handle, &records);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to Destroy SDR.");
		return result;
	}

	return result;
}

// Reads mcp power & thermal metric value
fpga_result get_pwr_thermal_max10_value(const char *sysfs_path,
	double *dvalue)
{
	fpga_result result = FPGA_OK;

	uint64_t value;

	if (sysfs_path == NULL ||
		dvalue == NULL) {
		FPGA_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	result = sysfs_read_u64(sysfs_path, &value);
	if (result != FPGA_OK) {
		FPGA_MSG("Failed to read Metrics values");
		return result;
	}

	*dvalue = ((double)value / MILLI);

	return result;
}

// Reads mcp power & thermal metric value
fpga_result get_pwr_thermal_value(const char *sysfs_path,
				uint64_t *value)
{
	fpga_result result       = FPGA_OK;
	char *ptr                = NULL;

	if (sysfs_path == NULL ||
		value == NULL) {
		FPGA_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	result = sysfs_read_u64(sysfs_path, value);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to read Metrics values");
		return result;
	}

	ptr = strstr(sysfs_path, FPGA_LIMIT);
	if (ptr)
		*value = *value / 8;

	ptr = NULL;
	ptr = strstr(sysfs_path, XEON_LIMIT);
	if (ptr)
		*value = *value / 8;

	return result;
}

// Reads mcp power & thermal metric value
fpga_result get_performance_counter_value(const char *group_sysfs,
				const char *metric_sysfs,
				uint64_t *value)
{
	fpga_result result                  = FPGA_OK;
	char sysfs_path[SYSFS_PATH_MAX]     = { 0 };
	uint64_t val                        = 0;

	if (group_sysfs == NULL ||
		metric_sysfs == NULL ||
		value == NULL) {
		FPGA_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", group_sysfs, PERF_ENABLE);

	result = metric_sysfs_path_is_file(sysfs_path);
	if (result == FPGA_OK) {
		result = sysfs_read_u64(sysfs_path, &val);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed to read perf fabric enable");
		}

		if (val == 0x0) {
			// Writer Fabric Enable
			result = sysfs_write_u64_decimal(sysfs_path, 1);;
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to read perf fabric enable");
			}

		}
	}

	snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", group_sysfs, PERF_FREEZE);

	result = metric_sysfs_path_is_file(sysfs_path);
	if (result == FPGA_OK) {

		result = sysfs_read_u64(sysfs_path, &val);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed to read perf fabric freeze");
		}

		if (val != 0x1) {
			// Write Fabric Freeze
			result = sysfs_write_u64(sysfs_path, 1);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to write perf fabric freeze");
			}

		}
	}

	*value = 0;
	result = sysfs_read_u64(metric_sysfs, value);
	if (result != FPGA_OK) {
		FPGA_ERR("--Failed to read Metrics values");
		return result;
	}

	snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s", group_sysfs, PERF_FREEZE);
	result = metric_sysfs_path_is_file(sysfs_path);
	if (result == FPGA_OK) {

		result = sysfs_read_u64(sysfs_path, &val);
		if (result != FPGA_OK) {
			FPGA_ERR("Failed to read perf fabric freeze");

		}

		if (val == 0x1) {
			// Write Fabric Freeze
			result = sysfs_write_u64(sysfs_path, 0);
			if (result != FPGA_OK) {
				FPGA_ERR("Failed to write perf fabric freeze");
			}

		}
	}

	result = FPGA_OK;
	return result;
}

// Reads fme metric value
fpga_result  get_fme_metric_value(fpga_handle handle,
					fpga_metric_vector *enum_vector,
					uint64_t metric_num,
					struct fpga_metric *fpga_metric)
{
	fpga_result result                          = FPGA_OK;
	uint64_t index                              = 0;
	struct _fpga_enum_metric *_fpga_enum_metric = NULL;
	uint64_t num_enun_metrics                  = 0;
	metric_value value = {0};

	if (enum_vector == NULL ||
		fpga_metric == NULL) {
		FPGA_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	result = fpga_vector_total(enum_vector, &num_enun_metrics);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get metric total");
		return FPGA_NOT_FOUND;
	}

	fpga_metric->isvalid = false;
	result = FPGA_NOT_FOUND;
	for (index = 0; index < num_enun_metrics ; index++) {

		_fpga_enum_metric = (struct _fpga_enum_metric *)	fpga_vector_get(enum_vector, index);

		if (metric_num == _fpga_enum_metric->metric_num) {

			// Found Metic
			memset_s(&value, sizeof(value), 0);

			// DCP Power & Thermal
			if ((_fpga_enum_metric->hw_type == FPGA_HW_DCP_RC) &&
				((_fpga_enum_metric->metric_type == FPGA_METRIC_TYPE_POWER) ||
				(_fpga_enum_metric->metric_type == FPGA_METRIC_TYPE_THERMAL))) {


				result  = get_bmc_metrics_values(handle, _fpga_enum_metric, fpga_metric);
				if (result != FPGA_OK) {
					FPGA_MSG("Failed to get BMC metric value");
				} else {
					fpga_metric->isvalid = true;
				}
				fpga_metric->metric_num = metric_num;

			 }

			if ((_fpga_enum_metric->hw_type == FPGA_HW_MCP) &&
				((_fpga_enum_metric->metric_type == FPGA_METRIC_TYPE_POWER) ||
				(_fpga_enum_metric->metric_type == FPGA_METRIC_TYPE_THERMAL))) {

				result = get_pwr_thermal_value(_fpga_enum_metric->metric_sysfs, &value.ivalue);
				if (result != FPGA_OK) {
					FPGA_MSG("Failed to get BMC metric value");
				} else {
					fpga_metric->isvalid = true;
				}
				fpga_metric->value = value;
				fpga_metric->metric_num = metric_num;

			}

			// Read power theraml values from Max10
			if (((_fpga_enum_metric->hw_type == FPGA_HW_DCP_VC) ||
				(_fpga_enum_metric->hw_type == FPGA_HW_DCP_VC)) &&
				((_fpga_enum_metric->metric_type == FPGA_METRIC_TYPE_POWER) ||
				(_fpga_enum_metric->metric_type == FPGA_METRIC_TYPE_THERMAL))) {

				result = get_pwr_thermal_max10_value(_fpga_enum_metric->metric_sysfs, &value.dvalue);
				if (result != FPGA_OK) {
					FPGA_MSG("Failed to get Max10 metric value");
				} else {
					fpga_metric->isvalid = true;
				}
				fpga_metric->value = value;
				fpga_metric->metric_num = metric_num;

			}


			if (_fpga_enum_metric->metric_type == FPGA_METRIC_TYPE_PERFORMANCE_CTR) {


				result = get_performance_counter_value(_fpga_enum_metric->group_sysfs, _fpga_enum_metric->metric_sysfs, &value.ivalue);
				if (result != FPGA_OK) {
					FPGA_MSG("Failed to get perf metric value");
				} else {
					fpga_metric->isvalid = true;
				}
				fpga_metric->value = value;
				fpga_metric->metric_num = metric_num;

			}

			break;
		}
	}

	return result;
}


// parses metric name strings
fpga_result  parse_metric_num_name(const char *search_string,
				fpga_metric_vector *fpga_enum_metrics_vector,
				uint64_t *metric_num)
{
	fpga_result result                          = FPGA_OK;
	size_t init_size                            = 0;
	char *str                                   = NULL;
	char *str_last                              = NULL;
	uint64_t i                                  = 0;
	struct _fpga_enum_metric *fpga_enum_metric  = NULL;
	char qualifier_name[SYSFS_PATH_MAX]         = { 0 };
	char metrics_name[SYSFS_PATH_MAX]           = { 0 };
	int qualifier_indicator                     = 0;
	int metric_indicator                        = 0;
	errno_t  err                                = 0;
	uint64_t num_enun_metrics                   = 0;

	if (search_string == NULL ||
		fpga_enum_metrics_vector == NULL ||
		metric_num == NULL) {
		FPGA_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	err = strlastchar_s((char *)search_string,
			strnlen_s(search_string, FPGA_METRIC_STR_SIZE), ':', &str);
	if (err != 0 &&
		str == NULL) {
		FPGA_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	// Metric Name
	err = strncpy_s(metrics_name, strnlen_s(search_string, FPGA_METRIC_STR_SIZE) + 1,
			str + 1, strnlen_s(str + 1, FPGA_METRIC_STR_SIZE));
	if (err != 0) {
		FPGA_ERR("Failed to copy metric name");
		return FPGA_INVALID_PARAM;
	}

	// qualifier_name
	err = strlastchar_s((char *)search_string, strnlen_s(search_string, FPGA_METRIC_STR_SIZE), ':', &str_last);
	if (err != 0) {
		FPGA_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	init_size = strnlen_s(search_string, FPGA_METRIC_STR_SIZE) - strnlen_s(str_last, FPGA_METRIC_STR_SIZE) + 1;


	err = strncpy_s(qualifier_name, init_size + 1, search_string, init_size);
	if (err != 0) {
		FPGA_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}
	if (init_size < FPGA_METRIC_STR_SIZE)
		qualifier_name[init_size - 1] = '\0';

	result = fpga_vector_total(fpga_enum_metrics_vector, &num_enun_metrics);
	if (result != FPGA_OK) {
		FPGA_ERR("Failed to get metric total");
		return FPGA_NOT_FOUND;
	}


	for (i = 0; i < num_enun_metrics; i++) {
		fpga_enum_metric = (struct _fpga_enum_metric *) fpga_vector_get(fpga_enum_metrics_vector, i);

		strcasecmp_s(fpga_enum_metric->qualifier_name, sizeof(fpga_enum_metric->qualifier_name),
			qualifier_name, &qualifier_indicator);

		strcasecmp_s(fpga_enum_metric->metric_name, sizeof(fpga_enum_metric->metric_name),
			metrics_name, &metric_indicator);

		if (qualifier_indicator == 0 &&
			metric_indicator == 0) {

			*metric_num = fpga_enum_metric->metric_num;
			return result;
		}

	} // end of for loop

	return FPGA_NOT_FOUND;
}

// clears BMC values
fpga_result  clear_cached_values(fpga_handle handle)
{
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	fpga_result result           = FPGA_OK;

	if (_handle->_bmc_metric_cache_value) {
		free(_handle->_bmc_metric_cache_value);
		_handle->_bmc_metric_cache_value = NULL;
	}

	_handle->num_bmc_metric = 0;
	return result;
}
