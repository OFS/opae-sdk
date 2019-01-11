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
 * ap6.c : handles NULL bitstream programming on AP6
 */

#include <opae/fpga.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include "ap6.h"
#include "config_int.h"
#include "log.h"
#include "safe_string/safe_string.h"
#include "fpgaconf/bitstream-tools.h"
/*
 * macro to check FPGA return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_GOTO(cond, label, desc, ...)                                      \
	do {                                                                 \
		if (cond) {                                                  \
			dlog("ap6[%i]: " desc "\n", c->socket, ## __VA_ARGS__);  \
			goto label;                                          \
		}                                                            \
	} while (0)

sem_t ap6_sem[MAX_SOCKETS];

struct bitstream_info {
	const char *filename;
	uint8_t *data;
	size_t data_len;
	uint8_t *rbf_data;
	size_t rbf_len;
	fpga_guid interface_id;
};

/*
 * Check for bitstream header and fill out bistream_info fields
 */
#define MAGIC 0x1d1f8680
#define MAGIC_SIZE 4
#define HEADER_SIZE 20
int parse_metadata(struct bitstream_info *info)
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
			info->data[MAGIC_SIZE+sizeof(info->interface_id)-1-i];

	info->rbf_data = &info->data[HEADER_SIZE];
	info->rbf_len = info->data_len - HEADER_SIZE;

	return 0;
}

/*
 * Read inferface id from bistream
 */
fpga_result get_bitstream_ifc_id(const uint8_t *bitstream, fpga_guid *guid)
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

	json_len = read_int_from_bitstream(bitstream + METADATA_GUID_LEN, sizeof(uint32_t));
	if (json_len == 0) {
		OPAE_MSG("Bitstream has no metadata");
		result = FPGA_OK;
		goto out_free;
	}

	json_metadata_ptr = bitstream + METADATA_GUID_LEN + sizeof(uint32_t);

	json_metadata = (char *) malloc(json_len + 1);
	if (json_metadata == NULL) {
		OPAE_ERR("Could not allocate memory for metadata!");
		return FPGA_NO_MEMORY;
	}

	e = memcpy_s(json_metadata, json_len+1,
			json_metadata_ptr, json_len);
	if (EOK != e) {
		OPAE_ERR("memcpy_s failed");
		result = FPGA_EXCEPTION;
		goto out_free;
	}
	json_metadata[json_len] = '\0';

	root = json_tokener_parse(json_metadata);

	if (root != NULL) {
		if (json_object_object_get_ex(root, GBS_AFU_IMAGE, &afu_image)) {
			json_object_object_get_ex(afu_image, BBS_INTERFACE_ID, &interface_id);

			if (interface_id == NULL) {
				OPAE_ERR("Invalid metadata");
				result = FPGA_INVALID_PARAM;
				goto out_free;
			}

			result = string_to_guid(json_object_get_string(interface_id), guid);
			if (result != FPGA_OK) {
				OPAE_ERR("Invalid BBS interface id ");
				goto out_free;
			}
		} else {
			OPAE_ERR("Invalid metadata");
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
//TODO: remove this check when all bitstreams conform to JSON
//metadata spec.
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

		printf(" 	skip_header_checks = true;\n");

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

void *ap6_thread(void *thread_context)
{
	struct ap6_context *c = (struct ap6_context *)thread_context;
	unsigned i;
	int ret;
	struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000 }; /* 100ms */

	fpga_token fme_token = NULL;
	fpga_handle fme_handle;
	fpga_properties filter;
	fpga_result res;
	uint32_t num_matches = 0;

	struct bitstream_info null_gbs_info ;
	memset_s(&null_gbs_info, sizeof(null_gbs_info), 0);

	ON_GOTO(c->config->num_null_gbs == 0, out_exit, "no NULL bitstreams registered.");

	res = fpgaGetProperties(NULL, &filter);
	ON_GOTO(res != FPGA_OK, out_exit, "enumeration failed");

	for (i = 0; i < c->config->num_null_gbs; i++) {
		ret = read_bitstream(c->config->null_gbs[i], &null_gbs_info);
		if (ret < 0) {
			dlog("ap6[%i]: \tfailed to read bitstream\n", c->socket);
			if (null_gbs_info.data)
				free((void *)null_gbs_info.data);
			null_gbs_info.data = NULL;
			continue;
		}

		res = fpgaClearProperties(filter);
		ON_GOTO(res != FPGA_OK, out_destroy_filter, "enumeration failed");

		res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
		res += fpgaPropertiesSetSocketID(filter, c->socket);
		res += fpgaPropertiesSetGUID(filter, null_gbs_info.interface_id);
		ON_GOTO(res != FPGA_OK, out_destroy_filter, "enumeration failed");

		res = fpgaEnumerate(&filter, 1, &fme_token, 1, &num_matches);
		ON_GOTO(res != FPGA_OK, out_destroy_filter, "enumeration failed");

		if (num_matches > 0)
			break;
	}

	res = fpgaDestroyProperties(&filter);
	ON_GOTO(res != FPGA_OK, out_exit, "enumeration failed");

	/* if we didn't find a matching FPGA, bail out */
	if (i == c->config->num_null_gbs)
		goto out_exit;

	/* now, fme_token holds the token for an FPGA on our socket matching the
	 * interface ID of the NULL GBS */

	dlog("ap6[%i]: waiting for AP6, will write the following bitstream: \"%s\"\n", c->socket, c->config->null_gbs[i]);

	while (c->config->running) {
		/* wait for event */
		ret = sem_timedwait(&ap6_sem[c->socket], &ts);

		/* if AP6 */
		if (ret == 0) {
			/* program NULL bitstream */
			dlog("ap6[%i]: writing NULL bitstreams.\n", c->socket);
			res = fpgaOpen(fme_token, &fme_handle, 0);
			if (res != FPGA_OK) {
				dlog("ap6[%i]: failed to open FPGA.\n", c->socket);
				/* TODO: retry? */
				continue;
			}

			res = fpgaReconfigureSlot(fme_handle, 0, null_gbs_info.data, null_gbs_info.data_len, 0);
			if (res != FPGA_OK) {
				dlog("ap6[%i]: failed to write bitstream.\n", c->socket);
				/* TODO: retry? */
			}

			res = fpgaClose(fme_handle);
			if (res != FPGA_OK) {
				dlog("ap6[%i]: failed to close FPGA.\n", c->socket);
			}
		}
	}

out_exit:
	if (fme_token)
		fpgaDestroyToken(&fme_token);
	if (null_gbs_info.data)
		free(null_gbs_info.data);
	return NULL;

out_destroy_filter:
	fpgaDestroyProperties(&filter);
	goto out_exit;
}
