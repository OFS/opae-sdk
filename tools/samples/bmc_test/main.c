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
#include <errno.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <wchar.h>
#include <time.h>
#include <float.h>

#include "safe_string/safe_string.h"
#include "opae/fpga.h"
#include "bmc/bmc.h"

/*
 * macro to check FPGA return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_FPGAINFO_ERR_GOTO(res, label, desc)                                 \
	do {                                                                   \
		if ((res) != FPGA_OK) {                                        \
			fpgainfo_print_err((desc), (res));                     \
			goto label;                                            \
		}                                                              \
	} while (0)

// Timing parameters
typedef struct {
	double tot_time;
	double avg_time;
	double min_time;
	double max_time;
} TimerVals;

#define ITERATIONS 100

/*
 * Print readable error message for fpga_results
 */
void fpgainfo_print_err(const char *s, fpga_result res)
{
	if (s && res)
		fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

// return elapsed time
static inline double getTime(struct timespec *start, struct timespec *end)
{
	uint64_t diff = 1000000000L * (end->tv_sec - start->tv_sec)
			+ end->tv_nsec - start->tv_nsec;
	return (double)diff / (double)1000000000L;
}

void time_query(bmc_sdr_handle sdrs, TimerVals *times)
{
	int i;
	fpga_result res = FPGA_OK;
	bmc_values_handle values;
	uint32_t num_values;
	struct timespec start, end;

	times->tot_time = 0.0;
	times->min_time = DBL_MAX;
	times->max_time = -DBL_MAX;

	for (i = 0; i < ITERATIONS; i++) {
		clock_gettime(CLOCK_MONOTONIC, &start);
		res = bmcReadSensorValues(sdrs, &values, &num_values);
		clock_gettime(CLOCK_MONOTONIC, &end);

		if (res != FPGA_OK) {
			printf(" bmcReadSensorValues failed with error %s",
			       fpgaErrStr(res));
		}

		res = bmcDestroySensorValues(&values);
		if (res != FPGA_OK) {
			printf(" bmcDestroySensorValues failed with error %s",
			       fpgaErrStr(res));
		}

		double et = getTime(&start, &end);
		times->tot_time += et;

		if (et < times->min_time) {
			times->min_time = et;
		}

		if (et > times->max_time) {
			times->max_time = et;
		}
	}

	times->avg_time = times->tot_time / (double)ITERATIONS;
}

int main()
{
	fpga_result res = FPGA_OK;
	uint32_t matches = 0;
	fpga_token token = NULL;
	fpga_properties filter;
	bmc_sdr_handle sdrs = NULL;
	bmc_values_handle values = NULL;
	uint32_t num_sensors = 0;
	uint32_t num_values = 0;
	sdr_details *details = NULL;
	uint32_t i;
	uint32_t is_valid = 0;
	char *string = NULL;
	uint32_t version = 0;
	tripped_thresholds *tripped = NULL;
	uint32_t num_tripped = 0;
	TimerVals times;

	// Set locale for wide strings
	setlocale(LC_ALL, "");

	printf("\nBMC library test\n\n");

	// start a filter - grab the first FPGA_DEVICE
	res = fpgaGetProperties(NULL, &filter);
	ON_FPGAINFO_ERR_GOTO(res, out_err, "creating properties object");

	res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "setting type to FPGA_DEVICE");

	res = fpgaEnumerate(&filter, 1, NULL, 0, &matches);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "enumerating resources");

	if (matches < 1) {
		fprintf(stderr, "No suitable device found\n");
		goto out_destroy;
	}

	if (matches > 1) {
		fprintf(stderr,
			"Found more than one PAC.  Using first in list.\n");
	}

	res = fpgaEnumerate(&filter, 1, &token, 1, &matches);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "enumerating resources");

	res = bmcGetFirmwareVersion(token, &version);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy_token, "firmware version");
	printf("BMC firmware version: %d\n", version);

	res = bmcGetLastPowerdownCause(token, &string);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy_token, "last powerdown cause");
	printf("Last powerdown cause: '%s'\n", string);
	free(string);

	res = bmcGetLastResetCause(token, &string);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy_token, "last reset cause");
        if (string) {
                printf("Last reset cause: '%s'\n", string);
        }
	free(string);

	// Read in sensor data records
	res = bmcLoadSDRs(token, &sdrs, &num_sensors);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "loading SDRs");

	if (0 == num_sensors) {
		fprintf(stderr, "No sensor data records available\n");
		goto out_destroy_sdrs;
	}

	printf("BMC reports %d sensor records available.\n", num_sensors);

	// Allocate space for sensor details
	details = (sdr_details *)calloc(num_sensors, sizeof(sdr_details));
	if (!details) {
		fprintf(stderr, "Error allocating sensor details.\n");
		goto out_destroy_sdrs;
	}

	printf("\nRunning timing test:\n");
	time_query(sdrs, &times);

	printf("  Average time per snapshot = %fms\n", times.avg_time * 1000.0);
	printf("    Total: %fs, Min: %fms, Max: %fms\n\n", times.tot_time,
	       times.min_time * 1000.0, times.max_time * 1000.0);

	printf("\tSnapshotting sensor values\n");

	res = bmcReadSensorValues(sdrs, &values, &num_values);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy_details, "loading SDRs");

	if (num_values != num_sensors) {
		fprintf(stderr, "Sensor / value size mismatch\n");
		goto out_destroy_details;
	}

	// Populate the sensor details
	for (i = 0; i < num_sensors; i++) {
		res = bmcGetSDRDetails(values, i, &details[i]);
		if (FPGA_OK != res) {
			fprintf(stderr,
				"Error retrieving details for sensor %d\n", i);
		}
	}

	// Print out the details of each sensor
	for (i = 0; i < num_sensors; i++) {
		double sensor_value = 0.0;
		sdr_details *d = &details[i];

		res = bmcGetSensorReading(values, i, &is_valid, &sensor_value);
		if (FPGA_OK != res) {
			fprintf(stderr,
				"Error retrieving value for sensor %d\n", i);
			continue;
		}

		printf("Sensor number %d (%d) - '%s':\n", i, d->sensor_number,
		       d->name);
		printf("\tType: %s\n",
		       d->type == BMC_THERMAL ? "THERMAL" : "POWER");

		if (!is_valid) {
			printf("\tValue: <unavailable> %ls\n", d->units);
			printf("\tNo further valid information\n\n");
			continue;
		}

		printf("\tValue: %f %ls\n", sensor_value, d->units);
		printf("\tCalculation: (%f * raw + %f) * 10e%d\n", d->M, d->B,
		       d->result_exp);
		printf("\t\tAccuracy : %f (1/100)%%\n", d->accuracy);
		printf("\t\tTolerance: %d (1/2) raw units\n", d->tolerance);

		printf("\tThresholds:\n");
		printf("\t\tUpper non-recoverable: ");
		if (d->thresholds.upper_nr_thresh.is_valid) {
			printf("%f %ls\n", d->thresholds.upper_nr_thresh.value,
			       d->units);
		} else {
			printf("<ignored>\n");
		}
		printf("\t\tUpper critical       : ");
		if (d->thresholds.upper_c_thresh.is_valid) {
			printf("%f %ls\n", d->thresholds.upper_c_thresh.value,
			       d->units);
		} else {
			printf("<ignored>\n");
		}
		printf("\t\tUpper non-critical   : ");
		if (d->thresholds.upper_nc_thresh.is_valid) {
			printf("%f %ls\n", d->thresholds.upper_nc_thresh.value,
			       d->units);
		} else {
			printf("<ignored>\n");
		}

		printf("\t\tLower non-recoverable: ");
		if (d->thresholds.lower_nr_thresh.is_valid) {
			printf("%f %ls\n", d->thresholds.lower_nr_thresh.value,
			       d->units);
		} else {
			printf("<ignored>\n");
		}
		printf("\t\tLower critical       : ");
		if (d->thresholds.lower_c_thresh.is_valid) {
			printf("%f %ls\n", d->thresholds.lower_c_thresh.value,
			       d->units);
		} else {
			printf("<ignored>\n");
		}
		printf("\t\tLower non-critical   : ");
		if (d->thresholds.lower_nc_thresh.is_valid) {
			printf("%f %ls\n", d->thresholds.lower_nc_thresh.value,
			       d->units);
		} else {
			printf("<ignored>\n");
		}

		printf("\n");
	}

	res = bmcThresholdsTripped(values, &tripped, &num_tripped);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy_values,
			     "Obtaining tripped thresholds");

	printf("Number of tripped thresholds: %d\n", num_tripped);

	for (i = 0; i < num_tripped; i++) {
		uint32_t sens = tripped[i].sensor_number;
		printf("\t%s sensor %d ('%s') tripped: ",
		       tripped[i].type == BMC_THERMAL ? "THERMAL" : "POWER",
		       sens, details[sens].name);
		if (BMC_UPPER_NR_TRIPPED(tripped[i])) {
			printf("UNR ");
		}
		if (BMC_UPPER_C_TRIPPED(tripped[i])) {
			printf("UC ");
		}
		if (BMC_UPPER_NC_TRIPPED(tripped[i])) {
			printf("UNC ");
		}
		if (BMC_LOWER_NR_TRIPPED(tripped[i])) {
			printf("LNR ");
		}
		if (BMC_LOWER_C_TRIPPED(tripped[i])) {
			printf("LC ");
		}
		if (BMC_LOWER_NC_TRIPPED(tripped[i])) {
			printf("LNC ");
		}
		printf("\n\n");
	}

	if (tripped) {
		free(tripped);
	}

out_destroy_values:
	res = bmcDestroySensorValues(&values);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy_details, "Destroying values");

out_destroy_details:
	free(details);

out_destroy_sdrs:
	res = bmcDestroySDRs(&sdrs);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "Destroying SDRs");

out_destroy:
	res = fpgaDestroyProperties(&filter);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy_token,
			     "destroying properties object");

out_destroy_token:
	res = fpgaDestroyToken(&token);
	ON_FPGAINFO_ERR_GOTO(res, out_err, "destroying properties object");

out_err:
	return res;
}
