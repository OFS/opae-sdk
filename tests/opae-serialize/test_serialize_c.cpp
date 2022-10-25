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


TEST(serialize, test1)
{

}


#include <stdio.h>
#include <assert.h>
#include "serialize.h"
#include "mock/opae_std.h"
#include <opae/properties.h>

const char *TEST_GUID_STR = "ae2878a7-926f-4332-aba1-2b952ad6df8e";

const uint16_t segment = 0x0b5a;
const uint8_t bus = 0x5e;
const uint8_t device = 0xe5;
const uint8_t function = 1;
const uint8_t socket_id = 3;

const uint16_t vendor_id = 0x8086;
const uint16_t device_id = 0xbcce;
fpga_guid guid;

const uint64_t object_id = 0xdeadbeefdecafbad;
const uint32_t num_errors = 50;

const fpga_interface ifc = FPGA_IFC_DFL;

const uint16_t subsystem_vendor_id = 0x8087;
const uint16_t subsystem_device_id = 0x1770;

const uint32_t num_slots = 2;
const uint64_t bbs_id = 0xabadbeefdeadc01a;
const fpga_version bbs_version = { 0, 1, 0 };

const fpga_accelerator_state accelerator_state = FPGA_ACCELERATOR_ASSIGNED;
const uint32_t num_mmio = 2;
const uint32_t num_interrupts = 4;

const char *hostname = "machine.company.net";

fpga_properties device_props(void)
{
	fpga_properties props = NULL;
	fpgaGetProperties(NULL, &props);

	fpgaPropertiesSetObjectType(props, FPGA_DEVICE);
	fpgaPropertiesSetSegment(props, segment);
	fpgaPropertiesSetBus(props, bus);
	fpgaPropertiesSetDevice(props, device);
	fpgaPropertiesSetFunction(props, function);
	fpgaPropertiesSetSocketID(props, socket_id);
	fpgaPropertiesSetVendorID(props, vendor_id);
	fpgaPropertiesSetDeviceID(props, device_id);

	uuid_parse(TEST_GUID_STR, guid);
	fpgaPropertiesSetGUID(props, guid);

	fpgaPropertiesSetObjectID(props, object_id);
	fpgaPropertiesSetNumErrors(props, num_errors);
	fpgaPropertiesSetInterface(props, ifc);
	fpgaPropertiesSetSubsystemVendorID(props, subsystem_vendor_id);
	fpgaPropertiesSetSubsystemDeviceID(props, subsystem_device_id);
	fpgaPropertiesSetHostname(props, hostname, strlen(hostname));

	fpgaPropertiesSetNumSlots(props, num_slots);
	fpgaPropertiesSetBBSID(props, bbs_id);
	fpgaPropertiesSetBBSVersion(props, bbs_version);

	return props;
}

fpga_properties short_props(void)
{
	fpga_properties props = NULL;
	fpgaGetProperties(NULL, &props);

	fpgaPropertiesSetObjectType(props, FPGA_DEVICE);

	fpgaPropertiesSetSegment(props, segment);
	fpgaPropertiesSetBus(props, bus);
	fpgaPropertiesSetDevice(props, device);
	fpgaPropertiesSetFunction(props, function);
	fpgaPropertiesSetSocketID(props, socket_id);
	fpgaPropertiesSetVendorID(props, vendor_id);
	fpgaPropertiesSetDeviceID(props, device_id);

	uuid_parse(TEST_GUID_STR, guid);
	fpgaPropertiesSetGUID(props, guid);

	fpgaPropertiesSetHostname(props, hostname, strlen(hostname));

	return props;
}

void verify_short_props(fpga_properties props)
{
	fpga_result res;
	uint8_t u8;
	uint16_t u16;
	fpga_guid guid;
	char buf[64];
	fpga_objtype objtype;

	assert(props);

	res = fpgaPropertiesGetObjectType(props, &objtype);
	assert(!res && (objtype == FPGA_DEVICE));

	res = fpgaPropertiesGetSegment(props, &u16);
	assert(!res && (u16 == segment));

	res = fpgaPropertiesGetBus(props, &u8);
	assert(!res && (u8 == bus));

	res = fpgaPropertiesGetDevice(props, &u8);
	assert(!res && (u8 == device));

	res = fpgaPropertiesGetFunction(props, &u8);
	assert(!res && (u8 == function));

	res = fpgaPropertiesGetSocketID(props, &u8);
	assert(!res && (u8 == socket_id));

	res = fpgaPropertiesGetVendorID(props, &u16);
	assert(!res && (u16 == vendor_id));

	res = fpgaPropertiesGetDeviceID(props, &u16);
	assert(!res && (u16 == device_id));

	res = fpgaPropertiesGetGUID(props, &guid);
	assert(!res);
	uuid_unparse(guid, buf);
	assert(!strcmp(buf, TEST_GUID_STR));

	res = fpgaPropertiesGetHostname(props, buf, sizeof(buf) - 1);
	assert(!res && !strcmp(buf, hostname));
}

void verify_device_props(fpga_properties props)
{
	fpga_objtype objtype;
	fpga_result res;
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;
	fpga_guid guid;
	char buf[64];
	fpga_interface interface;
	fpga_version ver;

	assert(props);

	res = fpgaPropertiesGetObjectType(props, &objtype);
	assert(!res && (objtype == FPGA_DEVICE));

	res = fpgaPropertiesGetSegment(props, &u16);
	assert(!res && (u16 == segment));

	res = fpgaPropertiesGetBus(props, &u8);
	assert(!res && (u8 == bus));

	res = fpgaPropertiesGetDevice(props, &u8);
	assert(!res && (u8 == device));

	res = fpgaPropertiesGetFunction(props, &u8);
	assert(!res && (u8 == function));

	res = fpgaPropertiesGetSocketID(props, &u8);
	assert(!res && (u8 == socket_id));

	res = fpgaPropertiesGetVendorID(props, &u16);
	assert(!res && (u16 == vendor_id));

	res = fpgaPropertiesGetDeviceID(props, &u16);
	assert(!res && (u16 == device_id));

	res = fpgaPropertiesGetGUID(props, &guid);
	assert(!res);
	uuid_unparse(guid, buf);
	assert(!strcmp(buf, TEST_GUID_STR));

	res = fpgaPropertiesGetObjectID(props, &u64);
	assert(!res && (u64 == object_id));

	res = fpgaPropertiesGetNumErrors(props, &u32);
	assert(!res && (u32 == num_errors));

	res = fpgaPropertiesGetInterface(props, &interface);
	assert(!res && (interface == ifc));

	res = fpgaPropertiesGetSubsystemVendorID(props, &u16);
	assert(!res && (u16 == subsystem_vendor_id));

	res = fpgaPropertiesGetSubsystemDeviceID(props, &u16);
	assert(!res && (u16 == subsystem_device_id));

	res = fpgaPropertiesGetHostname(props, buf, sizeof(buf) - 1);
	assert(!res && !strcmp(buf, hostname));

	res = fpgaPropertiesGetNumSlots(props, &u32);
	assert(!res && (u32 == num_slots));

	res = fpgaPropertiesGetBBSID(props, &u64);
	assert(!res && (u64 == bbs_id));

	res = fpgaPropertiesGetBBSVersion(props, &ver);
	assert(!res && (ver.major == bbs_version.major));
	assert(!res && (ver.minor == bbs_version.minor));
	assert(!res && (ver.patch == bbs_version.patch));
}

fpga_properties accelerator_props(void)
{
	fpga_properties props = NULL;
	fpgaGetProperties(NULL, &props);

	fpgaPropertiesSetObjectType(props, FPGA_ACCELERATOR);
	fpgaPropertiesSetSegment(props, segment);
	fpgaPropertiesSetBus(props, bus);
	fpgaPropertiesSetDevice(props, device);
	fpgaPropertiesSetFunction(props, function);
	fpgaPropertiesSetSocketID(props, socket_id);
	fpgaPropertiesSetVendorID(props, vendor_id);
	fpgaPropertiesSetDeviceID(props, device_id);

	uuid_parse(TEST_GUID_STR, guid);
	fpgaPropertiesSetGUID(props, guid);

	fpgaPropertiesSetObjectID(props, object_id);
	fpgaPropertiesSetNumErrors(props, num_errors);
	fpgaPropertiesSetInterface(props, ifc);
	fpgaPropertiesSetSubsystemVendorID(props, subsystem_vendor_id);
	fpgaPropertiesSetSubsystemDeviceID(props, subsystem_device_id);
	fpgaPropertiesSetHostname(props, hostname, strlen(hostname));

	fpgaPropertiesSetAcceleratorState(props, accelerator_state);
	fpgaPropertiesSetNumMMIO(props, num_mmio);
	fpgaPropertiesSetNumInterrupts(props, num_interrupts);

	return props;
}

void verify_accelerator_props(fpga_properties props)
{
	fpga_objtype objtype;
	fpga_result res;
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint64_t u64;
	fpga_guid guid;
	char buf[64];
	fpga_interface interface;
	fpga_accelerator_state state;
	uint32_t mmios;
	uint32_t irqs;

	assert(props);

	res = fpgaPropertiesGetObjectType(props, &objtype);
	assert(!res && (objtype == FPGA_ACCELERATOR));

	res = fpgaPropertiesGetSegment(props, &u16);
	assert(!res && (u16 == segment));

	res = fpgaPropertiesGetBus(props, &u8);
	assert(!res && (u8 == bus));

	res = fpgaPropertiesGetDevice(props, &u8);
	assert(!res && (u8 == device));

	res = fpgaPropertiesGetFunction(props, &u8);
	assert(!res && (u8 == function));

	res = fpgaPropertiesGetSocketID(props, &u8);
	assert(!res && (u8 == socket_id));

	res = fpgaPropertiesGetVendorID(props, &u16);
	assert(!res && (u16 == vendor_id));

	res = fpgaPropertiesGetDeviceID(props, &u16);
	assert(!res && (u16 == device_id));

	res = fpgaPropertiesGetGUID(props, &guid);
	assert(!res);
	uuid_unparse(guid, buf);
	assert(!strcmp(buf, TEST_GUID_STR));

	res = fpgaPropertiesGetObjectID(props, &u64);
	assert(!res && (u64 == object_id));

	res = fpgaPropertiesGetNumErrors(props, &u32);
	assert(!res && (u32 == num_errors));

	res = fpgaPropertiesGetInterface(props, &interface);
	assert(!res && (interface == ifc));

	res = fpgaPropertiesGetSubsystemVendorID(props, &u16);
	assert(!res && (u16 == subsystem_vendor_id));

	res = fpgaPropertiesGetSubsystemDeviceID(props, &u16);
	assert(!res && (u16 == subsystem_device_id));

	res = fpgaPropertiesGetHostname(props, buf, sizeof(buf) - 1);
	assert(!res && !strcmp(buf, hostname));

	res = fpgaPropertiesGetAcceleratorState(props, &state);
	assert(!res && (state == accelerator_state));

	res = fpgaPropertiesGetNumMMIO(props, &mmios);
	assert(!res && (mmios == num_mmio));

	res = fpgaPropertiesGetNumInterrupts(props, &irqs);
	assert(!res && (irqs == num_interrupts));
}

void test_properties_serialize(void)
{
	char *json;
	int json_flags = JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY;
	fpga_properties props;

	props = device_props();
	//props = short_props();
	json = opae_ser_properties_to_json(props, json_flags);
	fpgaDestroyProperties(&props);

	printf("%s\n", json);

	opae_ser_json_to_properties(json, &props);
	opae_free(json);
	verify_device_props(props);
	//verify_short_props(props);
	fpgaDestroyProperties(&props);

	printf("\n");
	props = accelerator_props();
	json = opae_ser_properties_to_json(props, json_flags);
	fpgaDestroyProperties(&props);

	printf("%s\n", json);

	opae_ser_json_to_properties(json, &props);
	opae_free(json);
	verify_accelerator_props(props);
	fpgaDestroyProperties(&props);
}

void test_token_header_serialize(void)
{
	char *json;
	int json_flags = JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY;
	char buf[64];

	fpga_token_header before = {
		.magic = 0x46504741544f4b4e,
		.vendor_id = 0x8086,
		.device_id = 0xbcce,
		.segment = 0x0001,
		.bus = 0x5e,
		.device = 0xe5,
		.function = 7,
		.interface = FPGA_IFC_DFL,
		.objtype = FPGA_DEVICE,
		.object_id = 0xa500000000ef0000,
		.guid = { 0, },
		.subsystem_vendor_id = 0x8087,
		.subsystem_device_id = 0x1770,
		.hostname = { 'm', 'a', 'c', 'h', 'i', 'n', 'e',
                              '.', 'c', 'o', 'm', 'p', 'a', 'n', 'y',
                              '.', 'n', 'e', 't', 0 },
		.remote_id = 3
	};
	fpga_token_header after;

	uuid_parse(TEST_GUID_STR, before.guid);

	json = opae_ser_token_header_to_json(&before, json_flags);
	if (!opae_ser_json_to_token_header(json, &after))
		printf("whoops!\n");

	printf("%s\n", json);
	opae_free(json);

	assert(after.magic == 0x46504741544f4b4e);
	assert(after.vendor_id == 0x8086);
	assert(after.device_id == 0xbcce);
	assert(after.segment == 0x0001);
	assert(after.bus == 0x5e);
	assert(after.device == 0xe5);
	assert(after.function == 7);
	assert(after.interface == FPGA_IFC_DFL);
	assert(after.objtype == FPGA_DEVICE);
	assert(after.object_id == 0xa500000000ef0000);
	uuid_unparse(after.guid, buf);
	assert(!strcmp(buf, TEST_GUID_STR));
	assert(after.subsystem_vendor_id == 0x8087);
	assert(!strcmp(after.hostname, "machine.company.net"));
	assert(after.remote_id == 3);
}

struct json_object *root = NULL;
enum json_tokener_error j_err = json_tokener_success;

#define JSON_START_PARSE(__str)                           \
do {                                                      \
	root = json_tokener_parse_verbose(__str, &j_err); \
	assert(root && (j_err == json_tokener_success));  \
} while(0)

#define JSON_END_PARSE(__str)  \
do {                           \
	json_object_put(root); \
	opae_free(__str);      \
} while(0)

#define JSON_START_ENCODE()              \
do {                                     \
	root = json_object_new_object(); \
	assert(root);                    \
} while(0)

#define JSON_END_ENCODE                                                                         \
({                                                                                              \
	char *json = opae_strdup(json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN)); \
	json_object_put(root);                                                                  \
	root = NULL;                                                                            \
	j_err = json_tokener_success;                                                           \
	json;                                                                                   \
})

void test_result_serialize(void)
{
	char *json;
	fpga_result res;

	JSON_START_ENCODE();
	assert(opae_ser_fpga_result_to_json_obj(FPGA_OK, root));
	json = JSON_END_ENCODE;
	printf("%s\n", json);
	assert(!strcmp(json, "{\"fpga_result\":\"FPGA_OK\"}"));

	JSON_START_PARSE(json);
	assert(opae_ser_json_to_fpga_result_obj(root, &res));
	assert(res == FPGA_OK);
	JSON_END_PARSE(json);


	JSON_START_ENCODE();
	assert(opae_ser_fpga_result_to_json_obj(FPGA_INVALID_PARAM, root));
	json = JSON_END_ENCODE;
	printf("%s\n", json);
	assert(!strcmp(json, "{\"fpga_result\":\"FPGA_INVALID_PARAM\"}"));

	JSON_START_PARSE(json);
	assert(opae_ser_json_to_fpga_result_obj(root, &res));
	assert(res == FPGA_INVALID_PARAM);
	JSON_END_PARSE(json);


	JSON_START_ENCODE();
	assert(opae_ser_fpga_result_to_json_obj(FPGA_BUSY, root));
	json = JSON_END_ENCODE;
	printf("%s\n", json);
	assert(!strcmp(json, "{\"fpga_result\":\"FPGA_BUSY\"}"));

	JSON_START_PARSE(json);
	assert(opae_ser_json_to_fpga_result_obj(root, &res));
	assert(res == FPGA_BUSY);
	JSON_END_PARSE(json);


	JSON_START_ENCODE();
	assert(opae_ser_fpga_result_to_json_obj(FPGA_EXCEPTION, root));
	json = JSON_END_ENCODE;
	printf("%s\n", json);
	assert(!strcmp(json, "{\"fpga_result\":\"FPGA_EXCEPTION\"}"));

	JSON_START_PARSE(json);
	assert(opae_ser_json_to_fpga_result_obj(root, &res));
	assert(res == FPGA_EXCEPTION);
	JSON_END_PARSE(json);


	JSON_START_ENCODE();
	assert(opae_ser_fpga_result_to_json_obj(FPGA_NOT_FOUND, root));
	json = JSON_END_ENCODE;
	printf("%s\n", json);
	assert(!strcmp(json, "{\"fpga_result\":\"FPGA_NOT_FOUND\"}"));

	JSON_START_PARSE(json);
	assert(opae_ser_json_to_fpga_result_obj(root, &res));
	assert(res == FPGA_NOT_FOUND);
	JSON_END_PARSE(json);


	JSON_START_ENCODE();
	assert(opae_ser_fpga_result_to_json_obj(FPGA_NO_MEMORY, root));
	json = JSON_END_ENCODE;
	printf("%s\n", json);
	assert(!strcmp(json, "{\"fpga_result\":\"FPGA_NO_MEMORY\"}"));

	JSON_START_PARSE(json);
	assert(opae_ser_json_to_fpga_result_obj(root, &res));
	assert(res == FPGA_NO_MEMORY);
	JSON_END_PARSE(json);


	JSON_START_ENCODE();
	assert(opae_ser_fpga_result_to_json_obj(FPGA_NOT_SUPPORTED, root));
	json = JSON_END_ENCODE;
	printf("%s\n", json);
	assert(!strcmp(json, "{\"fpga_result\":\"FPGA_NOT_SUPPORTED\"}"));

	JSON_START_PARSE(json);
	assert(opae_ser_json_to_fpga_result_obj(root, &res));
	assert(res == FPGA_NOT_SUPPORTED);
	JSON_END_PARSE(json);


	JSON_START_ENCODE();
	assert(opae_ser_fpga_result_to_json_obj(FPGA_NO_DRIVER, root));
	json = JSON_END_ENCODE;
	printf("%s\n", json);
	assert(!strcmp(json, "{\"fpga_result\":\"FPGA_NO_DRIVER\"}"));

	JSON_START_PARSE(json);
	assert(opae_ser_json_to_fpga_result_obj(root, &res));
	assert(res == FPGA_NO_DRIVER);
	JSON_END_PARSE(json);


	JSON_START_ENCODE();
	assert(opae_ser_fpga_result_to_json_obj(FPGA_NO_DAEMON, root));
	json = JSON_END_ENCODE;
	printf("%s\n", json);
	assert(!strcmp(json, "{\"fpga_result\":\"FPGA_NO_DAEMON\"}"));

	JSON_START_PARSE(json);
	assert(opae_ser_json_to_fpga_result_obj(root, &res));
	assert(res == FPGA_NO_DAEMON);
	JSON_END_PARSE(json);


	JSON_START_ENCODE();
	assert(opae_ser_fpga_result_to_json_obj(FPGA_NO_ACCESS, root));
	json = JSON_END_ENCODE;
	printf("%s\n", json);
	assert(!strcmp(json, "{\"fpga_result\":\"FPGA_NO_ACCESS\"}"));

	JSON_START_PARSE(json);
	assert(opae_ser_json_to_fpga_result_obj(root, &res));
	assert(res == FPGA_NO_ACCESS);
	JSON_END_PARSE(json);


	JSON_START_ENCODE();
	assert(opae_ser_fpga_result_to_json_obj(FPGA_RECONF_ERROR, root));
	json = JSON_END_ENCODE;
	printf("%s\n", json);
	assert(!strcmp(json, "{\"fpga_result\":\"FPGA_RECONF_ERROR\"}"));

	JSON_START_PARSE(json);
	assert(opae_ser_json_to_fpga_result_obj(root, &res));
	assert(res == FPGA_RECONF_ERROR);
	JSON_END_PARSE(json);
}

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	test_properties_serialize();
	test_token_header_serialize();
	test_result_serialize();

	return 0;
}
