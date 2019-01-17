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

/*
 * reset_bmc.c : Builds and tears down context for bmc_thermal
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

#include "safe_string/safe_string.h"

#include <opae/fpga.h>
#include "bitstream-tools.h"
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <float.h>
#include "bmc_thermal.h"
#include "config_int.h"
#include "log.h"
#include "safe_string/safe_string.h"
#include "bmc/bmc.h"
#include "reset_bmc.h"

static struct timespec wait_for_card_ts = {
	.tv_sec = PACD_WAIT_FOR_CARD,
	.tv_nsec = 0,
};

#define PRINT_MSG printf
#define PRINT_ERR(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

/*
 * macro to check FPGA return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_GOTO(cond, label, desc, ...)                                        \
	do {                                                                   \
		if (cond) {                                                    \
			dlog("pacd[%d]: " desc "\n", c->PAC_index,             \
			     ##__VA_ARGS__);                                   \
			goto label;                                            \
		}                                                              \
	} while (0)

/*
 * Check for bitstream header and fill out bistream_info fields
 */
#define MAGIC 0x1d1f8680
#define MAGIC_SIZE 4
#define HEADER_SIZE 20
static int parse_metadata(struct bitstream_info *info)
{
	unsigned i;

	if (!info)
		return -EINVAL;

	if (info->data_len < HEADER_SIZE) {
		fprintf(stderr, "File too small to be GBS\n");
		return -1;
	}

	if (((uint32_t *)info->data)[0] != MAGIC) {
		fprintf(stderr, "No valid GBS header\n");
		return -1;
	}

	/* reverse byte order when reading GBS */
	for (i = 0; i < sizeof(info->interface_id); i++)
		info->interface_id[i] =
			info->data[MAGIC_SIZE + sizeof(info->interface_id) - 1
				   - i];

	info->rbf_data = &info->data[HEADER_SIZE];
	info->rbf_len = info->data_len - HEADER_SIZE;

	return 0;
}

/*
 * Read inferface id from bistream
 */
static fpga_result get_bitstream_ifc_id(const uint8_t *bitstream,
					fpga_guid *guid)
{
	fpga_result result = FPGA_EXCEPTION;
	char *json_metadata = NULL;
	uint32_t json_len = 0;
	const uint8_t *json_metadata_ptr = NULL;
	json_object *root = NULL;
	json_object *afu_image = NULL;
	json_object *interface_id = NULL;
	errno_t e;

	if (check_bitstream_guid(bitstream) != FPGA_OK)
		goto out_free;

	json_len = read_int_from_bitstream(bitstream + METADATA_GUID_LEN,
					   sizeof(uint32_t));
	if (json_len == 0) {
		PRINT_MSG("Bitstream has no metadata");
		result = FPGA_OK;
		goto out_free;
	}

	json_metadata_ptr = bitstream + METADATA_GUID_LEN + sizeof(uint32_t);

	json_metadata = (char *)malloc(json_len + 1);
	if (json_metadata == NULL) {
		PRINT_ERR("Could not allocate memory for metadata!");
		return FPGA_NO_MEMORY;
	}

	e = memcpy_s(json_metadata, json_len + 1, json_metadata_ptr, json_len);
	if (EOK != e) {
		PRINT_ERR("memcpy_s failed");
		result = FPGA_EXCEPTION;
		goto out_free;
	}
	json_metadata[json_len] = '\0';

	root = json_tokener_parse(json_metadata);

	if (root != NULL) {
		if (json_object_object_get_ex(root, GBS_AFU_IMAGE,
					      &afu_image)) {
			json_object_object_get_ex(afu_image, BBS_INTERFACE_ID,
						  &interface_id);

			if (interface_id == NULL) {
				PRINT_ERR("Invalid metadata");
				result = FPGA_INVALID_PARAM;
				goto out_free;
			}

			result = string_to_guid(
				json_object_get_string(interface_id), guid);
			if (result != FPGA_OK) {
				PRINT_ERR("Invalid BBS interface id ");
				goto out_free;
			}
		} else {
			PRINT_ERR("Invalid metadata");
			result = FPGA_INVALID_PARAM;
			goto out_free;
		}
	}

out_free:
	if (root)
		json_object_put(root);
	if (json_metadata)
		free(json_metadata);

	return result;
}


/*
 * Read bitstream from file and populate bitstream_info structure
 */
// TODO: remove this check when all bitstreams conform to JSON
// metadata spec.
static bool skip_header_checks;
int read_bitstream(const char *filename, struct bitstream_info *info)
{
	FILE *f;
	long len;
	int ret;

	if (!filename || !info)
		return -EINVAL;

	info->filename = filename;

	/* open file */
	f = fopen(filename, "rb");
	if (!f) {
		perror(filename);
		return -1;
	}

	/* get filesize */
	ret = fseek(f, 0, SEEK_END);
	if (ret < 0) {
		perror(filename);
		goto out_close;
	}
	len = ftell(f);
	if (len < 0) {
		perror(filename);
		goto out_close;
	}

	/* allocate memory */
	info->data = (uint8_t *)malloc(len);
	if (!info->data) {
		perror("malloc");
		goto out_close;
	}

	/* read bistream data */
	ret = fseek(f, 0, SEEK_SET);
	if (ret < 0) {
		perror(filename);
		goto out_free;
	}
	info->data_len = fread(info->data, 1, len, f);
	if (ferror(f)) {
		perror(filename);
		goto out_free;
	}
	if (info->data_len != (size_t)len) {
		fprintf(stderr,
			"Filesize and number of bytes read don't match\n");
		goto out_free;
	}


	if (check_bitstream_guid(info->data) == FPGA_OK) {
		skip_header_checks = true;

//		printf(" 	skip_header_checks = true;\n");

		if (get_bitstream_ifc_id(info->data, &(info->interface_id))
		    != FPGA_OK) {
			fprintf(stderr, "Invalid metadata in the bitstream\n");
			goto out_free;
		}
	}

	if (!skip_header_checks) {
		/* populate remaining bitstream_info fields */
		ret = parse_metadata(info);
		if (ret < 0)
			goto out_free;
	}

	fclose(f);
	return 0;

out_free:
	if (info->data)
		free((void *)info->data);
	info->data = NULL;
out_close:
	fclose(f);
	return -1;
}

fpga_result pacd_bmc_reinit(pacd_bmc_reset_context *ctx)
{
	struct bmc_thermal_context *c = ctx->c;
	unsigned i;
	int ret;

	fpga_properties filter = NULL;
	fpga_result res = FPGA_OK;
	uint32_t num_matches = 0;

	memset_s(&ctx->null_gbs_info, sizeof(ctx->null_gbs_info), 0);

	ON_GOTO(c->config->num_null_gbs == 0, out_exit,
		"no default bitstreams registered.");

	res = fpgaGetProperties(NULL, &filter);
	ON_GOTO(res != FPGA_OK, out_exit, "enumeration failed");

	for (i = 0; i < c->config->num_null_gbs; i++) {
		ret = read_bitstream(c->config->null_gbs[i],
				     &ctx->null_gbs_info);
		if (ret < 0) {
			dlog("pacd[%d]: \tfailed to read bitstream\n",
			     c->PAC_index);
			if (ctx->null_gbs_info.data)
				free((void *)ctx->null_gbs_info.data);
			ctx->null_gbs_info.data = NULL;
			continue;
		}

		res = fpgaClearProperties(filter);
		ON_GOTO(res != FPGA_OK, out_destroy_filter,
			"enumeration failed");

		res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
		res += fpgaPropertiesSetGUID(filter,
					     ctx->null_gbs_info.interface_id);
		ON_GOTO(res != FPGA_OK, out_destroy_filter,
			"enumeration failed");

		res = fpgaEnumerate(&filter, 1, &ctx->fme_token, 1,
				    &num_matches);
		ON_GOTO(res != FPGA_OK, out_destroy_filter,
			"enumeration failed");

		if (num_matches > 0)
			break;
	}

	res = fpgaDestroyProperties(&filter);
	ON_GOTO(res != FPGA_OK, out_exit, "enumeration failed");

	/* if we didn't find a matching FPGA, bail out */
	if (i == c->config->num_null_gbs) {
		res = FPGA_NOT_FOUND;
		goto out_exit;
	}

	ctx->gbs_found = 1;
	ctx->gbs_index = i;

	/* now, fme_token holds the token for an FPGA on our socket matching the
	 * interface ID of the default GBS */

	// Load the BMC sensor data
	uint32_t num_values;
	int done = 1;
	int first_msg = 0;
	do {
		res = bmcLoadSDRs(ctx->fme_token, &ctx->records,
				  &ctx->num_sensors);
		if (!c->config->daemon) {
			ON_GOTO(res != FPGA_OK, out_destroy_filter,
				"BMC Sensors could not be loaded");
		} else if (res != FPGA_OK) {
			if (!first_msg) {
				first_msg = 1;
				dlog("pacd[%d]: Failure to load SDRs, retrying.\n",
				     c->PAC_index);
			}
			clock_nanosleep(CLOCK_MONOTONIC, 0, &wait_for_card_ts,
					NULL);
			done = 0;
		} else {
			done = 1;
		}
	} while (!done);

	if (first_msg) {
		dlog("pacd[%d]: SDRs loaded.\n", c->PAC_index);
	}

	for (i = 0; i < c->config->num_thresholds; i++) {
		if (c->config->sensor_number[i]
		    > (int32_t)ctx->num_sensors - 1) {
			dlog("pacd[%d]: Invalid sensor number: %d.\n",
			     c->PAC_index, c->config->sensor_number[i]);
			if (!c->config->daemon) {
				goto out_destroy_sdr;
			} else {
				dlog("pacd[%d]: Sensor number %d ignored.\n",
				     c->PAC_index, c->config->sensor_number[i]);
				c->config->sensor_number[i] = 0;
				c->config->upper_trigger_value[i] = DBL_MAX;
				c->config->upper_reset_value[i] = -DBL_MAX;
				c->config->lower_trigger_value[i] = -DBL_MAX;
				c->config->lower_reset_value[i] = DBL_MAX;
			}
		}
	}

	ctx->sensor_names = (char **)calloc(ctx->num_sensors, sizeof(char *));
	uint32_t x;
	sdr_details details;
	res = bmcReadSensorValues(ctx->records, &ctx->values, &num_values);
	ON_GOTO(res != FPGA_OK, out_destroy_sdr,
		"BMC Sensor values could not be loaded");
	for (x = 0; x < ctx->num_sensors; x++) {
		res = bmcGetSDRDetails(ctx->values, x, &details);
		ON_GOTO(res != FPGA_OK, out_destroy_values,
			"BMC Sensor details could not be loaded");
		ctx->sensor_names[x] = strdup(details.name);
	}

	bmcDestroySensorValues(&ctx->values);

	return res;

out_exit:
	if (ctx->null_gbs_info.data)
		free(ctx->null_gbs_info.data);

	for (x = 0; x < ctx->num_sensors; x++) {
		if (ctx->sensor_names[x]) {
			free(ctx->sensor_names[x]);
		}
	}

	free(ctx->sensor_names);

	return res;

out_destroy_values:
	bmcDestroySensorValues(&ctx->values);

out_destroy_sdr:
	bmcDestroySDRs(&ctx->records);

out_destroy_filter:
	fpgaDestroyProperties(&filter);
	goto out_exit;
}

fpga_result pacd_bmc_shutdown(pacd_bmc_reset_context *ctx)
{
	fpga_result res = FPGA_OK;
	uint32_t x;

	if (ctx->values) {
		res = bmcDestroySensorValues(&ctx->values);
	}

	if (ctx->records) {
		res += bmcDestroySDRs(&ctx->records);
	}

	if (ctx->null_gbs_info.data)
		free(ctx->null_gbs_info.data);

	for (x = 0; x < ctx->num_sensors; x++) {
		if (ctx->sensor_names[x]) {
			free(ctx->sensor_names[x]);
		}
	}

	free(ctx->sensor_names);
	ctx->sensor_names = NULL;

	if (ctx->fme_token != NULL) {
		fpgaDestroyToken(&ctx->fme_token);
	}
	ctx->fme_token = NULL;
	ctx->gbs_found = 0;
	ctx->gbs_index = 0;
	ctx->num_sensors = 0;

	memset_s(&ctx->null_gbs_info, sizeof(struct bitstream_info), 0);

	return res;
}
