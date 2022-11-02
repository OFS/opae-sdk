// Copyright(c) 2022, Intel Corporation
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

#include <opae/types.h>

#include "mock/opae_std.h"

#include "remote.h"
#include "serialize.h"
#include "request.h"
#include "response.h"

fpga_result __REMOTE_API__
remote_fpgaWriteMMIO32(fpga_handle handle,
		       uint32_t mmio_num,
		       uint64_t offset,
		       uint32_t value)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) mmio_num;
(void) offset;
(void) value;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaReadMMIO32(fpga_handle handle,
		      uint32_t mmio_num,
		      uint64_t offset,
		      uint32_t *value)
{
	opae_fpgaReadMMIO32_request req;
	opae_fpgaReadMMIO32_response resp;
	struct _remote_handle *h;
	struct _remote_token *tok;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];

	if (!handle) {
		OPAE_ERR("NULL handle");
		return FPGA_INVALID_PARAM;
	}

	if (mmio_num >= OPAE_MMIO_REGIONS_MAX) {
		OPAE_ERR("mmio_num out of range 0-%d",
			OPAE_MMIO_REGIONS_MAX - 1);
		return FPGA_INVALID_PARAM;
	}

	if (!value) {
		OPAE_ERR("NULL value pointer");
		return FPGA_INVALID_PARAM;
	}

	h = (struct _remote_handle *)handle;
	tok = h->token;

	if (!h->mmio_regions[mmio_num]) {
		OPAE_ERR("MMIO %d is not mapped.", mmio_num);
		return FPGA_INVALID_PARAM;
	}

	req.handle = h->hdr;
	req.mmio_num = mmio_num;
	req.offset = offset;

	req_json = opae_encode_fpgaReadMMIO32_request_11(
		&req, tok->json_to_string_flags);

	if (!req_json)
		return FPGA_NO_MEMORY;

	len = strlen(req_json);

	slen = tok->ifc->send(tok->ifc->connection,
			      req_json,
			      len + 1);
	if (slen < 0) {
		opae_free(req_json);
		return FPGA_EXCEPTION;
	}

	opae_free(req_json);

	slen = tok->ifc->receive(tok->ifc->connection,
				 recvbuf,
				 sizeof(recvbuf));
	if (slen < 0)
		return FPGA_EXCEPTION;

printf("%s\n", recvbuf);

	if (!opae_decode_fpgaReadMMIO32_response_11(recvbuf, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK)
		*value = resp.value;

	return resp.result;
}

fpga_result __REMOTE_API__
remote_fpgaWriteMMIO64(fpga_handle handle,
		       uint32_t mmio_num,
		       uint64_t offset,
		       uint64_t value)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) mmio_num;
(void) offset;
(void) value;


	return result;
}

fpga_result __REMOTE_API__
remote_fpgaReadMMIO64(fpga_handle handle,
		      uint32_t mmio_num,
		      uint64_t offset,
		      uint64_t *value)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) mmio_num;
(void) offset;
(void) value;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaWriteMMIO512(fpga_handle handle,
			uint32_t mmio_num,
			uint64_t offset,
			const void *value)
{
	fpga_result result = FPGA_OK;
(void) handle;
(void) mmio_num;
(void) offset;
(void) value;



	return result;
}

fpga_result __REMOTE_API__
remote_fpgaMapMMIO(fpga_handle handle,
		   uint32_t mmio_num,
		   uint64_t **mmio_ptr)
{
	opae_fpgaMapMMIO_request req;
	opae_fpgaMapMMIO_response resp;
	struct _remote_handle *h;
	struct _remote_token *tok;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];
	fpga_remote_id *mmio_id;

	if (!handle) {
		OPAE_ERR("NULL handle");
		return FPGA_INVALID_PARAM;
	}

	if (mmio_num >= OPAE_MMIO_REGIONS_MAX) {
		OPAE_ERR("mmio_num out of range 0-%d",
			OPAE_MMIO_REGIONS_MAX - 1);
		return FPGA_INVALID_PARAM;
	}

	h = (struct _remote_handle *)handle;
	tok = h->token;

	if (h->mmio_regions[mmio_num]) {
		OPAE_MSG("MMIO %d is already mapped. "
			 "Proceeding anyway..", mmio_num);
		opae_free(h->mmio_regions[mmio_num]);
		h->mmio_regions[mmio_num] = NULL;
	}

	req.handle = h->hdr;
	req.mmio_num = mmio_num;

	req_json = opae_encode_fpgaMapMMIO_request_9(
		&req, tok->json_to_string_flags);

	if (!req_json)
		return FPGA_NO_MEMORY;

	len = strlen(req_json);

	slen = tok->ifc->send(tok->ifc->connection,
			      req_json,
			      len + 1);
	if (slen < 0) {
		opae_free(req_json);
		return FPGA_EXCEPTION;
	}

	opae_free(req_json);

	slen = tok->ifc->receive(tok->ifc->connection,
				 recvbuf,
				 sizeof(recvbuf));
	if (slen < 0)
		return FPGA_EXCEPTION;

printf("%s\n", recvbuf);

	if (!opae_decode_fpgaMapMMIO_response_9(recvbuf, &resp))
		return FPGA_EXCEPTION;

	mmio_id = opae_calloc(1, sizeof(fpga_remote_id));
	if (!mmio_id) {
		OPAE_ERR("calloc failed");
		return FPGA_NO_MEMORY;
	}

	*mmio_id = resp.mmio_id;
	h->mmio_regions[mmio_num] = mmio_id;

	if (mmio_ptr)
		*mmio_ptr = NULL;

	return resp.result;
}

fpga_result __REMOTE_API__
remote_fpgaUnmapMMIO(fpga_handle handle, uint32_t mmio_num)
{
	opae_fpgaUnmapMMIO_request req;
	opae_fpgaUnmapMMIO_response resp;
	struct _remote_handle *h;
	struct _remote_token *tok;
	char *req_json;
	size_t len;
	ssize_t slen;
	char recvbuf[OPAE_RECEIVE_BUF_MAX];
	fpga_remote_id *mmio_id;

	if (!handle) {
		OPAE_ERR("NULL handle");
		return FPGA_INVALID_PARAM;
	}

	if (mmio_num >= OPAE_MMIO_REGIONS_MAX) {
		OPAE_ERR("mmio_num out of range 0-%d",
			OPAE_MMIO_REGIONS_MAX - 1);
		return FPGA_INVALID_PARAM;
	}

	h = (struct _remote_handle *)handle;
	tok = h->token;

	if (!h->mmio_regions[mmio_num]) {
		OPAE_ERR("MMIO %d is not mapped.", mmio_num);
		return FPGA_INVALID_PARAM;
	}

	mmio_id = h->mmio_regions[mmio_num];

	req.handle = h->hdr;
	req.mmio_id = *mmio_id;
	req.mmio_num = mmio_num;

	req_json = opae_encode_fpgaUnmapMMIO_request_10(
		&req, tok->json_to_string_flags);

	if (!req_json)
		return FPGA_NO_MEMORY;

	len = strlen(req_json);

	slen = tok->ifc->send(tok->ifc->connection,
			      req_json,
			      len + 1);
	if (slen < 0) {
		opae_free(req_json);
		return FPGA_EXCEPTION;
	}

	opae_free(req_json);

	slen = tok->ifc->receive(tok->ifc->connection,
				 recvbuf,
				 sizeof(recvbuf));
	if (slen < 0)
		return FPGA_EXCEPTION;

printf("%s\n", recvbuf);

	if (!opae_decode_fpgaUnmapMMIO_response_10(recvbuf, &resp))
		return FPGA_EXCEPTION;

	if (resp.result == FPGA_OK) {
		opae_free(mmio_id);
		h->mmio_regions[mmio_num] = NULL;
	}

	return resp.result;
}
