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

#include "mock/opae_fixtures.h"

using namespace opae::testing;

#include <stdio.h>
#include <assert.h>
#include <opae/properties.h>
#include "request.h"
#include "action.h"
#include "mock/opae_std.h"

extern "C" {
int opae_get_host_name_buf(char *name, size_t len);
const char *opae_get_host_name(void);
}


TEST(request, test0)
{

}

const uint16_t segment = 0x0000;
const uint8_t bus = 0x3f;
const uint8_t device = 0x00;
const uint8_t function = 0;
const uint16_t vendor_id = 0x8086;
const uint16_t device_id = 0x0b30;

const int json_flags = JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY;

const char *TEST_GUID_STR = "ae2878a7-926f-4332-aba1-2b952ad6df8e";

void device_filter(fpga_properties props)
{
        fpgaPropertiesSetObjectType(props, FPGA_DEVICE);
        fpgaPropertiesSetSegment(props, segment);
        fpgaPropertiesSetBus(props, bus);
        fpgaPropertiesSetDevice(props, device);
        fpgaPropertiesSetFunction(props, function);
        fpgaPropertiesSetVendorID(props, vendor_id);
        fpgaPropertiesSetDeviceID(props, device_id);
}

void assert_device_filter(fpga_properties props)
{
	fpga_objtype objtype;
	uint16_t u16;
	uint8_t u8;
	fpga_result res;

	res = fpgaPropertiesGetObjectType(props, &objtype);
	assert(!res && (objtype == FPGA_DEVICE));

	u16 = 0;
	res = fpgaPropertiesGetSegment(props, &u16);
	assert(!res && (u16 == segment));

	u8 = 0;
	res = fpgaPropertiesGetBus(props, &u8);
	assert(!res && (u8 == bus));

	u8 = 0;
	res = fpgaPropertiesGetDevice(props, &u8);
	assert(!res && (u8 == device));

	u8 = 0;
	res = fpgaPropertiesGetFunction(props, &u8);
	assert(!res && (u8 == function));

	u16 = 0;
	res = fpgaPropertiesGetVendorID(props, &u16);
	assert(!res && (u16 == vendor_id));

	u16 = 0;
	res = fpgaPropertiesGetDeviceID(props, &u16);
	assert(!res && (u16 == device_id));
}

void accel_filter(fpga_properties props)
{
        fpgaPropertiesSetObjectType(props, FPGA_ACCELERATOR);
        fpgaPropertiesSetSegment(props, segment);
        fpgaPropertiesSetBus(props, bus);
        fpgaPropertiesSetDevice(props, device);
        fpgaPropertiesSetFunction(props, function);
        fpgaPropertiesSetVendorID(props, vendor_id);
        fpgaPropertiesSetDeviceID(props, device_id);
}

void assert_accel_filter(fpga_properties props)
{
	fpga_objtype objtype;
	uint16_t u16;
	uint8_t u8;
	fpga_result res;

	res = fpgaPropertiesGetObjectType(props, &objtype);
	assert(!res && (objtype == FPGA_ACCELERATOR));

	u16 = 0;
	res = fpgaPropertiesGetSegment(props, &u16);
	assert(!res && (u16 == segment));

	u8 = 0;
	res = fpgaPropertiesGetBus(props, &u8);
	assert(!res && (u8 == bus));

	u8 = 0;
	res = fpgaPropertiesGetDevice(props, &u8);
	assert(!res && (u8 == device));

	u8 = 0;
	res = fpgaPropertiesGetFunction(props, &u8);
	assert(!res && (u8 == function));

	u16 = 0;
	res = fpgaPropertiesGetVendorID(props, &u16);
	assert(!res && (u16 == vendor_id));

	u16 = 0;
	res = fpgaPropertiesGetDeviceID(props, &u16);
	assert(!res && (u16 == device_id));
}

opae_remote_context remote_context;

void test_fpgaEnumerate_request_0(void)
{
	char *json = NULL;
	const uint32_t num_filters = 2;
	const uint32_t max_tokens = 5;

	opae_fpgaEnumerate_request req;
	opae_fpgaEnumerate_response resp;

	char *response = NULL;

	req.filters = (fpga_properties *)
		opae_calloc(num_filters, sizeof(fpga_properties));
	req.num_filters = num_filters;
	req.max_tokens = max_tokens;

	fpgaGetProperties(NULL, &req.filters[0]);
	device_filter(req.filters[0]);
	fpgaGetProperties(NULL, &req.filters[1]);
	accel_filter(req.filters[1]);

	json = opae_encode_fpgaEnumerate_request_0(&req, json_flags);

	fpgaDestroyProperties(&req.filters[0]);
	fpgaDestroyProperties(&req.filters[1]);
	opae_free(req.filters);

	printf("%s\n", json);

	memset(&req, 0, sizeof(req));
	assert(opae_decode_fpgaEnumerate_request_0(json, &req));

	assert(req.header.request_id == 0);
	assert(!strcmp(req.header.request_name, "fpgaEnumerate_request_0"));
	assert(req.header.serial == 0);
	assert(!strcmp(opae_get_host_name(), req.header.from));

	assert(req.filters);
	assert(req.num_filters == num_filters);
	assert(req.max_tokens == max_tokens);

	assert_device_filter(req.filters[0]);
	assert_accel_filter(req.filters[1]);

	fpgaDestroyProperties(&req.filters[0]);
	fpgaDestroyProperties(&req.filters[1]);
	opae_free(req.filters);

	opae_remote_handle_client_request(&remote_context, json, &response);

	printf("%s\n", response);

	assert(opae_decode_fpgaEnumerate_response_0(response, &resp));

	assert(resp.header.request_id == 0);
	assert(!strcmp(resp.header.request_name, "fpgaEnumerate_request_0"));
	assert(!strcmp(resp.header.response_name, "fpgaEnumerate_response_0"));
	assert(resp.header.serial == 0);
	assert(!strcmp(opae_get_host_name(), resp.header.from));
	assert(!strcmp(opae_get_host_name(), resp.header.to));
	assert(resp.num_matches == 0);
	assert(resp.result == FPGA_OK);

	opae_free(response);
	opae_free(json);
}

void test_fpgaDestroyToken_request_1(void)
{
	fpga_remote_id token_id = {
		.hostname = { 0, },
		.unique_id = 2
	};

	opae_fpgaDestroyToken_request req;
	opae_fpgaDestroyToken_response resp;

	opae_get_host_name_buf(token_id.hostname, HOST_NAME_MAX);

	req.token_id = token_id;

	char *json;
	char *response = NULL;

	json = opae_encode_fpgaDestroyToken_request_1(&req, json_flags);

	printf("%s\n", json);

	memset(&req, 0, sizeof(req));
	assert(opae_decode_fpgaDestroyToken_request_1(json, &req));
	
	assert(req.header.request_id == 1);
	assert(!strcmp(req.header.request_name, "fpgaDestroyToken_request_1"));
	assert(req.header.serial == 1);
	assert(!strcmp(opae_get_host_name(), req.header.from));

	assert(!strcmp(req.token_id.hostname, token_id.hostname));
	assert(req.token_id.unique_id == 2);

	opae_remote_handle_client_request(&remote_context, json, &response);

	printf("%s\n", response);

	assert(opae_decode_fpgaDestroyToken_response_1(response, &resp));

	assert(resp.header.request_id == 1);
	assert(!strcmp(resp.header.request_name, "fpgaDestroyToken_request_1"));
	assert(!strcmp(resp.header.response_name, "fpgaDestroyToken_response_1"));
	assert(resp.header.serial == 1);
	assert(!strcmp(opae_get_host_name(), resp.header.from));
	assert(!strcmp(opae_get_host_name(), resp.header.to));
	assert(resp.result == FPGA_INVALID_PARAM);

	opae_free(json);
	opae_free(response);
}

void test_fpgaCloneToken_request_2(void)
{
	fpga_remote_id src_token_id = {
		.hostname = { 0, },
		.unique_id = 2
	};

	opae_fpgaCloneToken_request req;
	opae_fpgaCloneToken_response resp;

	opae_get_host_name_buf(src_token_id.hostname, HOST_NAME_MAX);

	req.src_token_id = src_token_id;

	char *json;
	char *response = NULL;

	json = opae_encode_fpgaCloneToken_request_2(&req, json_flags);

	printf("%s\n", json);

	memset(&req, 0, sizeof(req));
	assert(opae_decode_fpgaCloneToken_request_2(json, &req));

	assert(req.header.request_id == 2);
	assert(!strcmp(req.header.request_name, "fpgaCloneToken_request_2"));
	assert(req.header.serial == 2);
	assert(!strcmp(opae_get_host_name(), req.header.from));

	assert(!strcmp(req.src_token_id.hostname, src_token_id.hostname));
	assert(req.src_token_id.unique_id == 2);

	opae_remote_handle_client_request(&remote_context, json, &response);

	printf("%s\n", response);

	assert(opae_decode_fpgaCloneToken_response_2(response, &resp));

	assert(resp.header.request_id == 2);
	assert(!strcmp(resp.header.request_name, "fpgaCloneToken_request_2"));
	assert(!strcmp(resp.header.response_name, "fpgaCloneToken_response_2"));
	assert(resp.header.serial == 2);
	assert(!strcmp(opae_get_host_name(), resp.header.from));
	assert(!strcmp(opae_get_host_name(), resp.header.to));
	assert(resp.result == FPGA_INVALID_PARAM);

	opae_free(json);
	opae_free(response);
}

void test_fpgaGetProperties_request_3(void)
{
	fpga_remote_id token_id = {
		.hostname = { 0, },
		.unique_id = 3
	};

	opae_fpgaGetProperties_request req;
	opae_fpgaGetProperties_response resp;

	opae_get_host_name_buf(token_id.hostname, HOST_NAME_MAX);

	req.token_id = token_id;

	char *json;
	char *response = NULL;

	json = opae_encode_fpgaGetProperties_request_3(&req, json_flags);

	printf("%s\n", json);

	memset(&req, 0, sizeof(req));
        assert(opae_decode_fpgaGetProperties_request_3(json, &req));

	assert(req.header.request_id == 3);
	assert(!strcmp(req.header.request_name, "fpgaGetProperties_request_3"));
	assert(req.header.serial == 3);
	assert(!strcmp(opae_get_host_name(), req.header.from));

	assert(!strcmp(req.token_id.hostname, token_id.hostname));
	assert(req.token_id.unique_id == 3);

	opae_remote_handle_client_request(&remote_context, json, &response);

	printf("%s\n", response);

	assert(opae_decode_fpgaGetProperties_response_3(response, &resp));

	assert(resp.header.request_id == 3);
	assert(!strcmp(resp.header.request_name, "fpgaGetProperties_request_3"));
	assert(!strcmp(resp.header.response_name, "fpgaGetProperties_response_3"));
	assert(resp.header.serial == 3);
	assert(!strcmp(opae_get_host_name(), resp.header.from));
	assert(!strcmp(opae_get_host_name(), resp.header.to));
	assert(resp.result == FPGA_INVALID_PARAM);

	opae_free(json);
	opae_free(response);
}

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	opae_init_remote_context(&remote_context);

	test_fpgaEnumerate_request_0();
	test_fpgaDestroyToken_request_1();
	test_fpgaCloneToken_request_2();
	test_fpgaGetProperties_request_3();




	opae_release_remote_context(&remote_context);

	return 0;
}
