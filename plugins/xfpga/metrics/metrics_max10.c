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
* \file metrics_max10.h
* \brief fpga metrics max10 functions
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H


#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>


#include "common_int.h"
#include "metrics_int.h"
#include "types_int.h"
#include "sysfs_int.h"
#include "opae/metrics.h"
#include "metrics/vector.h"
#include "xfpga.h"
#include "metrics_max10.h"

// Max10 Metric limits
#define THERMAL_HIGH_LIMIT             300.00
#define THERMAL_LOW_LIMIT              -273.00
#define POWER_HIGH_LIMIT               1000.00
#define POWER_LOW_LIMIT                0.00
#define VOLTAMP_HIGH_LIMIT             500.00
#define VOLTAMP_LOW_LIMIT              0.00


fpga_result read_sensor_sysfs_file(const char *sysfs, const char *file,
			void **buf, uint32_t *tot_bytes_ret)
{
	char sysfspath[SYSFS_PATH_MAX] = { 0, };
	struct stat stats;
	int fd = 0;
	fpga_result res = FPGA_OK;
	size_t len;

	if (sysfs == NULL ||
		buf == NULL ||
		tot_bytes_ret == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}
	*buf = NULL;
	*tot_bytes_ret = 0;
	if (file) {
		snprintf(sysfspath, sizeof(sysfspath),
			"%s/%s", sysfs, file);
	} else {
		len = strnlen(sysfs, sizeof(sysfspath) - 1);
		memcpy(sysfspath, sysfs, len);
		sysfspath[len] = '\0';
	}

	glob_t pglob;
	int gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
	if ((gres) || (1 != pglob.gl_pathc)) {
		globfree(&pglob);
		return FPGA_NOT_FOUND;
	}

	fd = open(pglob.gl_pathv[0], O_RDONLY);
	globfree(&pglob);
	if (fd < 0) {
		return FPGA_NOT_FOUND;
	}

	if (fstat(fd, &stats) != 0) {
		close(fd);
		return FPGA_NOT_FOUND;
	}

	// fstat for a sysfs file is not accurate for the BMC
	// Read the entire file into a temp buffer to get actual size of file
	*buf = (void *)calloc(stats.st_size, 1);

	int32_t tot_bytes = 0;
	int32_t bytes_read = 0;
	do {
		bytes_read = (int32_t)read(fd, *buf, stats.st_size);
		if (bytes_read < 0) {
			if (errno == EINTR) {
				bytes_read = 1; // Fool the while loop
				continue;
			}
		}
		tot_bytes += bytes_read;
	} while ((tot_bytes < stats.st_size) && (bytes_read > 0));

	close(fd);

	if ((tot_bytes > stats.st_size) || (bytes_read < 0)) {
		res = FPGA_EXCEPTION;
		free(*buf);
		*buf = NULL;
		goto out;
	}

	*tot_bytes_ret = tot_bytes;

out:
	return res;
}



fpga_result  dfl_enum_max10_metrics_info(struct _fpga_handle *_handle,
	fpga_metric_vector *vector,
	uint64_t *metric_num,
	enum fpga_hw_type  hw_type)
{
	fpga_result result                         = FPGA_OK;
	struct _fpga_token *_token                 = NULL;
	size_t i                                   = 0;
	char *tmp                                  = NULL;
	uint32_t tot_bytes                         = 0;
	enum fpga_metric_type metric_type          = FPGA_METRIC_TYPE_POWER;
	char sysfspath[SYSFS_PATH_MAX]             = { 0, };
	char metrics_sysfs_path[SYSFS_PATH_MAX]    = { 0, };
	char metric_name[SYSFS_PATH_MAX]           = { 0, };
	char group_name[SYSFS_PATH_MAX]            = { 0, };
	char group_sysfs[SYSFS_PATH_MAX]           = { 0, };
	char qualifier_name[SYSFS_PATH_MAX]        = { 0, };
	char metric_units[SYSFS_PATH_MAX]          = { 0, };
	glob_t pglob;
	size_t len;

	if (_handle == NULL ||
		vector == NULL ||
		metric_num == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	_token = (struct _fpga_token *)_handle->token;
	if (_token == NULL) {
		OPAE_ERR("Invalid token within handle");
		return FPGA_INVALID_PARAM;
	}

	// metrics group
	if (snprintf(sysfspath, sizeof(sysfspath),
		"%s/%s", _token->sysfspath, DFL_MAX10_SYSFS_PATH) < 0) {
		OPAE_ERR("snprintf failed");
		return FPGA_EXCEPTION;
	}

	int gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
	if ((gres) || (1 != pglob.gl_pathc)) {
		OPAE_ERR("Failed pattern match %s: %s", sysfspath, strerror(errno));
		globfree(&pglob);
		return FPGA_NOT_FOUND;
	}

	len = strnlen(pglob.gl_pathv[0], sizeof(group_sysfs) - 1);
	memcpy(group_sysfs, pglob.gl_pathv[0], len);
	group_sysfs[len] = '\0';
	globfree(&pglob);

	// Enum sensors
	if (snprintf(sysfspath, sizeof(sysfspath),
		"%s/%s", _token->sysfspath, DFL_MAX10_SENSOR_SYSFS_PATH) < 0) {
		OPAE_ERR("snprintf failed");
		return FPGA_EXCEPTION;
	}

	gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
	if (gres) {
		OPAE_ERR("Failed pattern match %s: %s", sysfspath, strerror(errno));
		globfree(&pglob);
		return FPGA_NOT_FOUND;
	}


	// for loop
	for (i = 0; i < pglob.gl_pathc; i++) {

		// Sensor name
		result = read_sensor_sysfs_file(pglob.gl_pathv[i], NULL, (void **)&tmp, &tot_bytes);
		if (FPGA_OK != result || !tmp) {
			if (tmp) {
				free(tmp);
			}
			continue;
		}

		memset(&metric_name, 0, sizeof(metric_name));
		len = strnlen(tmp, sizeof(metric_name) - 1);
		memcpy(metric_name, tmp, len);
		metric_name[len] = '\0';
		if (metric_name[len - 1] == '\n')
			metric_name[len - 1] = '\0';

		if (tmp) {
			free(tmp);
		}

		// Metrics group name and qualifier name
		if ((strstr(pglob.gl_pathv[i], DFL_VOLTAGE) || strstr(pglob.gl_pathv[i],
			DFL_CURRENT) || strstr(pglob.gl_pathv[i], DFL_POWER))) {
			metric_type = FPGA_METRIC_TYPE_POWER;

			// group name
			len = strnlen(PWRMGMT, sizeof(group_name) - 1);
			memcpy(group_name, PWRMGMT, len);
			group_name[len] = '\0';

			//qualifier name
			if (snprintf(qualifier_name, sizeof(qualifier_name),
				"%s:%s", PWRMGMT, metric_name) < 0) {
				OPAE_ERR("snprintf failed");
				result = FPGA_EXCEPTION;
				goto out;
			}

		} else if (strstr(pglob.gl_pathv[i], DFL_TEMPERATURE)) {
			metric_type = FPGA_METRIC_TYPE_THERMAL;

			// group name
			len = strnlen(THERLGMT, sizeof(group_name) - 1);
			memcpy(group_name, THERLGMT, len);
			group_name[len] = '\0';

			//qualifier name
			if (snprintf(qualifier_name, sizeof(qualifier_name),
				"%s:%s", THERLGMT, metric_name) < 0) {
				OPAE_ERR("snprintf failed");
				result = FPGA_EXCEPTION;
				goto out;
			}

		}
		else {
			printf("FPGA_METRIC_TYPE_UNKNOWN \n");
			metric_type = FPGA_METRIC_TYPE_UNKNOWN;
		}

		// Metric Units
		if (strstr(pglob.gl_pathv[i], DFL_POWER)) {

			len = strnlen(POWER_UNITS, sizeof(metric_units) - 1);
			memcpy(metric_units, POWER_UNITS, len);
			metric_units[len] = '\0';

		} else if (strstr(pglob.gl_pathv[i], DFL_VOLTAGE)) {

			len = strnlen(VOLTAGE_UNITS, sizeof(metric_units) - 1);
			memcpy(metric_units, VOLTAGE_UNITS, len);
			metric_units[len] = '\0';

		} else if (strstr(pglob.gl_pathv[i], DFL_CURRENT)) {

			len = strnlen(CURRENT_UNITS, sizeof(metric_units) - 1);
			memcpy(metric_units, CURRENT_UNITS, len);
			metric_units[len] = '\0';

		} else if (strstr(pglob.gl_pathv[i], DFL_TEMPERATURE)) {

			len = strnlen(TEMPERATURE_UNITS, sizeof(metric_units) - 1);
			memcpy(metric_units, TEMPERATURE_UNITS, len);
			metric_units[len] = '\0';

		} else if (strstr(pglob.gl_pathv[i], CLOCK)) {

			len = strnlen(CLOCK_UNITS, sizeof(metric_units) - 1);
			memcpy(metric_units, CLOCK_UNITS, len);
			metric_units[len] = '\0';

		} else {
			strncpy(metric_units, "N/A", 4);
		}

		// value sysfs path
		memset(&metrics_sysfs_path, 0, sizeof(metrics_sysfs_path));
		len = strlen(pglob.gl_pathv[i]) - 5;

		memcpy(metrics_sysfs_path, pglob.gl_pathv[i], len);
		metrics_sysfs_path[len] = '\0';
		strncat(metrics_sysfs_path, DFL_VALUE, strlen(DFL_VALUE)+1);

		result = add_metric_vector(vector, *metric_num, qualifier_name,
			group_name, group_sysfs, metric_name,
			metrics_sysfs_path, metric_units,
			FPGA_METRIC_DATATYPE_DOUBLE, metric_type, hw_type, 0);
		if (result != FPGA_OK) {
			OPAE_ERR("Failed to add metrics");
			goto out;
		}

		*metric_num = *metric_num + 1;

	} // end for loop

out:
	globfree(&pglob);
	return result;
}


fpga_result read_max10_value(struct _fpga_enum_metric *_fpga_enum_metric,
					double *dvalue)
{
	fpga_result result     = FPGA_OK;
	uint64_t value         = 0;

	if (_fpga_enum_metric == NULL ||
		dvalue == NULL) {
		OPAE_ERR("Invalid Input Parameters");
		return FPGA_INVALID_PARAM;
	}

	result = sysfs_read_u64(_fpga_enum_metric->metric_sysfs, &value);
	if (result != FPGA_OK) {
		OPAE_MSG("Failed to read Metrics values");
		return result;
	}

	*dvalue = ((double)value / MILLI);

	// Check for limits
	if (strstr(_fpga_enum_metric->metric_name, DFL_POWER)) {

		if (*dvalue  < POWER_LOW_LIMIT || *dvalue  > POWER_HIGH_LIMIT)
			result = FPGA_EXCEPTION;

	} else if (strstr(_fpga_enum_metric->metric_name, DFL_VOLTAGE)) {

		if (*dvalue < VOLTAMP_LOW_LIMIT || *dvalue > VOLTAMP_HIGH_LIMIT)
			result = FPGA_EXCEPTION;

	} else if (strstr(_fpga_enum_metric->metric_name, DFL_CURRENT)) {

		if (*dvalue < VOLTAMP_LOW_LIMIT || *dvalue > VOLTAMP_HIGH_LIMIT)
			result = FPGA_EXCEPTION;

	} else if (strstr(_fpga_enum_metric->metric_name, DFL_TEMPERATURE)) {

		if (*dvalue < THERMAL_LOW_LIMIT || *dvalue > THERMAL_HIGH_LIMIT)
			result = FPGA_EXCEPTION;

	}

	return result;
}
