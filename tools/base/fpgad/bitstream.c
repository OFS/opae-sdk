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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <uuid/uuid.h>
#include <json-c/json.h>
#include "bitstream.h"
#include "fpgad.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("bitstream: " format, ##__VA_ARGS__)

#define METADATA_GUID     "58656F6E-4650-4741-B747-425376303031"
#define METADATA_GUID_LEN 16
#define GBS_AFU_IMAGE     "afu-image"
#define BBS_INTERFACE_ID  "interface-uuid"

/*
 * Check for bitstream header and fill out bistream_info fields
 */
#define MAGIC 0x1d1f8680
#define MAGIC_SIZE 4
#define HEADER_SIZE 20
STATIC int parse_metadata(struct bitstream_info *info)
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

STATIC fpga_result string_to_guid(const char *guid, fpga_guid *result)
{
	if (uuid_parse(guid, *result) < 0) {
		OPAE_ERR("Error parsing guid %s\n", guid);
		return FPGA_INVALID_PARAM;
	}

	return FPGA_OK;
}

STATIC fpga_result check_bitstream_guid(const uint8_t *bitstream)
{
	fpga_guid bitstream_guid;
	fpga_guid expected_guid;
	errno_t e;

	e = memcpy_s(bitstream_guid, sizeof(bitstream_guid), bitstream,
		     sizeof(fpga_guid));
	if (EOK != e) {
		OPAE_ERR("memcpy_s failed");
		return FPGA_EXCEPTION;
	}

	if (string_to_guid(METADATA_GUID, &expected_guid) != FPGA_OK)
		return FPGA_INVALID_PARAM;

	if (uuid_compare(bitstream_guid, expected_guid) != 0)
		return FPGA_INVALID_PARAM;

	return FPGA_OK;
}

STATIC uint64_t read_int_from_bitstream(const uint8_t *bitstream, uint8_t size)
{
	uint64_t ret = 0;
	switch (size) {
	case sizeof(uint8_t):
		ret = *((uint8_t *)bitstream);
		break;
	case sizeof(uint16_t):
		ret = *((uint16_t *)bitstream);
		break;
	case sizeof(uint32_t):
		ret = *((uint32_t *)bitstream);
		break;
	case sizeof(uint64_t):
		ret = *((uint64_t *)bitstream);
		break;
	default:
		OPAE_ERR("Unknown integer size");
	}

	return ret;
}

/*
 * Read inferface id from bitstream
 */
STATIC fpga_result get_bitstream_ifc_id(const uint8_t *bitstream, fpga_guid *guid)
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
STATIC int read_bitstream(const char *filename, struct bitstream_info *info)
{
	FILE *f;
	long len;
	int ret;

	if (!filename || !info)
		return -EINVAL;

	memset_s(info, sizeof(*info), 0);

	info->filename = (char *)filename;

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

int bitstr_load_bitstream(const char *file, struct bitstream_info *info)
{
	return read_bitstream(file, info);
}

void bitstr_unload_bitstream(struct bitstream_info *info)
{
	if (info->filename)
		free(info->filename);
	if (info->data)
		free(info->data);
	memset_s(info, sizeof(*info), 0);
}
