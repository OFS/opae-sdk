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
//#include "fpgainfo.h"
#include "bmcinfo.h"
#include "bmcdata.h"
#include "safe_string/safe_string.h"
#include <opae/fpga.h>
#include <unistd.h>
#include <uuid/uuid.h>
//#include "sysinfo.h"
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <wchar.h>
#include "types_int.h"
#include "sysfs_int.h"
#include "log_int.h"
#include "common_int.h"
#include <stdio.h>

#include "metrics_int.h"
#include "opae/metrics.h"
#include "metrics/vector.h"

#ifdef DEBUG
#define DBG_PRINT(...)                                                         \
do { \
	 \
	printf(__VA_ARGS__);                                           \
	fflush(stdout);                                                \
	fflush(stderr);                                                \
} while (0)
#else
#define DBG_PRINT(...)                                             \
	do { \
		fflush(stdout);                                                \
		fflush(stderr);                                                \
} while (0)
#endif



/*
* Print help
*/
void bmc_help(void)
{
	printf("\nPrint all Board Management Controller sensor values\n"
		"        fpgainfo bmc [-h]\n"
		"                -h,--help           Print this help\n"
		"\n");
}

fpga_result bmc_filter(fpga_properties *filter, int argc, char *argv[])
{
	(void) argc;
	(void) argv;
	fpga_result res = FPGA_OK;
	res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
//	fpgainfo_print_err("setting type to FPGA_DEVICE", res);
	return res;
}

Values *bmc_build_values(sensor_reading *reading, sdr_header *header,
	sdr_key *key, sdr_body *body)
{
	Values *val = (Values *) calloc(1, sizeof(Values));

	(void) header;

	if (NULL == val)
		return NULL;

	val->is_valid = true;

	if (!reading->sensor_validity.sensor_state.sensor_scanning_disabled) {
		val->annotation_1 = "scanning enabled";
		// val->is_valid = false;
	}
	if (reading->sensor_validity.sensor_state.reading_state_unavailable) {
		val->annotation_2 = "reading state unavailable";
		val->is_valid = false;
	}
	if (!reading->sensor_validity.sensor_state.event_messages_disabled) {
		val->annotation_3 = "event messages enabled";
	}

	if (body->id_string_type_length_code.bits.format == ASCII_8) {
		uint8_t len =
			body->id_string_type_length_code.bits.len_in_characters;
		if ((len == 0x1f) || (len == 0)) {
			val->name = strdup("**INVALID**");
			val->is_valid = false;
		} else {
			val->name = strdup((char *) &body->string_bytes[0]);
		}
	} else {
		val->name = strdup("**String type unimplemented**");
		fprintf(stderr, "String type other than ASCII8\n");
	}

	val->sensor_number = key->sensor_number;
	val->sensor_type = body->sensor_type;

	switch (body->sensor_units_1.bits.analog_data_format) {
	case 0x0: // unsigned
	case 0x1: // 1's compliment (signed)
	case 0x2: // 2's complement (signed)
		break;
	case 0x3: // Does not return a reading
		val->is_valid = false;
		break;
	}

	if (body->sensor_units_2 < max_base_units) {
		val->units = base_units[body->sensor_units_2];

		val->metrics_units = body->sensor_units_2;
	} else {
		val->units = L"*OUT OF RANGE*";
	}

	//int tmp = bmcdata_verbose;
	//bmcdata_verbose = 0;
	calc_params(body, val);
	//bmcdata_verbose = tmp;

	// val->value.i_val = (uint64_t)reading->sensor_reading;
	// val->val_type = SENSOR_INT;

	val->raw_value = (uint64_t) reading->sensor_reading;
	val->val_type = SENSOR_FLOAT;
	val->value.f_val = getvalue(val, val->raw_value);

	return val;
}

static int read_struct(int fd, char *data, int *offset, size_t size, char *path)
{
	int bytes_read = 0;
	DBG_PRINT("read_struct: data=%p, offset=%d, size=%d, path='%s'\n", data,
		*offset, (int) size, path);
	int old_offset = *offset;
	do {
		bytes_read = read(fd, data, size);
		DBG_PRINT("read_struct: bytes_read=%d\n", bytes_read);
		if (bytes_read < 0) {
			if (errno == EINTR) {
				bytes_read = 1; // Fool the while loop
				continue;
			}
			FPGA_MSG("Read from %s failed", path);
			return -1;
		}
		*offset += bytes_read;
		if (((size_t) *offset > size + old_offset) || (*offset <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
			return -1;
		}
	} while (((size_t) *offset < (size + old_offset)) && (bytes_read != 0));

	return bytes_read;
}

 void bmc_read_sensor_data(const char *sysfspath, Values **vals)
{
	char sdr_path[SYSFS_PATH_MAX];
	char sensor_path[SYSFS_PATH_MAX];
	int sdr_fd;
	int sensor_fd;
	char sdr_data[sizeof(sdr_header) +sizeof(sdr_key) +sizeof(sdr_body)];
	char sensor_data[sizeof(sensor_reading)];
	int sdr_offset = 0;
	int sensor_offset = 0;
	int num_sensors = 0;
	Values *last_val = NULL;

	if ((NULL == sysfspath) || (NULL == vals))
		return;
	*vals = NULL;

	snprintf_s_ss(sdr_path, sizeof(sdr_path), "%s/%s", sysfspath,
		SYSFS_SDR_FILE);

	snprintf_s_ss(sensor_path, sizeof(sensor_path), "%s/%s", sysfspath,
		SYSFS_SENSOR_FILE);

	sensor_fd = open(sensor_path, O_RDONLY);
	if (sensor_fd < 0) {
		FPGA_MSG("open(%s) failed", sensor_path);
		return;
	}

	if ((off_t) -1 == lseek(sensor_fd, 0, SEEK_SET)) {
		FPGA_MSG("seek failed");
		close(sensor_fd);
		return;
	}

	sdr_fd = open(sdr_path, O_RDONLY);
	if (sdr_fd < 0) {
		FPGA_MSG("open(%s) failed", sdr_path);
		return;
	}

	if ((off_t) -1 == lseek(sdr_fd, 0, SEEK_SET)) {
		FPGA_MSG("seek failed");
		close(sdr_fd);
		return;
	}

	sdr_header *header = (sdr_header *) &sdr_data[0];
	sdr_key *key = (sdr_key *) &sdr_data[sizeof(sdr_header)];
	sdr_body *body =
		(sdr_body *) &sdr_data[sizeof(sdr_header) +sizeof(sdr_key)];
	sensor_reading *reading = (sensor_reading *) &sensor_data[0];

	for (num_sensors = 0;; num_sensors++) {
		int ret;

		// Read each sensor's data
		ret = read_struct(sensor_fd, (char *) reading, &sensor_offset,
			sizeof(sensor_reading), sensor_path);

		if (0 == ret)
			break;

		if (ret < 0) {
			close(sensor_fd);
			close(sdr_fd);
			return;
		}

		// Read the SDR record for this sensor

		ret = read_struct(sdr_fd, (char *) header, &sdr_offset,
			sizeof(sdr_header), sdr_path);

		if (0 == ret)
			break;

		if (ret < 0) {
			close(sensor_fd);
			close(sdr_fd);
			return;
		}

		ret = read_struct(sdr_fd, (char *) key, &sdr_offset,
			sizeof(sdr_key), sdr_path);

		if (0 == ret)
			break;

		if (ret < 0) {
			close(sensor_fd);
			close(sdr_fd);
			return;
		}

		ret = read_struct(sdr_fd, (char *) body, &sdr_offset,
			header->record_length - sizeof(sdr_key),
			sdr_path);

		if (0 == ret)
			break;

		if (ret < 0) {
			close(sensor_fd);
			close(sdr_fd);
			return;
		}

		// Build a Values struct for display
		Values *val = bmc_build_values(reading, header, key, body);

		if (NULL == val) {
			close(sensor_fd);
			close(sdr_fd);
			return;
		}

		if (NULL == last_val) {
			*vals = val;
		} else {
			last_val->next = val;
		}
		last_val = val;

	//	bmc_print_detail(reading, header, key, body);
	}
}

fpga_result bmc_print_values(const char *sysfs_path, BMC_TYPE type)
{
	fpga_result res = FPGA_OK;
	Values *vals = NULL;
	Values *vptr;

	if (NULL == sysfs_path)
		return FPGA_INVALID_PARAM;

	bmc_read_sensor_data(sysfs_path, &vals);

	for (vptr = vals; NULL != vptr; vptr = vptr->next) {
		if (!(((BMC_THERMAL == type) && (SDR_SENSOR_IS_TEMP(vptr)))
			|| ((BMC_POWER == type) && (SDR_SENSOR_IS_POWER(vptr)))
			|| (BMC_SENSORS == type)))
			continue;


		printf("(%2d) %-24s : ", vptr->sensor_number, vptr->name);
		if (!vptr->is_valid) {
			printf("No reading");
		} else {

			if (vptr->val_type == SENSOR_INT) {
				printf("%" PRIu64 " %ls", vptr->value.i_val,
					vptr->units);
			} else if (vptr->val_type == SENSOR_FLOAT) {
				printf("%.2lf %ls", vptr->value.f_val,
					vptr->units);
			}
		}

		if (vptr->annotation_1) {
			printf(" (%s)", vptr->annotation_1);
		}
		if (vptr->annotation_2) {
			printf(" (%s)", vptr->annotation_2);
		}
		if (vptr->annotation_3) {
			printf(" (%s)", vptr->annotation_3);
		}


		printf("\n");

	}

	return res;
}
/*
void print_bmc_info(const char *sysfspath)
{
	device_id dev;
	powerdown_cause pd;
	reset_cause rst;
	int fd = -1;
	char path[SYSFS_PATH_MAX];
	int ret = 0;
	int off = 0;

	printf("Board Management Controller, microcontroller FW version ");

	snprintf_s_ss(path, sizeof(path), "%s/%s", sysfspath, SYSFS_DEVID_FILE);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("unavailable\n");
	}
	else {
		if ((off_t) -1 != lseek(fd, 0, SEEK_SET)) {
			ret = read_struct(fd, (char *) &dev, &off, sizeof(dev),
				path);
			if (0 == ret) {
				printf("unavailable (I/O error)\n");
			}
			else {
				int version = dev.aux_fw_rev_0_7
					| (dev.aux_fw_rev_8_15 << 8)
					| (dev.aux_fw_rev_16_23 << 16)
					| (dev.aux_fw_rev_24_31 << 24);
				printf("%d\n", version);
			}
		}
		else {
			printf("I/O Error\n");
		}
		close(fd);
	}

	off = 0;
	printf("Last Power Down Cause: ");

	snprintf_s_ss(path, sizeof(path), "%s/%s", sysfspath, SYSFS_PWRDN_FILE);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("unavailable\n");
	}
	else {
		if ((off_t) -1 != lseek(fd, 0, SEEK_SET)) {
			ret = read_struct(fd, (char *) &pd, &off,
				sizeof(pd) -sizeof(pd.message)
				- sizeof(pd.count),
				path);
			if (0 == ret) {
				printf("unavailable (I/O error)\n");
			}
			else if (pd.completion_code == 0) {
				char msg[50];
				ret = read_struct(fd, (char *) &pd.count, &off,
					sizeof(pd.count), path);
				ret = read_struct(fd, msg, &off, pd.count,
					path);
				msg[pd.count] = '\0';
				printf("%s\n", ret ? msg : "unavailable");
			}
			else {
				printf("unavailable (cc = %d)\n",
					pd.completion_code);
			}
		}
		else {
			printf("I/O Error\n");
		}
		close(fd);
	}

	printf("Last Reset Cause: ");

	snprintf_s_ss(path, sizeof(path), "%s/%s", sysfspath, SYSFS_RESET_FILE);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		printf("unavailable (can't open)\n");
	}
	else {
		if ((off_t) -1 != lseek(fd, 0, SEEK_SET)) {
			ret = read_struct(fd, (char *) &rst, &off, sizeof(rst),
				path);
			if (0 == ret) {
				rst.completion_code = -1;
			}
			print_reset_cause(&rst);
		}
		else {
			printf("I/O Error\n");
		}
		close(fd);
	}
}
*/
/*
fpga_result bmc_command(fpga_token *tokens, int num_tokens, int argc,
	char *argv[])
{
	(void) argc;
	(void) argv;

	fpga_result res = FPGA_OK;
	fpga_properties props;
	int verbose_opt = 0;

	optind = 0;
	struct option longopts[] = {
		{ "help", no_argument, NULL, 'h' },
		{ "verbose", no_argument, NULL, 'v' }, // **UNDOCUMENTED**
		{ 0, 0, 0, 0 },
	};

	int getopt_ret;
	int option_index;

	while (-1
		!= (getopt_ret = getopt_long(argc, argv, ":h", longopts,
		&option_index))) {
		const char *tmp_optarg = optarg;

		if ((optarg) && ('=' == *tmp_optarg)) {
			++tmp_optarg;
		}

		switch (getopt_ret) {
		case 'h':
			bmc_help();
			return res;

		case 'v':
			verbose_opt = 1;
			break;

		case ':':
			fprintf(stderr, "Missing option argument\n");
			bmc_help();
			return FPGA_INVALID_PARAM;

		case '?':
		default:
			fprintf(stderr, "Invalid cmdline options\n");
			bmc_help();
			return FPGA_INVALID_PARAM;
		}
	}

	int i = 0;
	for (i = 0; i < num_tokens; ++i) {
		res = fpgaGetProperties(tokens[i], &props);
		ON_FPGAINFO_ERR_GOTO(res, out_destroy,
			"reading properties from token");
		const char *sysfs_path =
			get_sysfs_path(props, FPGA_DEVICE, NULL);
		//print_bmc_info(sysfs_path);

		Values *vals = NULL;
		int tmp = bmcdata_verbose;
		if (verbose_opt) {
			bmcdata_verbose = verbose_opt;
			bmc_read_sensor_data(sysfs_path, &vals);
			bmcdata_verbose = tmp;
		}
		else {
			bmc_print_values(sysfs_path, BMC_SENSORS);
		}
		fpgaDestroyProperties(&props);
	}

	return res;

out_destroy:
	fpgaDestroyProperties(&props);
	return res;
}
*/
