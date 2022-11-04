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

#include <opae/properties.h>

#include "mock/opae_fixtures.h"
#include "serialize.h"
#include "mock/opae_std.h"

using namespace opae::testing;

const char *TEST_GUID_STR = "ae2878a7-926f-4332-aba1-2b952ad6df8e";

class serialize_base
{
 protected:
  serialize_base() :
    jroot_(nullptr)
  {}

  json_object *start_encode()
  {
    return jroot_ = json_object_new_object();
  }

  char *end_encode()
  {
    char *json = opae_strdup(json_object_to_json_string_ext(jroot_,
				json_to_string_flags_));

    json_object_put(jroot_);
    jroot_ = nullptr;

    return json;
  }

  json_object *start_decode(const char *json)
  {
    enum json_tokener_error j_err = json_tokener_success;
    jroot_ = json_tokener_parse_verbose(json, &j_err);
    EXPECT_NE(nullptr, jroot_);
    return jroot_;
  }

  void end_decode()
  {
    json_object_put(jroot_);
    jroot_ = nullptr;
  }

  json_object *jroot_;
  const int json_to_string_flags_ = JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY;
};

class serialize_f : public serialize_base, public ::testing::Test
{
 protected:
  virtual void SetUp() override
  {
    jroot_ = nullptr;
  }

  virtual void TearDown() override
  {
    if (jroot_) {
      json_object_put(jroot_);
      jroot_ = nullptr;
    }
  }
};

class serialize_props_f : public serialize_f
{
 protected:
  serialize_props_f()
  {
    uuid_parse(TEST_GUID_STR, guid_);
  }

  fpga_properties device_props()
  {
    fpga_properties props = NULL;
    fpgaGetProperties(NULL, &props);

    fpgaPropertiesSetObjectType(props, FPGA_DEVICE);
    fpgaPropertiesSetSegment(props, segment_);
    fpgaPropertiesSetBus(props, bus_);
    fpgaPropertiesSetDevice(props, device_);
    fpgaPropertiesSetFunction(props, function_);
    fpgaPropertiesSetSocketID(props, socket_id_);
    fpgaPropertiesSetVendorID(props, vendor_id_);
    fpgaPropertiesSetDeviceID(props, device_id_);

    fpgaPropertiesSetGUID(props, guid_);

    fpgaPropertiesSetObjectID(props, object_id_);
    fpgaPropertiesSetNumErrors(props, num_errors_);
    fpgaPropertiesSetInterface(props, ifc_);
    fpgaPropertiesSetSubsystemVendorID(props, subsystem_vendor_id_);
    fpgaPropertiesSetSubsystemDeviceID(props, subsystem_device_id_);
    fpgaPropertiesSetHostname(props, hostname_, strlen(hostname_));

    fpgaPropertiesSetNumSlots(props, num_slots_);
    fpgaPropertiesSetBBSID(props, bbs_id_);
    fpgaPropertiesSetBBSVersion(props, bbs_version_);

    return props;
  }

  fpga_properties accelerator_props()
  {
    fpga_properties props = NULL;
    fpgaGetProperties(NULL, &props);

    fpgaPropertiesSetObjectType(props, FPGA_ACCELERATOR);
    fpgaPropertiesSetSegment(props, segment_);
    fpgaPropertiesSetBus(props, bus_);
    fpgaPropertiesSetDevice(props, device_);
    fpgaPropertiesSetFunction(props, function_);
    fpgaPropertiesSetSocketID(props, socket_id_);
    fpgaPropertiesSetVendorID(props, vendor_id_);
    fpgaPropertiesSetDeviceID(props, device_id_);

    fpgaPropertiesSetGUID(props, guid_);

    fpgaPropertiesSetObjectID(props, object_id_);
    fpgaPropertiesSetNumErrors(props, num_errors_);
    fpgaPropertiesSetInterface(props, ifc_);
    fpgaPropertiesSetSubsystemVendorID(props, subsystem_vendor_id_);
    fpgaPropertiesSetSubsystemDeviceID(props, subsystem_device_id_);
    fpgaPropertiesSetHostname(props, hostname_, strlen(hostname_));

    fpgaPropertiesSetAcceleratorState(props, accelerator_state_);
    fpgaPropertiesSetNumMMIO(props, num_mmio_);
    fpgaPropertiesSetNumInterrupts(props, num_interrupts_);

    return props;
  }

  void verify_props(fpga_properties props, fpga_objtype objtype)
  {
    fpga_objtype ot;
    fpga_result res;
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    uint64_t u64;
    fpga_guid guid;
    char buf[64];
    fpga_interface interface;
    fpga_version ver;
    fpga_accelerator_state state;
    uint32_t mmios;
    uint32_t irqs;

    ASSERT_NE((fpga_properties)NULL, props);

    res = fpgaPropertiesGetObjectType(props, &ot);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(objtype, ot);

    res = fpgaPropertiesGetSegment(props, &u16);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(segment_, u16);

    res = fpgaPropertiesGetBus(props, &u8);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(bus_, u8);

    res = fpgaPropertiesGetDevice(props, &u8);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(device_, u8);

    res = fpgaPropertiesGetFunction(props, &u8);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(function_, u8);

    res = fpgaPropertiesGetSocketID(props, &u8);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(socket_id_, u8);

    res = fpgaPropertiesGetVendorID(props, &u16);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(vendor_id_, u16);

    res = fpgaPropertiesGetDeviceID(props, &u16);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(device_id_, u16);

    res = fpgaPropertiesGetGUID(props, &guid);
    EXPECT_EQ(FPGA_OK, res);
    uuid_unparse(guid, buf);
    ASSERT_STREQ(TEST_GUID_STR, buf);

    res = fpgaPropertiesGetObjectID(props, &u64);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(object_id_, u64);

    res = fpgaPropertiesGetNumErrors(props, &u32);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(num_errors_, u32);

    res = fpgaPropertiesGetInterface(props, &interface);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(interface, ifc_);

    res = fpgaPropertiesGetSubsystemVendorID(props, &u16);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(subsystem_vendor_id_, u16);

    res = fpgaPropertiesGetSubsystemDeviceID(props, &u16);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(subsystem_device_id_, u16);

    res = fpgaPropertiesGetHostname(props, buf, sizeof(buf) - 1);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_STREQ(hostname_, buf);

    if (FPGA_DEVICE == objtype) {
      res = fpgaPropertiesGetNumSlots(props, &u32);
      EXPECT_EQ(FPGA_OK, res);
      EXPECT_EQ(num_slots_, u32);

      res = fpgaPropertiesGetBBSID(props, &u64);
      EXPECT_EQ(FPGA_OK, res);
      EXPECT_EQ(bbs_id_, u64);

      res = fpgaPropertiesGetBBSVersion(props, &ver);
      EXPECT_EQ(FPGA_OK, res);
      EXPECT_EQ(bbs_version_.major, ver.major);
      EXPECT_EQ(bbs_version_.minor, ver.minor);
      EXPECT_EQ(bbs_version_.patch, ver.patch);
    } else {
      res = fpgaPropertiesGetAcceleratorState(props, &state);
      EXPECT_EQ(FPGA_OK, res);
      EXPECT_EQ(accelerator_state_, state);

      res = fpgaPropertiesGetNumMMIO(props, &mmios);
      EXPECT_EQ(FPGA_OK, res);
      EXPECT_EQ(num_mmio_, mmios);

      res = fpgaPropertiesGetNumInterrupts(props, &irqs);
      EXPECT_EQ(FPGA_OK, res);
      EXPECT_EQ(num_interrupts_, irqs);
    }
  }

  fpga_properties short_props()
  {
    fpga_properties props = NULL;
    fpgaGetProperties(NULL, &props);

    fpgaPropertiesSetObjectType(props, FPGA_DEVICE);

    fpgaPropertiesSetSegment(props, segment_);
    fpgaPropertiesSetBus(props, bus_);
    fpgaPropertiesSetDevice(props, device_);
    fpgaPropertiesSetFunction(props, function_);
    fpgaPropertiesSetSocketID(props, socket_id_);
    fpgaPropertiesSetVendorID(props, vendor_id_);
    fpgaPropertiesSetDeviceID(props, device_id_);

    fpgaPropertiesSetGUID(props, guid_);

    fpgaPropertiesSetHostname(props, hostname_, strlen(hostname_));

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

    ASSERT_NE((fpga_properties)NULL, props);

    res = fpgaPropertiesGetObjectType(props, &objtype);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(FPGA_DEVICE, objtype);

    res = fpgaPropertiesGetSegment(props, &u16);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(segment_, u16);

    res = fpgaPropertiesGetBus(props, &u8);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(bus_, u8);

    res = fpgaPropertiesGetDevice(props, &u8);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(device_, u8);

    res = fpgaPropertiesGetFunction(props, &u8);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(function_, u8);

    res = fpgaPropertiesGetSocketID(props, &u8);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(socket_id_, u8);

    res = fpgaPropertiesGetVendorID(props, &u16);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(vendor_id_, u16);

    res = fpgaPropertiesGetDeviceID(props, &u16);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_EQ(device_id_, u16);

    res = fpgaPropertiesGetGUID(props, &guid);
    EXPECT_EQ(FPGA_OK, res);
    uuid_unparse(guid, buf);
    EXPECT_STREQ(TEST_GUID_STR, buf);

    res = fpgaPropertiesGetHostname(props, buf, sizeof(buf) - 1);
    EXPECT_EQ(FPGA_OK, res);
    EXPECT_STREQ(hostname_, buf);
  }

  fpga_guid guid_;

  const uint16_t segment_ = 0x0b5a;
  const uint8_t bus_ = 0x5e;
  const uint8_t device_ = 0xe5;
  const uint8_t function_ = 1;
  const uint8_t socket_id_ = 3;
  const uint16_t vendor_id_ = 0x8086;
  const uint16_t device_id_ = 0xbcce;
  const uint64_t object_id_ = 0xdeadbeefdecafbad;
  const uint32_t num_errors_ = 50;
  const fpga_interface ifc_ = FPGA_IFC_DFL;
  const uint16_t subsystem_vendor_id_ = 0x8087;
  const uint16_t subsystem_device_id_ = 0x1770;
  const uint32_t num_slots_ = 2;
  const uint64_t bbs_id_ = 0xabadbeefdeadc01a;
  const fpga_version bbs_version_ = { 1, 2, 3 };
  const fpga_accelerator_state accelerator_state_ = FPGA_ACCELERATOR_ASSIGNED;
  const uint32_t num_mmio_ = 2;
  const uint32_t num_interrupts_ = 4;
  const char *hostname_ = "machine.company.net";
};

TEST_F(serialize_props_f, short_props_round_trip)
{
  fpga_properties props = short_props();
  json_object *jroot = start_encode();

  EXPECT_EQ(true, opae_ser_properties_to_json_obj(props, jroot));
  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&props));

  char *json = end_encode();

  jroot = start_decode(json);

  EXPECT_EQ(nullptr, props);
  EXPECT_EQ(true, opae_ser_json_to_properties_obj(jroot, &props));

  end_decode();
  opae_free(json);

  verify_short_props(props);
  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&props));
}

TEST_F(serialize_props_f, device_props_round_trip)
{
  fpga_properties props = device_props();
  json_object *jroot = start_encode();

  EXPECT_EQ(true, opae_ser_properties_to_json_obj(props, jroot));
  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&props));

  char *json = end_encode();

  jroot = start_decode(json);

  EXPECT_EQ(nullptr, props);
  EXPECT_EQ(true, opae_ser_json_to_properties_obj(jroot, &props));

  end_decode();
  opae_free(json);

  verify_props(props, FPGA_DEVICE);
  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&props));
}

TEST_F(serialize_props_f, accelerator_props_round_trip)
{
  fpga_properties props = accelerator_props();
  json_object *jroot = start_encode();

  EXPECT_EQ(true, opae_ser_properties_to_json_obj(props, jroot));
  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&props));

  char *json = end_encode();

  jroot = start_decode(json);

  EXPECT_EQ(nullptr, props);
  EXPECT_EQ(true, opae_ser_json_to_properties_obj(jroot, &props));

  end_decode();
  opae_free(json);

  verify_props(props, FPGA_ACCELERATOR);
  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&props));
}

class serialize_remote_id_f : public serialize_f
{
 protected:
  serialize_remote_id_f() {}

  virtual void SetUp() override
  {
    serialize_f::SetUp();

    strcpy(id_.hostname, "machine.co.net");
    id_.unique_id = 3;
  }

  fpga_remote_id id_;
};

TEST_F(serialize_remote_id_f, id)
{
  json_object *jroot = start_encode();

  EXPECT_EQ(true, opae_ser_remote_id_to_json_obj(&id_, jroot));

  char *json = end_encode();

  jroot = start_decode(json);

  fpga_remote_id after = {
    .hostname = { 0, },
    .unique_id = 99
  };

  EXPECT_EQ(true, opae_ser_json_to_remote_id_obj(jroot, &after));
  
  end_decode();
  opae_free(json);

  EXPECT_STREQ(id_.hostname, after.hostname);
  EXPECT_EQ(id_.unique_id, after.unique_id);
}

class serialize_token_header_f : public serialize_f
{
 protected:
  serialize_token_header_f() {}

  const fpga_token_header hdr_ = {
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
    .token_id = {
      .hostname = { 'm', 'a', 'c', 'h', 'i', 'n', 'e',
                    '.', 'c', 'o', 'm', 'p', 'a', 'n', 'y',
                    '.', 'n', 'e', 't', 0 },
      .unique_id = 3
    }
  };
};

TEST_F(serialize_token_header_f, header)
{
  json_object *jroot = start_encode();

  EXPECT_EQ(true, opae_ser_token_header_to_json_obj(&hdr_, jroot));

  char *json = end_encode();

  jroot = start_decode(json);

  fpga_token_header after;
  memset(&after, 0, sizeof(after));

  EXPECT_EQ(true, opae_ser_json_to_token_header_obj(jroot, &after));

  end_decode();
  opae_free(json);

  EXPECT_EQ(hdr_.magic, after.magic);
  EXPECT_EQ(hdr_.vendor_id, after.vendor_id);
  EXPECT_EQ(hdr_.device_id, after.device_id);
  EXPECT_EQ(hdr_.segment, after.segment);
  EXPECT_EQ(hdr_.bus, after.bus);
  EXPECT_EQ(hdr_.device, after.device);
  EXPECT_EQ(hdr_.function, after.function);
  EXPECT_EQ(hdr_.interface, after.interface);
  EXPECT_EQ(hdr_.objtype, after.objtype);
  EXPECT_EQ(hdr_.object_id, after.object_id);
  EXPECT_EQ(0, memcmp(hdr_.guid, after.guid, sizeof(fpga_guid)));
  EXPECT_EQ(hdr_.subsystem_vendor_id, after.subsystem_vendor_id);
  EXPECT_EQ(hdr_.subsystem_device_id, after.subsystem_device_id);
  EXPECT_STREQ(hdr_.token_id.hostname, after.token_id.hostname);
  EXPECT_EQ(hdr_.token_id.unique_id, after.token_id.unique_id);
}

class serialize_result_p : public serialize_base, public ::testing::TestWithParam<fpga_result>
{
 protected:
  virtual void SetUp() override
  {
    jroot_ = nullptr;
  }

  virtual void TearDown() override
  {
    if (jroot_) {
      json_object_put(jroot_);
      jroot_ = nullptr;
    }
  }
};

TEST_P(serialize_result_p, thetest)
{
  json_object *jroot = start_encode();

  EXPECT_EQ(true, opae_ser_fpga_result_to_json_obj(GetParam(), jroot));

  char *json = end_encode();

  jroot = start_decode(json);

  fpga_result after = FPGA_OK;

  EXPECT_EQ(true, opae_ser_json_to_fpga_result_obj(jroot, &after));

  end_decode();
  opae_free(json);

  EXPECT_EQ(GetParam(), after);
}

INSTANTIATE_TEST_SUITE_P(result, serialize_result_p,
                         ::testing::Values(
  FPGA_OK,
  FPGA_INVALID_PARAM,
  FPGA_BUSY,
  FPGA_EXCEPTION,
  FPGA_NOT_FOUND,
  FPGA_NO_MEMORY,
  FPGA_NOT_SUPPORTED,
  FPGA_NO_DRIVER,
  FPGA_NO_DAEMON,
  FPGA_NO_ACCESS,
  FPGA_RECONF_ERROR
));

class serialize_handle_header_f : public serialize_f
{
 protected:
  serialize_handle_header_f() {}

  const fpga_handle_header hdr_ = {
    .magic = 0x46504741544f4b4e,
    .token_id = {
      .hostname = { 'y', 'o', 'u', 'r', '.',
		    'h', 'o', 's', 't', '.',
		    'c', 'o', 'm', 0 },
      .unique_id = 2
    },
    .handle_id = {
      .hostname = { 'm', 'y', '.',
		    'h', 'o', 's', 't', '.',
		    'c', 'o', 'm', 0 },
      .unique_id = 3
    }
  };
};

TEST_F(serialize_handle_header_f, header)
{
  json_object *jroot = start_encode();

  EXPECT_EQ(true, opae_ser_handle_header_to_json_obj(&hdr_, jroot));

  char *json = end_encode();

  jroot = start_decode(json);

  fpga_handle_header after;
  memset(&after, 0, sizeof(after));

  EXPECT_EQ(true, opae_ser_json_to_handle_header_obj(jroot, &after));

  end_decode();
  opae_free(json);

  EXPECT_EQ(hdr_.magic, after.magic);
  EXPECT_STREQ(hdr_.token_id.hostname, after.token_id.hostname);
  EXPECT_EQ(hdr_.token_id.unique_id, after.token_id.unique_id);
  EXPECT_STREQ(hdr_.handle_id.hostname, after.handle_id.hostname);
  EXPECT_EQ(hdr_.handle_id.unique_id, after.handle_id.unique_id);
}

class serialize_error_info_f : public serialize_f
{
 protected:
  serialize_error_info_f() {}

  const struct fpga_error_info err_ = {
    .name = { 'm', 'y', 'e', 'r', 'r', 0 },
    .can_clear = true
  };
};

TEST_F(serialize_error_info_f, info)
{
  json_object *jroot = start_encode();

  EXPECT_EQ(true, opae_ser_error_info_to_json_obj(&err_, jroot));

  char *json = end_encode();

  jroot = start_decode(json);

  struct fpga_error_info after;
  memset(&after, 0, sizeof(after));

  EXPECT_EQ(true, opae_ser_json_to_error_info_obj(jroot, &after));

  end_decode();
  opae_free(json);

  EXPECT_STREQ(err_.name, after.name);
  EXPECT_EQ(err_.can_clear, after.can_clear);
}
