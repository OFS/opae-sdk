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

#ifndef __FPGA_BITSTREAM_INT_H__
#define __FPGA_BITSTREAM_INT_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define GUID_LEN		36
#define AFU_NAME_LEN		512

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

/**
 * Check the validity of GUID
 *
 *Extracts the 128 bit guid from passed bitstream
 *converts it to fpga_guid type anc checks it against
 *expected value
 *
 *
 * @param[in] bitstream   Pointer to the bitstream
 * @returns               FPGA_OK on success
 */
fpga_result check_bitstream_guid(const uint8_t *bitstream);

/**
 * Get total length of bitstream header
 *
 * Returns the total length of header which is
 * GUID + size of variable describing length of metadata + length of metadata
 *
 *
 * @param[in] bitstream   Pointer to the bitstream
 * @returns               int value of length, -1 on failure
 */
int get_bitstream_header_len(const uint8_t *bitstream);

/**
 * Get total length of json metadata in bitstream
 *
 * Returns the length of the json metadata from the
 * bitstream which is represented by a uint32 after the
 * GUID
 *
 *
 * @param[in] bitstream   Pointer to the bitstream
 * @returns               int value of length, -1 on failure
 */
int32_t get_bitstream_json_len(const uint8_t *bitstream);


/**
 * Check bitstream magic no and interface id
 *
 * Checks the bitstream magic no and interface id
 * with expected values
 *
 * @param[in] handle			Handle to previously opened FPGA object
 * @param[in] bitstream_magic_no	magic no. to be checked
 * @param[in] ifid_l			lower 64 bits of interface id
 * @param[in] ifid_h			higher 64 bits of interface id
 * @returns				FPGA_OK on success
 */
fpga_result check_interface_id(fpga_handle handle, uint32_t bitstream_magic_no,
				uint64_t ifid_l, uint64_t ifid_h);

/**
 * Check if the JSON metadata is valid
 *
 * Reads the bitstream magic no and interface
 * id values from the metadata and compares them
 * with expected values
 *
 * @param[in] handle	  Handle to previously opened FPGA object
 * @param[in] bitstream   Pointer to the bitstream
 * @returns		  FPGA_OK on success
 */
fpga_result validate_bitstream_metadata(fpga_handle handle,
					const uint8_t *bitstream);

/**
 * Reads GBS metadata
 *
 * Parses GBS JSON metadata.
 *
 * @param[in] bitstream    Pointer to the bitstream
 * @param[in] gbs_metadata Pointer to gbs metadata struct
 * @returns                FPGA_OK on success
 */
fpga_result read_gbs_metadata(const uint8_t *bitstream,
			      struct gbs_metadata *gbs_metadata);

/**
* Reads interface id high and low values
*
* Reads interface id from sysfs.
*
* @param[in] handle   FME handle
* @param[out] id_l    Interface id low
* @param[out] id_h    Interface id lHigh
* @returns             FPGA_OK on success
*/
fpga_result get_interface_id(fpga_handle handle,
							uint64_t *id_l, uint64_t *id_h);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_BITSTREAM_INT_H__
