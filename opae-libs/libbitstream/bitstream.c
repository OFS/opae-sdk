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

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <uuid/uuid.h>
#include <json-c/json.h>
#include "bitstream.h"
#include "bits_utils.h"
#include "metadatav1.h"

#include "safe_string/safe_string.h"
#include <opae/log.h>
#include <opae/properties.h>
#include <opae/sysobject.h>

STATIC fpga_result opae_bitstream_read_file(const char *file,
					    uint8_t **buf,
					    size_t *len)
{
	FILE *fp;
	fpga_result res = FPGA_EXCEPTION;
	long pos;
	size_t sz;

	fp = fopen(file, "rb");
	if (!fp) {
		OPAE_ERR("fopen failed");
		return FPGA_EXCEPTION;
	}

	if (fseek(fp, 0, SEEK_END) < 0) {
		OPAE_ERR("fseek failed");
		goto out_close;
	}

	pos = ftell(fp);
	if (pos < 0) {
		OPAE_ERR("ftell failed");
		goto out_close;
	}

	*len = (size_t)pos;

	*buf = (uint8_t *)malloc(*len);
	if (!*buf) {
		OPAE_ERR("malloc failed");
		res = FPGA_NO_MEMORY;
		*len = 0;
		goto out_close;
	}

	if (fseek(fp, 0, SEEK_SET) < 0) {
		OPAE_ERR("fseek failed");
		goto out_free;
	}

	sz = fread(*buf, 1, *len, fp);
	if (ferror(fp)) {
		OPAE_ERR("ferror after read");
		goto out_free;
	}

	if (sz != *len) {
		OPAE_ERR("file size and number "
			 "of bytes read mismatch");
		goto out_free;
	}

	fclose(fp);
	return FPGA_OK;

out_free:
	free(*buf);
	*buf = NULL;
	*len = 0;
out_close:
	fclose(fp);
	return res;
}

bool opae_is_legacy_bitstream(opae_bitstream_info *info)
{
	opae_legacy_bitstream_header *hdr;

	if (info->data_len < sizeof(opae_legacy_bitstream_header))
		return false;

	hdr = (opae_legacy_bitstream_header *)info->data;
	if (hdr->legacy_magic == OPAE_LEGACY_BITSTREAM_MAGIC)
		return true;

	return false;
}

STATIC void opae_resolve_legacy_bitstream(opae_bitstream_info *info)
{
	opae_legacy_bitstream_header *hdr =
		(opae_legacy_bitstream_header *)info->data;
	uint8_t *p = &hdr->legacy_pr_ifc_id[15];
	int i = 0;

	// The guid is encoded backwards.
	// Reverse it.
	while (p >= hdr->legacy_pr_ifc_id) {
		info->pr_interface_id[i++] = *p--;
	}

	info->rbf_data = info->data + sizeof(opae_legacy_bitstream_header);
	info->rbf_len = info->data_len - sizeof(opae_legacy_bitstream_header);
}

STATIC void *opae_bitstream_parse_metadata(const char *metadata,
					   fpga_guid pr_interface_id,
					   int *version)
{
	json_object *root = NULL;
	json_object *j_version = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	void *parsed = NULL;

	root = json_tokener_parse_verbose(metadata, &j_err);
	if (!root) {
		OPAE_ERR("invalid JSON metadata: %s",
			 json_tokener_error_desc(j_err));
		return NULL;
	}
	
	if (!json_object_object_get_ex(root,
				       "version",
				       &j_version)) {
		OPAE_ERR("metadata: failed to find \"version\" key");
		goto out_put;
	}

	if (!json_object_is_type(j_version, json_type_int)) {
		OPAE_ERR("metadata: \"version\" key not integer");
		goto out_put;
	}

	*version = json_object_get_int(j_version);

	switch (*version) {

	// Some invalid GBS's around the BBS 6.4.0 and
	// BBS 6.5.0 eras incorrectly set the metadata
	// version to 640/650 respectively.
	// Allow 640/650 to serve as an alias for 1.
	case 650:
	case 640:
		*version = 1; /* FALLTHROUGH */
	case 1:
		parsed = opae_bitstream_parse_metadata_v1(root,
							  pr_interface_id);
	break;

	default:
		OPAE_ERR("metadata: unsupported version: %d", *version);
	}

out_put:
	json_object_put(root);

	return parsed;
}

STATIC fpga_guid valid_GBS_guid = {
0x58, 0x65, 0x6f, 0x6e,
0x46, 0x50,
0x47, 0x41,
0xb7, 0x47,
0x42, 0x53, 0x76, 0x30, 0x30, 0x31
};
STATIC fpga_result opae_resolve_bitstream(opae_bitstream_info *info)
{
	opae_bitstream_header *hdr;
	size_t sz;
	char *buf;
	errno_t err;

	if (info->data_len < sizeof(opae_bitstream_header)) {
		OPAE_ERR("file length smaller than bitstream header: "
			 "\"%s\"", info->filename);
		return FPGA_INVALID_PARAM;
	}

	hdr = (opae_bitstream_header *)info->data;

	if (uuid_compare(hdr->valid_gbs_guid, valid_GBS_guid) != 0) {
		OPAE_ERR("GBS guid is invalid: \"%s\"", info->filename);
		return FPGA_INVALID_PARAM;
	}

	// Check that metadata_length makes sense
	// given that we know the total file size.

	sz = sizeof(fpga_guid) + sizeof(uint32_t);
	sz += (size_t)hdr->metadata_length;

	if (sz > info->data_len) {
		OPAE_ERR("invalid metadata length in \"%s\"", info->filename);
		return FPGA_INVALID_PARAM;
	}

	info->rbf_data = info->data + sz;
	info->rbf_len = info->data_len - sz;

	buf = (char *)malloc(hdr->metadata_length + 1);
	if (!buf) {
		OPAE_ERR("malloc failed");
		return FPGA_NO_MEMORY;
	}

	err = memcpy_s(buf, hdr->metadata_length,
			hdr->metadata, hdr->metadata_length);
	if (err != EOK) {
		OPAE_ERR("memcpy_s failed");
		free(buf);
		return FPGA_EXCEPTION;
	}

	buf[hdr->metadata_length] = '\0';

	info->parsed_metadata =
		opae_bitstream_parse_metadata(buf,
					      info->pr_interface_id,
					      &info->metadata_version);

	free(buf);

	return info->parsed_metadata ? FPGA_OK : FPGA_EXCEPTION;
}

fpga_result opae_load_bitstream(const char *file, opae_bitstream_info *info)
{
	fpga_result res;

	if (!file || !info)
		return FPGA_INVALID_PARAM;

	if (!opae_bitstream_path_is_valid(file,
					  OPAE_BITSTREAM_PATH_NO_SYMLINK)) {
		OPAE_ERR("invalid bitstream path \"%s\"", file);
		return FPGA_INVALID_PARAM;
	}

	memset_s(info, sizeof(opae_bitstream_info), 0);

	res = opae_bitstream_read_file(file, &info->data, &info->data_len);
	if (res != FPGA_OK) {
		OPAE_ERR("error loading \"%s\"", file);
		return res;
	}

	info->filename = file;

	if (opae_is_legacy_bitstream(info)) {
		opae_resolve_legacy_bitstream(info);
		OPAE_MSG("Legacy bitstream (GBS) format detected.");
		OPAE_MSG("Legacy GBS support is deprecated "
			 "and will be removed in a future release.");
		return FPGA_OK;
	}

	return opae_resolve_bitstream(info);
}

fpga_result opae_unload_bitstream(opae_bitstream_info *info)
{
	fpga_result res = FPGA_OK;

	if (!info)
		return FPGA_INVALID_PARAM;

	if (info->data)
		free(info->data);

	if (info->parsed_metadata) {

		switch (info->metadata_version) {

		case 1:
			opae_bitstream_release_metadata_v1(
			(opae_bitstream_metadata_v1 *)info->parsed_metadata);
		break;

		default:
			OPAE_ERR("metadata: unsupported version: %d",
				 info->metadata_version);
			res = FPGA_EXCEPTION;
		}

	}

	memset_s(info, sizeof(opae_bitstream_info), 0);

	return res;
}
