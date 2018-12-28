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
#include "fpgainfo.h"
#include "bmcinfo.h"
#include "bmcdata.h"
#include "safe_string/safe_string.h"
#include <opae/fpga.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include "sysinfo.h"
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <wchar.h>
#include <glob.h>
#include <dirent.h>
#include <ctype.h>

#ifdef DEBUG
#define DBG_PRINT(...)                                                         \
	do {                                                                   \
		printf(__VA_ARGS__);                                           \
		fflush(stdout);                                                \
		fflush(stderr);                                                \
	} while (0)
#else
#define DBG_PRINT(...)                                                         \
	do {                                                                   \
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
	(void)argc;
	(void)argv;
	fpga_result res = FPGA_OK;
	res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
	fpgainfo_print_err("setting type to FPGA_DEVICE", res);
	return res;
}

Values *bmc_build_values(sensor_reading *reading, sdr_header *header,
			 sdr_key *key, sdr_body *body)
{
	Values *val = (Values *)calloc(1, sizeof(Values));

	(void)header;

	if (NULL == val)
		return NULL;

	val->is_valid = true;

	if (!reading->sensor_validity.sensor_state.sensor_scanning_disabled) {
		val->annotation_1 = "scanning enabled";
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
			val->name = strdup((char *)&body->string_bytes[0]);
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
	} else {
		val->units = L"*OUT OF RANGE*";
	}

	int tmp = bmcdata_verbose;
	bmcdata_verbose = 0;
	calc_params(body, val);
	bmcdata_verbose = tmp;

	val->raw_value = (uint64_t)reading->sensor_reading;
	val->val_type = SENSOR_FLOAT;
	val->value.f_val = getvalue(val, val->raw_value);

	return val;
}

static int read_struct(int fd, char *data, int *offset, size_t size, char *path)
{
	int bytes_read = 0;
	DBG_PRINT("read_struct: data=%p, offset=%d, size=%d, path='%s'\n", data,
		  *offset, (int)size, path);
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
		if (((size_t)*offset > size + old_offset) || (*offset <= 0)) {
			FPGA_MSG("Unexpected size reading from %s", path);
			return -1;
		}
	} while (((size_t)*offset < (size + old_offset)) && (bytes_read != 0));

	return bytes_read;
}

static void bmc_read_sensor_data(const char *sysfspath, Values **vals)
{
	char sdr_path[SYSFS_PATH_MAX];
	char sensor_path[SYSFS_PATH_MAX];
	int sdr_fd;
	int sensor_fd;
	char sdr_data[sizeof(sdr_header) + sizeof(sdr_key) + sizeof(sdr_body)];
	char sensor_data[sizeof(sensor_reading)];
	int sdr_offset = 0;
	int sensor_offset = 0;
	int num_sensors = 0;
	Values *last_val = NULL;
	glob_t pglob;
	int gres;

	if ((NULL == sysfspath) || (NULL == vals))
		return;
	*vals = NULL;

	snprintf_s_ss(sdr_path, sizeof(sdr_path), "%s/%s", sysfspath,
		      SYSFS_SDR_FILE);

	gres = glob(sdr_path, GLOB_NOSORT, NULL, &pglob);
	if ((gres) || (1 != pglob.gl_pathc)) {
		globfree(&pglob);
		fprintf(stderr, "Cannot find SDR file (%s)\n", sdr_path);
		return;
	}

	strcpy_s(sdr_path, sizeof(sdr_path), pglob.gl_pathv[0]);
	globfree(&pglob);

	snprintf_s_ss(sensor_path, sizeof(sensor_path), "%s/%s", sysfspath,
		      SYSFS_SENSOR_FILE);

	gres = glob(sensor_path, GLOB_NOSORT, NULL, &pglob);
	if ((gres) || (1 != pglob.gl_pathc)) {
		globfree(&pglob);
		fprintf(stderr, "Cannot find Sensor file (%s)\n", sensor_path);
		return;
	}

	strcpy_s(sensor_path, sizeof(sdr_path), pglob.gl_pathv[0]);
	globfree(&pglob);

	sensor_fd = open(sensor_path, O_RDONLY);
	if (sensor_fd < 0) {
		FPGA_MSG("open(%s) failed", sensor_path);
		return;
	}

	if ((off_t)-1 == lseek(sensor_fd, 0, SEEK_SET)) {
		FPGA_MSG("seek failed");
		close(sensor_fd);
		return;
	}

	sdr_fd = open(sdr_path, O_RDONLY);
	if (sdr_fd < 0) {
		FPGA_MSG("open(%s) failed", sdr_path);
                close(sensor_fd);
		return;
	}

	if ((off_t)-1 == lseek(sdr_fd, 0, SEEK_SET)) {
		FPGA_MSG("seek failed");
                close(sensor_fd);
		close(sdr_fd);
		return;
	}

	sdr_header *header = (sdr_header *)&sdr_data[0];
	sdr_key *key = (sdr_key *)&sdr_data[sizeof(sdr_header)];
	sdr_body *body =
		(sdr_body *)&sdr_data[sizeof(sdr_header) + sizeof(sdr_key)];
	sensor_reading *reading = (sensor_reading *)&sensor_data[0];

	for (num_sensors = 0;; num_sensors++) {
		int ret;

		// Read each sensor's data
		ret = read_struct(sensor_fd, (char *)reading, &sensor_offset,
				  sizeof(sensor_reading), sensor_path);

		if (0 == ret)
			break;

		if (ret < 0) {
			close(sensor_fd);
			close(sdr_fd);
			return;
		}

		// Read the SDR record for this sensor

		ret = read_struct(sdr_fd, (char *)header, &sdr_offset,
				  sizeof(sdr_header), sdr_path);

		if (0 == ret)
			break;

		if (ret < 0) {
			close(sensor_fd);
			close(sdr_fd);
			return;
		}

		ret = read_struct(sdr_fd, (char *)key, &sdr_offset,
				  sizeof(sdr_key), sdr_path);

		if (0 == ret)
			break;

		if (ret < 0) {
			close(sensor_fd);
			close(sdr_fd);
			return;
		}

		ret = read_struct(sdr_fd, (char *)body, &sdr_offset,
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

		bmc_print_detail(reading, header, key, body);
	}
        close(sensor_fd);
        close(sdr_fd);
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

		// printf("Raw=%d, M=%f, B=%f, R_exp=%d, acc=%f, tol=%d\n",
		//       vptr->raw_value, vptr->M, vptr->B, vptr->result_exp,
		//       vptr->accuracy, vptr->tolerance);
		printf("\n");
	}

        if (vals) {
            free(vals);
        }
	return res;
}

void print_bmc_info(const char *sysfspath)
{
	device_id dev;
	powerdown_cause pd;
	reset_cause rst;
	int fd = -1;
	char path[SYSFS_PATH_MAX];
	int ret = 0;
	int off = 0;
	glob_t pglob;
	int gres = 0;

	if (!sysfspath) {
		return;
	}

	off = 0;
	fd = -1;
	printf("Board Management Controller, microcontroller FW version ");

	char buf[8];
	uint16_t devid = 0;
	uint32_t bmcfw_ver = 0;
	snprintf_s_ss(path, sizeof(path), "%s/%s", sysfspath, "../device/device");
	get_sysfs_attr(path, buf, sizeof(buf));
	devid = strtoul(buf, NULL, 16);
	if (devid != FPGA_DISCRETE_DEVICEID && devid != FPGA_INTEGRATED_DEVICEID) {
		if (0 == get_bmc_path(sysfspath, "spi", path, SYSFS_PATH_MAX)) {
			off = strlen(path);
			snprintf_s_s(path+off, sizeof(path)-off, "/%s",
						 "bmcfw_flash_ctrl/bmcfw_version");
			if (get_sysfs_attr(path, buf, sizeof(buf)) > 0) {
				bmcfw_ver = strtoul(buf, NULL, 16);
				printf("%u.%u.%u\n", (bmcfw_ver >> 16) & 0xff,
					   (bmcfw_ver >> 8) & 0xff, bmcfw_ver & 0xff);
			} else {
				printf("unavailable (I/O error)\n");
			}
		} else {
			printf("unavailable\n");
		}
		return;
	}

	snprintf_s_ss(path, sizeof(path), "%s/%s", sysfspath, SYSFS_DEVID_FILE);

	gres = glob(path, GLOB_NOSORT, NULL, &pglob);
	if (!((gres) || (1 != pglob.gl_pathc))) {
		strcpy_s(path, sizeof(path), pglob.gl_pathv[0]);

		fd = open(path, O_RDONLY);
	}

	globfree(&pglob);

	if (fd < 0) {
		printf("unavailable\n");
	} else {
		if ((off_t)-1 != lseek(fd, 0, SEEK_SET)) {
			ret = read_struct(fd, (char *)&dev, &off, sizeof(dev),
					  path);
			if (0 == ret) {
				printf("unavailable (I/O error)\n");
			} else {
				int version = dev.aux_fw_rev_0_7
					      | (dev.aux_fw_rev_8_15 << 8)
					      | (dev.aux_fw_rev_16_23 << 16)
					      | (dev.aux_fw_rev_24_31 << 24);
				printf("%d\n", version);
			}
		} else {
			printf("I/O Error\n");
		}
		close(fd);
	}

	off = 0;
	fd = -1;
	printf("Last Power Down Cause: ");

	snprintf_s_ss(path, sizeof(path), "%s/%s", sysfspath, SYSFS_PWRDN_FILE);

	gres = glob(path, GLOB_NOSORT, NULL, &pglob);
	if (!((gres) || (1 != pglob.gl_pathc))) {
		strcpy_s(path, sizeof(path), pglob.gl_pathv[0]);

		fd = open(path, O_RDONLY);
	}

	globfree(&pglob);

	if (fd < 0) {
		printf("unavailable\n");
	} else {
		if ((off_t)-1 != lseek(fd, 0, SEEK_SET)) {
			ret = read_struct(fd, (char *)&pd, &off,
					  sizeof(pd) - sizeof(pd.message)
						  - sizeof(pd.count),
					  path);
			if (0 == ret) {
				printf("unavailable (I/O error)\n");
			} else if (pd.completion_code == 0) {
				char msg[50];
				ret = read_struct(fd, (char *)&pd.count, &off,
						  sizeof(pd.count), path);
				ret = read_struct(fd, msg, &off, pd.count,
						  path);
				msg[pd.count] = '\0';
				printf("%s\n", ret ? msg : "unavailable");
			} else {
				printf("unavailable (cc = %d)\n",
				       pd.completion_code);
			}
		} else {
			printf("I/O Error\n");
		}
		close(fd);
	}

	off = 0;
	fd = -1;
	printf("Last Reset Cause: ");

	snprintf_s_ss(path, sizeof(path), "%s/%s", sysfspath, SYSFS_RESET_FILE);

	gres = glob(path, GLOB_NOSORT, NULL, &pglob);
	if (!((gres) || (1 != pglob.gl_pathc))) {
		strcpy_s(path, sizeof(path), pglob.gl_pathv[0]);

		fd = open(path, O_RDONLY);
	}

	globfree(&pglob);

	if (fd < 0) {
		printf("unavailable (can't open)\n");
	} else {
		if ((off_t)-1 != lseek(fd, 0, SEEK_SET)) {
			ret = read_struct(fd, (char *)&rst, &off, sizeof(rst),
					  path);
			if (0 == ret) {
				rst.completion_code = -1;
			}
			print_reset_cause(&rst);
		} else {
			printf("I/O Error\n");
		}
		close(fd);
	}
}

#define BMC_FW_NAME 	"bmcfw"

int get_bmc_path(const char *in_path, const char *key_str, char *out_path,
				 int size)
{
	DIR *dir = NULL;
	struct dirent *dirent = NULL;
	char path[SYSFS_PATH_MAX] = {0};
	char *substr;
	int ret = -1;
	int result;

	if (in_path == NULL || key_str == NULL || out_path == NULL)
		return ret;
	strncpy_s(path, sizeof(path), in_path, strlen(in_path));

	while (1) {
		dir = opendir(path);
		if (NULL == dir) {
			break;
		}
		while (NULL != (dirent = readdir(dir))) {
			if (EOK == strcmp_s(dirent->d_name, strlen(dirent->d_name),
								".", &result)) {
				if (result == 0)
					continue;
			}
			if (EOK == strcmp_s(dirent->d_name, strlen(dirent->d_name),
								"..", &result)) {
				if (result == 0)
					continue;
			}

			if (EOK == strstr_s(dirent->d_name, strlen(dirent->d_name),
								BMC_FW_NAME, strlen(BMC_FW_NAME), &substr)) {
				strncpy_s(out_path, size, path, strlen(path));
				ret = 0;
				break;
			}
			if (EOK == strstr_s(dirent->d_name, strlen(dirent->d_name),
								key_str, strlen(key_str), &substr)) {
				snprintf_s_s(path+strlen(path), sizeof(path)-strlen(path),
							 "/%s", dirent->d_name);
				break;
			}
		}
		closedir(dir);
		if (dirent == NULL || ret == 0)
			break;
	}
	return ret;
}


int get_sysfs_attr(const char *attr_path, char *buf, int size)
{
	int fd;
	ssize_t sz;

	fd = open(attr_path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Open %s failed\n", attr_path);
		return -1;
	}
	sz = read(fd, buf, size);
	close(fd);

	if (sz > 0) {
		buf[sz-1] = '\0';
		return (int)sz;
	} else {
		return 0;
	}
}


static inline void strlower(char *str)
{
    for (; *str!='\0'; str++)
        *str = tolower(*str);
}

BMC_TYPE get_bmc_sensor_type(sensor_attr *sensor)
{
	if (sensor->type == SENSOR_TYPE_THERMAL) {
		return BMC_THERMAL;
	} else if (sensor->type == SENSOR_TYPE_POWER ||
			   sensor->type == SENSOR_TYPE_VOLTAGE ||
			   sensor->type == SENSOR_TYPE_CURRENT) {
		return BMC_POWER;
	} else {
		return BMC_SENSORS;
	}
}

void print_sensor_verbose_value(sensor_attr *sensors, BMC_TYPE type)
{
	sensor_attr *attr;

	for (attr = sensors; attr != NULL; attr = attr->next) {
		if (type == BMC_SENSORS || type == get_bmc_sensor_type(attr)) {
			printf("Sensor ID %d:\n", attr->id);

			printf("\tSensor type '%s' (0x%x): Flag (%02xh)\n",
				   attr->stype, attr->type, attr->flag);

			printf("\tSensor name:\n");
			printf("\t\tLength is %lu bytes\n", strlen(attr->name));
			printf("\t\tString type is 8-bit ASCII\n");
			printf("\t\tName is '%s'\n", attr->name);

			if (attr->flag & ~0x1) {
				printf("\tThreshold values:\n");
				if (attr->type == SENSOR_TYPE_CLOCK ||
					attr->type == SENSOR_TYPE_OTHER) {
					if (attr->flag & SENSOR_FLAG_HIGH_FATAL)
						printf("\t\tUpper critical threshold value is %d\n",
							   attr->high_fatal.i_val);
					if (attr->flag & SENSOR_FLAG_HIGH_WARN)
						printf("\t\tUpper warning threshold value is %d\n",
							   attr->high_warn.i_val);
					if (attr->flag & SENSOR_FLAG_LOW_FATAL)
						printf("\t\tLower critical threshold value is %d\n",
							   attr->low_fatal.i_val);
					if (attr->flag & SENSOR_FLAG_LOW_WARN)
						printf("\t\tLower warning threshold value is %d\n",
							   attr->low_warn.i_val);
				    if (attr->flag & SENSOR_FLAG_HYSTERESIS)
						printf("\t\tHysteresis value is %d\n",
							   attr->hysteresis.i_val);
				} else {
					if (attr->flag & SENSOR_FLAG_HIGH_FATAL)
						printf("\t\tUpper critical threshold value is %.6f\n",
							   attr->high_fatal.f_val);
					if (attr->flag & SENSOR_FLAG_HIGH_WARN)
						printf("\t\tUpper warning threshold value is %.6f\n",
							   attr->high_warn.f_val);
					if (attr->flag & SENSOR_FLAG_LOW_FATAL)
						printf("\t\tLower critical threshold value is %.6f\n",
							   attr->low_fatal.f_val);
					if (attr->flag & SENSOR_FLAG_LOW_WARN)
						printf("\t\tLower warning threshold value is %.6f\n",
							   attr->low_warn.f_val);
				    if (attr->flag & SENSOR_FLAG_HYSTERESIS)
						printf("\t\tHysteresis value is %.6f\n",
							   attr->hysteresis.f_val);
				}
			} else {
				printf("\tNo threshold provided\n");
			}

			printf("\tSensor reading:\n");
			printf("\t\tRaw value: 0x%x\n", attr->value.r_val);
			if (attr->type == SENSOR_TYPE_CLOCK ||
				attr->type == SENSOR_TYPE_OTHER) {
				printf("\t\t\tScaled value is %d\n", attr->value.i_val);
			} else {
				printf("\t\t\tScaled value is %.6f\n", attr->value.f_val);
			}
		}
	}
}

void print_sensor_value(sensor_attr *sensors, BMC_TYPE type)
{
	sensor_attr *attr;

	for (attr = sensors; attr != NULL; attr = attr->next) {
		if (type == BMC_SENSORS || type == get_bmc_sensor_type(attr)) {
			if (attr->flag & SENSOR_FLAG_VALUE) {
				if (attr->type == SENSOR_TYPE_THERMAL) {
					printf("(%2d) %-24s : %.2f %ls\n",
						   attr->id, attr->name, attr->value.f_val,
						   L"\x00b0\x0043");
				} else if (attr->type == SENSOR_TYPE_POWER) {
					printf("(%2d) %-24s : %.2f Watts\n",
						   attr->id, attr->name, attr->value.f_val);
				} else if (attr->type == SENSOR_TYPE_VOLTAGE) {
					printf("(%2d) %-24s : %.2f Volts\n",
						   attr->id, attr->name, attr->value.f_val);
				} else if (attr->type == SENSOR_TYPE_CURRENT) {
					printf("(%2d) %-24s : %.2f Amps\n",
						   attr->id, attr->name, attr->value.f_val);
				} else if (attr->type == SENSOR_TYPE_CLOCK) {
					printf("(%2d) %-24s : %d Hz\n",
						   attr->id, attr->name, attr->value.i_val);
				} else {
					printf("(%2d) %-24s : %d\n",
						   attr->id, attr->name, attr->value.i_val);
				}
			} else {
				printf("(%2d) %-24s : N/A\n", attr->id, attr->name);
			}
		}
	}
}

#define POWER_SENSOR	"power"
#define VOLTAGE_SENSOR	"voltage"
#define CURRENT_SENSOR	"current"
#define THERMAL_SENSOR	"temperature"
#define CLOCK_SENSOR	"clock"

#define THERMAL_HIGH_LIMIT	300.00
#define THERMAL_LOW_LIMIT	-273.00
#define POWER_HIGH_LIMIT	1000.00
#define POWER_LOW_LIMIT		0.00
#define VOLTAMP_HIGH_LIMIT	100.00
#define VOLTAMP_LOW_LIMIT	-100.00

static void convert_sensor_value(sensor_attr *attr, char *value, int flag)
{
	int *r_val;
	int *i_val;
	double *f_val;

	switch (flag) {
	case SENSOR_FLAG_VALUE:
		r_val = &(attr->value.r_val);
		i_val = &(attr->value.i_val);
		f_val = &(attr->value.f_val);
		break;
	case SENSOR_FLAG_LOW_WARN:
		r_val = &(attr->low_warn.r_val);
		i_val = &(attr->low_warn.i_val);
		f_val = &(attr->low_warn.f_val);
		break;
	case SENSOR_FLAG_LOW_FATAL:
		r_val = &(attr->low_fatal.r_val);
		i_val = &(attr->low_fatal.i_val);
		f_val = &(attr->low_fatal.f_val);
		break;
	case SENSOR_FLAG_HIGH_WARN:
		r_val = &(attr->high_warn.r_val);
		i_val = &(attr->high_warn.i_val);
		f_val = &(attr->high_warn.f_val);
		break;
	case SENSOR_FLAG_HIGH_FATAL:
		r_val = &(attr->high_fatal.r_val);
		i_val = &(attr->high_fatal.i_val);
		f_val = &(attr->high_fatal.f_val);
		break;
	case SENSOR_FLAG_HYSTERESIS:
		r_val = &(attr->hysteresis.r_val);
		i_val = &(attr->hysteresis.i_val);
		f_val = &(attr->hysteresis.f_val);
		break;
	default:
		return;
	}

	*r_val = atoi(value);
	if (attr->type == SENSOR_TYPE_THERMAL) {
		*f_val = (*r_val) * 0.001;
		if (*f_val < THERMAL_LOW_LIMIT || *f_val > THERMAL_HIGH_LIMIT)
			return;
	} else if (attr->type == SENSOR_TYPE_POWER) {
		*f_val = (*r_val) * 0.001;
		if (*f_val < POWER_LOW_LIMIT || *f_val > POWER_HIGH_LIMIT)
			return;
	} else if (attr->type == SENSOR_TYPE_VOLTAGE ||
			   attr->type == SENSOR_TYPE_CURRENT) {
		*f_val = (*r_val) * 0.001;
		if (*f_val < VOLTAMP_LOW_LIMIT || *f_val > VOLTAMP_HIGH_LIMIT)
			return;
	} else {
		*i_val = *r_val;
	}
	attr->flag |= flag;
}

void build_sensor_list(const char *sensor_path, sensor_attr **head)
{
	sensor_attr *attr = (sensor_attr *)calloc(1, sizeof(sensor_attr));
	sensor_attr *prev = NULL;
	sensor_attr *cur = NULL;
	char path[SYSFS_PATH_MAX];
	char id[16] = {0};
	char type[32] = {0};
	char value[16] = {0};
	char *substr;

	if (attr == NULL)
		return;
	attr->next = NULL;
	attr->flag = 0;

	/* read sensor attributes */
	snprintf_s_ss(path, sizeof(path), "%s/%s", sensor_path, "id");
	get_sysfs_attr(path, id, sizeof(id));
	attr->id = atoi(id);

	snprintf_s_ss(path, sizeof(path), "%s/%s", sensor_path, "name");
	get_sysfs_attr(path, attr->name, sizeof(attr->name));

	snprintf_s_ss(path, sizeof(path), "%s/%s", sensor_path, "type");
	get_sysfs_attr(path, type, sizeof(type));
	strncpy_s(attr->stype, sizeof(attr->stype), type, strlen(type));
	strlower(type);
	if (EOK == strstr_s(type, strlen(type), POWER_SENSOR,
						strlen(POWER_SENSOR), &substr)) {
		attr->type = SENSOR_TYPE_POWER;
	} else if (EOK == strstr_s(type, strlen(type), VOLTAGE_SENSOR,
							   strlen(VOLTAGE_SENSOR), &substr)) {
		attr->type = SENSOR_TYPE_VOLTAGE;
	} else if (EOK == strstr_s(type, strlen(type), CURRENT_SENSOR,
							   strlen(CURRENT_SENSOR), &substr)) {
		attr->type = SENSOR_TYPE_CURRENT;
	} else if (EOK == strstr_s(type, strlen(type), CLOCK_SENSOR,
							   strlen(CLOCK_SENSOR), &substr)) {
		attr->type = SENSOR_TYPE_CLOCK;
	} else if (EOK == strstr_s(type, strlen(type), THERMAL_SENSOR,
						strlen(THERMAL_SENSOR), &substr)) {
		attr->type = SENSOR_TYPE_THERMAL;
	} else {
		attr->type = SENSOR_TYPE_OTHER;
	}

	snprintf_s_ss(path, sizeof(path), "%s/%s", sensor_path, "value");
	if (get_sysfs_attr(path, value, sizeof(value)) > 0) {
		convert_sensor_value(attr, value, SENSOR_FLAG_VALUE);
	}
	snprintf_s_ss(path, sizeof(path), "%s/%s", sensor_path, "low_warn");
	if (get_sysfs_attr(path, value, sizeof(value)) > 0) {
		convert_sensor_value(attr, value, SENSOR_FLAG_LOW_WARN);
	}
	snprintf_s_ss(path, sizeof(path), "%s/%s", sensor_path, "low_fatal");
	if (get_sysfs_attr(path, value, sizeof(value)) > 0) {
		convert_sensor_value(attr, value, SENSOR_FLAG_LOW_FATAL);
	}
	snprintf_s_ss(path, sizeof(path), "%s/%s", sensor_path, "high_warn");
	if (get_sysfs_attr(path, value, sizeof(value)) > 0) {
		convert_sensor_value(attr, value, SENSOR_FLAG_HIGH_WARN);
	}
	snprintf_s_ss(path, sizeof(path), "%s/%s", sensor_path, "high_fatal");
	if (get_sysfs_attr(path, value, sizeof(value)) > 0) {
		convert_sensor_value(attr, value, SENSOR_FLAG_HIGH_FATAL);
	}
	snprintf_s_ss(path, sizeof(path), "%s/%s", sensor_path, "hysteresis");
	if (get_sysfs_attr(path, value, sizeof(value)) > 0) {
		convert_sensor_value(attr, value, SENSOR_FLAG_HYSTERESIS);
	}

	/* add sensor to list */
	if (*head == NULL) {
		*head = attr;
	} else {
		for (cur = *head; cur != NULL; cur = cur->next) {
			if (attr->id < cur->id) {
				if (prev == NULL) {
					attr->next = cur;
					*head = attr;
					return;
				}
				break;
			}
			prev = cur;
		}
		attr->next = cur;
		prev->next = attr;
	}
}

void destroy_sensor_list(sensor_attr **head)
{
	sensor_attr *attr = *head;
	while (attr) {
		*head = attr->next;
		free(attr);
		attr = *head;
	}
}

#define SENSOR_NAME 	"sensor"

void print_sensor_info(const char *sysfspath, BMC_TYPE type, int verbose)
{
	DIR *dir = NULL;
	struct dirent *dirent = NULL;
	char path[SYSFS_PATH_MAX];
	char *substr;
	int len;
	int result;
	sensor_attr *sensors = NULL;

	if (!sysfspath) {
		return;
	}
	if (0 != get_bmc_path(sysfspath, "spi", path, SYSFS_PATH_MAX)) {
		fprintf(stderr, "WARNING: bmc not found\n");
		return;
	}

	len = strlen(path);
	dir = opendir(path);
	if (NULL == dir) {
		return;
	}
	while (NULL != (dirent = readdir(dir))) {
		if (EOK == strcmp_s(dirent->d_name, strlen(dirent->d_name),
							".", &result)) {
			if (result == 0)
				continue;
		}
		if (EOK == strcmp_s(dirent->d_name, strlen(dirent->d_name),
							"..", &result)) {
			if (result == 0)
				continue;
		}

		if (EOK == strstr_s(dirent->d_name, strlen(dirent->d_name),
							SENSOR_NAME, strlen(SENSOR_NAME), &substr)) {
			snprintf_s_s(path+len, sizeof(path)-len, "/%s", dirent->d_name);
			build_sensor_list(path, &sensors);
		}
	}
	if (dirent == NULL && sensors == NULL) {
		fprintf(stderr, "WARNING: sensor not found\n");
	}
	closedir(dir);

	if (verbose) {
		print_sensor_verbose_value(sensors, type);
	} else {
		print_sensor_value(sensors, type);
	}
	destroy_sensor_list(&sensors);
}


fpga_result bmc_command(fpga_token *tokens, int num_tokens, int argc,
			char *argv[])
{
	(void)argc;
	(void)argv;

	fpga_result res = FPGA_OK;
	fpga_properties props;
	int verbose_opt = 0;

	optind = 0;
	struct option longopts[] = {
		{"help", no_argument, NULL, 'h'},
		{"verbose", no_argument, NULL, 'v'}, // **UNDOCUMENTED**
		{0, 0, 0, 0},
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
		case 'h': /* help */
			bmc_help();
			return res;

		case 'v': /* verbose - UNDOCUMENTED */
			verbose_opt = 1;
			break;

		case ':': /* missing option argument */
			fprintf(stderr, "Missing option argument\n");
			bmc_help();
			return FPGA_INVALID_PARAM;

		case '?':
		default: /* invalid option */
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
		// print_bmc_info(sysfs_path);

		fpgainfo_print_common("//****** BMC SENSORS ******//", props);

		uint16_t devid = 0;
		if (FPGA_OK == fpgaPropertiesGetDeviceID(props, &devid)) {
			if (devid != FPGA_DISCRETE_DEVICEID &&
				devid != FPGA_INTEGRATED_DEVICEID) {
				print_sensor_info(sysfs_path, BMC_SENSORS, verbose_opt);
				fpgaDestroyProperties(&props);
				continue;
			}
		}

		Values *vals = NULL;
		int tmp = bmcdata_verbose;
		if (verbose_opt) {
			bmcdata_verbose = verbose_opt;
			bmc_read_sensor_data(sysfs_path, &vals);
			bmcdata_verbose = tmp;
		} else {
			bmc_print_values(sysfs_path, BMC_SENSORS);
		}
		fpgaDestroyProperties(&props);
                if (vals) {
                    free(vals);
                }
	}

	return res;

out_destroy:
	fpgaDestroyProperties(&props);
	return res;
}
