// Copyright(c) 2017-2018, Intel Corporation
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
#include <sys/types.h>
#include <sys/stat.h>

#include "safe_string/safe_string.h"
#include "opae/utils.h"

#include "common_int.h"
#include "bitstream_int.h"

#define METADATA_GUID "58656F6E-4650-4741-B747-425376303031"
#define METADATA_GUID_LEN 16
#define FPGA_GBS_6_3_0_MAGIC	0x1d1f8680 // dec: 488605312
#define PR_INTERFACE_ID 	"pr/interface_id"
#define INTFC_ID_LOW_LEN	16
#define INTFC_ID_HIGH_LEN	16
#define BUFFER_SIZE		32


// GBS json metadata
// GBS version
#define GBS_VERSION                                 "version"

// AFU image
#define GBS_AFU_IMAGE                               "afu-image"
#define GBS_MAGIC_NUM                               "magic-no"
#define BBS_INTERFACE_ID                            "interface-uuid"
#define GBS_CLOCK_FREQUENCY_HIGH                    "clock-frequency-high"
#define GBS_CLOCK_FREQUENCY_LOW                     "clock-frequency-low"
#define GBS_AFU_POWER                               "power"

// AFU Clusters
#define GBS_ACCELERATOR_CLUSTERS                    "accelerator-clusters"
#define GBS_AFU_NAME                                "name"
#define GBS_ACCELERATOR_TYPE_UUID                   "accelerator-type-uuid"
#define GBS_ACCELERATOR_TOTAL_CONTEXTS              "total-contexts"


fpga_result string_to_guid(const char *guid, fpga_guid *result)
{
	if (uuid_parse(guid, *result) < 0) {
		FPGA_MSG("Error parsing GUID %s\n", guid);
		return FPGA_INVALID_PARAM;
	}

	return FPGA_OK;
}

STATIC json_bool get_json_object(json_object **object, json_object **parent,
				char *field_name)
{
	return json_object_object_get_ex(*parent, field_name, &(*object));
}

STATIC uint64_t read_int_from_bitstream(const uint8_t *bitstream, uint8_t size)
{
	uint64_t ret = 0;
	switch (size) {

	case sizeof(uint8_t):
		ret = *((uint8_t *) bitstream);
		break;
	case sizeof(uint16_t):
		ret = *((uint16_t *) bitstream);
		break;
	case sizeof(uint32_t):
		ret = *((uint32_t *) bitstream);
		break;
	case sizeof(uint64_t):
		ret = *((uint64_t *) bitstream);
		break;
	default:
		FPGA_ERR("Unknown integer size");
	}

	return ret;
}

STATIC int64_t int64_be_to_le(int64_t val)
{
	val = ((val << 8) & 0xFF00FF00FF00FF00ULL) |
					((val >> 8) & 0x00FF00FF00FF00FFULL);
	val = ((val << 16) & 0xFFFF0000FFFF0000ULL) |
					((val >> 16) & 0x0000FFFF0000FFFFULL);
	return (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
}

fpga_result __FIXME_MAKE_VISIBLE__ get_interface_id(fpga_handle handle, uint64_t *id_l, uint64_t *id_h)
{

	struct _fpga_token  *_token;
	struct _fpga_handle *_handle = (struct _fpga_handle *)handle;
	fpga_result result = FPGA_OK;
	fpga_guid guid;
	errno_t e;

	_token = (struct _fpga_token *)_handle->token;
	if (!_token) {
		FPGA_MSG("Token is NULL");
		return FPGA_INVALID_PARAM;
	}

	if (_token->magic != FPGA_TOKEN_MAGIC) {
		FPGA_MSG("Invalid token in handle");
		return FPGA_INVALID_PARAM;
	}

	if (id_l == NULL || id_h == NULL) {
		FPGA_MSG("id_l or id_h are NULL");
		return FPGA_INVALID_PARAM;
	}

	// PR Interface id
	result = sysfs_get_interface_id(_token, guid);
	if (FPGA_OK != result) {
		FPGA_ERR("Failed to get PR interface id");
		return FPGA_EXCEPTION;
	}

	e = memcpy_s(id_h, sizeof(id_h),
		guid, sizeof(uint64_t));
	if (EOK != e) {
		FPGA_ERR("memcpy_s failed");
		return FPGA_EXCEPTION;
	}

	*id_h = int64_be_to_le(*id_h);

	e = memcpy_s(id_l, sizeof(id_l),
		guid + sizeof(uint64_t), sizeof(uint64_t));
	if (EOK != e) {
		FPGA_ERR("memcpy_s failed");
		return FPGA_EXCEPTION;
	}
	*id_l = int64_be_to_le(*id_l);

	return FPGA_OK;
}

fpga_result check_interface_id(fpga_handle handle,
				uint32_t bitstream_magic_no,
				uint64_t ifid_l, uint64_t ifid_h)
{
	uint64_t intfc_id_l = 0;
	uint64_t intfc_id_h = 0;
	fpga_result result  = FPGA_OK;

	if (bitstream_magic_no != FPGA_GBS_6_3_0_MAGIC) {
		FPGA_MSG("Invalid bitstream magic number");
		return FPGA_NOT_FOUND;
	}

	if (get_interface_id(handle, &intfc_id_l, &intfc_id_h)) {
		FPGA_MSG("Get interface ID failed");
		return FPGA_NOT_FOUND;
	}

	if ((ifid_l != intfc_id_l) ||
		(ifid_h != intfc_id_h)) {
		FPGA_MSG("Interface id doesn't match metadata");
		return FPGA_NOT_FOUND;
	}

	return result;
}

//TODO: this shouldn't be exported once all bistreams use JSON metadata format
fpga_result __FPGA_API__ check_bitstream_guid(const uint8_t *bitstream)
{
	fpga_guid bitstream_guid;
	fpga_guid expected_guid;
	errno_t e;

	e = memcpy_s(bitstream_guid, sizeof(bitstream_guid),
			bitstream, sizeof(fpga_guid));
	if (EOK != e) {
		FPGA_ERR("memcpy_s failed");
		return FPGA_EXCEPTION;
	}

	if (string_to_guid(METADATA_GUID, &expected_guid) != FPGA_OK)
		return FPGA_INVALID_PARAM;

	if (uuid_compare(bitstream_guid, expected_guid) != 0)
		return FPGA_INVALID_PARAM;

	return FPGA_OK;
}

int get_bitstream_header_len(const uint8_t *bitstream)
{
	uint32_t json_len = 0;

	if (check_bitstream_guid(bitstream) != FPGA_OK)
		return -1;

	json_len = read_int_from_bitstream(bitstream + METADATA_GUID_LEN, sizeof(uint32_t));

	return (METADATA_GUID_LEN + sizeof(uint32_t) + json_len);
}

int32_t get_bitstream_json_len(const uint8_t *bitstream)
{
	uint32_t json_len = 0;

	if (check_bitstream_guid(bitstream) != FPGA_OK)
		return -1;

	json_len = read_int_from_bitstream(bitstream + METADATA_GUID_LEN, sizeof(uint32_t));

	return json_len;
}

fpga_result validate_bitstream_metadata(fpga_handle handle,
			const uint8_t *bitstream)
{
	fpga_result result = FPGA_EXCEPTION;
	char *json_metadata = NULL;
	uint32_t json_len = 0;
	uint32_t bitstream_magic_no = 0;
	uint64_t ifc_id_val_l, ifc_id_val_h;
	const uint8_t *json_metadata_ptr = NULL;
	json_object *root = NULL;
	json_object *afu_image = NULL, *magic_no = NULL;
	json_object *interface_id = NULL;
	fpga_guid expected_guid;
	errno_t e;

	if (check_bitstream_guid(bitstream) != FPGA_OK)
		goto out_free;

	json_len = read_int_from_bitstream(bitstream + METADATA_GUID_LEN, sizeof(uint32_t));
	if (json_len == 0) {
		FPGA_MSG("Bitstream has no metadata");
		result = FPGA_OK;
		goto out_free;
	}

	json_metadata_ptr = bitstream + METADATA_GUID_LEN + sizeof(uint32_t);

	json_metadata = (char *) malloc(json_len + 1);
	if (json_metadata == NULL) {
		FPGA_ERR("Could not allocate memory for metadata");
		return FPGA_NO_MEMORY;
	}

	e = memcpy_s(json_metadata, json_len+1,
			json_metadata_ptr, json_len);
	if (EOK != e) {
		FPGA_ERR("memcpy_s failed");
		result = FPGA_EXCEPTION;
		goto out_free;
	}
	json_metadata[json_len] = '\0';

	root = json_tokener_parse(json_metadata);

	if (root != NULL) {
		if (get_json_object(&afu_image, &root, GBS_AFU_IMAGE)) {
			get_json_object(&magic_no, &afu_image, GBS_MAGIC_NUM);
			get_json_object(&interface_id, &afu_image,
					BBS_INTERFACE_ID);

			if (magic_no == NULL || interface_id == NULL) {
				FPGA_ERR("Invalid metadata");
				result = FPGA_INVALID_PARAM;
				goto out_free;
			}

			result = string_to_guid(
				json_object_get_string(interface_id),
				&expected_guid);
			if (result != FPGA_OK) {
				FPGA_ERR("Invalid BBS interface ID");
				goto out_free;
			}

			e = memcpy_s(&ifc_id_val_h, sizeof(ifc_id_val_h),
					expected_guid, sizeof(uint64_t));
			if (EOK != e) {
				FPGA_ERR("memcpy_s failed");
				result = FPGA_EXCEPTION;
				goto out_free;
			}
			ifc_id_val_h = int64_be_to_le(ifc_id_val_h);

			e = memcpy_s(&ifc_id_val_l, sizeof(ifc_id_val_l),
					expected_guid + sizeof(uint64_t),
					sizeof(uint64_t));
			if (EOK != e) {
				FPGA_ERR("memcpy_s failed");
				result = FPGA_EXCEPTION;
				goto out_free;
			}
			ifc_id_val_l = int64_be_to_le(ifc_id_val_l);

			bitstream_magic_no = json_object_get_int(magic_no);

			result = check_interface_id(handle, bitstream_magic_no,
						ifc_id_val_l, ifc_id_val_h);

			if (result != FPGA_OK) {
				FPGA_ERR("Interface ID check failed");
				goto out_free;
			}
		} else {
			FPGA_ERR("Invalid metadata");
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

fpga_result read_gbs_metadata(const uint8_t *bitstream,
				struct gbs_metadata *gbs_metadata)
{
	uint32_t json_len                   = 0;
	fpga_result result                  = FPGA_OK;
	const uint8_t *json_metadata_ptr    = NULL;
	char *json_metadata                 = NULL;
	json_object *root                   = NULL;
	json_object *magic_num              = NULL;
	json_object *interface_id           = NULL;
	json_object *afu_image              = NULL;
	json_object *version                = NULL;
	json_object *accelerator_clusters   = NULL;
	json_object *cluster                = NULL;
	json_object *uuid                   = NULL;
	json_object *name                   = NULL;
	json_object *contexts               = NULL;
	json_object *power                  = NULL;
	json_object *userclk1               = NULL;
	json_object *userclk2               = NULL;
	errno_t e;

	if (gbs_metadata == NULL) {
		FPGA_ERR("Invalid input metadata");
		return FPGA_INVALID_PARAM;
	}

	if (bitstream == NULL) {
		FPGA_ERR("Invalid input bitstream");
		return FPGA_INVALID_PARAM;
	}

	if (check_bitstream_guid(bitstream) != FPGA_OK) {
		FPGA_ERR("Failed to read GUID");
		return FPGA_INVALID_PARAM;
	}

	json_len = *((uint32_t *) (bitstream + METADATA_GUID_LEN));
	if (!json_len) {
		FPGA_ERR("Bitstream has no metadata");
		return FPGA_INVALID_PARAM;
	}

	json_metadata_ptr = bitstream + METADATA_GUID_LEN + sizeof(uint32_t);

	json_metadata = (char *) malloc(json_len + 1);
	if (!json_metadata) {
		FPGA_ERR("Could not allocate memory for metadata");
		return FPGA_NO_MEMORY;
	}

	e = memcpy_s(json_metadata, json_len+1,
			json_metadata_ptr, json_len);
	if (EOK != e) {
		FPGA_ERR("memcpy_s failed");
		result = FPGA_EXCEPTION;
		goto out_free;
	}
	json_metadata[json_len] = '\0';

	root = json_tokener_parse(json_metadata);

	if (root) {

		// GBS version
		if (get_json_object(&version, &root, GBS_VERSION)) {
			gbs_metadata->version = json_object_get_double(version);
		} else {
			FPGA_ERR("No GBS version");
			result = FPGA_INVALID_PARAM;
			goto out_free;
		}

		// afu-image
		if (get_json_object(&afu_image, &root, GBS_AFU_IMAGE)) {

			// magic number
			if (get_json_object(&magic_num, &afu_image, GBS_MAGIC_NUM)) {
				gbs_metadata->afu_image.magic_num = json_object_get_int64(magic_num);
			}

			// Interface type GUID
			if (get_json_object(&interface_id, &afu_image, BBS_INTERFACE_ID)) {
				e = memcpy_s(gbs_metadata->afu_image.interface_uuid,
						GUID_LEN,
						json_object_get_string(interface_id),
						GUID_LEN);
				if (EOK != e) {
					FPGA_ERR("memcpy_s failed");
					result = FPGA_EXCEPTION;
					goto out_free;
				}
				gbs_metadata->afu_image.interface_uuid[GUID_LEN] = '\0';
			} else {
				FPGA_ERR("No interface ID found in JSON metadata");
				result = FPGA_INVALID_PARAM;
				goto out_free;
			}

			// AFU user clock frequency High
			if (get_json_object(&userclk1, &afu_image, GBS_CLOCK_FREQUENCY_HIGH)) {
				gbs_metadata->afu_image.clock_frequency_high = json_object_get_int64(userclk1);
			}

			// AFU user clock frequency Low
			if (get_json_object(&userclk2, &afu_image, GBS_CLOCK_FREQUENCY_LOW)) {
				gbs_metadata->afu_image.clock_frequency_low = json_object_get_int64(userclk2);
			}

			// GBS power
			if (get_json_object(&power, &afu_image, GBS_AFU_POWER)) {
				gbs_metadata->afu_image.power = json_object_get_int64(power);
			}

		} else {
			FPGA_ERR("No AFU image in metadata");
			result = FPGA_INVALID_PARAM;
			goto out_free;
		}

		// afu clusters
		if (get_json_object(&afu_image, &root, GBS_AFU_IMAGE) &&
			get_json_object(&accelerator_clusters, &afu_image, GBS_ACCELERATOR_CLUSTERS)) {

			cluster = json_object_array_get_idx(accelerator_clusters, 0);

			// AFU GUID
			if (get_json_object(&uuid, &cluster, GBS_ACCELERATOR_TYPE_UUID)) {
				e = memcpy_s(gbs_metadata->afu_image.afu_clusters.afu_uuid,
						GUID_LEN,
						json_object_get_string(uuid),
						GUID_LEN);
				if (EOK != e) {
					FPGA_ERR("memcpy_s failed");
					result = FPGA_EXCEPTION;
					goto out_free;
				}
				gbs_metadata->afu_image.afu_clusters.afu_uuid[GUID_LEN] = '\0';
			} else {
				FPGA_ERR("No accelerator-type-uuid in JSON metadata");
				result = FPGA_INVALID_PARAM;
				goto out_free;
			}

			// AFU Name
			if (get_json_object(&name, &cluster, GBS_AFU_NAME)) {
				e = memcpy_s(gbs_metadata->afu_image.afu_clusters.name,
						AFU_NAME_LEN,
						json_object_get_string(name),
						json_object_get_string_len(name));
				if (EOK != e) {
					FPGA_ERR("memcpy_s failed");
					result = FPGA_EXCEPTION;
					goto out_free;
				}
			}

			// AFU Total number of contexts
			if (get_json_object(&contexts, &cluster, GBS_ACCELERATOR_TOTAL_CONTEXTS)) {
				gbs_metadata->afu_image.afu_clusters.total_contexts = json_object_get_int64(contexts);
			}

		} else {
			FPGA_ERR("No accelerator clusters in metadata");
			result = FPGA_INVALID_PARAM;
			goto out_free;
		}
	} else {
		FPGA_ERR("Invalid JSON in metadata");
		result = FPGA_INVALID_PARAM;
		goto out_free;
	}

out_free:
	if (root)
		json_object_put(root);
	if (json_metadata)
		free(json_metadata);

	return result;
}
