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
 * @file fpgametrics.c
 * @brief A code sample illustrates the basic usage metric API.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "safe_string/safe_string.h"


/*
 * macro to check return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_ERR_GOTO(res, label, desc)              \
	do {                                       \
		if ((res) != FPGA_OK) {            \
			print_err((desc), (res));  \
			goto label;                \
		}                                  \
	} while (0)

void print_err(const char *s, fpga_result res)
{
	fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

/*
 * Global configuration of bus, set during parse_args()
 * */
struct config {
	struct target {
		int bus;
		bool fme_metrics;
		bool afu_metrics;
		int open_flags;
	} target;
}

config = {
	.target = {
		.bus = -1,
		.fme_metrics = true,
		.afu_metrics = false,
		.open_flags = 0
	}
};

// Metric Command line input help
void FpgaMetricsAppShowHelp()
{
	printf("Usage:\n");
	printf("fpgametrics\n");
	printf("<Bus>               --bus=<BUS NUMBER>           "
		"OR  -B=<BUS NUMBER>\n");
	printf("<FME metrics>       --fme-metrics               OR  -F \n");
	printf("<AFU metrics>       --afu-metrics               OR  -A \n");

	printf("\n");

}

#define GETOPT_STRING "B:FAsv"
fpga_result parse_args(int argc, char *argv[])
{
	struct option longopts[] = {
		{"bus",	   required_argument, NULL, 'B'},
		{"fme-metrics"  ,no_argument, NULL, 'F'},
		{"afu-metrics"  ,no_argument, NULL, 'A'},
		{"shared", no_argument,       NULL, 's'},
		{"version", no_argument,      NULL, 'v'},
		{NULL,     0,                 NULL,  0 },
	};
	
	int getopt_ret;
	int option_index;
	char *endptr = NULL;

	while (-1 != (getopt_ret = getopt_long(argc, argv, GETOPT_STRING,
						longopts, &option_index))) {
		const char *tmp_optarg = optarg;
		/* Checks to see if optarg is null and if not it goes to value of optarg */
		if ((optarg) && ('=' == *tmp_optarg)){
			++tmp_optarg;
		}

		switch (getopt_ret){
		case 'B': /* bus */
			if (NULL == tmp_optarg){
				return FPGA_EXCEPTION;
			}
			endptr = NULL;
			config.target.bus = (int) strtoul(tmp_optarg, &endptr, 0);
			if (endptr != tmp_optarg + strnlen(tmp_optarg, 100)) {
				fprintf(stderr, "invalid bus: %s\n", tmp_optarg);
				return FPGA_EXCEPTION;
			}
			break;
		case 's':
			config.target.open_flags |= FPGA_OPEN_SHARED;
			break;
		case 'F':
			config.target.fme_metrics = true;
			break;
		
		case 'A':
			config.target.afu_metrics = true;
			config.target.fme_metrics = false;
			break;

		case 'v':
			fprintf(stdout, "fpgametrics %s %s%s\n",
					INTEL_FPGA_API_VERSION,
					INTEL_FPGA_API_HASH,
					INTEL_FPGA_TREE_DIRTY ? "*":"");
			return -2;

		default: /* invalid option */
			fprintf(stderr, "Invalid cmdline option \n");
			return FPGA_EXCEPTION;
		}
	}
	
	return FPGA_OK;
}




	
/* function to get the bus number when there are multiple buses */
/* TODO: add device and function information */
struct bdf_info {
	uint8_t bus;
};

fpga_result get_bus_info(fpga_token tok, struct bdf_info *finfo){
	fpga_result res    = FPGA_OK;
	fpga_result resval = FPGA_OK;
	fpga_properties props;

	res = fpgaGetProperties(tok, &props);
	ON_ERR_GOTO(res, out, "reading properties from Token");

	res = fpgaPropertiesGetBus(props, &finfo->bus);
	ON_ERR_GOTO(res, out_destroy, "Reading bus from properties");

out_destroy:
	resval = (res != FPGA_OK) ? res : resval;
	res = fpgaDestroyProperties(&props);
	ON_ERR_GOTO(res, out, "fpgaDestroyProps");
out:
	resval = (res != FPGA_OK) ? res : resval;
	return resval;
}

void print_bus_info(struct bdf_info *info){
	printf("Running on bus 0x%02X. \n", info->bus);
}


int main(int argc, char *argv[])
{
	fpga_token         fpga_token;
	fpga_handle        fpga_handle;

	uint32_t num_matches_fpgas                 = 0;
	uint64_t num_metrics                       = 0;
	fpga_result resval                         = FPGA_OK;
	fpga_result res                            = FPGA_OK;
	struct bdf_info info                       = { 0 };
	uint64_t *id_array                         = NULL;
	uint64_t i                                 = 0;
	struct fpga_metric_info  *metric_info      = NULL;
	struct fpga_metric  *metric_array          = NULL;
	fpga_properties filter                     = NULL;

	if (argc < 2) {
		FpgaMetricsAppShowHelp();
		return 1;
	}
	else if (0 != (res = parse_args(argc, argv))) {
		if ((int)res > 0) {
			OPAE_ERR("Error scanning command line \n.");
		}
		return 2;
	}

	/* Get number of FPGAs in system */
	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out_exit, "creating properties object");

	if (config.target.fme_metrics) {
		res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
		ON_ERR_GOTO(res, out_destroy, "setting object type");

	} else if (config.target.afu_metrics) {

		res = fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);
		ON_ERR_GOTO(res, out_destroy, "setting object type");
	}


	if (-1 != config.target.bus) {
		res = fpgaPropertiesSetBus(filter, config.target.bus);
		ON_ERR_GOTO(res, out_destroy, "setting bus");
	}

	res = fpgaEnumerate(&filter, 1, &fpga_token, 1, &num_matches_fpgas);
	ON_ERR_GOTO(res, out_destroy, "enumerating fpga");


	if (num_matches_fpgas <= 0) {
		res = FPGA_NOT_FOUND;
	}
	ON_ERR_GOTO(res, out_exit, "no matching fpga");

	if (num_matches_fpgas > 1) {
		fprintf(stderr, "Found more than one suitable fpga. ");
		res = get_bus_info(fpga_token, &info);
		ON_ERR_GOTO(res, out_destroy_tok, "getting bus num");
		printf("Running on bus 0x%02X. \n", info.bus);
	}

	/* Open fpga  */
	res = fpgaOpen(fpga_token, &fpga_handle, config.target.open_flags);
	ON_ERR_GOTO(res, out_destroy_tok, "opening fpga");
 


	res = fpgaGetNumMetrics(fpga_handle, &num_metrics);
	ON_ERR_GOTO(res, out_close, "get num of metrics");
	printf("\n\n ------Number of Metrics Discoverd = %ld ------- \n\n\n", num_metrics);
	
	metric_info = calloc(sizeof(struct fpga_metric_info), num_metrics);
	if (metric_info == NULL) {
		printf(" Failed to allocate memroy \n");
		res = FPGA_NO_MEMORY;
		goto out_close;
	}

	res = fpgaGetMetricsInfo(fpga_handle, metric_info, &num_metrics);
	ON_ERR_GOTO(res, out_close, "get num of metrics info");

	id_array = calloc(sizeof(uint64_t), num_metrics);
	if (id_array == NULL) {
		printf(" Failed to allocate memroy \n");
		res = FPGA_NO_MEMORY;
		goto out_close;
	}

	printf("---------------------------------------------------------------------------------------------------\n");
	printf("metric_num              qualifier_name      group_name             metric_name            metric_units\n");
	printf("---------------------------------------------------------------------------------------------------\n");

	for (i = 0; i < num_metrics; i++){

		printf("%-3ld  | %-30s  | %-15s  | %-30s  | %-10s \n",
						metric_info[i].metric_num,
						metric_info[i].qualifier_name,
						metric_info[i].group_name,
						metric_info[i].metric_name,
						metric_info[i].metric_units);
		id_array[i] = i;
	}

	metric_array = calloc(sizeof(struct fpga_metric), num_metrics);
	if (metric_array == NULL) {
		printf(" Failed to allocate memroy \n");
		res = FPGA_NO_MEMORY;
		goto out_close;
	}

	res = fpgaGetMetricsByIndex(fpga_handle, id_array, num_metrics, metric_array);
	ON_ERR_GOTO(res, out_close, "get num of metrics value by index");

	printf("\n\n\n");
	printf("-------------------------------------------------------------------------------------------------\n ");
	printf("    metric_num              qualifier_name                    metric_name              value     \n ");
	printf("-------------------------------------------------------------------------------------------------\n ");

	for (i = 0; i < num_metrics; i++){

		uint64_t num = metric_array[i].metric_num;
		if (metric_info[num].metric_datatype == FPGA_METRIC_DATATYPE_INT &&
			metric_array[i].isvalid) {

				printf("%-20ld  | %-30s   | %-25s  | %ld %-20s \n ",
					metric_info[num].metric_num,
					metric_info[num].qualifier_name,
					metric_info[num].metric_name,
					metric_array[i].value.ivalue,
					metric_info[num].metric_units);
		} else if (metric_info[num].metric_datatype == FPGA_METRIC_DATATYPE_DOUBLE &&
			metric_array[i].isvalid) {

			printf("%-20ld  | %-30s   | %-25s  | %0.2f %-20s \n ",
				metric_info[num].metric_num,
				metric_info[num].qualifier_name,
				metric_info[num].metric_name,
				metric_array[i].value.dvalue,
				metric_info[num].metric_units);
		} else {

			printf("%-20ld  | %-30s   | %-25s  | %s \n ",
				metric_info[num].metric_num,
				metric_info[num].qualifier_name,
				metric_info[num].metric_name,
				"Fails to read metric value");
		}

	}

	/* Release fpga */
out_close:
	resval = (res != FPGA_OK) ? res : resval;
	if (metric_array)
		free(metric_array);

	if (metric_info)
		free(metric_info);

	if (id_array)
		free(id_array);

	res = fpgaClose(fpga_handle);
	ON_ERR_GOTO(res, out_destroy_tok, "closing fpga");

	/* Destroy token */
out_destroy_tok:
	resval = (res != FPGA_OK) ? res : resval;
	res = fpgaDestroyToken(&fpga_token);
	ON_ERR_GOTO(res, out_destroy, "destroying token");

	/* Destroy Properties */
out_destroy:
	resval = (res != FPGA_OK) ? res : resval;
	res = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(res, out_exit, "destroying properties object");
 
out_exit:
	resval = (res != FPGA_OK) ? res : resval;
	return (resval == FPGA_OK) ? 0 : 1;
}
