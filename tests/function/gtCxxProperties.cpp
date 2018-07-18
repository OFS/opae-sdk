#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#include <opae/cxx/core/except.h>
#include <opae/cxx/core/token.h>
#include <uuid/uuid.h>
#include "gtest/gtest.h"

using namespace opae::fpga::types;
const char* TEST_GUID_STR = "ae2878a7-926f-4332-aba1-2b952ad6df8e";

/**
 * @test set_guid
 * Given a new properties object and a valid fpga_guid object
 * When I set the guid property to the fpga_guid object
 * And I retrieve the same property using fpgaGetPropertiesGUID
 * Then the known guid matches the one retrieved
 */
TEST(LibopaecppPropsCommonALL, set_guid) {
  fpga_guid guid_in, guid_out;
  auto p = properties::get();
  // set the guid to an fpga_guid
  uuid_parse(TEST_GUID_STR, guid_in);
  p->guid = guid_in;

  // now check we set the guid using C APIs
  ASSERT_EQ(fpgaPropertiesGetGUID(p->c_type(), &guid_out), FPGA_OK);
  EXPECT_EQ(memcmp(guid_in, guid_out, sizeof(fpga_guid)), 0);
}

/**
 * @test parse_guid
 * Given a new properties object
 * When I set the guid property using its `parse` method
 * And I retrieve the same property using fpgaGetPropertiesGUID
 * Then the known guid string parsed matches the one retrieved
 */
TEST(LibopaecppPropsCommonALL, parse_guid) {
  fpga_guid guid_out;
  auto p = properties::get();
  // set the guid to an fpga_guid
  p->guid.parse(TEST_GUID_STR);

  // now check we set the guid using C APIs
  ASSERT_EQ(fpgaPropertiesGetGUID(p->c_type(), &guid_out), FPGA_OK);
  char guid_str[84];
  uuid_unparse(guid_out, guid_str);
  EXPECT_STREQ(TEST_GUID_STR, guid_str);
}

/**
 * @test get_guid
 * Given a new properties object and a valid fpga_guid object
 * When I set the guid property using fpgaPropertiesSetGUID
 * And I get a pointer to the guid member variable of the property object
 * Then the known guid matches the one retrieved
 */
TEST(LibopaecppPropsCommonALL, get_guid) {
  fpga_guid guid_in;
  auto p = properties::get();
  // set the guid using fpgaPropertiesSetGUID
  uuid_parse(TEST_GUID_STR, guid_in);
  fpgaPropertiesSetGUID(p->c_type(), guid_in);

  uint8_t* guid_ptr = p->guid;
  ASSERT_NE(nullptr, guid_ptr);
  EXPECT_EQ(memcmp(guid_in, guid_ptr, sizeof(fpga_guid)), 0);
}

/**
 * @test compare_guid
 * Given a new properties object with a known guid
 * When I set compare its guid with the known guid
 * Then the result is true
 */
TEST(LibopaecppPropsCommonALL, compare_guid) {
  fpga_guid guid_in;
  auto p = properties::get();
  uuid_parse(TEST_GUID_STR, guid_in);
  EXPECT_FALSE(p->guid == guid_in);
  p->guid = guid_in;
  ASSERT_EQ(memcmp(p->guid.c_type(), guid_in, sizeof(fpga_guid)), 0);
  EXPECT_TRUE(p->guid == guid_in);
}

/**
 * @test props_ctor_01
 * Given a new properties object with a known guid
 * passed in the constructor
 * When I set compare its guid with the known guid
 * Then the result is true
 */
TEST(LibopaecppPropsCommonALL, props_ctor_01) {
  fpga_guid guid_in;
  uuid_parse(TEST_GUID_STR, guid_in);
  auto p = properties::get(guid_in);
  ASSERT_EQ(memcmp(p->guid.c_type(), guid_in, sizeof(fpga_guid)), 0);
  EXPECT_TRUE(p->guid == guid_in);
}

/**
 * @test set_objtype
 * Given a new properties object
 * When I set the object type to a known value
 * Then the property is set
 */
TEST(LibopaecppPropsCommonALL, set_objtype) {
  auto p = properties::get();
  p->type = FPGA_ACCELERATOR;
  fpga_objtype t = p->type;
  fpga_objtype other_t =
      (t == FPGA_ACCELERATOR) ? FPGA_DEVICE : FPGA_ACCELERATOR;
  p->type = other_t;
  EXPECT_TRUE(p->type == other_t);
}

/**
 * @test get_model
 * Given a properties object
 * When I get the model property
 * Then I get an empty string
 */
TEST(LibopaecppPropsCommonALL, get_model) {
  auto p = properties::get();
  std::string model = "";
  // Model is currently not supported in libopae-c
  EXPECT_THROW(model = p->model, not_supported);
}

/**
 * @test set_num_errors
 * Given a properties properties object with the num_errors property set to a
 * known value
 * When I get the num_errors property
 * Then the number is the expected value
 */
TEST(LibopaecppPropsCommonALL, get_num_errors) {
  auto p = properties::get();
  p->num_errors = 9;
  EXPECT_EQ(static_cast<uint32_t>(p->num_errors), 9);
}

/**
 * @test segment
 * Given a properties properties object with the segment property set to a
 * known value
 * When I get the segment property
 * Then the number is the expected value
 */
TEST(LibopaecppPropsCommonALL, get_segment) {
  auto p = properties::get();
  p->segment = 9090;
  EXPECT_EQ(static_cast<uint16_t>(p->segment), 9090);
}
