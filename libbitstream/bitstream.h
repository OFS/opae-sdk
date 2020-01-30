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

/**
 * @file bitstream.h
 * @brief API for manipulating Green Bitstreams (GBS)
 *
 * GBS files store the AFU logic as well as versioned metadata.
 * These routines parse a disk-resident GBS file, expanding its
 * metadata and loading the GBS logic into memory.
 *
 */

#ifndef __OPAE_BITSTREAM_H__
#define __OPAE_BITSTREAM_H__

#include <stdint.h>
#include <stdbool.h>
#include <opae/types.h>

#define OPAE_LEGACY_BITSTREAM_MAGIC 0x1d1f8680

#pragma pack(push, 1)

/**
 * @deprecated Legacy (pre-6.4.0 BBS) format support.
 */
typedef struct _opae_legacy_bitstream_header {
	uint32_t legacy_magic;
	fpga_guid legacy_pr_ifc_id;
} opae_legacy_bitstream_header;

#pragma pack(pop)

#pragma pack(push, 1)

/**
 * Format of the GBS header.
 */
typedef struct _opae_bitstream_header {
	fpga_guid valid_gbs_guid;	/**< indentifies a GBS file */
	uint32_t metadata_length;	/**< length of metadata in bytes */
	char metadata[1];		/**< GBS metadata (JSON) */
} opae_bitstream_header;

#pragma pack(pop)

/**
 * Memory-resident GBS format.
 *
 * `metadata_version` begins at 1 and increments upward.
 * `parsed_metadata` is the expanded metadata structure.
 *
 * If `metadata_version` is 1, then `parsed_metadata`
 * can be safely typecasted to an `opae_bitstream_metadata_v1 *`.
 */
typedef struct _opae_bitstream_info {
	const char *filename;		/**< location of the file on disk */
	uint8_t *data;			/**< entire GBS file contents */
	size_t data_len;		/**< length in bytes of data */
	uint8_t *rbf_data;		/**< start of AFU logic in data */
	size_t rbf_len;			/**< length of AFU logic in bytes */
	fpga_guid pr_interface_id;	/**< identifies GBS compatibility */
	int metadata_version;		/**< identifies metadata format */
	void *parsed_metadata;		/**< the expanded metadata */
} opae_bitstream_info;

#define OPAE_BITSTREAM_INFO_INITIALIZER \
{ NULL, NULL, 0, NULL, 0, { 0, }, 0, NULL }

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Load a GBS file from disk into memory
 *
 * Used to validate and load a GBS file into its memory-resident format.
 *
 * @param[in] file Location of the GBS file on disk.
 * @param[out] info Storage for the loaded GBS file contents
 *                  and its expanded metadata.
 *
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if the bitstream
 * format is invalid. FPGA_NO_MEMORY if memory allocation fails.
 * FPGA_EXCEPTION if a metadata parsing error was encountered.
 */
fpga_result opae_load_bitstream(const char *file, opae_bitstream_info *info);

/**
 * @deprecated Determine whether a loaded GBS is in legacy format.
 *
 * Legacy GBS files have no metadata.
 */ 
bool opae_is_legacy_bitstream(opae_bitstream_info *info);

/**
 * Unload a memory-resident GBS
 *
 * Used to free the resources allocated by `opae_load_bitstream`.
 *
 * @param[in] info The loaded GBS info to be released.
 *
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if info is NULL.
 * FPGA_EXCEPTION if the metadata version is not supported.
 */
fpga_result opae_unload_bitstream(opae_bitstream_info *info);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OPAE_BITSTREAM_H__ */
