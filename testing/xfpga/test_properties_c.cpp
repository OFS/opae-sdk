// Copyright(c) 2017-2018, Intel Corporation
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

#include <opae/fpga.h>
#include <algorithm>
#include "gtest/gtest.h"
#include "test_system.h"
#include "types_int.h"
#include "xfpga.h"
#include "props.h"

fpga_guid known_guid = {0xc5, 0x14, 0x92, 0x82, 0xe3, 0x4f, 0x11, 0xe6,
                        0x8e, 0x3a, 0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28};

using namespace opae::testing;

class properties_p1 : public ::testing::TestWithParam<std::string> {
 protected:
  properties_p1() : tmpsysfs_("mocksys-XXXXXX") {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    tmpsysfs_ = system_->prepare_syfs(platform_);

    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &props_), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_),
              FPGA_OK);
    EXPECT_EQ(num_matches_, platform_.devices.size());
    ASSERT_EQ(xfpga_fpgaOpen(tokens_[0], &accel_, 0), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaClearProperties(filter_), FPGA_OK);
    num_matches_ = 0xc01a;
    invalid_device_ = test_device::unknown();
  }

  virtual void TearDown() override {
    EXPECT_EQ(xfpga_fpgaDestroyProperties(&filter_), FPGA_OK);
    EXPECT_EQ(xfpga_fpgaDestroyProperties(&props_), FPGA_OK);
    EXPECT_EQ(xfpga_fpgaClose(accel_), FPGA_OK);
    if (!tmpsysfs_.empty() && tmpsysfs_.size() > 1) {
      std::string cmd = "rm -rf " + tmpsysfs_;
      std::system(cmd.c_str());
    }
    system_->finalize();
  }

  std::string tmpsysfs_;
  fpga_properties filter_;
  fpga_properties props_;
  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  fpga_handle accel_;
  uint32_t num_matches_;
  test_platform platform_;
  test_device invalid_device_;
  test_system* system_;
};

/**
 * @test    get_parent01
 * @brief   Tests: xfpga_fpgaPropertiesGetParent
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the parent field set<br>
 *          And a field in its parent object is a known value<br>
 *          When I call xfpga_fpgaPropertiesGetParent with a pointer to a token
 *          object<br>
 *          Then the return value is FPGA_OK<br>
 *          And the field in the token object is set to the known value<br>
 * */
TEST_P(properties_p1, get_parent01) {
  EXPECT_EQ(xfpga_fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size());

  // set the token to a known value
  auto _prop = (_fpga_properties*)props_;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);
  _prop->parent = tokens_[0];

  // now get the parent token from the prop structure
  EXPECT_EQ(xfpga_fpgaPropertiesGetParent(props_, &tokens_[1]), FPGA_OK);
  // GetParent clones the token so compare object_id of the two
  fpga_properties p1, p2;
  ASSERT_EQ(xfpga_fpgaGetProperties(tokens_[0], &p1), FPGA_OK);
  ASSERT_EQ(xfpga_fpgaGetProperties(tokens_[1], &p2), FPGA_OK);
  EXPECT_EQ(((_fpga_properties*)p1)->object_id,
            ((_fpga_properties*)p2)->object_id);
}

/**
 * @test    get_parent02
 * @brief   Tests: xfpga_fpgaPropertiesGetParent
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the parent field set<br>
 *          When I call xfpga_fpgaPropertiesGetParent with a pointer to a token
 *          object<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_p1, get_parent02) {
  struct _fpga_properties* _prop = (struct _fpga_properties*)props_;

  // make sure the FPGA_PROPERTY_PARENT bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_PARENT) & 1, 0);

  EXPECT_EQ(xfpga_fpgaPropertiesGetParent(_prop, &tokens_[0]), FPGA_NOT_FOUND);
}

INSTANTIATE_TEST_CASE_P(test_platforms, properties_p1,
                        ::testing::ValuesIn(test_platform::keys(true)));

/**
 * @test    set_parent01
 * @brief   Tests: xfpga_fpgaPropertiesSetParent
 * @details Given a non-null fpga_properties* object<br>
 *          And a fpga_token* object with a known value<br>
 *          When I call xfpga_fpgaPropertiesSetParent with the property and the
 *          token<br>
 *          Then the parent object in the properties object is the token<br>
 */
TEST_P(properties_p1, set_parent01) {
  EXPECT_EQ(xfpga_fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(
      xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(), &num_matches_),
      FPGA_OK);
  EXPECT_EQ(num_matches_, platform_.devices.size());

  // now get the parent token from the prop structure
  EXPECT_EQ(xfpga_fpgaPropertiesSetParent(props_, tokens_[0]), FPGA_OK);
  // now get the parent token from the prop structure
  EXPECT_EQ(xfpga_fpgaPropertiesGetParent(props_, &tokens_[1]), FPGA_OK);
  // GetParent clones the token so compare object_id of the two
  fpga_properties p1, p2;
  ASSERT_EQ(xfpga_fpgaGetProperties(tokens_[0], &p1), FPGA_OK);
  ASSERT_EQ(xfpga_fpgaGetProperties(tokens_[1], &p2), FPGA_OK);
  EXPECT_EQ(((_fpga_properties*)p1)->object_id,
            ((_fpga_properties*)p2)->object_id);
}

TEST_P(properties_p1, from_handle) {
  EXPECT_EQ(xfpga_fpgaGetPropertiesFromHandle(accel_, &props_), FPGA_OK);
}

/**
 * @test    get_parent_null_props
 * @brief   Tests: xfpga_fpgaPropertiesGetParent
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetParent with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 **/
TEST(properties, get_parent_null_props) {
  fpga_properties prop = NULL;

  fpga_token token;
  fpga_result result = xfpga_fpgaPropertiesGetParent(prop, &token);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_parent_null_token
 * @brief   Tests: xfpga_fpgaPropertiesSetParent
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetParent with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_parent_null_token) {
  fpga_properties prop = NULL;

  // Call the API to set the token on the property
  fpga_token token;
  fpga_result result = xfpga_fpgaPropertiesSetParent(prop, &token);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    create
 * @brief   Tests: fpgaCreateProperties
 * @details Given a null fpga_properties object<br>
 *          When I call fpgaCreateProperties with the object<br>
 *          Then the return value is FPGA_OK<br>
 *          And the fpga_properties object is non-null<br>
 */
TEST(properties, create) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_EQ(NULL, ((struct _fpga_properties*)prop)->parent);
  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(result, FPGA_OK);
}

/**
 * @test    destroy01
 * @brief   Tests: xfpga_fpgaDestroyProperties
 * @details Given a non-null fpga_properties object<br>
 *          When I call xfpga_fpgaDestroyProperties with that object<br>
 *          Then the result is FPGA_OK<br>
 *          And that object is null<br>
 */
TEST(properties, destroy01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(FPGA_OK, result);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    destroy02
 * @brief   Tests: xfpga_fpgaDestroyProperties
 * @details Given a null fpga_properties object<br>
 *          When I call xfpga_fpgaDestroyProperties with that object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, destroy02) {
  fpga_properties prop = NULL;

  fpga_result result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(FPGA_INVALID_PARAM, result);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    clear01
 * @brief   Tests: xfpga_fpgaClearProperties
 * @details Given a non-null fpga_properties object<br>
 *          When I call xfpga_fpgaClearProperties with the object<br>
 *          Then the result is FPGA_OK<br>
 *          And the properties object is cleared<br>
 */
TEST(properties, clear01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the bus field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_BUS);
  _prop->bus = 0xAB;

  result = xfpga_fpgaClearProperties(prop);
  EXPECT_EQ(FPGA_OK, result);
  EXPECT_EQ(_prop, prop);
  EXPECT_EQ(0, _prop->valid_fields);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(FPGA_OK, result);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    clear02
 * @brief   Tests: xfpga_fpgaClearProperties
 * @details Given a null fpga_properties object<br>
 *          When I call xfpga_fpgaClearProperties with the object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, clear02) {
  fpga_properties prop = NULL;

  fpga_result result = xfpga_fpgaClearProperties(prop);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * ObjectType *//
/**
 * @test    get_object_type01
 * @brief   Tests: xfpga_fpgaPropertiesGetObjectType
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the objtype field set<br>
 *          And its objtype field is a known value<br>
 *          When I call xfpga_fpgaPropertiesGetObjectType with a pointer to an
 *          fpga_objtype<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_object_type01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // now get the parent token from the prop structure
  fpga_objtype objtype;
  result = xfpga_fpgaPropertiesGetObjectType(prop, &objtype);
  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  EXPECT_EQ(FPGA_DEVICE, objtype);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_object_type02
 * @brief   Tests: xfpga_fpgaPropertiesGetObjectType
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the objtype field set<br>
 *          When I call xfpga_fpgaPropertiesGetObjectType with a pointer to an
 *          fpga_objtype<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_object_type02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_OBJTYPE bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_OBJTYPE) & 1, 0);

  fpga_objtype objtype;
  result = xfpga_fpgaPropertiesGetObjectType(prop, &objtype);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_object_type01
 * @brief   Tests: xfpga_fpgaPropertiesSetObjectType
 * @details Given a non-null fpga_properties* object<br>
 *          And a fpga_objtype object set to a known value<br>
 *          When I call xfpga_fpgaPropertiesSetObjectType with the properties
 object
 *          and the objtype<br>
 *          Then the objtype in the properties object is the known value<br>
 */
TEST(properties, set_object_type01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  fpga_objtype objtype = FPGA_DEVICE;
  result = xfpga_fpgaPropertiesSetObjectType(prop, objtype);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  EXPECT_EQ(FPGA_DEVICE, _prop->objtype);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_object_type03
 * @brief   Tests: xfpga_fpgaPropertiesGetObjectType
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetObjectType with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_object_type03) {
  fpga_properties prop = NULL;

  fpga_objtype objtype;
  fpga_result result = xfpga_fpgaPropertiesGetObjectType(prop, &objtype);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_object_type02
 * @brief   Tests: xfpga_fpgaPropertiesSetObjectType
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetObjectType with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_object_type02) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_objtype objtype = FPGA_DEVICE;
  fpga_result result = xfpga_fpgaPropertiesSetObjectType(prop, objtype);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * Segment field tests *//
/**
 * @test    get_segment01
 * @brief   Tests: xfpga_fpgaPropertiesGetSegment
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the bus field set<br>
 *          And it is set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetSegment with a pointer to an
 integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 */
TEST(properties, get_segment01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the segment field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_SEGMENT);
  _prop->segment = 0xc001;

  // now get the segment number using the API
  uint16_t segment = 0;
  result = xfpga_fpgaPropertiesGetSegment(prop, &segment);
  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  // (Get the subfield manually)
  EXPECT_EQ(0xc001, segment);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_segment02
 * @brief   Tests: xfpga_fpgaPropertiesGetSegment
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the bus field set<br>
 *          When I call xfpga_fpgaPropertiesGetSegment with a pointer to an
 integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_segment02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_SEGMENT bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_SEGMENT) & 1, 0);

  uint16_t segment;
  result = xfpga_fpgaPropertiesGetSegment(prop, &segment);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_segment01
 * @brief   Tests: xfpga_fpgaPropertiesSetSegment
 * @details Given a non-null fpga_properties* object<br>
 *          And segment variable set to a known value<br>
 *          When I call xfpga_fpgaPropertiesSetSegment with the properties object
 and
 *          the segment variable<br>
 *          Then the segment field in the properties object is the known
 value<br>
 */
TEST(properties, set_segment01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  uint16_t segment = 0xc001;
  // make sure the FPGA_PROPERTY_SEGMENT bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_SEGMENT) & 1, 0);
  // Call the API to set the segment on the property
  result = xfpga_fpgaPropertiesSetSegment(prop, segment);

  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_SEGMENT bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_SEGMENT) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xc001, _prop->segment);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_segment03
 * @brief   Tests: xfpga_fpgaPropertiesGetSegment
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetSegment with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_segment03) {
  fpga_properties prop = NULL;

  uint16_t segment;
  fpga_result result = xfpga_fpgaPropertiesGetSegment(prop, &segment);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_segment02
 * @brief   Tests: xfpga_fpgaPropertiesSetSegment
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetSegment with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_segment02) {
  fpga_properties prop = NULL;

  // Call the API to set the segment on the property
  fpga_result result = xfpga_fpgaPropertiesSetSegment(prop, 0xc001);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * Bus field tests *//
/**
 * @test    get_bus01
 * @brief   Tests: xfpga_fpgaPropertiesGetBus
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the bus field set<br>
 *          And it is set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetBus with a pointer to an integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_bus01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_BUS);
  _prop->bus = 0xAE;

  // now get the bus number using the API
  uint8_t bus;
  result = xfpga_fpgaPropertiesGetBus(prop, &bus);
  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  // (Get the subfield manually)
  EXPECT_EQ(0xAE, bus);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bus02
 * @brief   Tests: xfpga_fpgaPropertiesGetBus
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the bus field set<br>
 *          When I call xfpga_fpgaPropertiesGetBus with a pointer to an integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_bus02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_BUS bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BUS) & 1, 0);

  uint8_t bus;
  result = xfpga_fpgaPropertiesGetBus(prop, &bus);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_bus01
 * @brief   Tests: xfpga_fpgaPropertiesSetBus
 * @details Given a non-null fpga_properties* object<br>
 *          And bus variable set to a known value<br>
 *          When I call xfpga_fpgaPropertiesSetBus with the properties object and
 *          the bus variable<br>
 *          Then the bus field in the properties object is the known
 value<br>
 */
TEST(properties, set_bus01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  uint8_t bus = 0xAE;
  // make sure the FPGA_PROPERTY_BUS bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BUS) & 1, 0);
  // Call the API to set the bus on the property
  result = xfpga_fpgaPropertiesSetBus(prop, bus);

  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_BUS bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BUS) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xAE, _prop->bus);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bus03
 * @brief   Tests: xfpga_fpgaPropertiesGetBus
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetBus with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_bus03) {
  fpga_properties prop = NULL;

  uint8_t bus;
  fpga_result result = xfpga_fpgaPropertiesGetBus(prop, &bus);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_bus02
 * @brief   Tests: xfpga_fpgaPropertiesSetBus
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetBus with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_bus02) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_result result = xfpga_fpgaPropertiesSetBus(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * Device field tests *//
/**
 * @test    get_device01
 * @brief   Tests: xfpga_fpgaPropertiesGetDevice
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the device field set<br>
 *          And it is set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetDevice with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_device01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_DEVICE);
  _prop->device = 0xAE;

  // now get the device number using the API
  uint8_t device;
  result = xfpga_fpgaPropertiesGetDevice(prop, &device);
  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  // (Get the subfield manually)
  EXPECT_EQ(0xAE, device);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_device02
 * @brief   Tests: xfpga_fpgaPropertiesGetDevice
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the device field set<br>
 *          When I call xfpga_fpgaPropertiesGetDevice with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_device02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_DEVICE bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_DEVICE) & 1, 0);

  uint8_t device;
  result = xfpga_fpgaPropertiesGetDevice(prop, &device);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_device01
 * @brief   Tests: xfpga_fpgaPropertiesSetDevice
 * @details Given a non-null fpga_properties* object<br>
 *          And device variable set to a known value<br>
 *          When I call xfpga_fpgaPropertiesSetDevice with the properties object
 *          and the device variable<br>
 *          Then the device field in the properties object is the known
 *          value<br>
 */
TEST(properties, set_device01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  uint8_t device = 0x1f;  // max of 32 devices

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_DEVICE bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_DEVICE) & 1, 0);

  // Call the API to set the device on the property
  result = xfpga_fpgaPropertiesSetDevice(prop, device);

  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_DEVICE bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_DEVICE) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0x1f, _prop->device);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_device03
 * @brief   Tests: xfpga_fpgaPropertiesGetDevice
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetDevice with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_device03) {
  fpga_properties prop = NULL;

  uint8_t device;
  fpga_result result = xfpga_fpgaPropertiesGetDevice(prop, &device);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_device02
 * @brief   Tests: xfpga_fpgaPropertiesSetDevice
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetDevice with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_device02) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_result result = xfpga_fpgaPropertiesSetDevice(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * Function field tests *//
/**
 * @test    get_function01
 * @brief   Tests: xfpga_fpgaPropertiesGetFunction
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the function field set<br>
 *          And it is set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetFunction with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_function01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_FUNCTION);
  _prop->function = 0xAE;

  // now get the function number using the API
  uint8_t function;
  result = xfpga_fpgaPropertiesGetFunction(prop, &function);
  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  // (Get the subfield manually)
  EXPECT_EQ(0xAE, function);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_function02
 * @brief   Tests: xfpga_fpgaPropertiesGetFunction
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the function field set<br>
 *          When I call xfpga_fpgaPropertiesGetFunction with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_function02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_FUNCTION bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_FUNCTION) & 1, 0);

  uint8_t function;
  result = xfpga_fpgaPropertiesGetFunction(prop, &function);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_function01
 * @brief   Tests: xfpga_fpgaPropertiesSetFunction
 * @details Given a non-null fpga_properties* object<br>
 *          And function variable set to a known value<br>
 *          When I call xfpga_fpgaPropertiesSetFunction with the properties object
 *          and the function variable<br>
 *          Then the function field in the properties object is the known
 *          value<br>
 */
TEST(properties, set_function01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  uint8_t function = 7;  // max of 8 functions

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_FUNCTION bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_FUNCTION) & 1, 0);

  // Call the API to set the function on the property
  result = xfpga_fpgaPropertiesSetFunction(prop, function);

  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_FUNCTION bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_FUNCTION) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(7, _prop->function);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_function03
 * @brief   Tests: xfpga_fpgaPropertiesGetFunction
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetFunction with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_function03) {
  fpga_properties prop = NULL;

  uint8_t function;
  fpga_result result = xfpga_fpgaPropertiesGetFunction(prop, &function);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_function02
 * @brief   Tests: xfpga_fpgaPropertiesSetFunction
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetFunction with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_function02) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_result result = xfpga_fpgaPropertiesSetFunction(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * SocketID field tests *//
/**
 * @test    get_socket_id01
 * @brief   Tests: xfpga_fpgaPropertiesGetSocketID
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the socket_id field set<br>
 *          And it is set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetSocketID with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_socket_id01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_SOCKETID);
  _prop->socket_id = 0xAE;

  // now get the socket_id number using the API
  uint8_t socket_id;
  result = xfpga_fpgaPropertiesGetSocketID(prop, &socket_id);
  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  // (Get the subfield manually)
  EXPECT_EQ(0xAE, socket_id);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_socket_id02
 * @brief   Tests: xfpga_fpgaPropertiesGetSocketID
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the socket_id field set<br>
 *          When I call xfpga_fpgaPropertiesGetSocketID with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_socket_id02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_SOCKETID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_SOCKETID) & 1, 0);

  uint8_t socket_id;
  result = xfpga_fpgaPropertiesGetSocketID(prop, &socket_id);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_socket_id01
 * @brief   Tests: xfpga_fpgaPropertiesSetSocketID
 * @details Given a non-null fpga_properties* object<br>
 *          And socket_id variable set to a known value<br>
 *          When I call xfpga_fpgaPropertiesSetSocketID with the properties object
 *          and the socket_id variable<br>
 *          Then the socket_id field in the properties object is the known
 *          value<br>
 */
TEST(properties, set_socket_id01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  uint8_t socket_id = 0xAE;

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_SOCKETID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_SOCKETID) & 1, 0);

  // Call the API to set the socket_id on the property
  result = xfpga_fpgaPropertiesSetSocketID(prop, socket_id);

  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_SOCKETID bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_SOCKETID) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xAE, _prop->socket_id);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_socket_id03
 * @brief   Tests: xfpga_fpgaPropertiesGetSocketID
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetSocketID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_socket_id03) {
  fpga_properties prop = NULL;

  uint8_t socket_id;
  fpga_result result = xfpga_fpgaPropertiesGetSocketID(prop, &socket_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_socket_id02
 * @brief   Tests: xfpga_fpgaPropertiesSetSocketID
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetSocketID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_socket_id02) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_result result = xfpga_fpgaPropertiesSetSocketID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** fpga.num_slots field tests **/
/**
 * @test    get_num_slots01
 * @brief   Tests: xfpga_fpgaPropertiesGetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the num_slots field set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetNumSlots with a pointer to an
 integer
 *          variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_num_slots01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_SLOTS);

  // set the object type field
  _prop->objtype = FPGA_DEVICE;

  // set the num_slots to a known value
  _prop->u.fpga.num_slots = 0xCAFE;

  // now get the num_slots from the prop structure
  uint32_t num_slots;
  result = xfpga_fpgaPropertiesGetNumSlots(prop, &num_slots);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0xCAFE, num_slots);

  // now delete the properties object
  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_slots02
 * @brief   Tests: xfpga_fpgaPropertiesGetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call xfpga_fpgaPropertiesGetNumSlots with a pointer to an
 integer
 *          variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_num_slots02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field to a different type
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // now get the num_slots from the prop structure
  uint32_t num_slots;
  result = xfpga_fpgaPropertiesGetNumSlots(prop, &num_slots);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_slots03
 * @brief   Tests: xfpga_fpgaPropertiesGetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_DEVICE
 *          And it does NOT have the num_slots field set<br>
 *          When I call xfpga_fpgaPropertiesGetNumSlots with the property object
 *          and a pointer to bool variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_num_slots03) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);

  // make sure the FPGA_PROPERTY_NUM_SLOTS bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_SLOTS) & 1, 0);

  uint32_t num_slots;
  result = xfpga_fpgaPropertiesGetNumSlots(prop, &num_slots);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_num_slots01
 * @brief   Tests: xfpga_fpgaPropertiesSetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an integer variable set to a known value<br>
 *          When I call xfpga_fpgaPropertiesSetNumSlots with the properties object
 *          and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the num_slots in the properties object is the known value<br>
 */
TEST(properties, set_num_slots01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  // set the object type field
  _prop->objtype = FPGA_DEVICE;

  // make sure the FPGA_PROPERTY_NUM_SLOTS bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_SLOTS) & 1, 0);

  uint32_t num_slots = 0xCAFE;
  // Call the API to set the token on the property
  result = xfpga_fpgaPropertiesSetNumSlots(prop, num_slots);

  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_NUM_SLOTS bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_SLOTS) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xCAFE, _prop->u.fpga.num_slots);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_num_slots02
 * @brief   Tests: xfpga_fpgaPropertiesSetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_DEVICE<br>
 *          When I call xfpga_fpgaPropertiesSetNumSlots with the properties object
 *          and a num_slots variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST(properties, set_num_slots02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;

  // Call the API to set the token on the property
  result = xfpga_fpgaPropertiesSetNumSlots(prop, 0);

  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_slots04
 * @brief   Tests: xfpga_fpgaPropertiesGetNumSlots
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetNumSlots with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_num_slots04) {
  fpga_properties prop = NULL;

  uint32_t num_slots;
  fpga_result result = xfpga_fpgaPropertiesGetNumSlots(prop, &num_slots);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_num_slots03
 * @brief   Tests: xfpga_fpgaPropertiesSetNumSlots
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetNumSlots with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_num_slots03) {
  fpga_properties prop = NULL;

  // Call the API to set the num_slots on the property
  fpga_result result = xfpga_fpgaPropertiesSetNumSlots(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** fpga.bbs_id field tests **/
/**
 * @test    get_bbs_id01
 * @brief   Tests: xfpga_fpgaPropertiesGetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the bbs_id field set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetBBSID with a pointer to an integer
 *          variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_bbs_id01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_BBSID);

  // set the object type field
  _prop->objtype = FPGA_DEVICE;

  // set the bbs_id to a known value
  _prop->u.fpga.bbs_id = 0xCAFE;

  // now get the bbs_id from the prop structure
  uint64_t bbs_id;
  result = xfpga_fpgaPropertiesGetBBSID(prop, &bbs_id);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0xCAFE, bbs_id);

  // now delete the properties object
  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bbs_id02
 * @brief   Tests: xfpga_fpgaPropertiesGetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call xfpga_fpgaPropertiesGetBBSID with a pointer to an integer
 *          variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_bbs_id02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field to a different type
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // now get the bbs_id from the prop structure
  uint64_t bbs_id;
  result = xfpga_fpgaPropertiesGetBBSID(prop, &bbs_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bbs_id03
 * @brief   Tests: xfpga_fpgaPropertiesGetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_DEVICE
 *          And it does NOT have the bbs_id field set<br>
 *          When I call xfpga_fpgaPropertiesGetBBSID with the property object
 *          and a pointer to bool variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_bbs_id03) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);

  // make sure the FPGA_PROPERTY_BBSID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BBSID) & 1, 0);

  uint64_t bbs_id;
  result = xfpga_fpgaPropertiesGetBBSID(prop, &bbs_id);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_bbs_id01
 * @brief   Tests: xfpga_fpgaPropertiesSetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an integer variable set to a known value<br>
 *          When I call xfpga_fpgaPropertiesSetBBSID with the properties object
 *          and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the bbs_id in the properties object is the known value<br>
 */
TEST(properties, set_bbs_id01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  // set the object type field
  _prop->objtype = FPGA_DEVICE;

  // make sure the FPGA_PROPERTY_BBSID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BBSID) & 1, 0);

  uint64_t bbs_id = 0xCAFE;
  // Call the API to set the token on the property
  result = xfpga_fpgaPropertiesSetBBSID(prop, bbs_id);
  EXPECT_EQ(result, FPGA_OK);

#ifndef BUILD_ASE
  // make sure the FPGA_PROPERTY_BBSID bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BBSID) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xCAFE, _prop->u.fpga.bbs_id);
#endif

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_bbs_id02
 * @brief   Tests: xfpga_fpgaPropertiesSetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_DEVICE<br>
 *          When I call xfpga_fpgaPropertiesSetBBSID with the properties object
 *          and a bbs_id variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_bbs_id02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // Call the API to set the token on the property
  result = xfpga_fpgaPropertiesSetBBSID(prop, 0);

  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bbs_id04
 * @brief   Tests: xfpga_fpgaPropertiesGetBBSID
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetBBSID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_bbs_id04) {
  fpga_properties prop = NULL;

  uint64_t bbs_id;
  fpga_result result = xfpga_fpgaPropertiesGetBBSID(prop, &bbs_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_bbs_id03
 * @brief   Tests: xfpga_fpgaPropertiesSetBBSID
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetBBSID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_bbs_id03) {
  fpga_properties prop = NULL;

  // Call the API to set the bbs_id on the property
  fpga_result result = xfpga_fpgaPropertiesSetBBSID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** fpga.bbs_version field tests **/
/**
 * @test    get_bbs_version01
 * @brief   Tests: xfpga_fpgaPropertiesGetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the bbs_version field set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetBBSVersion with a pointer to an
 *          fpga_version variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_bbs_version01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_BBSVERSION);

  // set the object type field
  _prop->objtype = FPGA_DEVICE;

  // set the bbs_version to a known value
  fpga_version v = {1, 2, 3};
  _prop->u.fpga.bbs_version = v;

  // now get the bbs_version from the prop structure
  fpga_version bbs_version;
  result = xfpga_fpgaPropertiesGetBBSVersion(prop, &bbs_version);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

// assert it is set to what we set it to above
#ifndef BUILD_ASE
  EXPECT_EQ(0, memcmp(&v, &bbs_version, sizeof(fpga_version)));
#endif

  // now delete the properties object
  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bbs_version02
 * @brief   Tests: xfpga_fpgaPropertiesGetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call xfpga_fpgaPropertiesGetBBSVersion with a pointer to an
 *          fpga_version variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_bbs_version02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field to a different type
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // now get the bbs_version from the prop structure
  fpga_version bbs_version;
  result = xfpga_fpgaPropertiesGetBBSVersion(prop, &bbs_version);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bbs_version03
 * @brief   Tests: xfpga_fpgaPropertiesGetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_DEVICE
 *          And it does NOT have the bbs_version field set<br>
 *          When I call xfpga_fpgaPropertiesGetBBSVersion with the property object
 *          and a pointer to an fpga_version variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_bbs_version03) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);

  // make sure the FPGA_PROPERTY_BBSVERSION bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BBSVERSION) & 1, 0);

  fpga_version bbs_version;
  result = xfpga_fpgaPropertiesGetBBSVersion(prop, &bbs_version);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_bbs_version01
 * @brief   Tests: xfpga_fpgaPropertiesSetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an fpga_version variable set to a known value<br>
 *          When I call xfpga_fpgaPropertiesSetBBSVersion with the properties
 object
 *          and the fpga_version variable<br>
 *          Then the return value is FPGA_OK
 *          And the bbs_version in the properties object is the known
 value<br>
 */
TEST(properties, set_bbs_version01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  // set the object type field
  _prop->objtype = FPGA_DEVICE;

  fpga_version bbs_version = {1, 2, 3};

  // make sure the FPGA_PROPERTY_BBSVERSION bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BBSVERSION) & 1, 0);

  // Call the API to set the bbs version on the property
  result = xfpga_fpgaPropertiesSetBBSVersion(prop, bbs_version);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_BBSVERSION bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BBSVERSION) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0, memcmp(&bbs_version, &(_prop->u.fpga.bbs_version),
                      sizeof(fpga_version)));

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_bbs_version02
 * @brief   Tests: xfpga_fpgaPropertiesSetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_DEVICE<br>
 *          When I call xfpga_fpgaPropertiesSetBBSVersion with the properties
 object
 *          and a bbs_version variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST(properties, set_bbs_version02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // Call the API to set the token on the property
  fpga_version v = {0, 0, 0};
  result = xfpga_fpgaPropertiesSetBBSVersion(prop, v);

  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bbs_version04
 * @brief   Tests: xfpga_fpgaPropertiesGetBBSVersion
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetBBSVersion with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_bbs_version04) {
  fpga_properties prop = NULL;

  fpga_version bbs_version;
  fpga_result result = xfpga_fpgaPropertiesGetBBSVersion(prop, &bbs_version);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_bbs_version03
 * @brief   Tests: xfpga_fpgaPropertiesSetBBSVersion
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetBBSVersion with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_bbs_version03) {
  fpga_properties prop = NULL;

  // Call the API to set the bbs_version on the property
  fpga_version v = {0, 0, 0};
  fpga_result result = xfpga_fpgaPropertiesSetBBSVersion(prop, v);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    fpga_clone_poperties01
 * @brief   Tests: fpgaClonePoperties
 * @details Given a fpga_properties object cloned with
 xfpga_fpgaCloneProperties<br>
 *          When I call xfpga_fpgaDestroyProperties with the cloned object<br>
 *          Then the result is FPGA_OK<br>
 *          And the properties object is destroyed appropriately<br>
 */
TEST(properties, fpga_clone_poperties01) {
  fpga_properties prop = NULL;
  fpga_properties clone = NULL;
  uint8_t s1 = 0xEF, s2 = 0;
  ASSERT_EQ(FPGA_OK, xfpga_fpgaGetProperties(NULL, &prop));
  EXPECT_EQ(FPGA_OK, xfpga_fpgaPropertiesSetSocketID(prop, s1));
  ASSERT_EQ(FPGA_OK, xfpga_fpgaCloneProperties(prop, &clone));
  EXPECT_EQ(FPGA_OK, xfpga_fpgaPropertiesGetSocketID(clone, &s2));
  EXPECT_EQ(s1, s2);
  ASSERT_EQ(FPGA_OK, xfpga_fpgaDestroyProperties(&clone));
  ASSERT_EQ(FPGA_OK, xfpga_fpgaDestroyProperties(&prop));
  ASSERT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaCloneProperties(NULL, &clone));
}

/**
 * @test    set_model01
 * @brief   Tests: xfpga_fpgaPropertiesSetModel
 * @details xfpga_fpgaPropertiesSetModel is not currently supported.
 *
 */
TEST(properties, set_model01) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaPropertiesSetModel(NULL, 0));
}

/**
 * @test    get_model01
 * @brief   Tests: xfpga_fpgaPropertiesGetModel
 * @details xfpga_fpgaPropertiesGetModel is not currently supported.
 *
 */
TEST(properties, get_model01) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaPropertiesGetModel(NULL, NULL));
}

/**
 * @test    set_capabilities01
 * @brief   Tests: xfpga_fpgaPropertiesSetCapabilities
 * @details xfpga_fpgaPropertiesSetCapabilities is not currently supported.
 *
 */
TEST(properties, set_capabilities01) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaPropertiesSetCapabilities(NULL, 0));
}

/**
 * @test    get_capabilities01
 * @brief   Tests: xfpga_fpgaPropertiesGetCapabilities
 * @details xfpga_fpgaPropertiesGetCapabilities is not currently supported.
 *
 */
TEST(properties, get_capabilities01) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaPropertiesGetCapabilities(NULL, NULL));
}

/**
 * @test    fpga_get_properties01
 * @brief   Tests: xfpga_fpgaGetProperties
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaGetProperties with an invalid token,
 *          expected result is FPGA_INVALID_PARAM.<br>
 */
TEST(properties, fpga_get_properties01) {
  fpga_token token;
  ((_fpga_token*)token)->magic = 0xbeef;
  fpga_properties prop;
  EXPECT_EQ(xfpga_fpgaGetProperties(token, &prop), FPGA_INVALID_PARAM);
}

/** (afu | accelerator).guid field tests **/
/**
 * @test    get_guid01
 * @brief   Tests: xfpga_fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR
 *          And it has the guid field set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetGUID with a pointer to an
 *          guid variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_guid01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_GUID);

  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;
  std::copy(&known_guid[0], &known_guid[16], _prop->guid);

  // now get the guid from the prop structure
  fpga_guid guid;
  result = xfpga_fpgaPropertiesGetGUID(prop, &guid);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0, memcmp(guid, known_guid, 16));

  // now delete the properties object
  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_guid02
 * @brief   Tests: xfpga_fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR
 *          And it has the guid field set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetGUID with a pointer to an
 *          guid variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_guid02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_GUID);

  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;

  // set the guid to a known value
  std::copy(&known_guid[0], &known_guid[16], _prop->guid);

  // now get the guid from the prop structure
  fpga_guid guid = {0};
  result = xfpga_fpgaPropertiesGetGUID(prop, &guid);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0, memcmp(guid, known_guid, 16));

  // now delete the properties object
  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_guid03
 * @brief   Tests: xfpga_fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_DEVICE<br>
 *          And it does NOT have the guid field set<br>
 *          When I call xfpga_fpgaPropertiesGetGUID with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_guid03) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type to FPGA_DEVICE
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // make sure the FPGA_PROPERTY_GUID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);

  fpga_guid guid;
  result = xfpga_fpgaPropertiesGetGUID(prop, &guid);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_guid04
 * @brief   Tests: xfpga_fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the guid field set<br>
 *          When I call xfpga_fpgaPropertiesGetGUID with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_guid04) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type to FPGA_ACCELERATOR
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_GUID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);

  fpga_guid guid;
  result = xfpga_fpgaPropertiesGetGUID(prop, &guid);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_guid05
 * @brief   Tests: xfpga_fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the guid field set<br>
 *          When I call xfpga_fpgaPropertiesGetGUID with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_guid05) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type to FPGA_ACCELERATOR
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_GUID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);

  fpga_guid guid;
  result = xfpga_fpgaPropertiesGetGUID(prop, &guid);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_guid01
 * @brief   Tests: xfpga_fpgaPropertiesSetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_ACCELERATOR<br>
 *          And an integer variable set to a known value<br>
 *          When I call xfpga_fpgaPropertiesSetGUID with the properties
 *          object and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the guid in the properties object is the known
 *          value<br>
 */
TEST(properties, set_guid01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_GUID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);
  fpga_guid guid;
  std::copy(&known_guid[0], &known_guid[16], guid);
  // Call the API to set the token on the property
  result = xfpga_fpgaPropertiesSetGUID(prop, guid);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_GUID bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0, memcmp(guid, _prop->guid, 16));

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_guid02
 * @brief   Tests: xfpga_fpgaPropertiesSetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_ACCELERATOR<br>
 *          And an integer variable set to a known value<br>
 *          When I call xfpga_fpgaPropertiesSetGUID with the properties
 *          object and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the guid in the properties object is the known
 *          value<br>
 */
TEST(properties, set_guid02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_GUID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);

  fpga_guid guid;
  std::copy(&known_guid[0], &known_guid[16], guid);
  // Call the API to set the token on the property
  result = xfpga_fpgaPropertiesSetGUID(prop, guid);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_GUID bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0, memcmp(guid, _prop->guid, 16));

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_guid03
 * @brief   Tests: xfpga_fpgaPropertiesSetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_DEVICE<br>
 *          And an integer variable set to a known value<br>
 *          When I call xfpga_fpgaPropertiesSetGUID with the properties
 *          object and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the guid in the properties object is the known
 *          value<br>
 */
TEST(properties, set_guid03) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  // set the object type field
  _prop->objtype = FPGA_DEVICE;

  // make sure the FPGA_PROPERTY_GUID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);
  fpga_guid guid;
  std::copy(&known_guid[0], &known_guid[16], guid);
  // Call the API to set the token on the property
  result = xfpga_fpgaPropertiesSetGUID(prop, guid);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_GUID bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0, memcmp(guid, _prop->guid, 16));

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_guid06
 * @brief   Tests: xfpga_fpgaPropertiesGetGUID
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetGUID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_guid06) {
  fpga_properties prop = NULL;

  fpga_guid guid;
  fpga_result result = xfpga_fpgaPropertiesGetGUID(prop, &guid);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    get_guid07
 * @brief   Tests: xfpga_fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetGUID with a null guid parameter<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_guid07) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  result = xfpga_fpgaPropertiesGetGUID(prop, NULL);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(FPGA_OK, result);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_guid04
 * @brief   Tests: xfpga_fpgaPropertiesSetGUID
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetGUID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_guid04) {
  fpga_properties prop = NULL;
  fpga_guid guid;
  // Call the API to set the guid on the property
  fpga_result result = xfpga_fpgaPropertiesSetGUID(prop, guid);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** accelerator.mmio_spaces field tests **/
/**
 * @test    get_num_mmio01
 * @brief   Tests: xfpga_fpgaPropertiesGetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR
 *          And it has the mmio_spaces field set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetNumMMIO with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_num_mmio01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_MMIO);

  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;
  // set the slot mmio_spaces to a known value
  _prop->u.accelerator.num_mmio = 0xAE;

  // now get the mmio_spaces from the prop structure
  uint32_t mmio_spaces;
  result = xfpga_fpgaPropertiesGetNumMMIO(prop, &mmio_spaces);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0xAE, mmio_spaces);

  // now delete the properties object
  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_mmio02
 * @brief   Tests: xfpga_fpgaPropertiesGetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call xfpga_fpgaPropertiesGetNumMMIO with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_num_mmio02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field to a different type
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // now get the mmio_spaces from the prop structure
  uint32_t mmio_spaces;
  result = xfpga_fpgaPropertiesGetNumMMIO(prop, &mmio_spaces);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_mmio03
 * @brief   Tests: xfpga_fpgaPropertiesGetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the mmio_spaces field set<br>
 *          When I call xfpga_fpgaPropertiesGetNumMMIO with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_num_mmio03) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type to FPGA_AFU
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_NUM_MMIO bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_MMIO) & 1, 0);

  uint32_t mmio_spaces;
  result = xfpga_fpgaPropertiesGetNumMMIO(prop, &mmio_spaces);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_num_mmio01
 * @brief   Tests: xfpga_fpgaPropertiesSetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_ACCELERATOR<br>
 *          When I call xfpga_fpgaPropertiesSetNumMMIO with the properties
 *          object and a known value for mmio_spaces parameter<br>
 *          Then the return value is FPGA_OK<br>
 *          And the mmio_spaces in the properties object is the known
 *          value<br>
 */
TEST(properties, set_num_mmio01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_NUM_MMIO bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_MMIO) & 1, 0);

  // Call the API to set the number of afus
  result = xfpga_fpgaPropertiesSetNumMMIO(prop, 0xAE);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_NUM_MMIO bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_MMIO) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xAE, _prop->u.accelerator.num_mmio);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_num_mmio02
 * @brief   Tests: xfpga_fpgaPropertiesSetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call xfpga_fpgaPropertiesSetNumMMIO with the properties
 *          object<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST(properties, set_num_mmio02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // Call the API to set the slot mmio_spaces
  result = xfpga_fpgaPropertiesSetNumMMIO(prop, 0);

  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_mmio04
 * @brief   Tests: xfpga_fpgaPropertiesGetNumMMIO
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetNumMMIO with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_num_mmio04) {
  fpga_properties prop = NULL;

  uint32_t mmio_spaces;
  fpga_result result = xfpga_fpgaPropertiesGetNumMMIO(prop, &mmio_spaces);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_num_mmio03
 * @brief   Tests: xfpga_fpgaPropertiesSetNumMMIO
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetNumMMIO with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_num_mmio03) {
  fpga_properties prop = NULL;
  // Call the API to set the mmio_spaces on the property
  fpga_result result = xfpga_fpgaPropertiesSetNumMMIO(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** accelerator.state field tests **/
/**
 * @test    get_accelerator_state01
 * @brief   Tests: xfpga_fpgaPropertiesGetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR
 *          And it has the state field set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetAcceleratorState with a pointer to
 an
 *          fpga_accelerator_state variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_accelerator_state01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of accelerators fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_ACCELERATOR_STATE);

  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;
  // set the accelerator state to a known value
  _prop->u.accelerator.state = FPGA_ACCELERATOR_UNASSIGNED;

  // now get the state from the prop structure
  fpga_accelerator_state state;
  result = xfpga_fpgaPropertiesGetAcceleratorState(prop, &state);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(FPGA_ACCELERATOR_UNASSIGNED, state);

  // now delete the properties object
  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_accelerator_state02
 * @brief   Tests: xfpga_fpgaPropertiesGetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call xfpga_fpgaPropertiesGetAcceleratorState with a pointer to
 an
 *          fpga_accelerator_state variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_accelerator_state02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field to a different type
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // now get the state from the prop structure
  fpga_accelerator_state state;
  result = xfpga_fpgaPropertiesGetAcceleratorState(prop, &state);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_accelerator_state03
 * @brief   Tests: xfpga_fpgaPropertiesGetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the state field set<br>
 *          When I call xfpga_fpgaPropertiesGetAcceleratorState with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_accelerator_state03) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type to FPGA_DEVICE
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_ACCELERATOR_STATE bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_ACCELERATOR_STATE) & 1, 0);

  fpga_accelerator_state state;
  result = xfpga_fpgaPropertiesGetAcceleratorState(prop, &state);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_accelerator_state01
 * @brief   Tests: xfpga_fpgaPropertiesSetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_ACCELERATOR<br>
 *          When I call xfpga_fpgaPropertiesSetAcceleratorState with the properties
 *          object and a known accelerator state variable<br>
 *          Then the return value is FPGA_OK
 *          And the state in the properties object is the known
 *          value<br>
 */
TEST(properties, set_accelerator_state01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of accelerators fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_ACCELERATOR_STATE bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_ACCELERATOR_STATE) & 1, 0);

  // Call the API to set the token on the property
  result = xfpga_fpgaPropertiesSetAcceleratorState(prop, FPGA_ACCELERATOR_UNASSIGNED);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_ACCELERATOR_STATE bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_ACCELERATOR_STATE) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(FPGA_ACCELERATOR_UNASSIGNED, _prop->u.accelerator.state);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_accelerator_state02
 * @brief   Tests: xfpga_fpgaPropertiesSetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call xfpga_fpgaPropertiesSetAcceleratorState with the properties
 *          object and a state variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST(properties, set_accelerator_state02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // Call the API to set the accelerator state
  fpga_accelerator_state state = FPGA_ACCELERATOR_ASSIGNED;
  result = xfpga_fpgaPropertiesSetAcceleratorState(prop, state);

  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_accelerator_state04
 * @brief   Tests: xfpga_fpgaPropertiesGetAcceleratorState
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetAcceleratorState with the null
 * object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_accelerator_state04) {
  fpga_properties prop = NULL;

  fpga_accelerator_state state;
  fpga_result result = xfpga_fpgaPropertiesGetAcceleratorState(prop, &state);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    get_accelerator_state05
 * @brief   Tests: xfpga_fpgaPropertiesGetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetAcceleratorState with a null state
 * pointer<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_accelerator_state05) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  result = xfpga_fpgaPropertiesGetAcceleratorState(prop, NULL);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_accelerator_state03
 * @brief   Tests: xfpga_fpgaPropertiesSetAcceleratorState
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetAcceleratorState with the null
 * object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_accelerator_state03) {
  fpga_properties prop = NULL;
  // Call the API to set the state on the property
  fpga_result result =
      xfpga_fpgaPropertiesSetAcceleratorState(prop, FPGA_ACCELERATOR_UNASSIGNED);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** accelerator.num_interrupts field tests **/
/**
 * @test    get_num_interrupts01
 * @brief   Tests: xfpga_fpgaPropertiesGetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          And it has the num_interrupts field set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetNumInterrupts with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_num_interrupts01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_INTERRUPTS);

  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;
  // set the slot num_interrupts to a known value
  _prop->u.accelerator.num_interrupts = 0xAE;

  // now get the num_interrupts from the prop structure
  uint32_t num_interrupts;
  result = xfpga_fpgaPropertiesGetNumInterrupts(prop, &num_interrupts);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0xAE, num_interrupts);

  // now delete the properties object
  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_interrupts02
 * @brief   Tests: xfpga_fpgaPropertiesGetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call xfpga_fpgaPropertiesGetNumInterrupts with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_num_interrupts02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field to a different type
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // now get the num_interrupts from the prop structure
  uint32_t num_interrupts;
  result = xfpga_fpgaPropertiesGetNumInterrupts(prop, &num_interrupts);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_interrupts03
 * @brief   Tests: xfpga_fpgaPropertiesGetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the num_interrupts field set<br>
 *          When I call xfpga_fpgaPropertiesGetNumInterrupts with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(properties, get_num_interrupts03) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);
  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;
  // set the object type to FPGA_AFU
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_NUM_INTERRUPTS bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_INTERRUPTS) & 1, 0);

  uint32_t num_interrupts;
  result = xfpga_fpgaPropertiesGetNumInterrupts(prop, &num_interrupts);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_num_interrupts01
 * @brief   Tests: xfpga_fpgaPropertiesSetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_ACCELERATOR<br>
 *          When I call xfpga_fpgaPropertiesSetNumInterrupts with the properties
 *          object and a known value for num_interrupts parameter<br>
 *          Then the return value is FPGA_OK<br>
 *          And the num_interrupts in the properties object is the known
 *          value<br>
 */
TEST(properties, set_num_interrupts01) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_NUM_INTERRUPTS bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_INTERRUPTS) & 1, 0);

  // Call the API to set the number of afus
  result = xfpga_fpgaPropertiesSetNumInterrupts(prop, 0xAE);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_NUM_INTERRUPTS bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_INTERRUPTS) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xAE, _prop->u.accelerator.num_interrupts);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_num_interrupts02
 * @brief   Tests: xfpga_fpgaPropertiesSetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call xfpga_fpgaPropertiesSetNumInterrupts with the properties
 *          object<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST(properties, set_num_interrupts02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // Call the API to set the slot num_interrupts
  result = xfpga_fpgaPropertiesSetNumInterrupts(prop, 0);

  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_interrupts04
 * @brief   Tests: xfpga_fpgaPropertiesGetNumInterrupts
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetNumInterrupts with the null
 object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_num_interrupts04) {
  fpga_properties prop = NULL;

  uint32_t num_interrupts;
  fpga_result result = xfpga_fpgaPropertiesGetNumInterrupts(prop, &num_interrupts);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_num_interrupts03
 * @brief   Tests: xfpga_fpgaPropertiesSetNumInterrupts
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetNumInterrupts with the null
 object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_num_interrupts03) {
  fpga_properties prop = NULL;
  // Call the API to set the num_interrupts on the property
  fpga_result result = xfpga_fpgaPropertiesSetNumInterrupts(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    prop_213
 * @brief   Tests: xfpga_fpgaGetProperties
 * @details When creating a properties object<br>
 *          Then the internal magic should be set to FPGA_PROPERTY_MAGIC<br>
 */
TEST(properties, prop_213) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  EXPECT_EQ(FPGA_OK, result);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  EXPECT_EQ(FPGA_PROPERTY_MAGIC, _prop->magic);
}

/**
 * @test    prop_214
 * @brief   Tests: xfpga_fpgaGetProperties
 * @details When creating a properties object with a null properties
 * argument<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, prop_214) {
  fpga_result result = FPGA_OK;
  ASSERT_NO_THROW(result = xfpga_fpgaGetProperties(NULL, NULL));
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    get_vendor_id01
 * @brief   Tests: xfpga_fpgaPropertiesGetVendorID
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetVendorID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_vendor_id01) {
  fpga_properties prop = NULL;

  uint16_t vendor_id;
  fpga_result result = xfpga_fpgaPropertiesGetVendorID(prop, &vendor_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_vendor_id01
 * @brief   Tests: xfpga_fpgaPropertiesSetVendorID
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetVendorID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_vendor_id01) {
  fpga_properties prop = NULL;
  // Call the API to set the vendor_id on the property
  fpga_result result = xfpga_fpgaPropertiesSetVendorID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    get_vendor_id02
 * @brief   Tests: xfpga_fpgaPropertiesGetVendorID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          And it has the vendor_id field set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetVendorID with a pointer to an
 *          16-bit integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_vendor_id02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_VENDORID);

  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;
  // set the slot num_interrupts to a known value
  _prop->vendor_id = 0x8087;

  // now get the num_interrupts from the prop structure
  uint16_t vendor_id;
  result = xfpga_fpgaPropertiesGetVendorID(prop, &vendor_id);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0x8087, vendor_id);

  // now delete the properties object
  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_device_id01
 * @brief   Tests: xfpga_fpgaPropertiesGetDeviceID
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetDeviceID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_device_id01) {
  fpga_properties prop = NULL;

  uint16_t device_id;
  fpga_result result = xfpga_fpgaPropertiesGetDeviceID(prop, &device_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_device_id01
 * @brief   Tests: xfpga_fpgaPropertiesSetDeviceID
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetDeviceID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_device_id01) {
#ifndef BUILD_ASE
  fpga_properties prop = NULL;
  // Call the API to set the device_id on the property
  fpga_result result = xfpga_fpgaPropertiesSetDeviceID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
#endif
}

/**
 * @test    get_device_id02
 * @brief   Tests: xfpga_fpgaPropertiesGetDeviceID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          And it has the device_id field set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetDeviceID with a pointer to an
 *          16-bit integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_device_id02) {
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_DEVICEID);

  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;
  // set the slot num_interrupts to a known value
  _prop->device_id = 0xAFFE;

  // now get the num_interrupts from the prop structure
  uint16_t device_id;
  result = xfpga_fpgaPropertiesGetDeviceID(prop, &device_id);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0xAFFE, device_id);

  // now delete the properties object
  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_object_id01
 * @brief   Tests: xfpga_fpgaPropertiesGetObjectID
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesGetObjectID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_object_id01) {
  fpga_properties prop = NULL;

  uint64_t object_id;
  fpga_result result = xfpga_fpgaPropertiesGetObjectID(prop, &object_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_object_id01
 * @brief   Tests: xfpga_fpgaPropertiesSetObjectID
 * @details Given a null fpga_properties* object<br>
 *          When I call xfpga_fpgaPropertiesSetObjectID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_object_id01) {
  fpga_properties prop = NULL;
  // Call the API to set the object_id on the property
  fpga_result result = xfpga_fpgaPropertiesSetObjectID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    get_object_id02
 * @brief   Tests: xfpga_fpgaPropertiesGetObjectID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          And it has the object_id field set to a known value<br>
 *          When I call xfpga_fpgaPropertiesGetObjectID with a pointer to an
 *          64-bit integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(properties, get_object_id02) {
  uint64_t object_id = 0x8000000000000000UL;
  fpga_properties prop = NULL;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and object ID fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJECTID);

  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;
  // set the object ID to a known value
  _prop->object_id = object_id;

  // now get the object ID from the prop structure
  uint64_t tmp_object_id = 0;
  result = xfpga_fpgaPropertiesGetObjectID(prop, &tmp_object_id);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(object_id, tmp_object_id);

  // now delete the properties object
  result = xfpga_fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    fpga_destroy_properties01
 * @brief   Tests: xfpga_fpgaDestroyProperties
 * @details When the fpga_properties* object<br>
 *          to xfpga_fpgaDestroyProperties is NULL<br>
 *          Then the function returns FPGA_INVALID_PARAM<br>
 *
 */
TEST(properties, fpga_destroy_properties01) {
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaDestroyProperties(NULL));
#endif
}

TEST(properties, get_num_errors01)
{
  fpga_properties prop;
  fpga_result result = xfpga_fpgaGetProperties(NULL, &prop);
  auto _prop = (_fpga_properties*)prop;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_ERRORS);
  _prop->num_errors = 9;
  uint32_t num_errors = 0;
  // now get the parent token from the prop structure
  EXPECT_EQ(xfpga_fpgaPropertiesGetNumErrors(prop, &num_errors), FPGA_OK);
}

