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

typedef struct {
	opae_response_header header;
	uint64_t value;
	fpga_result result;
} opae_fpgaReadError_response;

char *opae_encode_fpgaReadError_response_19(opae_fpgaReadError_response *resp,
					    int json_flags);
bool opae_decode_fpgaReadError_response_19(const char *json,
					   opae_fpgaReadError_response *resp);

typedef struct {
	opae_response_header header;
	struct fpga_error_info error_info;
	fpga_result result;
} opae_fpgaGetErrorInfo_response;

char *opae_encode_fpgaGetErrorInfo_response_20(opae_fpgaGetErrorInfo_response *resp,
					       int json_flags);
bool opae_decode_fpgaGetErrorInfo_response_20(const char *json,
					      opae_fpgaGetErrorInfo_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaClearError_response;

char *opae_encode_fpgaClearError_response_21(opae_fpgaClearError_response *resp,
					     int json_flags);
bool opae_decode_fpgaClearError_response_21(const char *json,
					    opae_fpgaClearError_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaClearAllErrors_response;

char *opae_encode_fpgaClearAllErrors_response_22(
	opae_fpgaClearAllErrors_response *resp,
	int json_flags);
bool opae_decode_fpgaClearAllErrors_response_22(
	const char *json,
	opae_fpgaClearAllErrors_response *resp);

typedef struct {
	opae_response_header header;
	fpga_remote_id object_id;
	fpga_result result;
} opae_fpgaTokenGetObject_response;

char *opae_encode_fpgaTokenGetObject_response_23(
	opae_fpgaTokenGetObject_response *resp,
	int json_flags);
bool opae_decode_fpgaTokenGetObject_response_23(
	const char *json,
	opae_fpgaTokenGetObject_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaDestroyObject_response;

char *opae_encode_fpgaDestroyObject_response_24(
	opae_fpgaDestroyObject_response *resp,
	int json_flags);
bool opae_decode_fpgaDestroyObject_response_24(
	const char *json,
	opae_fpgaDestroyObject_response *resp);

typedef struct {
	opae_response_header header;
	enum fpga_sysobject_type type;
	fpga_result result;
} opae_fpgaObjectGetType_response;

char *opae_encode_fpgaObjectGetType_response_25(
	opae_fpgaObjectGetType_response *resp,
	int json_flags);
bool opae_decode_fpgaObjectGetType_response_25(
	const char *json,
	opae_fpgaObjectGetType_response *resp);

#ifndef OPAE_SYSOBJECT_NAME_MAX 
#define OPAE_SYSOBJECT_NAME_MAX 256
#endif // OPAE_SYSOBJECT_NAME_MAX

#ifndef OPAE_SYSOBJECT_VALUE_MAX
#define OPAE_SYSOBJECT_VALUE_MAX 4096
#endif // OPAE_SYSOBJECT_VALUE_MAX

typedef struct {
	opae_response_header header;
	char name[OPAE_SYSOBJECT_NAME_MAX];
	fpga_result result;
} opae_fpgaObjectGetName_response;

char *opae_encode_fpgaObjectGetName_response_26(
	opae_fpgaObjectGetName_response *resp,
	int json_flags);
bool opae_decode_fpgaObjectGetName_response_26(
	const char *json,
	opae_fpgaObjectGetName_response *resp);

typedef struct {
	opae_response_header header;
	uint32_t value;
	fpga_result result;
} opae_fpgaObjectGetSize_response;

char *opae_encode_fpgaObjectGetSize_response_27(
	opae_fpgaObjectGetSize_response *resp,
	int json_flags);
bool opae_decode_fpgaObjectGetSize_response_27(
	const char *json,
	opae_fpgaObjectGetSize_response *resp);

typedef struct {
	opae_response_header header;
	char value[OPAE_SYSOBJECT_VALUE_MAX];
	fpga_result result;
} opae_fpgaObjectRead_response;

char *opae_encode_fpgaObjectRead_response_28(
	opae_fpgaObjectRead_response *resp,
	int json_flags);
bool opae_decode_fpgaObjectRead_response_28(
	const char *json,
	opae_fpgaObjectRead_response *resp);

typedef struct {
	opae_response_header header;
	uint64_t value;
	fpga_result result;
} opae_fpgaObjectRead64_response;

char *opae_encode_fpgaObjectRead64_response_29(
	opae_fpgaObjectRead64_response *resp,
	int json_flags);
bool opae_decode_fpgaObjectRead64_response_29(
	const char *json,
	opae_fpgaObjectRead64_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaObjectWrite64_response;

char *opae_encode_fpgaObjectWrite64_response_30(
	opae_fpgaObjectWrite64_response *resp,
	int json_flags);
bool opae_decode_fpgaObjectWrite64_response_30(
	const char *json,
	opae_fpgaObjectWrite64_response *resp);

typedef struct {
	opae_response_header header;
	fpga_remote_id object_id;
	fpga_result result;
} opae_fpgaHandleGetObject_response;

char *opae_encode_fpgaHandleGetObject_response_31(
	opae_fpgaHandleGetObject_response *resp,
	int json_flags);
bool opae_decode_fpgaHandleGetObject_response_31(
	const char *json,
	opae_fpgaHandleGetObject_response *resp);

typedef struct {
	opae_response_header header;
	fpga_remote_id object_id;
	fpga_result result;
} opae_fpgaObjectGetObject_response;

char *opae_encode_fpgaObjectGetObject_response_32(
	opae_fpgaObjectGetObject_response *resp,
	int json_flags);
bool opae_decode_fpgaObjectGetObject_response_32(
	const char *json,
	opae_fpgaObjectGetObject_response *resp);

typedef struct {
	opae_response_header header;
	fpga_remote_id object_id;
	fpga_result result;
} opae_fpgaObjectGetObjectAt_response;

char *opae_encode_fpgaObjectGetObjectAt_response_33(
	opae_fpgaObjectGetObjectAt_response *resp,
	int json_flags);
bool opae_decode_fpgaObjectGetObjectAt_response_33(
	const char *json,
	opae_fpgaObjectGetObjectAt_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaSetUserClock_response;

char *opae_encode_fpgaSetUserClock_response_34(
	opae_fpgaSetUserClock_response *resp,
	int json_flags);
bool opae_decode_fpgaSetUserClock_response_34(
	const char *json,
	opae_fpgaSetUserClock_response *resp);

typedef struct {
	opae_response_header header;
	uint64_t high_clk;
	uint64_t low_clk;
	fpga_result result;
} opae_fpgaGetUserClock_response;

char *opae_encode_fpgaGetUserClock_response_35(
	opae_fpgaGetUserClock_response *resp,
	int json_flags);
bool opae_decode_fpgaGetUserClock_response_35(
	const char *json,
	opae_fpgaGetUserClock_response *resp);

typedef struct {
	opae_response_header header;
	uint64_t num_metrics;
	fpga_result result;
} opae_fpgaGetNumMetrics_response;

char *opae_encode_fpgaGetNumMetrics_response_36(
	opae_fpgaGetNumMetrics_response *resp,
	int json_flags);
bool opae_decode_fpgaGetNumMetrics_response_36(
	const char *json,
	opae_fpgaGetNumMetrics_response *resp);

typedef struct {
	opae_response_header header;
	fpga_metric_info *info;
	uint64_t num_metrics;
	fpga_result result;
} opae_fpgaGetMetricsInfo_response;

char *opae_encode_fpgaGetMetricsInfo_response_37(
	opae_fpgaGetMetricsInfo_response *resp,
	int json_flags);
bool opae_decode_fpgaGetMetricsInfo_response_37(
	const char *json,
	opae_fpgaGetMetricsInfo_response *resp);

typedef struct {
	opae_response_header header;
	uint64_t num_metric_indexes;
	fpga_metric *metrics;
	fpga_result result;
} opae_fpgaGetMetricsByIndex_response;

char *opae_encode_fpgaGetMetricsByIndex_response_38(
	opae_fpgaGetMetricsByIndex_response *resp,
	int json_flags);
bool opae_decode_fpgaGetMetricsByIndex_response_38(
	const char *json,
	opae_fpgaGetMetricsByIndex_response *resp);

typedef struct {
	opae_response_header header;
	uint64_t num_metric_names;
	fpga_metric *metrics;
	fpga_result result;
} opae_fpgaGetMetricsByName_response;

char *opae_encode_fpgaGetMetricsByName_response_39(
	opae_fpgaGetMetricsByName_response *resp,
	int json_flags);
bool opae_decode_fpgaGetMetricsByName_response_39(
	const char *json,
	opae_fpgaGetMetricsByName_response *resp);

typedef struct {
	opae_response_header header;
	metric_threshold *metric_threshold;
	uint32_t num_thresholds;
	fpga_result result;
} opae_fpgaGetMetricsThresholdInfo_response;

char *opae_encode_fpgaGetMetricsThresholdInfo_response_40(
	opae_fpgaGetMetricsThresholdInfo_response *resp,
	int json_flags);
bool opae_decode_fpgaGetMetricsThresholdInfo_response_40(
	const char *json,
	opae_fpgaGetMetricsThresholdInfo_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaReconfigureSlotByName_response;

char *opae_encode_fpgaReconfigureSlotByName_response_41(
	opae_fpgaReconfigureSlotByName_response *resp,
	int json_flags);
bool opae_decode_fpgaReconfigureSlotByName_response_41(
	const char *json,
	opae_fpgaReconfigureSlotByName_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaBufMemSet_response;

char *opae_encode_fpgaBufMemSet_response_42(
	opae_fpgaBufMemSet_response *resp,
	int json_flags);
bool opae_decode_fpgaBufMemSet_response_42(
	const char *json,
	opae_fpgaBufMemSet_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaBufMemCpyToRemote_response;

char *opae_encode_fpgaBufMemCpyToRemote_response_43(
	opae_fpgaBufMemCpyToRemote_response *resp,
	int json_flags);
bool opae_decode_fpgaBufMemCpyToRemote_response_43(
	const char *json,
	opae_fpgaBufMemCpyToRemote_response *resp);

typedef struct {
	opae_response_header header;
	fpga_result result;
} opae_fpgaBufPoll_response;

char *opae_encode_fpgaBufPoll_response_44(
	opae_fpgaBufPoll_response *resp,
	int json_flags);
bool opae_decode_fpgaBufPoll_response_44(
	const char *json,
	opae_fpgaBufPoll_response *resp);

typedef struct {
	opae_response_header header;
	int cmp_result;
	fpga_result result;
} opae_fpgaBufMemCmp_response;

char *opae_encode_fpgaBufMemCmp_response_45(
	opae_fpgaBufMemCmp_response *resp,
	int json_flags);
bool opae_decode_fpgaBufMemCmp_response_45(
	const char *json,
	opae_fpgaBufMemCmp_response *resp);




#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __OPAE_SERIALIZE_RESPONSE_H__
