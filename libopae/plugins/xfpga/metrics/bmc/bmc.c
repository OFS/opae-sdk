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

#include "bmc.h"
#define _TIMESPEC_DEFINED
#include "../../types_int.h"
#include "safe_string/safe_string.h"
#include "bmcdata.h"
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

#include <glob.h>

#define NULL_CHECK(x)                                                          \
	do {                                                                   \
		if (NULL == (x)) {                                             \
			return FPGA_INVALID_PARAM;                             \
		}                                                              \
	} while (0)

fpga_result read_sysfs_file(fpga_token token, const char *file,
		   void **buf, uint32_t *tot_bytes_ret)
{
	char sysfspath[SYSFS_PATH_MAX];
	struct stat stats;
	int fd = 0;
	fpga_result res = FPGA_OK;

	NULL_CHECK(token);
	NULL_CHECK(buf);
	NULL_CHECK(file);
	NULL_CHECK(tot_bytes_ret);

	*buf = NULL;
	*tot_bytes_ret = 0;

	// TODO: Remove need for this
	struct _fpga_token *tok = (struct _fpga_token *)token;
	if (FPGA_TOKEN_MAGIC != tok->magic) {
		return FPGA_INVALID_PARAM;
	}

	snprintf_s_ss(sysfspath, sizeof(sysfspath), "%s/%s", tok->sysfspath, file);

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

fpga_result bmcLoadSDRs(fpga_token token, bmc_sdr_handle *records,
			uint32_t *num_sensors)
{
	fpga_result res = FPGA_OK;

	NULL_CHECK(token);
	NULL_CHECK(num_sensors);

	struct _sdr_rec *recs = NULL;

	struct _sdr_content *tmp;
	uint32_t tot_bytes;

	res = read_sysfs_file(token, SYSFS_SDR_FILE, (void **)&tmp, &tot_bytes);
	if (FPGA_OK != res) {
		if (tmp) {
			free(tmp);
		}
		goto out;
	}

	uint32_t sz = sizeof(sdr_header) + sizeof(sdr_key) + sizeof(sdr_body);
	uint32_t num_of_sensors = (tot_bytes + sz - 1) / sz;

	*num_sensors = num_of_sensors;
	if (NULL == records) {
		free(tmp);
		return FPGA_OK;
	}

	*records = (bmc_sdr_handle)calloc(1, sizeof(struct _sdr_rec));
	recs = (struct _sdr_rec *)*records;

	recs->contents = tmp;

	recs->magic = BMC_SDR_MAGIC;
	recs->num_records = num_of_sensors;

	// TODO: Remove need for this
	struct _fpga_token *tok = (struct _fpga_token *)token;

	strcpy_s(recs->sysfs_path, SYSFS_PATH_MAX, tok->sysfspath);
	recs->token = token;

out:
	return res;
}

fpga_result bmcReadSensorValues(bmc_sdr_handle records, bmc_values_handle *values,
				uint32_t *num_values)
{
	fpga_result res = FPGA_OK;
	struct _bmc_values *vals = NULL;

	NULL_CHECK(records);
	struct _sdr_rec *sdr = (struct _sdr_rec *)records;

	if (BMC_SDR_MAGIC != sdr->magic) {
		res = FPGA_INVALID_PARAM;
		goto out;
	}

	NULL_CHECK(num_values);

	if (NULL == values) {
		*num_values = sdr->num_records;
		res = FPGA_OK;
		goto out;
	}

	sensor_reading *tmp = NULL;
	uint32_t tot_bytes;

	res = read_sysfs_file(sdr->token, SYSFS_SENSOR_FILE, (void **)&tmp,
			      &tot_bytes);
	if (FPGA_OK != res) {
		if (tmp) {
			free(tmp);
		}
		goto out;
	}

	if (tot_bytes != (sdr->num_records * sizeof(sensor_reading))) {
		fprintf(stderr,
			"Struct / file size mismatch: file size %d,"
			" struct size %d.\n",
			(int)tot_bytes,
			(int)(sdr->num_records * sizeof(sensor_reading)));
		res = FPGA_EXCEPTION;
		goto out;
	}

	*num_values = sdr->num_records;

	*values = (bmc_values_handle)calloc(1, sizeof(struct _bmc_values));
	vals = (struct _bmc_values *)*values;

	vals->contents = tmp;

	vals->magic = BMC_VALUES_MAGIC;
	vals->num_records = sdr->num_records;

	vals->values = (Values **)calloc(sdr->num_records, sizeof(Values *));

	uint32_t i;
	for (i = 0; i < sdr->num_records; i++) {
		vals->values[i] = bmc_build_values(
			&vals->contents[i], &sdr->contents[i].header,
			&sdr->contents[i].key, &sdr->contents[i].body);
		vals->values[i]->sdr = &sdr->contents[i];
	}

out:
	return res;
}

fpga_result bmcGetSensorReading(bmc_values_handle values,
				uint32_t sensor_number, uint32_t *is_valid,
				double *value)
{
	fpga_result res = FPGA_OK;

	NULL_CHECK(values);
	NULL_CHECK(value);
	struct _bmc_values *vals = (struct _bmc_values *)values;

	if (BMC_VALUES_MAGIC != vals->magic) {
		res = FPGA_INVALID_PARAM;
		goto out;
	}

	if (sensor_number >= vals->num_records) {
		res = FPGA_INVALID_PARAM;
		goto out;
	}

	*is_valid = vals->values[sensor_number]->is_valid;

	*value = vals->values[sensor_number]->value.f_val;

out:
	return res;
}

fpga_result bmcThresholdsTripped(bmc_values_handle values,
				 tripped_thresholds **tripped,
				 uint32_t *num_tripped)
{
	fpga_result res = FPGA_OK;
	int num_tripd = 0;

	NULL_CHECK(values);
	NULL_CHECK(num_tripped);

	struct _bmc_values *vals = (struct _bmc_values *)values;
	uint32_t i;

	if (BMC_VALUES_MAGIC != vals->magic) {
		res = FPGA_INVALID_PARAM;
		goto out;
	}

	// Count the number tripped
	for (i = 0; i < vals->num_records; i++) {
		uint8_t indicators = vals->contents[i].threshold_events._value
				     & BMC_THRESHOLD_EVENT_MASK;

		if (0 == indicators) {
			continue;
		}

		num_tripd++;
	}

	*num_tripped = num_tripd;
	if (0 == num_tripd) {
		if (NULL != tripped) {
			*tripped = NULL;
		}
		goto out;
	}

	*tripped = (tripped_thresholds *)calloc(num_tripd,
						sizeof(tripped_thresholds));
	tripped_thresholds *rets = *tripped;
	int index = 0;

	// Fill in the tripped structures
	for (i = 0; i < vals->num_records; i++) {
		struct _sdr_content *sdr = vals->values[i]->sdr;
		uint8_t indicators = vals->contents[i].threshold_events._value
				     & BMC_THRESHOLD_EVENT_MASK;

		if (0 == indicators) {
			continue;
		}

		rets[index].sensor_number = i;
		rets[index].type = SDR_SENSOR_IS_POWER(&sdr->body)
					   ? BMC_POWER
					   : BMC_THERMAL;
		rets[index].which_thresholds = indicators;
		index++;
	}

out:
	return res;
}

fpga_result bmcDestroySDRs(bmc_sdr_handle *records)
{
	fpga_result res = FPGA_OK;

	NULL_CHECK(records);
	struct _sdr_rec *sdr = (struct _sdr_rec *)*records;

	if (BMC_SDR_MAGIC != sdr->magic) {
		res = FPGA_INVALID_PARAM;
		goto out;
	}

	free(sdr->contents);

	sdr->magic = 0;
	free(sdr);

	*records = NULL;

out:
	return res;
}

fpga_result bmcDestroySensorValues(bmc_values_handle *values)
{
	fpga_result res = FPGA_OK;

	NULL_CHECK(values);
	NULL_CHECK(*values);
	struct _bmc_values *vals = (struct _bmc_values *)*values;
	uint32_t i;

	if (BMC_VALUES_MAGIC != vals->magic) {
		res = FPGA_INVALID_PARAM;
		goto out;
	}

	for (i = 0; i < vals->num_records; i++) {
		free(vals->values[i]->name);
		free(vals->values[i]);
	}

	free(vals->contents);
	free(vals->values);

	vals->magic = 0;
	free(vals);

	*values = NULL;

out:
	return res;
}

static void getThresholdValues(sdr_details *details, Values *this_val,
			       struct _sdr_content *sdr)
{
	uint8_t settable =
		(sdr->body.discrete_settable_readable_threshold_mask._value
		 & 0x3f00)
		>> 8;

	details->thresholds.upper_nr_thresh.is_valid = 0;
	details->thresholds.upper_c_thresh.is_valid = 0;
	details->thresholds.upper_nc_thresh.is_valid = 0;
	details->thresholds.lower_nr_thresh.is_valid = 0;
	details->thresholds.lower_c_thresh.is_valid = 0;
	details->thresholds.lower_nc_thresh.is_valid = 0;

	if (!settable) {
		return;
	}

	if (settable & (1 << 5)) {
		details->thresholds.upper_nr_thresh.is_valid = 1;
		details->thresholds.upper_nr_thresh.value =
			getvalue(this_val, sdr->body.upper_nr_threshold);
	}

	if (settable & (1 << 4)) {
		details->thresholds.upper_c_thresh.is_valid = 1;
		details->thresholds.upper_c_thresh.value =
			getvalue(this_val, sdr->body.upper_c_threshold);
	}

	if (settable & (1 << 3)) {
		details->thresholds.upper_nc_thresh.is_valid = 1;
		details->thresholds.upper_nc_thresh.value =
			getvalue(this_val, sdr->body.upper_nc_threshold);
	}

	if (settable & (1 << 2)) {
		details->thresholds.lower_nr_thresh.is_valid = 1;
		details->thresholds.lower_nr_thresh.value =
			getvalue(this_val, sdr->body.lower_nr_threshold);
	}

	if (settable & (1 << 1)) {
		details->thresholds.lower_c_thresh.is_valid = 1;
		details->thresholds.lower_c_thresh.value =
			getvalue(this_val, sdr->body.lower_c_threshold);
	}

	if (settable & (1 << 0)) {
		details->thresholds.lower_nc_thresh.is_valid = 1;
		details->thresholds.lower_nc_thresh.value =
			getvalue(this_val, sdr->body.lower_nc_threshold);
	}
}

fpga_result bmcGetSDRDetails(bmc_values_handle values, uint32_t sensor_number,
			     sdr_details *details)
{
	fpga_result res = FPGA_OK;

	NULL_CHECK(values);
	NULL_CHECK(details);
	struct _bmc_values *vals = (struct _bmc_values *)values;
	Values *this_val = NULL;

	if (BMC_VALUES_MAGIC != vals->magic) {
		res = FPGA_INVALID_PARAM;
		goto out;
	}

	if (sensor_number >= vals->num_records) {
		res = FPGA_INVALID_PARAM;
		goto out;
	}

	this_val = vals->values[sensor_number];

	details->sensor_number = sensor_number;
	details->type = this_val->sensor_type;
	details->name = this_val->name;
	details->units = this_val->units;
	details->M = this_val->M;
	details->B = this_val->B;
	details->accuracy = this_val->accuracy;
	details->tolerance = this_val->tolerance;
	details->result_exp = this_val->result_exp;

	getThresholdValues(details, this_val, vals->values[sensor_number]->sdr);

out:
	return res;
}

fpga_result bmcDestroyTripped(tripped_thresholds *tripped)
{
	fpga_result res = FPGA_OK;

	NULL_CHECK(tripped);

	free(tripped);

	return res;
}

fpga_result bmcGetFirmwareVersion(fpga_token token, uint32_t *version)
{
	fpga_result res = FPGA_OK;

	NULL_CHECK(token);
	NULL_CHECK(version);
	*version = (uint32_t)-1;

	device_id *tmp = NULL;
	uint32_t tot_bytes;

	res = read_sysfs_file(token, SYSFS_DEVID_FILE, (void **)&tmp,
			      &tot_bytes);
	if (FPGA_OK != res) {
		goto out;
	}

	if (tmp->completion_code != 0) {
		res = FPGA_NOT_FOUND;
		goto out;
	}

	*version = tmp->aux_fw_rev_0_7 | (tmp->aux_fw_rev_8_15 << 8)
		   | (tmp->aux_fw_rev_16_23 << 16)
		   | (tmp->aux_fw_rev_24_31 << 24);

out:
	if (tmp) {
		free(tmp);
	}

	return res;
}

fpga_result bmcGetLastPowerdownCause(fpga_token token, char **cause)
{
	fpga_result res = FPGA_OK;

	NULL_CHECK(token);
	NULL_CHECK(cause);
	*cause = NULL;

	powerdown_cause *tmp = NULL;
	uint32_t tot_bytes;

	res = read_sysfs_file(token, SYSFS_PWRDN_FILE, (void **)&tmp,
			      &tot_bytes);
	if (FPGA_OK != res) {
		goto out;
	}

	if (tmp->completion_code != 0) {
		res = FPGA_NOT_FOUND;
		goto out;
	}

	*cause = strdup((const char *)tmp->message);

out:
	if (tmp) {
		free(tmp);
	}

	return res;
}

fpga_result bmcGetLastResetCause(fpga_token token, char **cause)
{
	fpga_result res = FPGA_OK;

	NULL_CHECK(token);
	NULL_CHECK(cause);
	*cause = NULL;

	reset_cause *tmp = NULL;
	uint32_t tot_bytes;

	res = read_sysfs_file(token, SYSFS_RESET_FILE, (void **)&tmp,
			      &tot_bytes);
	if (FPGA_OK != res) {
		goto out;
	}

	if (tmp->completion_code != 0) {
		res = FPGA_NOT_FOUND;
		*cause = strdup((const char *)"Unavailable");
		goto out;
	}

	if (0 == tmp->reset_cause) {
		*cause = strdup((const char *)"None");
		goto out;
	}

	if (tmp->reset_cause & CHIP_RESET_CAUSE_EXTRST) {
		*cause = strdup((const char *)"External reset");
	}

	if (tmp->reset_cause & CHIP_RESET_CAUSE_BOD_IO) {
		*cause = strdup((const char *)"Brown-out detected");
	}

	if (tmp->reset_cause & CHIP_RESET_CAUSE_OCD) {
		*cause = strdup((const char *)"On-chip debug system");
	}

	if (tmp->reset_cause & CHIP_RESET_CAUSE_POR) {
		*cause = strdup((const char *)"Power-on-reset");
	}

	if (tmp->reset_cause & CHIP_RESET_CAUSE_SOFT) {
		*cause = strdup((const char *)"Software reset");
	}

	if (tmp->reset_cause & CHIP_RESET_CAUSE_SPIKE) {
		*cause = strdup((const char *)"Spike detected");
	}

	if (tmp->reset_cause & CHIP_RESET_CAUSE_WDT) {
		*cause = strdup((const char *)"Watchdog timeout");
	}


out:
	if (tmp) {
		free(tmp);
	}

	return res;
}
