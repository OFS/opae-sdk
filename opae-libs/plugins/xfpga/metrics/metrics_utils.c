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

/**
* \file metrics_utils.c
* \brief fpga metrics utils functions
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <string.h>
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
#include "metrics/metrics_metadata.h"
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
	size_t len;

	if (vector == NULL ||
		group_name == NULL ||
		group_sysfs == NULL ||
		metric_name == NULL ||
		metric_sysfs == NULL ||
		qualifier_name == NULL ||
		metric_units == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	fpga_enum_metric = (struct _fpga_enum_metric *)malloc(sizeof(struct _fpga_enum_metric));
	if (fpga_enum_metric == NULL) {
		OPAE_ERR("Failed to allocate memory");
		return FPGA_NO_MEMORY;
	}

	len = strnlen(group_name, SYSFS_PATH_MAX - 1);
	memcpy(fpga_enum_metric->group_name, group_name, len);
	fpga_enum_metric->group_name[len] = '\0';

	len = strnlen(group_sysfs, SYSFS_PATH_MAX - 1);
	memcpy(fpga_enum_metric->group_sysfs, group_sysfs, len);
	fpga_enum_metric->group_sysfs[len] = '\0';

	len = strnlen(metric_name, SYSFS_PATH_MAX - 1);
	memcpy(fpga_enum_metric->metric_name, metric_name, len);
	fpga_enum_metric->metric_name[len] = '\0';

	len = strnlen(metric_sysfs, SYSFS_PATH_MAX - 1);
	memcpy(fpga_enum_metric->metric_sysfs, metric_sysfs, len);
	fpga_enum_metric->metric_sysfs[len] = '\0';

	len = strnlen(qualifier_name, SYSFS_PATH_MAX - 1);
	memcpy(fpga_enum_metric->qualifier_name, qualifier_name, len);
	fpga_enum_metric->qualifier_name[len] = '\0';

	len = strnlen(metric_units, SYSFS_PATH_MAX - 1);
	memcpy(fpga_enum_metric->metric_units, metric_units, len);
	fpga_enum_metric->metric_units[len] = '\0';

	fpga_enum_metric->metric_type = metric_type;
	fpga_enum_metric->metric_datatype = metric_datatype;
	fpga_enum_metric->hw_type = hw_type;
	fpga_enum_metric->metric_num = metric_num;
	fpga_enum_metric->mmio_offset = mmio_offset;

	fpga_vector_push(vector, fpga_enum_metric);

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
	char group_name[SYSFS_PATH_MAX]         = { 0, };
	char qualifier_name[SYSFS_PATH_MAX]     = { 0, };
	char units[SYSFS_PATH_MAX]              = { 0, };
	sdr_details details;
	bmc_sdr_handle records;
	bmc_values_handle values;
	size_t len;

	if (vector == NULL ||
		metric_num == NULL) {
		OPAE_ERR("Invalid input");
		return result;
	}
	result = xfpga_bmcLoadSDRs(_handle, &records, &num_sensors);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to load BMC SDR.");
		return result;
	}

	result = xfpga_bmcReadSensorValues(_handle, records, &values, &num_values);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to read BMC sensor values.");
		return result;
	}

	for (x = 0; x < num_sensors; x++) {
		result = xfpga_bmcGetSDRDetails(_handle, values, x, &details);
		if (result != FPGA_OK) {
			OPAE_ERR("Failed to get sensor details.");
			return result;
		}

		if (details.sensor_type == BMC_THERMAL) {

			metric_type = FPGA_METRIC_TYPE_THERMAL;

			len = strnlen(THERLGMT, sizeof(group_name) - 1);
			memcpy(group_name, THERLGMT, len);
			group_name[len] = '\0';

			len = strnlen(TEMP, sizeof(units) - 1);
			memcpy(units, TEMP, len);
			units[len] = '\0';

			snprintf(qualifier_name, sizeof(qualifier_name),
				 "%s:%s", THERLGMT, details.name);

		} else if (details.sensor_type == BMC_POWER) {

			metric_type = FPGA_METRIC_TYPE_POWER;

			len = strnlen(PWRMGMT, sizeof(group_name) - 1);
			memcpy(group_name, PWRMGMT, len);
			group_name[len] = '\0';

			snprintf(qualifier_name, sizeof(qualifier_name),
				 "%s:%s", PWRMGMT, details.name);

			snprintf(units, sizeof(units), "%ls", details.units);
		} else {
				continue;
		}

		result = add_metric_vector(vector, *metric_num,
				qualifier_name, group_name, "",
				details.name, "", units, FPGA_METRIC_DATATYPE_DOUBLE,
				metric_type, hw_type, 0);
		if (result != FPGA_OK) {
			OPAE_MSG("Failed to add metrics");
			return result;
		}

		*metric_num = *metric_num + 1;
	}

	result = xfpga_bmcDestroySensorValues(_handle, &values);
	if (result != FPGA_OK) {
		OPAE_MSG("Failed to Destroy Sensor value.");
	}

	result = xfpga_bmcDestroySDRs(_handle, &records);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to Destroy SDR.");
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
		OPAE_ERR("Invalid handle ");
		return FPGA_INVALID_PARAM;
	}

	if (_handle->magic != FPGA_HANDLE_MAGIC) {
		OPAE_MSG("Invalid handle");
		return FPGA_INVALID_PARAM;
	}

	result = fpga_vector_total(&(_handle->fpga_enum_metric_vector), &num_enun_metrics);
	if (result != FPGA_OK) {
		OPAE_MSG("Failed to get metric total");
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
		OPAE_ERR("Failed to get properties");
		return result;
	}

	result = fpgaPropertiesGetObjectType(prop, objtype);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to object type.");
	}

	resval = (result != FPGA_OK) ? result : resval;
	result = fpgaDestroyProperties(&prop);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to destroy properties");
	}

	resval = (result != FPGA_OK) ? result : resval;

	return resval;
}

void *metrics_load_bmc_lib(void)
{
	char plugin_path[PATH_MAX] = { 0, };
	const char *search_paths[] = { OPAE_MODULE_SEARCH_PATHS };
	unsigned i;
	void *dl_handle;

	for (i = 0 ;
		i < sizeof(search_paths) / sizeof(search_paths[0]) ;
		++i) {

		snprintf(plugin_path, sizeof(plugin_path),
			 "%s%s", search_paths[i], BMC_LIB);

		dl_handle = dlopen(plugin_path, RTLD_LAZY | RTLD_LOCAL);
		if (dl_handle)
			return dl_handle;
	}

	return NULL;
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
		OPAE_ERR("Invalid handle ");
		return FPGA_INVALID_PARAM;
	}

	if (_handle->metric_enum_status)
		return FPGA_OK;

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		OPAE_ERR("Invalid token within handle");
		return FPGA_INVALID_PARAM;
	}

	result = get_fpga_object_type(handle, &objtype);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to init vector");
		return result;
	}

	// Init vector
	result = fpga_vector_init(&(_handle->fpga_enum_metric_vector));
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to init vector");
		return result;
	}

	if (objtype == FPGA_ACCELERATOR) {
		// enum AFU
		result = discover_afu_metrics_feature(handle, &mmio_offset);
		if (result != FPGA_OK) {
			OPAE_ERR("Failed to discover AFU Metrics BBB");
			return result;
		}


		result = enum_afu_metrics(handle,
			&(_handle->fpga_enum_metric_vector),
			&metric_num,
			mmio_offset);
			if (result != FPGA_OK) {
				OPAE_ERR("Failed to enum AFU metrics BBB");
				return result;
			}


	} else	if (objtype == FPGA_DEVICE) {
		// enum FME

		// get fpga hw type.
		result = get_fpga_hw_type(_handle, &hw_type);
		if (result != FPGA_OK) {
			OPAE_ERR("Failed to discover hardware type.");
			return result;
		}


		switch (hw_type) {
			// MCP
		case FPGA_HW_MCP: {
			OPAE_MSG("Intel MCP  HW metrics");
			result = FPGA_NOT_FOUND;
		}
		break;

		 // DCP RC
		case FPGA_HW_DCP_RC: {


			memset(metrics_path, 0, SYSFS_PATH_MAX);
			if (sysfs_get_bmc_path(_token, metrics_path) == FPGA_OK) {

				if (_handle->bmc_handle == NULL)
					_handle->bmc_handle = metrics_load_bmc_lib();

				if (_handle->bmc_handle) {
					result = enum_bmc_metrics_info(_handle, &(_handle->fpga_enum_metric_vector), &metric_num, FPGA_HW_DCP_RC);
					if (result != FPGA_OK) {
						OPAE_ERR("Failed to enumerate BMC metrics.");
					}

				}
			}

		}
		break;

		// DCP VC DC
		case FPGA_HW_DCP_N3000:
		case FPGA_HW_DCP_D5005:
		case FPGA_HW_DCP_N5010: {

			memset(metrics_path, 0, SYSFS_PATH_MAX);
			if (sysfs_get_max10_path(_token, metrics_path) == FPGA_OK) {
					// Max10 Power & Thermal
					result = dfl_enum_max10_metrics_info(_handle,
						&(_handle->fpga_enum_metric_vector),
						&metric_num,
						hw_type);
						if (result != FPGA_OK) {
							OPAE_ERR("Failed to Enum Power and Thermal metrics.");
						}
			}

		}
		break;

		default:
			OPAE_MSG("Unknown hardware type.");
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
	fpga_result result = FPGA_OK;
	size_t len;

	if (_enum_metrics == NULL ||
		fpga_metric_info == NULL) {

		OPAE_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	len = strnlen(_enum_metrics->group_name, SYSFS_PATH_MAX - 1);
	memcpy(fpga_metric_info->group_name, _enum_metrics->group_name, len);
	fpga_metric_info->group_name[len] = '\0';

	len = strnlen(_enum_metrics->metric_name, SYSFS_PATH_MAX - 1);
	memcpy(fpga_metric_info->metric_name, _enum_metrics->metric_name, len);
	fpga_metric_info->metric_name[len] = '\0';

	len = strnlen(_enum_metrics->qualifier_name, SYSFS_PATH_MAX - 1);
	memcpy(fpga_metric_info->qualifier_name, _enum_metrics->qualifier_name, len);
	fpga_metric_info->qualifier_name[len] = '\0';

	len = strnlen(_enum_metrics->metric_units, SYSFS_PATH_MAX - 1);
	memcpy(fpga_metric_info->metric_units, _enum_metrics->metric_units, len);
	fpga_metric_info->metric_units[len] = '\0';

	fpga_metric_info->metric_num = _enum_metrics->metric_num;
	fpga_metric_info->metric_type = _enum_metrics->metric_type;
	fpga_metric_info->metric_datatype = _enum_metrics->metric_datatype;

	return result;
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
	size_t len;

	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;

	if (_handle->_bmc_metric_cache_value) {

		for (x = 0; x < _handle->num_bmc_metric; x++) {

			metric_indicator = strcasecmp(_handle->_bmc_metric_cache_value[x].metric_name,
				_fpga_enum_metric->metric_name);

			if (metric_indicator == 0) {
				fpga_metric->value.dvalue = _handle->_bmc_metric_cache_value[x].fpga_metric.value.dvalue;
				return result;
			}
		}
		return FPGA_NOT_FOUND;
	}

	result = xfpga_bmcLoadSDRs(_handle, &records, &num_sensors);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to load BMC SDR.");
		return result;
	}

	if (_handle->_bmc_metric_cache_value == NULL) {
		_handle->_bmc_metric_cache_value = calloc(sizeof(struct _fpga_bmc_metric), num_sensors);
		if (_handle->_bmc_metric_cache_value == NULL) {
			OPAE_ERR("Failed to allocate memory");
			result = FPGA_NO_MEMORY;
			goto out_destroy;
		}
		_handle->num_bmc_metric = num_sensors;
	}

	result = xfpga_bmcReadSensorValues(_handle, records, &values, &num_values);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to read BMC sensor values.");
		goto out_destroy;
	}

	for (x = 0; x < num_sensors; x++) {

		result = xfpga_bmcGetSDRDetails(_handle, values, x, &details);
		if (result != FPGA_OK) {
			OPAE_MSG("Failed to get SDR details.");
		}

		result = xfpga_bmcGetSensorReading(_handle, values, x, &is_valid, &tmp);
		if (result != FPGA_OK) {
			OPAE_MSG("Failed to read sensor readings.");
			continue;
		}

		if (!is_valid) {
			continue;
		}

		len = strnlen(details.name, sizeof(_handle->_bmc_metric_cache_value[x].metric_name) - 1);
		memcpy(_handle->_bmc_metric_cache_value[x].metric_name, details.name, len);
		_handle->_bmc_metric_cache_value[x].metric_name[len] = '\0';

		_handle->_bmc_metric_cache_value[x].fpga_metric.value.dvalue = tmp;

		metric_indicator = strcasecmp(details.name, _fpga_enum_metric->metric_name);
		if (metric_indicator == 0) {
			fpga_metric->value.dvalue = tmp;
		}

	}


	result = xfpga_bmcDestroySensorValues(_handle, &values);
	if (result != FPGA_OK) {
		OPAE_MSG("Failed to Destroy Sensor value.");
	}

out_destroy:
	result = xfpga_bmcDestroySDRs(_handle, &records);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to Destroy SDR.");
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
		OPAE_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	result = sysfs_read_u64(sysfs_path, &value);
	if (result != FPGA_OK) {
		OPAE_MSG("Failed to read Metrics values");
		return result;
	}

	*dvalue = ((double)value / MILLI);

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
		OPAE_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	result = fpga_vector_total(enum_vector, &num_enun_metrics);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get metric total");
		return FPGA_NOT_FOUND;
	}

	fpga_metric->isvalid = false;
	result = FPGA_NOT_FOUND;
	for (index = 0; index < num_enun_metrics ; index++) {

		_fpga_enum_metric = (struct _fpga_enum_metric *)	fpga_vector_get(enum_vector, index);

		if (metric_num == _fpga_enum_metric->metric_num) {

			// Found Metic
			memset(&value, 0, sizeof(value));

			// DCP Power & Thermal
			if ((_fpga_enum_metric->hw_type == FPGA_HW_DCP_RC) &&
				((_fpga_enum_metric->metric_type == FPGA_METRIC_TYPE_POWER) ||
				(_fpga_enum_metric->metric_type == FPGA_METRIC_TYPE_THERMAL))) {


				result  = get_bmc_metrics_values(handle, _fpga_enum_metric, fpga_metric);
				if (result != FPGA_OK) {
					OPAE_MSG("Failed to get BMC metric value");
				} else {
					fpga_metric->isvalid = true;
				}
				fpga_metric->metric_num = metric_num;

			 }


			// Read power theraml values from Max10
			if (((_fpga_enum_metric->hw_type == FPGA_HW_DCP_N3000) ||
				(_fpga_enum_metric->hw_type == FPGA_HW_DCP_D5005) ||
				(_fpga_enum_metric->hw_type == FPGA_HW_DCP_N5010)) &&
				((_fpga_enum_metric->metric_type == FPGA_METRIC_TYPE_POWER) ||
				(_fpga_enum_metric->metric_type == FPGA_METRIC_TYPE_THERMAL))) {

				result = read_max10_value(_fpga_enum_metric, &value.dvalue);
				if (result != FPGA_OK) {
					OPAE_MSG("Failed to get Max10 metric value");
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
	char *str                                   = NULL;
	char *str_last                              = NULL;
	uint64_t i                                  = 0;
	struct _fpga_enum_metric *fpga_enum_metric  = NULL;
	char qualifier_name[SYSFS_PATH_MAX]         = { 0, };
	char metrics_name[SYSFS_PATH_MAX]           = { 0, };
	int qualifier_indicator                     = 0;
	int metric_indicator                        = 0;
	uint64_t num_enun_metrics                   = 0;
	size_t len;

	if (search_string == NULL ||
		fpga_enum_metrics_vector == NULL ||
		metric_num == NULL) {
		OPAE_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	str = strrchr(search_string, ':');
	if (!str) {
		OPAE_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	// Metric Name
	len = strnlen(str + 1, FPGA_METRIC_STR_SIZE - 1);
	memcpy(metrics_name, str + 1, len);
	metrics_name[len] = '\0';

	// qualifier_name
	str_last = strrchr(search_string, ':');
	if (!str_last) {
		OPAE_ERR("Invalid Input Paramters");
		return FPGA_INVALID_PARAM;
	}

	memcpy(qualifier_name, search_string, str_last - search_string);
	qualifier_name[str_last - search_string] = '\0';

	result = fpga_vector_total(fpga_enum_metrics_vector, &num_enun_metrics);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get metric total");
		return FPGA_NOT_FOUND;
	}


	for (i = 0; i < num_enun_metrics; i++) {
		fpga_enum_metric = (struct _fpga_enum_metric *) fpga_vector_get(fpga_enum_metrics_vector, i);

		qualifier_indicator = strcasecmp(fpga_enum_metric->qualifier_name, qualifier_name);
		metric_indicator = strcasecmp(fpga_enum_metric->metric_name, metrics_name);

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
