// Copyright(c) 2018, Intel Corporation
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

#ifdef __cplusplus

extern "C" {
#endif
#include <opae/enum.h>
#include <opae/properties.h>
#include "bitstream_int.h"

#ifdef __cplusplus
}
#endif

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

#define DECLARE_GUID(var, ...) uint8_t var[16] = {__VA_ARGS__};

using namespace common_test;

/**
* @test    bs_metadata_01
* @brief   Tests: read_gbs_metadata 
* @details read_gbs_metadata returns BS metadata
*          Then the return value is FPGA_OK
*/
TEST(LibopaecBSMetadataCommonMOCKHW, bs_metadata_01) {

	// Invalid input parameters
	fpga_result result = read_gbs_metadata(NULL, NULL);
	EXPECT_NE(result, FPGA_OK);

	// Invalid input parameter 
	struct gbs_metadata gbs_metadata ;
	result = read_gbs_metadata(NULL, &gbs_metadata);
	EXPECT_NE(result, FPGA_OK);

	// Invalid input parameter 
	uint8_t bitstream[10] = "abcd";
	result = read_gbs_metadata(bitstream, NULL);
	EXPECT_NE(result, FPGA_OK);

	// Invalid input bitstream 
	result = read_gbs_metadata(bitstream, &gbs_metadata);
	EXPECT_NE(result, FPGA_OK);

	// Invalid bitstream metadata  size
	uint8_t bitstream_guid[] = "XeonFPGA·GBSv001S";
	result = read_gbs_metadata(bitstream_guid, &gbs_metadata);
	EXPECT_NE(result, FPGA_OK);

	// Zero metadata length  with no data
	uint8_t bitstream_guid_invalid1[] = "XeonFPGA·GBSv001";
	result = read_gbs_metadata(bitstream_guid_invalid1, &gbs_metadata);
	EXPECT_NE(result, FPGA_OK);

	// Invalid metadata length 
	uint8_t bitstream_guid_invalid2[] = "XeonFPGA·GBSv001S {\"version/\": 640, \"afu-image\":  \
		{\"clock-frequency-high\": 312, \"clock-frequency-low\": 156,\
		\"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
		\"magic-no\": 488605312, \"accelerator-clusters\"\
		: [{\"total-contexts\": 1, \"name\": \"nlb_400\", \"accelerator-type-uuid\": \
		\"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";

	result = read_gbs_metadata(bitstream_guid_invalid2, &gbs_metadata);
	EXPECT_NE(result, FPGA_OK);

	// Invalid input bitstream
	uint8_t bitstream_guid_invalid3[] = "XeonFPGA·GBSv001\53\02\00\00{\"version/\": 640, \"afu-image\": \
		{\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
		\"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
		\"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\": 1, \
		\"name\": \"nlb_400\", \"accelerator-type-uuid\":\
		\"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-99name\": \"MCP\"}";\
	result = read_gbs_metadata(bitstream_guid_invalid3, &gbs_metadata);
	EXPECT_NE(result, FPGA_OK);

	// Invalid metadata platform
	uint8_t bitstream_guid_invalid4[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\":\
		{\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
		\"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
		\"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\":1,\
		\"name\": \"nlb_400\", \"accelerator-type-uuid\":\
		\"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
	result = read_gbs_metadata(bitstream_guid_invalid4, &gbs_metadata);
	EXPECT_NE(result, FPGA_OK);

	// Invalid metadata afu-image node
	uint8_t bitstream_guid_invalid5[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640 }, \"platform-name\": \"MCP\"}";
	result = read_gbs_metadata(bitstream_guid_invalid5, &gbs_metadata);
	EXPECT_NE(result, FPGA_OK);

	// Invalid metadata interface-uuid
	uint8_t bitstream_guid_invalid6[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
		{\"clock-frequency-high\": 312, \"clock-frequency-low\": 156,\
		\"power\": 50,  \"magic-no\": 488605312, \"accelerator-clusters\": \
		[{\"total-contexts\": 1, \"name\": \"nlb_400\", \"accelerator-type-uuid\":\
		\"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
	result = read_gbs_metadata(bitstream_guid_invalid6, &gbs_metadata);
	EXPECT_NE(result, FPGA_OK);

	// Invalid metadata afu-uuid
	uint8_t bitstream_guid_invalid7[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\": \
		{\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
		\"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
		\"magic-no\": 488605312, \"accelerator-clusters\":\
		[{\"total-contexts\": 1, \"name\": \"nlb_400\"}]}, \"platform-name\": \"MCP\"}";
	result = read_gbs_metadata(bitstream_guid_invalid7, &gbs_metadata);
	EXPECT_NE(result, FPGA_OK);

	// Invalid input bitstream
	uint8_t bitstream_guid_invalid8[] = "XeonFPGA·GBSv001\00\00\00\00{\"version\": 640, \"afu-image\":\
		{\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
		\"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
		\"magic-no\": 488605312, \"accelerator-clusters\": [{\"total-contexts\":1,\
		\"name\": \"nlb_400\", \"accelerator-type-uuid\":\
		\"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
	result = read_gbs_metadata(bitstream_guid_invalid8, &gbs_metadata);
	EXPECT_NE(result, FPGA_OK);
}

/**
* @test    bs_metadata_02
* @brief   Tests: validate_bitstream_metadata
* @details validate_bitstream_metadata validates BS metadata
*          Retuns FPGA_OK if metadata is valid
*/
TEST(LibopaecBSMetadataCommonMOCKHW, bs_metadata_02) {
	fpga_result result;

	// Invalid input bitstream
	uint8_t bitstream_guid_invalid[] = "XeonFPGA·GBSv001\53\02\00\00{\"version\": 640, \"afu-image\":\
		{\"clock-frequency-high\": 312, \"clock-frequency-low\": 156, \
		\"power\": 50, \"interface-uuid\": \"1a422218-6dba-448e-b302-425cbcde1406\", \
		\"magic-no1\": 488605312, \"accelerator-clusters\": [{\"total-contexts\":1,\
		\"name\": \"nlb_400\", \"accelerator-type-uuid\":\
		\"d8424dc4-a4a3-c413-f89e-433683f9040b\"}]}, \"platform-name\": \"MCP\"}";
	result = validate_bitstream_metadata((void*) 1234, bitstream_guid_invalid);
	EXPECT_NE(result, FPGA_OK);

}
