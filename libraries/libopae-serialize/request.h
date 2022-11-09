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
#ifndef __OPAE_SERIALIZE_REQUEST_H__
#define __OPAE_SERIALIZE_REQUEST_H__
#include "serialize.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#define OPAE_REQUEST_NAME_MAX 64
typedef struct {
	uint64_t request_id;
	char request_name[OPAE_REQUEST_NAME_MAX];
	uint64_t serial;
	char from[HOST_NAME_MAX + 1];
} opae_request_header;

bool opae_decode_request_header_obj(struct json_object *root,
				    opae_request_header *header);

typedef struct {
	opae_request_header header;
	fpga_properties *filters;
	uint32_t num_filters;
	uint32_t max_tokens;
} opae_fpgaEnumerate_request;

char *opae_encode_fpgaEnumerate_request_0(opae_fpgaEnumerate_request *req,
					  int json_flags);
bool opae_decode_fpgaEnumerate_request_0(const char *json,
					 opae_fpgaEnumerate_request *req);

typedef struct {
	opae_request_header header;
	fpga_token_header token;
} opae_fpgaDestroyToken_request;

char *opae_encode_fpgaDestroyToken_request_1(opae_fpgaDestroyToken_request *req,
					     int json_flags);
bool opae_decode_fpgaDestroyToken_request_1(const char *json,
					    opae_fpgaDestroyToken_request *req);

typedef struct {
	opae_request_header header;
	fpga_token_header src_token;
} opae_fpgaCloneToken_request;

char *opae_encode_fpgaCloneToken_request_2(opae_fpgaCloneToken_request *req,
					   int json_flags);
bool opae_decode_fpgaCloneToken_request_2(const char *json,
					  opae_fpgaCloneToken_request *req);

typedef struct {
	opae_request_header header;
	fpga_token_header token;
} opae_fpgaGetProperties_request;

char *opae_encode_fpgaGetProperties_request_3(opae_fpgaGetProperties_request *req,
					      int json_flags);
bool opae_decode_fpgaGetProperties_request_3(const char *json,
					     opae_fpgaGetProperties_request *req);

typedef struct {
	opae_request_header header;
	fpga_token_header token;
} opae_fpgaUpdateProperties_request;

char *opae_encode_fpgaUpdateProperties_request_4(opae_fpgaUpdateProperties_request *req,
						 int json_flags);
bool opae_decode_fpgaUpdateProperties_request_4(const char *json,
						opae_fpgaUpdateProperties_request *req);

typedef struct {
	opae_request_header header;
	fpga_token_header token;
	int flags;
} opae_fpgaOpen_request;

char *opae_encode_fpgaOpen_request_5(opae_fpgaOpen_request *req,
				     int json_flags);
bool opae_decode_fpgaOpen_request_5(const char *json,
				    opae_fpgaOpen_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
} opae_fpgaClose_request;

char *opae_encode_fpgaClose_request_6(opae_fpgaClose_request *req,
				      int json_flags);
bool opae_decode_fpgaClose_request_6(const char *json,
				     opae_fpgaClose_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
} opae_fpgaReset_request;

char *opae_encode_fpgaReset_request_7(opae_fpgaReset_request *req,
				      int json_flags);
bool opae_decode_fpgaReset_request_7(const char *json,
				     opae_fpgaReset_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
} opae_fpgaGetPropertiesFromHandle_request;

char *opae_encode_fpgaGetPropertiesFromHandle_request_8(
	opae_fpgaGetPropertiesFromHandle_request *req,
	int json_flags);
bool opae_decode_fpgaGetPropertiesFromHandle_request_8(
	const char *json,
	opae_fpgaGetPropertiesFromHandle_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
	uint32_t mmio_num;
} opae_fpgaMapMMIO_request;

char *opae_encode_fpgaMapMMIO_request_9(opae_fpgaMapMMIO_request *req,
					int json_flags);
bool opae_decode_fpgaMapMMIO_request_9(const char *json,
				       opae_fpgaMapMMIO_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
	fpga_remote_id mmio_id;
	uint32_t mmio_num;
} opae_fpgaUnmapMMIO_request;

char *opae_encode_fpgaUnmapMMIO_request_10(opae_fpgaUnmapMMIO_request *req,
					   int json_flags);
bool opae_decode_fpgaUnmapMMIO_request_10(const char *json,
					  opae_fpgaUnmapMMIO_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
	uint32_t mmio_num;
	uint64_t offset;
} opae_fpgaReadMMIO32_request;

char *opae_encode_fpgaReadMMIO32_request_11(opae_fpgaReadMMIO32_request *req,
					    int json_flags);
bool opae_decode_fpgaReadMMIO32_request_11(const char *json,
					   opae_fpgaReadMMIO32_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
	uint32_t mmio_num;
	uint64_t offset;
	uint32_t value;
} opae_fpgaWriteMMIO32_request;

char *opae_encode_fpgaWriteMMIO32_request_12(opae_fpgaWriteMMIO32_request *req,
					     int json_flags);
bool opae_decode_fpgaWriteMMIO32_request_12(const char *json,
					    opae_fpgaWriteMMIO32_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
	uint32_t mmio_num;
	uint64_t offset;
} opae_fpgaReadMMIO64_request;

char *opae_encode_fpgaReadMMIO64_request_13(opae_fpgaReadMMIO64_request *req,
					    int json_flags);
bool opae_decode_fpgaReadMMIO64_request_13(const char *json,
					   opae_fpgaReadMMIO64_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
	uint32_t mmio_num;
	uint64_t offset;
	uint64_t value;
} opae_fpgaWriteMMIO64_request;

char *opae_encode_fpgaWriteMMIO64_request_14(opae_fpgaWriteMMIO64_request *req,
					     int json_flags);
bool opae_decode_fpgaWriteMMIO64_request_14(const char *json,
					    opae_fpgaWriteMMIO64_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
	uint32_t mmio_num;
	uint64_t offset;
	uint64_t values[8];
} opae_fpgaWriteMMIO512_request;

char *opae_encode_fpgaWriteMMIO512_request_15(opae_fpgaWriteMMIO512_request *req,
					      int json_flags);
bool opae_decode_fpgaWriteMMIO512_request_15(const char *json,
					     opae_fpgaWriteMMIO512_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
	uint64_t len;
	bool have_buf_addr;
	void *pre_allocated_addr;
	int flags;
} opae_fpgaPrepareBuffer_request;

char *opae_encode_fpgaPrepareBuffer_request_16(opae_fpgaPrepareBuffer_request *req,
					       int json_flags);
bool opae_decode_fpgaPrepareBuffer_request_16(const char *json,
					      opae_fpgaPrepareBuffer_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
	fpga_remote_id buf_id;
} opae_fpgaReleaseBuffer_request;

char *opae_encode_fpgaReleaseBuffer_request_17(opae_fpgaReleaseBuffer_request *req,
					       int json_flags);
bool opae_decode_fpgaReleaseBuffer_request_17(const char *json,
					      opae_fpgaReleaseBuffer_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
	fpga_remote_id buf_id;
} opae_fpgaGetIOAddress_request;

char *opae_encode_fpgaGetIOAddress_request_18(opae_fpgaGetIOAddress_request *req,
					      int json_flags);
bool opae_decode_fpgaGetIOAddress_request_18(const char *json,
					     opae_fpgaGetIOAddress_request *req);

typedef struct {
	opae_request_header header;
	fpga_token_header token;
	uint32_t error_num;
} opae_fpgaReadError_request;

char *opae_encode_fpgaReadError_request_19(opae_fpgaReadError_request *req,
					   int json_flags);
bool opae_decode_fpgaReadError_request_19(const char *json,
					  opae_fpgaReadError_request *req);

typedef struct {
	opae_request_header header;
	fpga_token_header token;
	uint32_t error_num;
} opae_fpgaGetErrorInfo_request;

char *opae_encode_fpgaGetErrorInfo_request_20(opae_fpgaGetErrorInfo_request *req,
					      int json_flags);
bool opae_decode_fpgaGetErrorInfo_request_20(const char *json,
					     opae_fpgaGetErrorInfo_request *req);

typedef struct {
	opae_request_header header;
	fpga_token_header token;
	uint32_t error_num;
} opae_fpgaClearError_request;

char *opae_encode_fpgaClearError_request_21(opae_fpgaClearError_request *req,
					    int json_flags);
bool opae_decode_fpgaClearError_request_21(const char *json,
					   opae_fpgaClearError_request *req);

typedef struct {
	opae_request_header header;
	fpga_token_header token;
} opae_fpgaClearAllErrors_request;

char *opae_encode_fpgaClearAllErrors_request_22(
	opae_fpgaClearAllErrors_request *req,
	int json_flags);
bool opae_decode_fpgaClearAllErrors_request_22(
	const char *json,
	opae_fpgaClearAllErrors_request *req);

#ifndef OPAE_SYSOBJECT_NAME_MAX 
#define OPAE_SYSOBJECT_NAME_MAX 256
#endif // OPAE_SYSOBJECT_NAME_MAX

#ifndef OPAE_SYSOBJECT_VALUE_MAX
#define OPAE_SYSOBJECT_VALUE_MAX 4096
#endif // OPAE_SYSOBJECT_VALUE_MAX

typedef struct {
	opae_request_header header;
	fpga_token_header token;
	char name[OPAE_SYSOBJECT_NAME_MAX];
	int flags;
} opae_fpgaTokenGetObject_request;

char *opae_encode_fpgaTokenGetObject_request_23(
	opae_fpgaTokenGetObject_request *req,
	int json_flags);
bool opae_decode_fpgaTokenGetObject_request_23(
	const char *json,
	opae_fpgaTokenGetObject_request *req);

typedef struct {
	opae_request_header header;
	fpga_remote_id object_id;
} opae_fpgaDestroyObject_request;

char *opae_encode_fpgaDestroyObject_request_24(
	opae_fpgaDestroyObject_request *req,
	int json_flags);
bool opae_decode_fpgaDestroyObject_request_24(
	const char *json,
	opae_fpgaDestroyObject_request *req);

typedef struct {
	opae_request_header header;
	fpga_remote_id object_id;
} opae_fpgaObjectGetType_request;

char *opae_encode_fpgaObjectGetType_request_25(
	opae_fpgaObjectGetType_request *req,
	int json_flags);
bool opae_decode_fpgaObjectGetType_request_25(
	const char *json,
	opae_fpgaObjectGetType_request *req);

typedef struct {
	opae_request_header header;
	fpga_remote_id object_id;
} opae_fpgaObjectGetName_request;

char *opae_encode_fpgaObjectGetName_request_26(
	opae_fpgaObjectGetName_request *req,
	int json_flags);
bool opae_decode_fpgaObjectGetName_request_26(
	const char *json,
	opae_fpgaObjectGetName_request *req);

typedef struct {
	opae_request_header header;
	fpga_remote_id object_id;
	int flags;
} opae_fpgaObjectGetSize_request;

char *opae_encode_fpgaObjectGetSize_request_27(
	opae_fpgaObjectGetSize_request *req,
	int json_flags);
bool opae_decode_fpgaObjectGetSize_request_27(
	const char *json,
	opae_fpgaObjectGetSize_request *req);

typedef struct {
	opae_request_header header;
	fpga_remote_id object_id;
	size_t offset;
	size_t len;
	int flags;
} opae_fpgaObjectRead_request;

char *opae_encode_fpgaObjectRead_request_28(
	opae_fpgaObjectRead_request *req,
	int json_flags);
bool opae_decode_fpgaObjectRead_request_28(
	const char *json,
	opae_fpgaObjectRead_request *req);

typedef struct {
	opae_request_header header;
	fpga_remote_id object_id;
	int flags;
} opae_fpgaObjectRead64_request;

char *opae_encode_fpgaObjectRead64_request_29(
	opae_fpgaObjectRead64_request *req,
	int json_flags);
bool opae_decode_fpgaObjectRead64_request_29(
	const char *json,
	opae_fpgaObjectRead64_request *req);

typedef struct {
	opae_request_header header;
	fpga_remote_id object_id;
	uint64_t value;
	int flags;
} opae_fpgaObjectWrite64_request;

char *opae_encode_fpgaObjectWrite64_request_30(
	opae_fpgaObjectWrite64_request *req,
	int json_flags);
bool opae_decode_fpgaObjectWrite64_request_30(
	const char *json,
	opae_fpgaObjectWrite64_request *req);

typedef struct {
	opae_request_header header;
	fpga_handle_header handle;
	char name[OPAE_SYSOBJECT_NAME_MAX];
	int flags;
} opae_fpgaHandleGetObject_request;

char *opae_encode_fpgaHandleGetObject_request_31(
	opae_fpgaHandleGetObject_request *req,
	int json_flags);
bool opae_decode_fpgaHandleGetObject_request_31(
	const char *json,
	opae_fpgaHandleGetObject_request *req);

typedef struct {
	opae_request_header header;
	fpga_remote_id object_id;
	char name[OPAE_SYSOBJECT_NAME_MAX];
	int flags;
} opae_fpgaObjectGetObject_request;

char *opae_encode_fpgaObjectGetObject_request_32(
	opae_fpgaObjectGetObject_request *req,
	int json_flags);
bool opae_decode_fpgaObjectGetObject_request_32(
	const char *json,
	opae_fpgaObjectGetObject_request *req);

typedef struct {
	opae_request_header header;
	fpga_remote_id object_id;
	size_t idx;
} opae_fpgaObjectGetObjectAt_request;

char *opae_encode_fpgaObjectGetObjectAt_request_33(
	opae_fpgaObjectGetObjectAt_request *req,
	int json_flags);
bool opae_decode_fpgaObjectGetObjectAt_request_33(
	const char *json,
	opae_fpgaObjectGetObjectAt_request *req);




#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OPAE_SERIALIZE_REQUEST_H__
