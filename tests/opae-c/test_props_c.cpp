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

#include <algorithm>

#include "gtest/gtest.h"
#include "mock/test_system.h"

#include <opae/fpga.h>
#include "opae_int.h"
#include "props.h"

fpga_guid known_guid = {0xc5, 0x14, 0x92, 0x82, 0xe3, 0x4f, 0x11, 0xe6,
                        0x8e, 0x3a, 0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28};

using namespace opae::testing;

class properties_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  properties_c_p()
  : tokens_device_{{nullptr, nullptr}},
    tokens_accel_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    filter_ = nullptr;
    accel_ = nullptr;
    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_device_.data(), tokens_device_.size(),
                            &num_matches_device_), FPGA_OK);

    ASSERT_EQ(fpgaClearProperties(filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_accel_.data(), tokens_accel_.size(),
                            &num_matches_accel_), FPGA_OK);

    ASSERT_EQ(fpgaOpen(tokens_accel_[0], &accel_, 0), FPGA_OK);
    ASSERT_EQ(fpgaClearProperties(filter_), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    EXPECT_EQ(fpgaClose(accel_), FPGA_OK);
    for (auto &t : tokens_accel_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    for (auto &t : tokens_device_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    fpgaFinalize();
    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_device_;
  std::array<fpga_token, 2> tokens_accel_;
  fpga_properties filter_;
  fpga_handle accel_;
  uint32_t num_matches_device_;
  uint32_t num_matches_accel_;
  test_platform platform_;
  test_system* system_;
};

/**
 * @test    get_parent01
 * @brief   Tests: fpgaPropertiesGetParent
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the parent field set<br>
 *          And a field in its parent object is a known value<br>
 *          When I call fpgaPropertiesGetParent with a pointer to a token
 *          object<br>
 *          Then the return value is FPGA_OK<br>
 *          And the field in the token object is set to the known value<br>
 * */
TEST_P(properties_c_p, get_parent01) {
  fpga_properties prop = nullptr;
  std::array<fpga_token, 2> toks = {{nullptr, nullptr}};
  fpga_token parent = nullptr;
  uint32_t matches = 0;

  ASSERT_EQ(fpgaGetProperties(NULL, &prop), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesSetObjectType(prop, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(
      fpgaEnumerate(&prop, 1, toks.data(), toks.size(), &matches),
      FPGA_OK);

  EXPECT_EQ(fpgaClearProperties(prop), FPGA_OK);

  // set the token to a known value
  auto _prop = (_fpga_properties*)prop;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);
  _prop->parent = toks[0];

  // now get the parent token from the prop structure
  EXPECT_EQ(fpgaPropertiesGetParent(prop, &parent), FPGA_OK);
  // GetParent clones the token so compare object_id of the two
  fpga_properties p1 = nullptr, p2 = nullptr;
  ASSERT_EQ(fpgaGetProperties(toks[0], &p1), FPGA_OK);
  ASSERT_EQ(fpgaGetProperties(parent,  &p2), FPGA_OK);
  EXPECT_EQ(((_fpga_properties*)p1)->object_id,
            ((_fpga_properties*)p2)->object_id);

  EXPECT_EQ(fpgaDestroyProperties(&p1), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&p2), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);

  EXPECT_EQ(fpgaDestroyToken(&parent), FPGA_OK);
  for (auto &t : toks) {
    if (t) {
      EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
      t = nullptr;
    }
  }
}

/**
 * @test    get_parent02
 * @brief   Tests: fpgaPropertiesGetParent
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the parent field set<br>
 *          When I call fpgaPropertiesGetParent with a pointer to a token
 *          object<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_parent02) {
  fpga_properties prop = nullptr;

  ASSERT_EQ(fpgaGetProperties(NULL, &prop), FPGA_OK);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;
  fpga_token tok = nullptr;

  // make sure the FPGA_PROPERTY_PARENT bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_PARENT) & 1, 0);

  EXPECT_EQ(fpgaPropertiesGetParent(_prop, &tok), FPGA_NOT_FOUND);
  EXPECT_EQ(tok, nullptr);

  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);
}

/**
 * @test    set_parent01
 * @brief   Tests: fpgaPropertiesSetParent
 * @details Given a non-null fpga_properties* object<br>
 *          And a fpga_token* object with a known value<br>
 *          When I call fpgaPropertiesSetParent with the property and the
 *          token<br>
 *          Then the parent object in the properties object is the token<br>
 */
TEST_P(properties_c_p, set_parent01) {
  fpga_properties prop = nullptr;
  std::array<fpga_token, 2> toks = {{nullptr, nullptr}};
  uint32_t matches = 0;
  fpga_token parent = nullptr;

  ASSERT_EQ(fpgaGetProperties(NULL, &prop), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesSetObjectType(prop, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(
      fpgaEnumerate(&prop, 1, toks.data(), toks.size(), &matches),
      FPGA_OK);
  EXPECT_GT(matches, 0);

  EXPECT_EQ(fpgaClearProperties(prop), FPGA_OK);

  // now get the parent token from the prop structure
  EXPECT_EQ(fpgaPropertiesSetParent(prop, toks[0]), FPGA_OK);
  // now get the parent token from the prop structure
  EXPECT_EQ(fpgaPropertiesGetParent(prop, &parent), FPGA_OK);
  // GetParent clones the token so compare object_id of the two
  fpga_properties p1 = nullptr, p2 = nullptr;
  ASSERT_EQ(fpgaGetProperties(toks[0], &p1), FPGA_OK);
  ASSERT_EQ(fpgaGetProperties(parent, &p2), FPGA_OK);
  EXPECT_EQ(((_fpga_properties*)p1)->object_id,
            ((_fpga_properties*)p2)->object_id);

  EXPECT_EQ(fpgaDestroyProperties(&p1), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&p2), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);

  EXPECT_EQ(fpgaDestroyToken(&parent), FPGA_OK);
  for (auto &t : toks) {
    if (t) {
      EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
      t = nullptr;
    }
  }
}

/**
 * @test    set_parent02
 * @brief   Tests: fpgaPropertiesSetParent
 * @details When setting the parent token in a properties object<br>
 *          that has a wrapped parent token resulting from fpgaGetProperties[FromParent]<br>
 *          or fpgaUpdateProperties,<br>
 *          fpgaPropertiesSetParent will free the token wrapper.<br>
 */
TEST_P(properties_c_p, set_parent02) {
  fpga_properties prop = nullptr;
  // The accelerator token will have a parent token set.
  ASSERT_EQ(fpgaGetProperties(tokens_accel_[0], &prop), FPGA_OK);
  // When this parent is set explicitly, the parent token wrapper is freed.
  EXPECT_EQ(fpgaPropertiesSetParent(prop, tokens_device_[0]), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);
}

TEST_P(properties_c_p, from_handle01) {
  fpga_properties props = nullptr;
  EXPECT_EQ(fpgaGetPropertiesFromHandle(accel_, &props), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&props), FPGA_OK);
}

/**
 * @test    from_token03
 * @brief   Tests: fpgaGetProperties
 * @details When the input token is valid<br>
 *          and the call is successful,<br>
 *          fpgaGetProperties returns FPGA_OK.<br>
 */
TEST_P(properties_c_p, from_token03) {
  fpga_properties props = nullptr;
  EXPECT_EQ(fpgaGetProperties(tokens_accel_[0], &props), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&props), FPGA_OK);
}

/**
 * @test    update01
 * @brief   Tests: fpgaUpdateProperties
 * @details When the input properties object has a parent token set,<br>
 *          fpgaUpdateProperties re-uses the wrapper object.<br>
 *          If a subsequent call to fpgaUpdateProperties results in a properites<br>
 *          object without a parent token,<br>
 *          then the wrapper object is freed.<br>
 */
TEST_P(properties_c_p, update01) {
  fpga_properties props = nullptr;
  ASSERT_EQ(fpgaGetProperties(NULL, &props), FPGA_OK);
  EXPECT_EQ(fpgaUpdateProperties(tokens_accel_[0], props), FPGA_OK);
  // The output properties for the accelerator will have a parent token.

  // Updating the properties again (accelerator) will re-use the existing token wrapper.
  EXPECT_EQ(fpgaUpdateProperties(tokens_accel_[0], props), FPGA_OK);

  // Updating the properties for a device token will not result in
  // a parent token. The token wrapper will be destroyed.
  EXPECT_EQ(fpgaUpdateProperties(tokens_device_[0], props), FPGA_OK);

  EXPECT_EQ(fpgaDestroyProperties(&props), FPGA_OK);
}

/**
 * @test    get_parent_null_props
 * @brief   Tests: fpgaPropertiesGetParent
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetParent with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 **/
TEST(properties, get_parent_null_props) {
  fpga_properties prop = NULL;

  fpga_token token;
  fpga_result result = fpgaPropertiesGetParent(prop, &token);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_parent_null_token
 * @brief   Tests: fpgaPropertiesSetParent
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetParent with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_parent_null_token) {
  fpga_properties prop = NULL;

  // Call the API to set the token on the property
  fpga_token token = nullptr;
  fpga_result result = fpgaPropertiesSetParent(prop, &token);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    create
 * @brief   Tests: fpgaGetProperties
 * @details Given a null fpga_properties object<br>
 *          When I call fpgaGetProperties with the object<br>
 *          Then the return value is FPGA_OK<br>
 *          And the fpga_properties object is non-null<br>
 */
TEST_P(properties_c_p, create) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_EQ(NULL, ((struct _fpga_properties*)prop)->parent);
  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(result, FPGA_OK);
}

/**
 * @test    destroy01
 * @brief   Tests: fpgaDestroyProperties
 * @details Given a non-null fpga_properties object<br>
 *          When I call fpgaDestroyProperties with that object<br>
 *          Then the result is FPGA_OK<br>
 *          And that object is null<br>
 */
TEST_P(properties_c_p, destroy01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(FPGA_OK, result);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    destroy02
 * @brief   Tests: fpgaDestroyProperties
 * @details Given a null fpga_properties object<br>
 *          When I call fpgaDestroyProperties with that object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, destroy02) {
  fpga_properties prop = NULL;

  fpga_result result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(FPGA_INVALID_PARAM, result);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    clear01
 * @brief   Tests: fpgaClearProperties
 * @details Given a non-null fpga_properties object<br>
 *          When I call fpgaClearProperties with the object<br>
 *          Then the result is FPGA_OK<br>
 *          And the properties object is cleared<br>
 */
TEST_P(properties_c_p, clear01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the bus field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_BUS);
  _prop->bus = 0xAB;

  result = fpgaClearProperties(prop);
  EXPECT_EQ(FPGA_OK, result);
  EXPECT_EQ(_prop, prop);
  EXPECT_EQ(0, _prop->valid_fields);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(FPGA_OK, result);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    clear02
 * @brief   Tests: fpgaClearProperties
 * @details Given a null fpga_properties object<br>
 *          When I call fpgaClearProperties with the object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, clear02) {
  fpga_properties prop = NULL;

  fpga_result result = fpgaClearProperties(prop);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * ObjectType *//
/**
 * @test    get_object_type01
 * @brief   Tests: fpgaPropertiesGetObjectType
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the objtype field set<br>
 *          And its objtype field is a known value<br>
 *          When I call fpgaPropertiesGetObjectType with a pointer to an
 *          fpga_objtype<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_object_type01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // now get the parent token from the prop structure
  fpga_objtype objtype;
  result = fpgaPropertiesGetObjectType(prop, &objtype);
  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  EXPECT_EQ(FPGA_DEVICE, objtype);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_object_type02
 * @brief   Tests: fpgaPropertiesGetObjectType
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the objtype field set<br>
 *          When I call fpgaPropertiesGetObjectType with a pointer to an
 *          fpga_objtype<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_object_type02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_OBJTYPE bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_OBJTYPE) & 1, 0);

  fpga_objtype objtype;
  result = fpgaPropertiesGetObjectType(prop, &objtype);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_object_type01
 * @brief   Tests: fpgaPropertiesSetObjectType
 * @details Given a non-null fpga_properties* object<br>
 *          And a fpga_objtype object set to a known value<br>
 *          When I call fpgaPropertiesSetObjectType with the properties
 object
 *          and the objtype<br>
 *          Then the objtype in the properties object is the known value<br>
 */
TEST_P(properties_c_p, set_object_type01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  fpga_objtype objtype = FPGA_DEVICE;
  result = fpgaPropertiesSetObjectType(prop, objtype);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  EXPECT_EQ(FPGA_DEVICE, _prop->objtype);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_object_type03
 * @brief   Tests: fpgaPropertiesGetObjectType
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetObjectType with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_object_type03) {
  fpga_properties prop = NULL;

  fpga_objtype objtype;
  fpga_result result = fpgaPropertiesGetObjectType(prop, &objtype);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_object_type02
 * @brief   Tests: fpgaPropertiesSetObjectType
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetObjectType with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_object_type02) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_objtype objtype = FPGA_DEVICE;
  fpga_result result = fpgaPropertiesSetObjectType(prop, objtype);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * Segment field tests *//
/**
 * @test    get_segment01
 * @brief   Tests: fpgaPropertiesGetSegment
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the bus field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetSegment with a pointer to an
 integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 */
TEST_P(properties_c_p, get_segment01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the segment field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_SEGMENT);
  _prop->segment = 0xc001;

  // now get the segment number using the API
  uint16_t segment = 0;
  result = fpgaPropertiesGetSegment(prop, &segment);
  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  // (Get the subfield manually)
  EXPECT_EQ(0xc001, segment);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_segment02
 * @brief   Tests: fpgaPropertiesGetSegment
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the bus field set<br>
 *          When I call fpgaPropertiesGetSegment with a pointer to an
 integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_segment02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_SEGMENT bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_SEGMENT) & 1, 0);

  uint16_t segment;
  result = fpgaPropertiesGetSegment(prop, &segment);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_segment01
 * @brief   Tests: fpgaPropertiesSetSegment
 * @details Given a non-null fpga_properties* object<br>
 *          And segment variable set to a known value<br>
 *          When I call fpgaPropertiesSetSegment with the properties object
 and
 *          the segment variable<br>
 *          Then the segment field in the properties object is the known
 value<br>
 */
TEST_P(properties_c_p, set_segment01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  uint16_t segment = 0xc001;
  // make sure the FPGA_PROPERTY_SEGMENT bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_SEGMENT) & 1, 0);
  // Call the API to set the segment on the property
  result = fpgaPropertiesSetSegment(prop, segment);

  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_SEGMENT bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_SEGMENT) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xc001, _prop->segment);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_segment03
 * @brief   Tests: fpgaPropertiesGetSegment
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetSegment with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_segment03) {
  fpga_properties prop = NULL;

  uint16_t segment;
  fpga_result result = fpgaPropertiesGetSegment(prop, &segment);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_segment02
 * @brief   Tests: fpgaPropertiesSetSegment
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetSegment with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_segment02) {
  fpga_properties prop = NULL;

  // Call the API to set the segment on the property
  fpga_result result = fpgaPropertiesSetSegment(prop, 0xc001);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * Bus field tests *//
/**
 * @test    get_bus01
 * @brief   Tests: fpgaPropertiesGetBus
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the bus field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetBus with a pointer to an integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_bus01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_BUS);
  _prop->bus = 0xAE;

  // now get the bus number using the API
  uint8_t bus;
  result = fpgaPropertiesGetBus(prop, &bus);
  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  // (Get the subfield manually)
  EXPECT_EQ(0xAE, bus);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bus02
 * @brief   Tests: fpgaPropertiesGetBus
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the bus field set<br>
 *          When I call fpgaPropertiesGetBus with a pointer to an integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_bus02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_BUS bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BUS) & 1, 0);

  uint8_t bus;
  result = fpgaPropertiesGetBus(prop, &bus);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_bus01
 * @brief   Tests: fpgaPropertiesSetBus
 * @details Given a non-null fpga_properties* object<br>
 *          And bus variable set to a known value<br>
 *          When I call fpgaPropertiesSetBus with the properties object and
 *          the bus variable<br>
 *          Then the bus field in the properties object is the known
 value<br>
 */
TEST_P(properties_c_p, set_bus01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  uint8_t bus = 0xAE;
  // make sure the FPGA_PROPERTY_BUS bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BUS) & 1, 0);
  // Call the API to set the bus on the property
  result = fpgaPropertiesSetBus(prop, bus);

  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_BUS bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BUS) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xAE, _prop->bus);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bus03
 * @brief   Tests: fpgaPropertiesGetBus
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetBus with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_bus03) {
  fpga_properties prop = NULL;

  uint8_t bus;
  fpga_result result = fpgaPropertiesGetBus(prop, &bus);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_bus02
 * @brief   Tests: fpgaPropertiesSetBus
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetBus with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_bus02) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_result result = fpgaPropertiesSetBus(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * Device field tests *//
/**
 * @test    get_device01
 * @brief   Tests: fpgaPropertiesGetDevice
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the device field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetDevice with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_device01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_DEVICE);
  _prop->device = 0xAE;

  // now get the device number using the API
  uint8_t device;
  result = fpgaPropertiesGetDevice(prop, &device);
  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  // (Get the subfield manually)
  EXPECT_EQ(0xAE, device);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_device02
 * @brief   Tests: fpgaPropertiesGetDevice
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the device field set<br>
 *          When I call fpgaPropertiesGetDevice with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_device02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_DEVICE bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_DEVICE) & 1, 0);

  uint8_t device;
  result = fpgaPropertiesGetDevice(prop, &device);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_device01
 * @brief   Tests: fpgaPropertiesSetDevice
 * @details Given a non-null fpga_properties* object<br>
 *          And device variable set to a known value<br>
 *          When I call fpgaPropertiesSetDevice with the properties object
 *          and the device variable<br>
 *          Then the device field in the properties object is the known
 *          value<br>
 */
TEST_P(properties_c_p, set_device01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  uint8_t device = 0x1f;  // max of 32 devices

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_DEVICE bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_DEVICE) & 1, 0);

  // Call the API to set the device on the property
  result = fpgaPropertiesSetDevice(prop, device);

  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_DEVICE bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_DEVICE) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0x1f, _prop->device);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_device03
 * @brief   Tests: fpgaPropertiesGetDevice
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetDevice with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_device03) {
  fpga_properties prop = NULL;

  uint8_t device;
  fpga_result result = fpgaPropertiesGetDevice(prop, &device);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_device02
 * @brief   Tests: fpgaPropertiesSetDevice
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetDevice with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_device02) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_result result = fpgaPropertiesSetDevice(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * Function field tests *//
/**
 * @test    get_function01
 * @brief   Tests: fpgaPropertiesGetFunction
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the function field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetFunction with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_function01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_FUNCTION);
  _prop->function = 0xAE;

  // now get the function number using the API
  uint8_t function;
  result = fpgaPropertiesGetFunction(prop, &function);
  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  // (Get the subfield manually)
  EXPECT_EQ(0xAE, function);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_function02
 * @brief   Tests: fpgaPropertiesGetFunction
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the function field set<br>
 *          When I call fpgaPropertiesGetFunction with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_function02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_FUNCTION bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_FUNCTION) & 1, 0);

  uint8_t function;
  result = fpgaPropertiesGetFunction(prop, &function);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_function01
 * @brief   Tests: fpgaPropertiesSetFunction
 * @details Given a non-null fpga_properties* object<br>
 *          And function variable set to a known value<br>
 *          When I call fpgaPropertiesSetFunction with the properties object
 *          and the function variable<br>
 *          Then the function field in the properties object is the known
 *          value<br>
 */
TEST_P(properties_c_p, set_function01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  uint8_t function = 7;  // max of 8 functions

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_FUNCTION bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_FUNCTION) & 1, 0);

  // Call the API to set the function on the property
  result = fpgaPropertiesSetFunction(prop, function);

  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_FUNCTION bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_FUNCTION) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(7, _prop->function);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_function03
 * @brief   Tests: fpgaPropertiesGetFunction
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetFunction with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_function03) {
  fpga_properties prop = NULL;

  uint8_t function;
  fpga_result result = fpgaPropertiesGetFunction(prop, &function);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_function02
 * @brief   Tests: fpgaPropertiesSetFunction
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetFunction with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_function02) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_result result = fpgaPropertiesSetFunction(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_function03
 * @brief   Tests: fpgaPropertiesSetFunction
 * @details When fpgaPropertiesSetFunction is called with an invalid<br>
 *          PCIe function number,<br>
 *          Then the result is FPGA_INVALID_PARAM.<br>
 */
TEST_P(properties_c_p, set_function03) {
  // Call the API to set the objtype on the property
  EXPECT_EQ(fpgaPropertiesSetFunction(filter_, 8), FPGA_INVALID_PARAM);
}

// * SocketID field tests *//
/**
 * @test    get_socket_id01
 * @brief   Tests: fpgaPropertiesGetSocketID
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the socket_id field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetSocketID with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_socket_id01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_SOCKETID);
  _prop->socket_id = 0xAE;

  // now get the socket_id number using the API
  uint8_t socket_id;
  result = fpgaPropertiesGetSocketID(prop, &socket_id);
  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  // (Get the subfield manually)
  EXPECT_EQ(0xAE, socket_id);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_socket_id02
 * @brief   Tests: fpgaPropertiesGetSocketID
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the socket_id field set<br>
 *          When I call fpgaPropertiesGetSocketID with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_socket_id02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_SOCKETID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_SOCKETID) & 1, 0);

  uint8_t socket_id;
  result = fpgaPropertiesGetSocketID(prop, &socket_id);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_socket_id01
 * @brief   Tests: fpgaPropertiesSetSocketID
 * @details Given a non-null fpga_properties* object<br>
 *          And socket_id variable set to a known value<br>
 *          When I call fpgaPropertiesSetSocketID with the properties object
 *          and the socket_id variable<br>
 *          Then the socket_id field in the properties object is the known
 *          value<br>
 */
TEST_P(properties_c_p, set_socket_id01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  uint8_t socket_id = 0xAE;

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_SOCKETID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_SOCKETID) & 1, 0);

  // Call the API to set the socket_id on the property
  result = fpgaPropertiesSetSocketID(prop, socket_id);

  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_SOCKETID bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_SOCKETID) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xAE, _prop->socket_id);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_socket_id03
 * @brief   Tests: fpgaPropertiesGetSocketID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetSocketID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_socket_id03) {
  fpga_properties prop = NULL;

  uint8_t socket_id;
  fpga_result result = fpgaPropertiesGetSocketID(prop, &socket_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_socket_id02
 * @brief   Tests: fpgaPropertiesSetSocketID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetSocketID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_socket_id02) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_result result = fpgaPropertiesSetSocketID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** fpga.num_slots field tests **/
/**
 * @test    get_num_slots01
 * @brief   Tests: fpgaPropertiesGetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the num_slots field set to a known value<br>
 *          When I call fpgaPropertiesGetNumSlots with a pointer to an
 integer
 *          variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_num_slots01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

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
  result = fpgaPropertiesGetNumSlots(prop, &num_slots);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0xCAFE, num_slots);

  // now delete the properties object
  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_slots02
 * @brief   Tests: fpgaPropertiesGetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call fpgaPropertiesGetNumSlots with a pointer to an
 integer
 *          variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST_P(properties_c_p, get_num_slots02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field to a different type
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // now get the num_slots from the prop structure
  uint32_t num_slots;
  result = fpgaPropertiesGetNumSlots(prop, &num_slots);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_slots03
 * @brief   Tests: fpgaPropertiesGetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_DEVICE
 *          And it does NOT have the num_slots field set<br>
 *          When I call fpgaPropertiesGetNumSlots with the property object
 *          and a pointer to bool variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_num_slots03) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);

  // make sure the FPGA_PROPERTY_NUM_SLOTS bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_SLOTS) & 1, 0);

  uint32_t num_slots;
  result = fpgaPropertiesGetNumSlots(prop, &num_slots);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_num_slots01
 * @brief   Tests: fpgaPropertiesSetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetNumSlots with the properties object
 *          and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the num_slots in the properties object is the known value<br>
 */
TEST_P(properties_c_p, set_num_slots01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
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
  result = fpgaPropertiesSetNumSlots(prop, num_slots);

  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_NUM_SLOTS bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_SLOTS) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xCAFE, _prop->u.fpga.num_slots);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_num_slots02
 * @brief   Tests: fpgaPropertiesSetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_DEVICE<br>
 *          When I call fpgaPropertiesSetNumSlots with the properties object
 *          and a num_slots variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST_P(properties_c_p, set_num_slots02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  _prop->objtype = FPGA_ACCELERATOR;

  // Call the API to set the token on the property
  result = fpgaPropertiesSetNumSlots(prop, 0);

  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_slots04
 * @brief   Tests: fpgaPropertiesGetNumSlots
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetNumSlots with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_num_slots04) {
  fpga_properties prop = NULL;

  uint32_t num_slots;
  fpga_result result = fpgaPropertiesGetNumSlots(prop, &num_slots);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_num_slots03
 * @brief   Tests: fpgaPropertiesSetNumSlots
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetNumSlots with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_num_slots03) {
  fpga_properties prop = NULL;

  // Call the API to set the num_slots on the property
  fpga_result result = fpgaPropertiesSetNumSlots(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** fpga.bbs_id field tests **/
/**
 * @test    get_bbs_id01
 * @brief   Tests: fpgaPropertiesGetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the bbs_id field set to a known value<br>
 *          When I call fpgaPropertiesGetBBSID with a pointer to an integer
 *          variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_bbs_id01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

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
  result = fpgaPropertiesGetBBSID(prop, &bbs_id);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0xCAFE, bbs_id);

  // now delete the properties object
  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bbs_id02
 * @brief   Tests: fpgaPropertiesGetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call fpgaPropertiesGetBBSID with a pointer to an integer
 *          variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST_P(properties_c_p, get_bbs_id02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field to a different type
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // now get the bbs_id from the prop structure
  uint64_t bbs_id;
  result = fpgaPropertiesGetBBSID(prop, &bbs_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bbs_id03
 * @brief   Tests: fpgaPropertiesGetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_DEVICE
 *          And it does NOT have the bbs_id field set<br>
 *          When I call fpgaPropertiesGetBBSID with the property object
 *          and a pointer to bool variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_bbs_id03) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);

  // make sure the FPGA_PROPERTY_BBSID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BBSID) & 1, 0);

  uint64_t bbs_id;
  result = fpgaPropertiesGetBBSID(prop, &bbs_id);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_bbs_id01
 * @brief   Tests: fpgaPropertiesSetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetBBSID with the properties object
 *          and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the bbs_id in the properties object is the known value<br>
 */
TEST_P(properties_c_p, set_bbs_id01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
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
  result = fpgaPropertiesSetBBSID(prop, bbs_id);
  EXPECT_EQ(result, FPGA_OK);

#ifndef BUILD_ASE
  // make sure the FPGA_PROPERTY_BBSID bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BBSID) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xCAFE, _prop->u.fpga.bbs_id);
#endif

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_bbs_id02
 * @brief   Tests: fpgaPropertiesSetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_DEVICE<br>
 *          When I call fpgaPropertiesSetBBSID with the properties object
 *          and a bbs_id variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 */
TEST_P(properties_c_p, set_bbs_id02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // Call the API to set the token on the property
  result = fpgaPropertiesSetBBSID(prop, 0);

  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bbs_id04
 * @brief   Tests: fpgaPropertiesGetBBSID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetBBSID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_bbs_id04) {
  fpga_properties prop = NULL;

  uint64_t bbs_id;
  fpga_result result = fpgaPropertiesGetBBSID(prop, &bbs_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_bbs_id03
 * @brief   Tests: fpgaPropertiesSetBBSID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetBBSID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_bbs_id03) {
  fpga_properties prop = NULL;

  // Call the API to set the bbs_id on the property
  fpga_result result = fpgaPropertiesSetBBSID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** fpga.bbs_version field tests **/
/**
 * @test    get_bbs_version01
 * @brief   Tests: fpgaPropertiesGetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the bbs_version field set to a known value<br>
 *          When I call fpgaPropertiesGetBBSVersion with a pointer to an
 *          fpga_version variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_bbs_version01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

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
  result = fpgaPropertiesGetBBSVersion(prop, &bbs_version);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

// assert it is set to what we set it to above
#ifndef BUILD_ASE
  EXPECT_EQ(0, memcmp(&v, &bbs_version, sizeof(fpga_version)));
#endif

  // now delete the properties object
  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bbs_version02
 * @brief   Tests: fpgaPropertiesGetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call fpgaPropertiesGetBBSVersion with a pointer to an
 *          fpga_version variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST_P(properties_c_p, get_bbs_version02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field to a different type
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // now get the bbs_version from the prop structure
  fpga_version bbs_version;
  result = fpgaPropertiesGetBBSVersion(prop, &bbs_version);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bbs_version03
 * @brief   Tests: fpgaPropertiesGetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_DEVICE
 *          And it does NOT have the bbs_version field set<br>
 *          When I call fpgaPropertiesGetBBSVersion with the property object
 *          and a pointer to an fpga_version variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_bbs_version03) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type and number of slots fields as valid
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);

  // make sure the FPGA_PROPERTY_BBSVERSION bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BBSVERSION) & 1, 0);

  fpga_version bbs_version;
  result = fpgaPropertiesGetBBSVersion(prop, &bbs_version);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_bbs_version01
 * @brief   Tests: fpgaPropertiesSetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an fpga_version variable set to a known value<br>
 *          When I call fpgaPropertiesSetBBSVersion with the properties
 object
 *          and the fpga_version variable<br>
 *          Then the return value is FPGA_OK
 *          And the bbs_version in the properties object is the known
 value<br>
 */
TEST_P(properties_c_p, set_bbs_version01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
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
  result = fpgaPropertiesSetBBSVersion(prop, bbs_version);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_BBSVERSION bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_BBSVERSION) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0, memcmp(&bbs_version, &(_prop->u.fpga.bbs_version),
                      sizeof(fpga_version)));

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_bbs_version02
 * @brief   Tests: fpgaPropertiesSetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_DEVICE<br>
 *          When I call fpgaPropertiesSetBBSVersion with the properties
 object
 *          and a bbs_version variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST_P(properties_c_p, set_bbs_version02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // Call the API to set the token on the property
  fpga_version v = {0, 0, 0};
  result = fpgaPropertiesSetBBSVersion(prop, v);

  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_bbs_version04
 * @brief   Tests: fpgaPropertiesGetBBSVersion
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetBBSVersion with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_bbs_version04) {
  fpga_properties prop = NULL;

  fpga_version bbs_version;
  fpga_result result = fpgaPropertiesGetBBSVersion(prop, &bbs_version);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_bbs_version03
 * @brief   Tests: fpgaPropertiesSetBBSVersion
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetBBSVersion with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_bbs_version03) {
  fpga_properties prop = NULL;

  // Call the API to set the bbs_version on the property
  fpga_version v = {0, 0, 0};
  fpga_result result = fpgaPropertiesSetBBSVersion(prop, v);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    fpga_clone_properties01
 * @brief   Tests: fpgaClonePoperties
 * @details Given a fpga_properties object cloned with
 fpgaCloneProperties<br>
 *          When I call fpgaDestroyProperties with the cloned object<br>
 *          Then the result is FPGA_OK<br>
 *          And the properties object is destroyed appropriately<br>
 */
TEST_P(properties_c_p, fpga_clone_properties01) {
  fpga_properties prop = NULL;
  fpga_properties clone = NULL;
  uint8_t s1 = 0xEF, s2 = 0;
  ASSERT_EQ(FPGA_OK, fpgaGetProperties(NULL, &prop));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetSocketID(prop, s1));
  ASSERT_EQ(FPGA_OK, fpgaCloneProperties(prop, &clone));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetSocketID(clone, &s2));
  EXPECT_EQ(s1, s2);
  ASSERT_EQ(FPGA_OK, fpgaDestroyProperties(&clone));
  ASSERT_EQ(FPGA_OK, fpgaDestroyProperties(&prop));
  ASSERT_EQ(FPGA_INVALID_PARAM, fpgaCloneProperties(NULL, &clone));
}

/**
 * @test    set_model01
 * @brief   Tests: fpgaPropertiesSetModel
 * @details fpgaPropertiesSetModel is not currently supported.
 *
 */
TEST(properties, set_model01) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesSetModel(NULL, 0));
}

/**
 * @test    get_model01
 * @brief   Tests: fpgaPropertiesGetModel
 * @details fpgaPropertiesGetModel is not currently supported.
 *
 */
TEST(properties, get_model01) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesGetModel(NULL, NULL));
}

/**
 * @test    get_lms01
 * @brief   Tests: fpgaPropertiesGetLocalMemorySize
 * @details fpgaPropertiesGetLocalMemorySize is not currently supported.
 *
 */
TEST(properties, get_lms01) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesGetLocalMemorySize(NULL, NULL));
}

/**
 * @test    set_lms01
 * @brief   Tests: fpgaPropertiesSetLocalMemorySize
 * @details fpgaPropertiesSetLocalMemorySize is not currently supported.
 *
 */
TEST(properties, set_lms01) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesSetLocalMemorySize(NULL, 0));
}

/**
 * @test    set_capabilities01
 * @brief   Tests: fpgaPropertiesSetCapabilities
 * @details fpgaPropertiesSetCapabilities is not currently supported.
 *
 */
TEST(properties, set_capabilities01) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesSetCapabilities(NULL, 0));
}

/**
 * @test    get_capabilities01
 * @brief   Tests: fpgaPropertiesGetCapabilities
 * @details fpgaPropertiesGetCapabilities is not currently supported.
 *
 */
TEST(properties, get_capabilities01) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesGetCapabilities(NULL, NULL));
}

/** (afu | accelerator).guid field tests **/
/**
 * @test    get_guid01
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR
 *          And it has the guid field set to a known value<br>
 *          When I call fpgaPropertiesGetGUID with a pointer to an
 *          guid variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_guid01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

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
  result = fpgaPropertiesGetGUID(prop, &guid);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0, memcmp(guid, known_guid, 16));

  // now delete the properties object
  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_guid02
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR
 *          And it has the guid field set to a known value<br>
 *          When I call fpgaPropertiesGetGUID with a pointer to an
 *          guid variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_guid02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

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
  result = fpgaPropertiesGetGUID(prop, &guid);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0, memcmp(guid, known_guid, 16));

  // now delete the properties object
  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_guid03
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_DEVICE<br>
 *          And it does NOT have the guid field set<br>
 *          When I call fpgaPropertiesGetGUID with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_guid03) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type to FPGA_DEVICE
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // make sure the FPGA_PROPERTY_GUID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);

  fpga_guid guid;
  result = fpgaPropertiesGetGUID(prop, &guid);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_guid04
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the guid field set<br>
 *          When I call fpgaPropertiesGetGUID with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_guid04) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type to FPGA_ACCELERATOR
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_GUID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);

  fpga_guid guid;
  result = fpgaPropertiesGetGUID(prop, &guid);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_guid05
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the guid field set<br>
 *          When I call fpgaPropertiesGetGUID with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_guid05) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type to FPGA_ACCELERATOR
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_GUID bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 0);

  fpga_guid guid;
  result = fpgaPropertiesGetGUID(prop, &guid);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_guid01
 * @brief   Tests: fpgaPropertiesSetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_ACCELERATOR<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetGUID with the properties
 *          object and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the guid in the properties object is the known
 *          value<br>
 */
TEST_P(properties_c_p, set_guid01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
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
  result = fpgaPropertiesSetGUID(prop, guid);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_GUID bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0, memcmp(guid, _prop->guid, 16));

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_guid02
 * @brief   Tests: fpgaPropertiesSetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_ACCELERATOR<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetGUID with the properties
 *          object and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the guid in the properties object is the known
 *          value<br>
 */
TEST_P(properties_c_p, set_guid02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
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
  result = fpgaPropertiesSetGUID(prop, guid);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_GUID bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0, memcmp(guid, _prop->guid, 16));

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_guid03
 * @brief   Tests: fpgaPropertiesSetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_DEVICE<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetGUID with the properties
 *          object and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the guid in the properties object is the known
 *          value<br>
 */
TEST_P(properties_c_p, set_guid03) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
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
  result = fpgaPropertiesSetGUID(prop, guid);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_GUID bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_GUID) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0, memcmp(guid, _prop->guid, 16));

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_guid06
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetGUID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_guid06) {
  fpga_properties prop = NULL;

  fpga_guid guid;
  fpga_result result = fpgaPropertiesGetGUID(prop, &guid);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    get_guid07
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetGUID with a null guid parameter<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST_P(properties_c_p, get_guid07) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  result = fpgaPropertiesGetGUID(prop, NULL);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(FPGA_OK, result);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_guid04
 * @brief   Tests: fpgaPropertiesSetGUID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetGUID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_guid04) {
  fpga_properties prop = NULL;
  fpga_guid guid;
  // Call the API to set the guid on the property
  fpga_result result = fpgaPropertiesSetGUID(prop, guid);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** accelerator.mmio_spaces field tests **/
/**
 * @test    get_num_mmio01
 * @brief   Tests: fpgaPropertiesGetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR
 *          And it has the mmio_spaces field set to a known value<br>
 *          When I call fpgaPropertiesGetNumMMIO with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_num_mmio01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

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
  result = fpgaPropertiesGetNumMMIO(prop, &mmio_spaces);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0xAE, mmio_spaces);

  // now delete the properties object
  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_mmio02
 * @brief   Tests: fpgaPropertiesGetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesGetNumMMIO with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST_P(properties_c_p, get_num_mmio02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field to a different type
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // now get the mmio_spaces from the prop structure
  uint32_t mmio_spaces;
  result = fpgaPropertiesGetNumMMIO(prop, &mmio_spaces);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_mmio03
 * @brief   Tests: fpgaPropertiesGetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the mmio_spaces field set<br>
 *          When I call fpgaPropertiesGetNumMMIO with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_num_mmio03) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type to FPGA_AFU
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_NUM_MMIO bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_MMIO) & 1, 0);

  uint32_t mmio_spaces;
  result = fpgaPropertiesGetNumMMIO(prop, &mmio_spaces);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_num_mmio01
 * @brief   Tests: fpgaPropertiesSetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesSetNumMMIO with the properties
 *          object and a known value for mmio_spaces parameter<br>
 *          Then the return value is FPGA_OK<br>
 *          And the mmio_spaces in the properties object is the known
 *          value<br>
 */
TEST_P(properties_c_p, set_num_mmio01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
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
  result = fpgaPropertiesSetNumMMIO(prop, 0xAE);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_NUM_MMIO bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_MMIO) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xAE, _prop->u.accelerator.num_mmio);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_num_mmio02
 * @brief   Tests: fpgaPropertiesSetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesSetNumMMIO with the properties
 *          object<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST_P(properties_c_p, set_num_mmio02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // Call the API to set the slot mmio_spaces
  result = fpgaPropertiesSetNumMMIO(prop, 0);

  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_mmio04
 * @brief   Tests: fpgaPropertiesGetNumMMIO
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetNumMMIO with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_num_mmio04) {
  fpga_properties prop = NULL;

  uint32_t mmio_spaces;
  fpga_result result = fpgaPropertiesGetNumMMIO(prop, &mmio_spaces);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_num_mmio03
 * @brief   Tests: fpgaPropertiesSetNumMMIO
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetNumMMIO with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_num_mmio03) {
  fpga_properties prop = NULL;
  // Call the API to set the mmio_spaces on the property
  fpga_result result = fpgaPropertiesSetNumMMIO(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** accelerator.state field tests **/
/**
 * @test    get_accelerator_state01
 * @brief   Tests: fpgaPropertiesGetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR
 *          And it has the state field set to a known value<br>
 *          When I call fpgaPropertiesGetAcceleratorState with a pointer to
 an
 *          fpga_accelerator_state variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_accelerator_state01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

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
  result = fpgaPropertiesGetAcceleratorState(prop, &state);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(FPGA_ACCELERATOR_UNASSIGNED, state);

  // now delete the properties object
  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_accelerator_state02
 * @brief   Tests: fpgaPropertiesGetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesGetAcceleratorState with a pointer to
 an
 *          fpga_accelerator_state variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST_P(properties_c_p, get_accelerator_state02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field to a different type
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // now get the state from the prop structure
  fpga_accelerator_state state;
  result = fpgaPropertiesGetAcceleratorState(prop, &state);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_accelerator_state03
 * @brief   Tests: fpgaPropertiesGetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the state field set<br>
 *          When I call fpgaPropertiesGetAcceleratorState with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_accelerator_state03) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type to FPGA_DEVICE
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_ACCELERATOR_STATE bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_ACCELERATOR_STATE) & 1, 0);

  fpga_accelerator_state state;
  result = fpgaPropertiesGetAcceleratorState(prop, &state);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_accelerator_state01
 * @brief   Tests: fpgaPropertiesSetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesSetAcceleratorState with the properties
 *          object and a known accelerator state variable<br>
 *          Then the return value is FPGA_OK
 *          And the state in the properties object is the known
 *          value<br>
 */
TEST_P(properties_c_p, set_accelerator_state01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
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
  result = fpgaPropertiesSetAcceleratorState(prop, FPGA_ACCELERATOR_UNASSIGNED);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_ACCELERATOR_STATE bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_ACCELERATOR_STATE) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(FPGA_ACCELERATOR_UNASSIGNED, _prop->u.accelerator.state);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_accelerator_state02
 * @brief   Tests: fpgaPropertiesSetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesSetAcceleratorState with the properties
 *          object and a state variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST_P(properties_c_p, set_accelerator_state02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // Call the API to set the accelerator state
  fpga_accelerator_state state = FPGA_ACCELERATOR_ASSIGNED;
  result = fpgaPropertiesSetAcceleratorState(prop, state);

  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_accelerator_state04
 * @brief   Tests: fpgaPropertiesGetAcceleratorState
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetAcceleratorState with the null
 * object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_accelerator_state04) {
  fpga_properties prop = NULL;

  fpga_accelerator_state state;
  fpga_result result = fpgaPropertiesGetAcceleratorState(prop, &state);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    get_accelerator_state05
 * @brief   Tests: fpgaPropertiesGetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetAcceleratorState with a null state
 * pointer<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST_P(properties_c_p, get_accelerator_state05) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  result = fpgaPropertiesGetAcceleratorState(prop, NULL);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);
}

/**
 * @test    set_accelerator_state03
 * @brief   Tests: fpgaPropertiesSetAcceleratorState
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetAcceleratorState with the null
 * object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_accelerator_state03) {
  fpga_properties prop = NULL;
  // Call the API to set the state on the property
  fpga_result result =
      fpgaPropertiesSetAcceleratorState(prop, FPGA_ACCELERATOR_UNASSIGNED);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** accelerator.num_interrupts field tests **/
/**
 * @test    get_num_interrupts01
 * @brief   Tests: fpgaPropertiesGetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          And it has the num_interrupts field set to a known value<br>
 *          When I call fpgaPropertiesGetNumInterrupts with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_num_interrupts01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

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
  result = fpgaPropertiesGetNumInterrupts(prop, &num_interrupts);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0xAE, num_interrupts);

  // now delete the properties object
  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_interrupts02
 * @brief   Tests: fpgaPropertiesGetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesGetNumInterrupts with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST_P(properties_c_p, get_num_interrupts02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field to a different type
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // now get the num_interrupts from the prop structure
  uint32_t num_interrupts;
  result = fpgaPropertiesGetNumInterrupts(prop, &num_interrupts);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_interrupts03
 * @brief   Tests: fpgaPropertiesGetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the num_interrupts field set<br>
 *          When I call fpgaPropertiesGetNumInterrupts with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST_P(properties_c_p, get_num_interrupts03) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);
  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;
  // set the object type to FPGA_AFU
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_ACCELERATOR;

  // make sure the FPGA_PROPERTY_NUM_INTERRUPTS bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_INTERRUPTS) & 1, 0);

  uint32_t num_interrupts;
  result = fpgaPropertiesGetNumInterrupts(prop, &num_interrupts);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_num_interrupts01
 * @brief   Tests: fpgaPropertiesSetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesSetNumInterrupts with the properties
 *          object and a known value for num_interrupts parameter<br>
 *          Then the return value is FPGA_OK<br>
 *          And the num_interrupts in the properties object is the known
 *          value<br>
 */
TEST_P(properties_c_p, set_num_interrupts01) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
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
  result = fpgaPropertiesSetNumInterrupts(prop, 0xAE);
  EXPECT_EQ(result, FPGA_OK);

  // make sure the FPGA_PROPERTY_NUM_INTERRUPTS bit is one
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_NUM_INTERRUPTS) & 1, 1);

  // Assert it is set to what we set it to above
  EXPECT_EQ(0xAE, _prop->u.accelerator.num_interrupts);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    set_num_interrupts02
 * @brief   Tests: fpgaPropertiesSetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesSetNumInterrupts with the properties
 *          object<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST_P(properties_c_p, set_num_interrupts02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the object type field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;

  // Call the API to set the slot num_interrupts
  result = fpgaPropertiesSetNumInterrupts(prop, 0);

  EXPECT_EQ(result, FPGA_INVALID_PARAM);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_num_interrupts04
 * @brief   Tests: fpgaPropertiesGetNumInterrupts
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetNumInterrupts with the null
 object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_num_interrupts04) {
  fpga_properties prop = NULL;

  uint32_t num_interrupts;
  fpga_result result = fpgaPropertiesGetNumInterrupts(prop, &num_interrupts);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_num_interrupts03
 * @brief   Tests: fpgaPropertiesSetNumInterrupts
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetNumInterrupts with the null
 object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_num_interrupts03) {
  fpga_properties prop = NULL;
  // Call the API to set the num_interrupts on the property
  fpga_result result = fpgaPropertiesSetNumInterrupts(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    prop_213
 * @brief   Tests: fpgaGetProperties
 * @details When creating a properties object<br>
 *          Then the internal magic should be set to FPGA_PROPERTY_MAGIC<br>
 */
TEST_P(properties_c_p, prop_213) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  EXPECT_EQ(FPGA_OK, result);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  EXPECT_EQ(FPGA_PROPERTY_MAGIC, _prop->magic);
  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);
}

/**
 * @test    prop_214
 * @brief   Tests: fpgaGetProperties
 * @details When creating a properties object with a null properties
 * argument<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, prop_214) {
  fpga_result result = FPGA_OK;
  ASSERT_NO_THROW(result = fpgaGetProperties(NULL, NULL));
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    get_vendor_id01
 * @brief   Tests: fpgaPropertiesGetVendorID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetVendorID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_vendor_id01) {
  fpga_properties prop = NULL;

  uint16_t vendor_id;
  fpga_result result = fpgaPropertiesGetVendorID(prop, &vendor_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_vendor_id01
 * @brief   Tests: fpgaPropertiesSetVendorID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetVendorID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_vendor_id01) {
  fpga_properties prop = NULL;
  // Call the API to set the vendor_id on the property
  fpga_result result = fpgaPropertiesSetVendorID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    get_vendor_id02
 * @brief   Tests: fpgaPropertiesGetVendorID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          And it has the vendor_id field set to a known value<br>
 *          When I call fpgaPropertiesGetVendorID with a pointer to an
 *          16-bit integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_vendor_id02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

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
  result = fpgaPropertiesGetVendorID(prop, &vendor_id);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0x8087, vendor_id);

  // now delete the properties object
  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_vendor_id03
 * @brief   Tests: fpgaPropertiesGetVendorID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          But the vendor_id field is not set,<br>
 *          When I call fpgaPropertiesGetVendorID with a pointer to an
 *          16-bit integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND.<br>
 */
TEST_P(properties_c_p, get_vendor_id03) {
  uint16_t vendor = 0;
  EXPECT_EQ(fpgaPropertiesGetVendorID(filter_, &vendor), FPGA_NOT_FOUND);
}

/**
 * @test    get_device_id01
 * @brief   Tests: fpgaPropertiesGetDeviceID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetDeviceID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_device_id01) {
  fpga_properties prop = NULL;

  uint16_t device_id;
  fpga_result result = fpgaPropertiesGetDeviceID(prop, &device_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_device_id01
 * @brief   Tests: fpgaPropertiesSetDeviceID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetDeviceID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_device_id01) {
#ifndef BUILD_ASE
  fpga_properties prop = NULL;
  // Call the API to set the device_id on the property
  fpga_result result = fpgaPropertiesSetDeviceID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
#endif
}

/**
 * @test    get_device_id02
 * @brief   Tests: fpgaPropertiesGetDeviceID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          And it has the device_id field set to a known value<br>
 *          When I call fpgaPropertiesGetDeviceID with a pointer to an
 *          16-bit integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_device_id02) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

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
  result = fpgaPropertiesGetDeviceID(prop, &device_id);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(0xAFFE, device_id);

  // now delete the properties object
  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_device_id03
 * @brief   Tests: fpgaPropertiesGetDeviceID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          But the device_id field is not set,<br>
 *          When I call fpgaPropertiesGetDeviceID with a pointer to an
 *          16-bit integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND.<br>
 */
TEST_P(properties_c_p, get_device_id03) {
  uint16_t devid = 0;
  EXPECT_EQ(fpgaPropertiesGetDeviceID(filter_, &devid), FPGA_NOT_FOUND);
}

/**
 * @test    get_object_id01
 * @brief   Tests: fpgaPropertiesGetObjectID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetObjectID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(properties, get_object_id01) {
  fpga_properties prop = NULL;

  uint64_t object_id;
  fpga_result result = fpgaPropertiesGetObjectID(prop, &object_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    set_object_id01
 * @brief   Tests: fpgaPropertiesSetObjectID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetObjectID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(properties, set_object_id01) {
  fpga_properties prop = NULL;
  // Call the API to set the object_id on the property
  fpga_result result = fpgaPropertiesSetObjectID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    get_object_id02
 * @brief   Tests: fpgaPropertiesGetObjectID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          And it has the object_id field set to a known value<br>
 *          When I call fpgaPropertiesGetObjectID with a pointer to an
 *          64-bit integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST_P(properties_c_p, get_object_id02) {
  uint64_t object_id = 0x8000000000000000UL;
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

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
  result = fpgaPropertiesGetObjectID(prop, &tmp_object_id);

  // assert the result was ok
  EXPECT_EQ(FPGA_OK, result);

  // assert it is set to what we set it to above
  EXPECT_EQ(object_id, tmp_object_id);

  // now delete the properties object
  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    get_object_id03
 * @brief   Tests: fpgaPropertiesGetObjectID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          But it has no object_id field set,<br>
 *          When I call fpgaPropertiesGetObjectID with a pointer to an
 *          64-bit integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND.<br>
 */
TEST_P(properties_c_p, get_object_id03) {
  uint64_t objid = 0;
  EXPECT_EQ(fpgaPropertiesGetObjectID(filter_, &objid), FPGA_NOT_FOUND);
}

/**
 * @test    fpga_destroy_properties01
 * @brief   Tests: fpgaDestroyProperties
 * @details When the fpga_properties* object<br>
 *          to fpgaDestroyProperties is NULL<br>
 *          Then the function returns FPGA_INVALID_PARAM<br>
 *
 */
TEST(properties, fpga_destroy_properties01) {
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaDestroyProperties(NULL));
#endif
}

TEST_P(properties_c_p, get_num_errors01)
{
  fpga_properties prop = nullptr;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  EXPECT_EQ(result, FPGA_OK);
  auto _prop = (_fpga_properties*)prop;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_ERRORS);
  _prop->num_errors = 9;
  uint32_t num_errors = 0;
  // now get the parent token from the prop structure
  EXPECT_EQ(fpgaPropertiesGetNumErrors(prop, &num_errors), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&prop), FPGA_OK);
}

/**
 * @test    get_num_errors02
 * @brief   Tests: fpgaPropertiesGetNumErrors
 * @details When the number of errors field is not set<br>
 *          in the properties object,<br>
 *          Then fpgaPropertiesGetNumErrors returns FPGA_NOT_FOUND.<br>
 *
 */
TEST_P(properties_c_p, get_num_errors02) {
  uint32_t errors = 0;
  EXPECT_EQ(fpgaPropertiesGetNumErrors(filter_, &errors), FPGA_NOT_FOUND);
}

/**
 * @test    validate01
 * @brief   Tests: opae_validate_and_lock_properties
 * @details When opae_validate_and_lock_properties is called with<br>
 *          a properties object that has an invalid magic field,<br>
 *          Then opae_validate_and_lock_properties returns NULL.<br>
 *
 */
TEST_P(properties_c_p, validate01) {
  struct _fpga_properties *p = (struct _fpga_properties *) filter_;
  ASSERT_EQ(p->magic, FPGA_PROPERTY_MAGIC);
  p->magic = 0;
  EXPECT_EQ(NULL, opae_validate_and_lock_properties(filter_));
  p->magic = FPGA_PROPERTY_MAGIC;
}

INSTANTIATE_TEST_CASE_P(properties_c, properties_c_p,
                        ::testing::ValuesIn(test_platform::platforms({})));

class properties_c_mock_p : public properties_c_p{
  protected:
    properties_c_mock_p() {};
};

/**
 * @test    from_handle02
 * @brief   Tests: fpgaGetPropertiesFromHandle
 * @details When the call to opae_allocate_wrapped_token() fails<br>
 *          fpgaGetPropertiesFromHandle returns FPGA_NO_MEMORY<br>
 */
TEST_P(properties_c_mock_p, from_handle02) {
  fpga_properties props = nullptr;
  // Invalidate the allocation of the wrapped token.
  system_->invalidate_malloc(0, "opae_allocate_wrapped_token");
  EXPECT_EQ(fpgaGetPropertiesFromHandle(accel_, &props), FPGA_NO_MEMORY);
  EXPECT_EQ(fpgaDestroyProperties(&props), FPGA_OK);
}

/**
 * @test    from_token01
 * @brief   Tests: fpgaGetProperties
 * @details When the input token is NULL<br>
 *          and the call to opae_properties_create() fails,<br>
 *          fpgaGetProperties returns FPGA_NO_MEMORY<br>
 */
TEST_P(properties_c_mock_p, from_token01) {
  fpga_properties props = nullptr;
  // Invalidate the allocation of the properties object.
  system_->invalidate_calloc(0, "opae_properties_create");
  EXPECT_EQ(fpgaGetProperties(NULL, &props), FPGA_NO_MEMORY);
}

/**
 * @test    from_token02
 * @brief   Tests: fpgaGetProperties
 * @details When the input token is valid<br>
 *          and the call to opae_allocate_wrapped_token() fails,<br>
 *          fpgaGetProperties returns FPGA_NO_MEMORY<br>
 */
TEST_P(properties_c_mock_p, from_token02) {
  fpga_properties props = nullptr;
  // Invalidate the allocation of the wrapped token.
  system_->invalidate_malloc(0, "opae_allocate_wrapped_token");
  EXPECT_EQ(fpgaGetProperties(tokens_accel_[0], &props), FPGA_NO_MEMORY);
  EXPECT_EQ(fpgaDestroyProperties(&props), FPGA_OK);
}

/**
 * @test    update02
 * @brief   Tests: fpgaUpdateProperties
 * @details When the resulting properties object has a parent token set,<br>
 *          but malloc fails during wrapper allocation,<br>
 *          fpgaUpdateProperties returns FPGA_NO_MEMORY.<br>
 */
TEST_P(properties_c_mock_p, update02) {
  fpga_properties props = nullptr;
  ASSERT_EQ(fpgaGetProperties(NULL, &props), FPGA_OK);
  system_->invalidate_malloc(0, "opae_allocate_wrapped_token");
  EXPECT_EQ(fpgaUpdateProperties(tokens_accel_[0], props), FPGA_NO_MEMORY);
  EXPECT_EQ(fpgaDestroyProperties(&props), FPGA_OK);
}

/**
 * @test    fpga_clone_properties02
 * @brief   Tests: fpgaCloneProperties
 * @details When calloc fails to allocate the new properties object,<br>
 *          fpgaClonePropeties returns FPGA_EXCEPTION.<br>
 */
TEST_P(properties_c_mock_p, fpga_clone_properties02) {
  fpga_properties clone = NULL;
  system_->invalidate_calloc(0, "opae_properties_create");
  ASSERT_EQ(fpgaCloneProperties(filter_, &clone), FPGA_EXCEPTION);
}

INSTANTIATE_TEST_CASE_P(properties_c, properties_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({})));

