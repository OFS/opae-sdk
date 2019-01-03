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

// TODO: Remove metadata parsing code duplication by using
// metadata parsing code in FPGA API

#ifndef __BITSTREAM_TOOLS_H__
#define __BITSTREAM_TOOLS_H__

#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>
#include <json-c/json.h>

#include "opae/fpga.h"

#define METADATA_GUID                      "58656F6E-4650-4741-B747-425376303031"
#define METADATA_GUID_LEN                   16
#define GBS_AFU_IMAGE                       "afu-image"
#define BBS_INTERFACE_ID                    "interface-uuid"
#define PR_INTERFACE_ID                     "pr/interface_id"
#define INTFC_ID_LOW_LEN                    16
#define INTFC_ID_HIGH_LEN                   16
#define GUID_LEN                            36
#define AFU_NAME_LEN                        512

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


// GBS Metadata format /json
struct gbs_metadata {

	double version;                             // version

	struct afu_image_content {
		uint64_t magic_num;                 // Magic number
		char interface_uuid[GUID_LEN + 1];  // Interface id
		int clock_frequency_high;            // user clock frequency hi
		int clock_frequency_low;             // user clock frequency low
		int power;                           // power

		struct afu_clusters_content {
			char name[AFU_NAME_LEN];     // AFU Name
			int  total_contexts;         // total contexts
			char afu_uuid[GUID_LEN + 1]; // afu guid
		} afu_clusters;

	} afu_image;

};



json_bool get_json_object(json_object **object, json_object **parent,
	char *field_name)
{
	return json_object_object_get_ex(*parent, field_name, &(*object));
}

fpga_result string_to_guid(const char *guid, fpga_guid *result)
{
	if (uuid_parse(guid, *result) < 0) {
		OPAE_ERR("Error parsing guid %s\n", guid);
		return FPGA_INVALID_PARAM;
	}

	return FPGA_OK;
}

uint64_t read_int_from_bitstream(const uint8_t *bitstream, uint8_t size)
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

fpga_result check_bitstream_guid(const uint8_t *bitstream)
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


fpga_result get_fpga_interface_id(fpga_token token, fpga_guid interface_id)
{
	fpga_result result = FPGA_OK;
	fpga_result resval = FPGA_OK;
	fpga_properties filter = NULL;
	fpga_objtype objtype;
	fpga_guid guid;
	errno_t e;

	result = fpgaGetProperties(token, &filter);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get Token Properties Object \n");
		goto out;
	}

	result = fpgaPropertiesGetObjectType(filter, &objtype);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get Token Properties Object \n");
		goto out_destroy;
	}

	if (objtype != FPGA_DEVICE) {
		OPAE_ERR("Invalid FPGA object type \n");
		result = FPGA_EXCEPTION;
		goto out_destroy;
	}

	result = fpgaPropertiesGetGUID(filter, &guid);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get PR guid \n");
		goto out_destroy;
	}

	e = memcpy_s(interface_id, sizeof(fpga_guid),
		guid, sizeof(guid));
	if (EOK != e) {
		OPAE_ERR("memcpy_s failed");
		goto out_destroy;
	}

out_destroy:
	resval = (result != FPGA_OK) ? result : resval;
	result = fpgaDestroyProperties(&filter);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to destroy properties \n");
	}

out:
	resval = (result != FPGA_OK) ? result : resval;
	return resval;
}

fpga_result read_gbs_metadata(const uint8_t *bitstream,
							struct gbs_metadata *gbs_metadata)
{
	uint32_t json_len = 0;
	fpga_result result = FPGA_OK;
	const uint8_t *json_metadata_ptr = NULL;
	char *json_metadata = NULL;
	json_object *root = NULL;
	json_object *magic_num = NULL;
	json_object *interface_id = NULL;
	json_object *afu_image = NULL;
	json_object *version = NULL;
	json_object *accelerator_clusters = NULL;
	json_object *cluster = NULL;
	json_object *uuid = NULL;
	json_object *name = NULL;
	json_object *contexts = NULL;
	json_object *power = NULL;
	json_object *userclk1 = NULL;
	json_object *userclk2 = NULL;
	errno_t e;

	if (gbs_metadata == NULL) {
		OPAE_ERR("Invalid input metadata");
		return FPGA_INVALID_PARAM;
	}

	if (bitstream == NULL) {
		OPAE_ERR("Invalid input bitstream");
		return FPGA_INVALID_PARAM;
	}

	if (check_bitstream_guid(bitstream) != FPGA_OK) {
		OPAE_ERR("Failed to read GUID");
		return FPGA_INVALID_PARAM;
	}

	json_len = *((uint32_t *) (bitstream + METADATA_GUID_LEN));
	if (!json_len) {
		OPAE_ERR("Bitstream has no metadata");
		return FPGA_INVALID_PARAM;
	}
	json_metadata_ptr = bitstream + METADATA_GUID_LEN + sizeof(uint32_t);

	json_metadata = (char *) malloc(json_len + 1);
	if (!json_metadata) {
		OPAE_ERR("Could not allocate memory for metadata");
		return FPGA_NO_MEMORY;
	}

	e = memcpy_s(json_metadata, json_len + 1,
		json_metadata_ptr, json_len);
	if (EOK != e) {
		OPAE_ERR("memcpy_s failed");
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
			OPAE_ERR("No GBS version");
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
					OPAE_ERR("memcpy_s failed");
					result = FPGA_EXCEPTION;
					goto out_free;
				}
				gbs_metadata->afu_image.interface_uuid[GUID_LEN] = '\0';
			} else {
				OPAE_ERR("No interface ID found in JSON metadata");
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
			OPAE_ERR("No AFU image in metadata");
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
					OPAE_ERR("memcpy_s failed");
					result = FPGA_EXCEPTION;
					goto out_free;
				}
				gbs_metadata->afu_image.afu_clusters.afu_uuid[GUID_LEN] = '\0';
			} else {
				OPAE_ERR("No accelerator-type-uuid in JSON metadata");
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
					OPAE_ERR("memcpy_s failed");
					result = FPGA_EXCEPTION;
					goto out_free;
				}
			}

			// AFU Total number of contexts
			if (get_json_object(&contexts, &cluster, GBS_ACCELERATOR_TOTAL_CONTEXTS)) {
				gbs_metadata->afu_image.afu_clusters.total_contexts = json_object_get_int64(contexts);
			}

		} else {
			OPAE_ERR("No accelerator clusters in metadata");
			result = FPGA_INVALID_PARAM;
			goto out_free;
		}
	} else {
		OPAE_ERR("Invalid JSON in metadata");
		result = FPGA_INVALID_PARAM;
		goto out_free;
	}

out_free:
	if (root) {
		json_object_put(root);
	}

	if (json_metadata) {
		free(json_metadata);
	}

	return result;
}

#endif
