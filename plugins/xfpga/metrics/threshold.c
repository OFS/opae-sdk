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
* \file threshold.c
* \brief fpga sensor threshold functions
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H


#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <glob.h>

#include "types_int.h"
#include "metrics_int.h"
#include "common_int.h"
#include "metrics/bmc/bmc.h"
#include "metrics_int.h"
#include "metrics_max10.h"
#include "threshold.h"


fpga_result xfpga_fpgaGetMetricsThresholdInfo(fpga_handle handle,
					metric_threshold *metric_thresholds,
					uint32_t *num_thresholds)
{
	fpga_result result             = FPGA_OK;
	struct _fpga_token *_token     = NULL;
	enum fpga_hw_type hw_type     = FPGA_HW_UNKNOWN;
	fpga_objtype objtype;


	if (handle == NULL ||
		(metric_thresholds == NULL &&
		num_thresholds == NULL)) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		OPAE_ERR("Invalid token within handle");
		return FPGA_INVALID_PARAM;
	}

	result = get_fpga_object_type(handle, &objtype);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to object type");
		return result;
	}

	if (objtype != FPGA_DEVICE) {
		OPAE_ERR("FPGA object type is not FPGA DEVICE ");
		return result;
	}

	// get fpga hw type.
	result = get_fpga_hw_type(_handle, &hw_type);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to discover hardware type.");
		return result;
	}

	switch (hw_type) {
	// MCP
	case FPGA_HW_MCP: {
		OPAE_ERR("Not Supported MCP thresholds.");
		result = FPGA_EXCEPTION;
	}
	break;

	// DCP RC
	case FPGA_HW_DCP_RC: {

		result = get_bmc_threshold_info(handle,
			metric_thresholds, num_thresholds);
		if (result != FPGA_OK) {
			OPAE_ERR("Failed to get bmc thresholds.");
			return result;
		}

	}
	break;

	// VC DC
	case FPGA_HW_DCP_DC:
	case FPGA_HW_DCP_VC: {
		// Max10
		result = get_max10_threshold_info(handle,
			metric_thresholds, num_thresholds);
		if (result != FPGA_OK) {
			OPAE_ERR("Failed to get max10 thresholds.");
			return result;
		}

	}
	break;

	default:
		OPAE_ERR("Unknown Device ID.");
		result = FPGA_EXCEPTION;
	}

	return result;
}

fpga_result get_bmc_threshold_info(fpga_handle handle,
					metric_threshold *metric_thresholds,
					uint32_t *num_thresholds)
{
	fpga_result result                = FPGA_OK;
	fpga_result resval                = FPGA_OK;
	uint32_t num_sensors              = 0;
	uint32_t num_values               = 0;
	uint32_t x                        = 0;

	sdr_details details;
	bmc_sdr_handle records;
	bmc_values_handle values;
	size_t len;

	if (handle == NULL ||
		num_thresholds == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;

	if (pthread_mutex_lock(&_handle->lock)) {
		OPAE_ERR("pthread_mutex_lock failed");
		return FPGA_EXCEPTION;
	}

	if (_handle->bmc_handle == NULL)
		_handle->bmc_handle = metrics_load_bmc_lib();
	if (!_handle->bmc_handle) {
		OPAE_ERR("Failed to load BMC module %s", dlerror());
		if (pthread_mutex_unlock(&_handle->lock)) {
			OPAE_ERR("pthread_mutex_unlock failed");
		}
		return FPGA_EXCEPTION;
	}
	if (pthread_mutex_unlock(&_handle->lock)) {
		OPAE_ERR("pthread_mutex_unlock failed");
	}

	result = xfpga_bmcLoadSDRs(_handle, &records, &num_sensors);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to load BMC SDR.");
		return result;
	}

	result = xfpga_bmcReadSensorValues(_handle, records, &values, &num_values);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to read BMC sensor values.");
		goto destroy_sdr;
	}

	// Return number of thresholds.
	if (metric_thresholds == NULL && num_thresholds != NULL) {
		*num_thresholds = num_values;
		goto destroy_values;
	}

	// Return number of threshold info and value.
	if (metric_thresholds != NULL && num_thresholds != NULL) {

		for (x = 0; x < num_sensors; x++) {

			// Sensor Name
			result = xfpga_bmcGetSDRDetails(_handle, values, x, &details);
			if (result != FPGA_OK) {
				OPAE_MSG("Failed to read sensor readings.");
				continue;
			}

			len = strnlen(details.name, sizeof(metric_thresholds[x].metric_name) - 1);
			memcpy(metric_thresholds[x].metric_name, details.name, len);
			metric_thresholds[x].metric_name[len] = '\0';

			// Upper Non-Recoverable Threshold
			if (details.thresholds.upper_nr_thresh.is_valid) {

				len = strnlen(UPPER_NR_THRESHOLD,
					sizeof(metric_thresholds[x].upper_nr_threshold.threshold_name) - 1);
				memcpy(metric_thresholds[x].upper_nr_threshold.threshold_name,
					UPPER_NR_THRESHOLD, len);
				metric_thresholds[x].upper_nr_threshold.threshold_name[len] = '\0';

				metric_thresholds[x].upper_nr_threshold.value = details.thresholds.upper_nr_thresh.value;
				metric_thresholds[x].upper_nr_threshold.is_valid = true;

			}


			// Upper Critical Threshold
			if (details.thresholds.upper_c_thresh.is_valid) {

				len = strnlen(UPPER_C_THRESHOLD,
					sizeof(metric_thresholds[x].upper_c_threshold.threshold_name) - 1);
				memcpy(metric_thresholds[x].upper_c_threshold.threshold_name,
					UPPER_C_THRESHOLD, len);
				metric_thresholds[x].upper_c_threshold.threshold_name[len] = '\0';

				metric_thresholds[x].upper_c_threshold.value = details.thresholds.upper_c_thresh.value;
				metric_thresholds[x].upper_c_threshold.is_valid = true;
			}


			// Upper Non-Critical Threshold
			if (details.thresholds.upper_nc_thresh.is_valid) {

				len = strnlen(UPPER_NC_THRESHOLD,
						sizeof(metric_thresholds[x].upper_nc_threshold.threshold_name) - 1);
				memcpy(metric_thresholds[x].upper_nc_threshold.threshold_name,
					UPPER_NC_THRESHOLD, len);
				metric_thresholds[x].upper_nc_threshold.threshold_name[len] = '\0';

				metric_thresholds[x].upper_nc_threshold.value = details.thresholds.upper_nc_thresh.value;
				metric_thresholds[x].upper_nc_threshold.is_valid = true;
			}


			// Lower Non-Recoverable Threshold
			if (details.thresholds.lower_nr_thresh.is_valid) {

				len = strnlen(LOWER_NR_THRESHOLD,
					sizeof(metric_thresholds[x].lower_nr_threshold.threshold_name) - 1);
				memcpy(metric_thresholds[x].lower_nr_threshold.threshold_name,
					LOWER_NR_THRESHOLD, len);
				metric_thresholds[x].lower_nr_threshold.threshold_name[len] = '\0';

				metric_thresholds[x].lower_nr_threshold.value = details.thresholds.lower_nr_thresh.value;
				metric_thresholds[x].lower_nr_threshold.is_valid = true;
			}


			// Lower Critical Threshold
			if (details.thresholds.lower_c_thresh.is_valid) {

				len = strnlen(LOWER_C_THRESHOLD,
						sizeof(metric_thresholds[x].lower_c_threshold.threshold_name) - 1);
				memcpy(metric_thresholds[x].lower_c_threshold.threshold_name,
					LOWER_C_THRESHOLD, len);
				metric_thresholds[x].lower_c_threshold.threshold_name[len] = '\0';

				metric_thresholds[x].lower_c_threshold.value = details.thresholds.lower_c_thresh.value;
				metric_thresholds[x].lower_c_threshold.is_valid = true;
			}

			// Lower Non-Critical Threshold
			if (details.thresholds.lower_nc_thresh.is_valid) {

				len = strnlen(LOWER_NC_THRESHOLD,
					sizeof(metric_thresholds[x].lower_nc_threshold.threshold_name) - 1);
				memcpy(metric_thresholds[x].lower_nc_threshold.threshold_name,
					LOWER_NC_THRESHOLD, len);
				metric_thresholds[x].lower_nc_threshold.threshold_name[len] = '\0';

				metric_thresholds[x].lower_nc_threshold.value = details.thresholds.lower_nc_thresh.value;
				metric_thresholds[x].lower_nc_threshold.is_valid = true;
			}

		} // for loop end

	} // endif

destroy_values:
	resval = (result != FPGA_OK) ? result : resval;

	result = xfpga_bmcDestroySensorValues(_handle, &values);
	if (result != FPGA_OK) {
		OPAE_MSG("Failed to Destroy Sensor value.");
	}

destroy_sdr:
	resval = (result != FPGA_OK) ? result : resval;

	result = xfpga_bmcDestroySDRs(_handle, &records);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to Destroy SDR.");
	}

	resval = (result != FPGA_OK) ? result : resval;
	return resval;
}


fpga_result get_max10_threshold_info(fpga_handle handle,
					metric_threshold *metric_thresholds,
					uint32_t *num_thresholds)
{
	fpga_result result                     = FPGA_OK;
	fpga_result resval                     = FPGA_OK;
	char sysfspath[SYSFS_PATH_MAX]         = { 0, };
	size_t i                               = 0;
	struct _fpga_token *_token             = NULL;
	char *tmp                              = NULL;
	uint32_t tot_bytes                     = 0;
	uint64_t value                         = 0;
	glob_t pglob;
	size_t len;

	if (handle == NULL ||
		num_thresholds == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		OPAE_ERR("Invalid token within handle");
		return FPGA_INVALID_PARAM;
	}

	// Sensor path
	if (snprintf(sysfspath, sizeof(sysfspath),
		     "%s/%s", _token->sysfspath, MAX10_SYSFS_PATH) < 0) {
		OPAE_ERR("buffer overflow in snprintf");
		return FPGA_EXCEPTION;
	}

	int gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
	if ((gres) || (1 != pglob.gl_pathc)) {
		OPAE_ERR("Failed pattern match %s: %s", sysfspath, strerror(errno));
		globfree(&pglob);
		return FPGA_NOT_FOUND;
	}
	globfree(&pglob);


	// scan sensors
	if (snprintf(sysfspath, sizeof(sysfspath),
		     "%s/%s", _token->sysfspath, MAX10_SENSOR_SYSFS_PATH) < 0) {
		OPAE_ERR("buffer overflow in snprintf");
		return FPGA_EXCEPTION;
	}

	gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
	if (gres) {
		OPAE_ERR("Failed pattern match %s: %s", sysfspath, strerror(errno));
		globfree(&pglob);
		return FPGA_NOT_FOUND;
	}


	if (metric_thresholds == NULL && num_thresholds != NULL) {
		*num_thresholds = pglob.gl_pathc;
		goto out;
	}

	// read thresholds
	for (i = 0; i < pglob.gl_pathc; i++) {

		// Sensor name
		result = read_sensor_sysfs_file(pglob.gl_pathv[i], SENSOR_SYSFS_NAME, (void **)&tmp, &tot_bytes);
		if (FPGA_OK != result || !tmp) {
			if (tmp) {
				free(tmp);
				tmp = NULL;
			}
			continue;
		}

		memset(&metric_thresholds[i].metric_name, 0, sizeof(metric_thresholds[i].metric_name));
		len = strnlen(tmp, sizeof(metric_thresholds[i].metric_name) - 1);
		memcpy(metric_thresholds[i].metric_name, tmp, len);
		metric_thresholds[i].metric_name[len] = '\0';
		if (tmp) {
			free(tmp);
			tmp = NULL;
		}

		// Upper Critical Threshold
		len = strnlen(UPPER_C_THRESHOLD,
			sizeof(metric_thresholds[i].upper_c_threshold.threshold_name) - 1);
		memcpy(metric_thresholds[i].upper_c_threshold.threshold_name,
			UPPER_C_THRESHOLD, len);
		metric_thresholds[i].upper_c_threshold.threshold_name[len] = '\0';

		snprintf(sysfspath, sizeof(sysfspath),
			 "%s/%s", pglob.gl_pathv[i], SYSFS_HIGH_FATAL);

		resval = sysfs_read_u64(sysfspath, &value);
		if (resval == FPGA_OK) {
			metric_thresholds[i].upper_c_threshold.value = ((double)value / MILLI);
			metric_thresholds[i].upper_c_threshold.is_valid = true;
		}

		// Upper Non-Critical Threshold
		len = strnlen(UPPER_NC_THRESHOLD,
				sizeof(metric_thresholds[i].upper_nc_threshold.threshold_name) - 1);
		memcpy(metric_thresholds[i].upper_nc_threshold.threshold_name,
			UPPER_NC_THRESHOLD, len);
		metric_thresholds[i].upper_nc_threshold.threshold_name[len] = '\0';

		snprintf(sysfspath, sizeof(sysfspath),
			 "%s/%s", pglob.gl_pathv[i], SYSFS_HIGH_WARN);

		resval = sysfs_read_u64(sysfspath, &value);
		if (resval == FPGA_OK) {
			metric_thresholds[i].upper_nc_threshold.value = ((double)value / MILLI);
			metric_thresholds[i].upper_nc_threshold.is_valid = true;
		}

		// Lower Critical Threshold
		len = strnlen(LOWER_C_THRESHOLD,
				sizeof(metric_thresholds[i].upper_nc_threshold.threshold_name) - 1);
		memcpy(metric_thresholds[i].upper_nc_threshold.threshold_name,
			LOWER_C_THRESHOLD, len);
		metric_thresholds[i].upper_nc_threshold.threshold_name[len] = '\0';

		snprintf(sysfspath, sizeof(sysfspath),
			 "%s/%s", pglob.gl_pathv[i], SYSFS_LOW_FATAL);

		resval = sysfs_read_u64(sysfspath, &value);
		if (resval == FPGA_OK) {
			metric_thresholds[i].lower_c_threshold.value = ((double)value / MILLI);
			metric_thresholds[i].lower_c_threshold.is_valid = true;
		}

		// Lower Non-Critical Threshold
		len = strnlen(LOWER_NC_THRESHOLD,
				sizeof(metric_thresholds[i].lower_nc_threshold.threshold_name) - 1);
		memcpy(metric_thresholds[i].lower_nc_threshold.threshold_name,
			LOWER_NC_THRESHOLD, len);
		metric_thresholds[i].lower_nc_threshold.threshold_name[len] = '\0';

		snprintf(sysfspath, sizeof(sysfspath),
			 "%s/%s", pglob.gl_pathv[i], SYSFS_LOW_WARN);

		resval = sysfs_read_u64(sysfspath, &value);
		if (resval == FPGA_OK) {
			metric_thresholds[i].lower_nc_threshold.value = ((double)value / MILLI);
			metric_thresholds[i].lower_nc_threshold.is_valid = true;
		}

		// Lower Non-Critical Threshold
		len = strnlen(SYSFS_HYSTERESIS,
				sizeof(metric_thresholds[i].hysteresis.threshold_name) - 1);
		memcpy(metric_thresholds[i].hysteresis.threshold_name,
			SYSFS_HYSTERESIS, len);
		metric_thresholds[i].hysteresis.threshold_name[len] = '\0';

		snprintf(sysfspath, sizeof(sysfspath),
			 "%s/%s", pglob.gl_pathv[i], SYSFS_HYSTERESIS);

		resval = sysfs_read_u64(sysfspath, &value);
		if (resval == FPGA_OK) {
			metric_thresholds[i].hysteresis.value = ((double)value / MILLI);
			metric_thresholds[i].hysteresis.is_valid = true;
		}

	} //end for loop

out:
	globfree(&pglob);
	return result;
}
