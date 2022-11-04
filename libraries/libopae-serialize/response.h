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
#ifndef __OPAE_SERIALIZE_RESPONSE_H__
#define __OPAE_SERIALIZE_RESPONSE_H__
#include "request.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef struct {
	uint64_t request_id;
	char request_name[OPAE_REQUEST_NAME_MAX];
	char response_name[OPAE_REQUEST_NAME_MAX];
	uint64_t serial;
	char from[HOST_NAME_MAX + 1];
	char to[HOST_NAME_MAX + 1];
} opae_response_header;

typedef struct {
	opae_response_header header;
	fpga_token_header *tokens;
	uint32_t max_tokens;
	uint32_t num_matches;
	fpga_result result;
} opae_fpgaEnumerate_response;

char *opae_encode_fpgaEnumerate_response_0(opae_fpgaEnumerate_response *resp,
					   int json_flags);
bool opae_decode_fpgaEnumerate_response_0(const char *json,
					  opae_fpgaEnumerate_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaDestroyToken_response;

char *opae_encode_fpgaDestroyToken_response_1(opae_fpgaDestroyToken_response *resp,
					      int json_flags);
bool opae_decode_fpgaDestroyToken_response_1(const char *json,
					     opae_fpgaDestroyToken_response *resp);

typedef struct {
	opae_response_header header;
	fpga_token_header dest_token;
	fpga_result result;
} opae_fpgaCloneToken_response;

char *opae_encode_fpgaCloneToken_response_2(opae_fpgaCloneToken_response *resp,
					    int json_flags);
bool opae_decode_fpgaCloneToken_response_2(const char *json,
					   opae_fpgaCloneToken_response *resp);

typedef struct {
	opae_response_header header;
	fpga_properties properties;
	fpga_result result;
} opae_fpgaGetProperties_response;

char *opae_encode_fpgaGetProperties_response_3(opae_fpgaGetProperties_response *resp,
					       int json_flags);
bool opae_decode_fpgaGetProperties_response_3(const char *json,
					      opae_fpgaGetProperties_response *resp);

typedef struct {
	opae_response_header header;
	fpga_properties properties;
	fpga_result result;
} opae_fpgaUpdateProperties_response;

char *opae_encode_fpgaUpdateProperties_response_4(opae_fpgaUpdateProperties_response *resp,
						  int json_flags);
bool opae_decode_fpgaUpdateProperties_response_4(const char *json,
						 opae_fpgaUpdateProperties_response *resp);

typedef struct {
	opae_response_header header;
	fpga_handle_header handle;
	fpga_result result;
} opae_fpgaOpen_response;

char *opae_encode_fpgaOpen_response_5(opae_fpgaOpen_response *resp,
				      int json_flags);
bool opae_decode_fpgaOpen_response_5(const char *json,
				     opae_fpgaOpen_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaClose_response;

char *opae_encode_fpgaClose_response_6(opae_fpgaClose_response *resp,
				       int json_flags);
bool opae_decode_fpgaClose_response_6(const char *json,
				      opae_fpgaClose_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaReset_response;

char *opae_encode_fpgaReset_response_7(opae_fpgaReset_response *resp,
				       int json_flags);
bool opae_decode_fpgaReset_response_7(const char *json,
				      opae_fpgaReset_response *resp);

typedef struct {
	opae_response_header header;
	fpga_properties properties;
	fpga_result result;
} opae_fpgaGetPropertiesFromHandle_response;

char *opae_encode_fpgaGetPropertiesFromHandle_response_8(
	opae_fpgaGetPropertiesFromHandle_response *resp,
	int json_flags);
bool opae_decode_fpgaGetPropertiesFromHandle_response_8(
	const char *json,
	opae_fpgaGetPropertiesFromHandle_response *resp);

typedef struct {
	opae_response_header header;
	fpga_remote_id mmio_id;
	fpga_result result;
} opae_fpgaMapMMIO_response;

char *opae_encode_fpgaMapMMIO_response_9(opae_fpgaMapMMIO_response *resp,
					 int json_flags);
bool opae_decode_fpgaMapMMIO_response_9(const char *json,
					opae_fpgaMapMMIO_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaUnmapMMIO_response;

char *opae_encode_fpgaUnmapMMIO_response_10(opae_fpgaUnmapMMIO_response *resp,
					    int json_flags);
bool opae_decode_fpgaUnmapMMIO_response_10(const char *json,
					   opae_fpgaUnmapMMIO_response *resp);

typedef struct {
	opae_response_header header;
	uint32_t value;
	fpga_result result;
} opae_fpgaReadMMIO32_response;

char *opae_encode_fpgaReadMMIO32_response_11(opae_fpgaReadMMIO32_response *resp,
					     int json_flags);
bool opae_decode_fpgaReadMMIO32_response_11(const char *json,
					    opae_fpgaReadMMIO32_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaWriteMMIO32_response;

char *opae_encode_fpgaWriteMMIO32_response_12(opae_fpgaWriteMMIO32_response *resp,
					      int json_flags);
bool opae_decode_fpgaWriteMMIO32_response_12(const char *json,
					     opae_fpgaWriteMMIO32_response *resp);

typedef struct {
	opae_response_header header;
	uint64_t value;
	fpga_result result;
} opae_fpgaReadMMIO64_response;

char *opae_encode_fpgaReadMMIO64_response_13(opae_fpgaReadMMIO64_response *resp,
					     int json_flags);
bool opae_decode_fpgaReadMMIO64_response_13(const char *json,
					    opae_fpgaReadMMIO64_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaWriteMMIO64_response;

char *opae_encode_fpgaWriteMMIO64_response_14(opae_fpgaWriteMMIO64_response *resp,
					      int json_flags);
bool opae_decode_fpgaWriteMMIO64_response_14(const char *json,
					     opae_fpgaWriteMMIO64_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaWriteMMIO512_response;

char *opae_encode_fpgaWriteMMIO512_response_15(opae_fpgaWriteMMIO512_response *resp,
					       int json_flags);
bool opae_decode_fpgaWriteMMIO512_response_15(const char *json,
					      opae_fpgaWriteMMIO512_response *resp);

typedef struct {
	opae_response_header header;
	fpga_remote_id buf_id;
	fpga_result result;
} opae_fpgaPrepareBuffer_response;

char *opae_encode_fpgaPrepareBuffer_response_16(opae_fpgaPrepareBuffer_response *resp,
						int json_flags);
bool opae_decode_fpgaPrepareBuffer_response_16(const char *json,
					       opae_fpgaPrepareBuffer_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaReleaseBuffer_response;

char *opae_encode_fpgaReleaseBuffer_response_17(opae_fpgaReleaseBuffer_response *resp,
						int json_flags);
bool opae_decode_fpgaReleaseBuffer_response_17(const char *json,
					       opae_fpgaReleaseBuffer_response *resp);

typedef struct {
	opae_response_header header;
	uint64_t ioaddr;
	fpga_result result;
} opae_fpgaGetIOAddress_response;

char *opae_encode_fpgaGetIOAddress_response_18(opae_fpgaGetIOAddress_response *resp,
					       int json_flags);
bool opae_decode_fpgaGetIOAddress_response_18(const char *json,
					      opae_fpgaGetIOAddress_response *resp);



#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OPAE_SERIALIZE_RESPONSE_H__
