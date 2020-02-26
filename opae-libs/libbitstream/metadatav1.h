// Copyright(c) 2019-2020, Intel Corporation
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
 * @file metadatav1.h
 * @brief GBS metadata version 1
 *
 * Defines the data types, parse function, and release function
 * for version 1 of the GBS metadata.
 *
 */

#ifndef __OPAE_METADATAV1_H__
#define __OPAE_METADATAV1_H__

#include <json-c/json.h>
#include <opae/types.h>

/**
 * Defines an AFU by its name and GUID.
 */
typedef struct _opae_metadata_accelerator_cluster_v1 {
	int total_contexts;
	char *name;
	char *accelerator_type_uuid;
} opae_metadata_accelerator_cluster_v1;

/**
 * Details of required clock frequencies, power threshold,
 * and Partial Reconfiguration interface.
 */
typedef struct _opae_metadata_afu_image_v1 {
	double clock_frequency_high;
	double clock_frequency_low;
	double power;
	char *interface_uuid;
	int magic_no;
	int num_clusters;
	opae_metadata_accelerator_cluster_v1 *accelerator_clusters;
} opae_metadata_afu_image_v1;

/**
 * Metadata version (1), image details, and platform name.
 *
 */
typedef struct _opae_bitstream_metadata_v1 {
	int version;
	opae_metadata_afu_image_v1 afu_image;
	char *platform_name;
} opae_bitstream_metadata_v1;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Parse a version 1 GBS metadata from its JSON object.
 *
 * @param[in] root The root of the JSON object as produced by
 * libjson.
 * @param[out] pr_interface_id The GUID for the Partial
 * Reconfiguration interface required by the GBS.
 *
 * @returns An allocated and populated version 1 metadata
 * object. NULL on failed memory allocation or invalid
 * metadata format.
 *
 * @note Allocates memory that must be tracked and
 * subsequently released by calling
 * `opae_bitstream_release_metadata_v1`.
 */
opae_bitstream_metadata_v1 *
opae_bitstream_parse_metadata_v1(json_object *root,
				 fpga_guid pr_interface_id);

/**
 * Release a parsed metadata version 1 object.
 *
 * @param[in] md The metadata object to be released.
 */
void opae_bitstream_release_metadata_v1(opae_bitstream_metadata_v1 *md);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OPAE_METADATAV1_H__ */
