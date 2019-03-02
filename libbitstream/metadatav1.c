// Copyright(c) 2019, Intel Corporation
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
#include <opae/log.h>

#include "bitstream.h"
#include "metadatav1.h"
#include "bits_utils.h"
#include "safe_string/safe_string.h"

STATIC fpga_result
opae_bitstream_parse_accelerator_cluster_v1(json_object *j_cluster,
			    opae_metadata_accelerator_cluster_v1 *cluster)
{
	fpga_result res;

	res = opae_bitstream_get_json_int(j_cluster,
					  "total-contexts",
					  &cluster->total_contexts);
	if (res != FPGA_OK)
		return res;

	res = opae_bitstream_get_json_string(j_cluster,
					     "name",
					     &cluster->name);
	if (res != FPGA_OK)
		goto out_free;

	res = opae_bitstream_get_json_string(j_cluster,
					     "accelerator-type-uuid",
					     &cluster->accelerator_type_uuid);
	if (res != FPGA_OK)
		goto out_free;

	return FPGA_OK;

out_free:
	if (cluster->name) {
		free(cluster->name);
		cluster->name = NULL;
	}
	return res;
}

STATIC fpga_result opae_bitstream_parse_afu_image_v1(json_object *j_afu_image,
					opae_metadata_afu_image_v1 *img,
					fpga_guid pr_interface_id)
{
	fpga_result res;
	json_object *j_accelerator_clusters = NULL;
	int i = 0;

	res = opae_bitstream_get_json_int(j_afu_image,
					  "clock-frequency-high",
					  &img->clock_frequency_high);
	if (res != FPGA_OK)
		return res;

	res = opae_bitstream_get_json_int(j_afu_image,
					  "clock-frequency-low",
					  &img->clock_frequency_low);
	if (res != FPGA_OK)
		return res;

	res = opae_bitstream_get_json_int(j_afu_image,
					  "power",
					  &img->power);
	if (res != FPGA_OK)
		return res;

	res = opae_bitstream_get_json_int(j_afu_image,
					  "magic-no",
					  &img->magic_no);
	if (res != FPGA_OK)
		return res;

	if (img->magic_no !=
	    OPAE_LEGACY_BITSTREAM_MAGIC) {
		OPAE_ERR("metadata: invalid GBS magic: %d",
			 img->magic_no);
		res = FPGA_EXCEPTION;
		goto out_free;
	}

	res = opae_bitstream_get_json_string(j_afu_image,
					     "interface-uuid",
					     &img->interface_uuid);
	if (res != FPGA_OK)
		goto out_free;

	if (uuid_parse(img->interface_uuid, pr_interface_id)) {
		OPAE_ERR("metadata: uuid_parse failed");
		res = FPGA_EXCEPTION;
		goto out_free;
	}

	if (!json_object_object_get_ex(j_afu_image,
				       "accelerator-clusters",
				       &j_accelerator_clusters)) {
		OPAE_ERR("metadata: failed to find "
			 "\"accelerator-clusters\" key");
		res = FPGA_EXCEPTION;
		goto out_free;
	}

	if (!json_object_is_type(j_accelerator_clusters, json_type_array)) {
		OPAE_ERR("metadata: \"accelerator-clusters\" key not array");
		res = FPGA_EXCEPTION;
		goto out_free;
	}

	img->num_clusters = json_object_array_length(j_accelerator_clusters);

	img->accelerator_clusters =
		calloc(img->num_clusters,
		       sizeof(opae_metadata_accelerator_cluster_v1));
	if (!img->accelerator_clusters) {
		OPAE_ERR("calloc failed");
		res = FPGA_NO_MEMORY;
		goto out_free;
	}

	for (i = 0 ; i < img->num_clusters ; ++i) {
		json_object *j_cluster =
			json_object_array_get_idx(j_accelerator_clusters, i);

		res = opae_bitstream_parse_accelerator_cluster_v1(j_cluster,
					&img->accelerator_clusters[i]);
		if (res != FPGA_OK)
			goto out_free;
	}

	return FPGA_OK;

out_free:
	if (img->interface_uuid) {
		free(img->interface_uuid);
		img->interface_uuid = NULL;
	}
	if (img->accelerator_clusters) {
		int j;
		for (j = 0 ; j < i ; ++j) {
			free(img->accelerator_clusters[j].name);
			free(img->accelerator_clusters[j].accelerator_type_uuid);
		}

		free(img->accelerator_clusters);
		img->accelerator_clusters = NULL;
	}
	return res;
}

opae_bitstream_metadata_v1 *
opae_bitstream_parse_metadata_v1(json_object *root,
				 fpga_guid pr_interface_id)
{
	opae_bitstream_metadata_v1 *md;
	fpga_result res;
	json_object *j_afu_image = NULL;

	md = calloc(1, sizeof(opae_bitstream_metadata_v1));
	if (!md) {
		OPAE_ERR("calloc failed");
		return NULL;
	}

	md->version = 1;

	res = opae_bitstream_get_json_string(root,
					     "platform-name",
					     &md->platform_name);
	if (res != FPGA_OK)
		goto out_free;

	if (!json_object_object_get_ex(root,
				       "afu-image",
				       &j_afu_image)) {
		OPAE_ERR("metadata: failed to find \"afu-image\" key");
		goto out_free;
	}

	res = opae_bitstream_parse_afu_image_v1(j_afu_image,
						&md->afu_image,
						pr_interface_id);
	if (res != FPGA_OK)
		goto out_free;

	return md;

out_free:
	if (md->platform_name)
		free(md->platform_name);
	free(md);
	return NULL;
}

void opae_bitstream_release_metadata_v1(opae_bitstream_metadata_v1 *md)
{
	int i;

	if (md->afu_image.accelerator_clusters) {

		for (i = 0 ; i < md->afu_image.num_clusters ; ++i) {
			opae_metadata_accelerator_cluster_v1 *c =
				&md->afu_image.accelerator_clusters[i];

			if (c->name)
				free(c->name);
			if (c->accelerator_type_uuid)
				free(c->accelerator_type_uuid);
		}

		free(md->afu_image.accelerator_clusters);
	}

	if (md->afu_image.interface_uuid)
		free(md->afu_image.interface_uuid);

	if (md->platform_name)
		free(md->platform_name);

	free(md);
}
