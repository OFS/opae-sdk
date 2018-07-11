// Copyright(c) 2017, Intel Corporation
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
#include "properties_int.h"

#ifdef __cplusplus
}
#endif

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

#define DECLARE_GUID(var, ...) uint8_t var[16] = {__VA_ARGS__};

using namespace common_test;

/**
 * @test    nodrv_prop_001
 * @brief   Tests: fpgaCreateProperties
 * @details Given a null fpga_properties object<br>
 *          When I call fpgaCreateProperties with the object<br>
 *          Then the return value is FPGA_OK<br>
 *          And the fpga_properties object is non-null<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_001) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_EQ(NULL, ((struct _fpga_properties*)prop)->parent);
  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(result, FPGA_OK);
}

/**
 * @test    nodrv_prop_002
 * @brief   Tests: fpgaDestroyProperties
 * @details Given a non-null fpga_properties object<br>
 *          When I call fpgaDestroyProperties with that object<br>
 *          Then the result is FPGA_OK<br>
 *          And that object is null<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_002) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(FPGA_OK, result);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    nodrv_prop_003
 * @brief   Tests: fpgaDestroyProperties
 * @details Given a null fpga_properties object<br>
 *          When I call fpgaDestroyProperties with that object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_003) {
  fpga_properties prop = NULL;

  fpga_result result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(FPGA_INVALID_PARAM, result);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    nodrv_prop_004
 * @brief   Tests: fpgaClearProperties
 * @details Given a non-null fpga_properties object<br>
 *          When I call fpgaClearProperties with the object<br>
 *          Then the result is FPGA_OK<br>
 *          And the properties object is cleared<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_004) {
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
 * @test    nodrv_prop_005
 * @brief   Tests: fpgaClearProperties
 * @details Given a null fpga_properties object<br>
 *          When I call fpgaClearProperties with the object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_005) {
  fpga_properties prop = NULL;

  fpga_result result = fpgaClearProperties(prop);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_010
 * @brief   Tests: fpgaPropertiesGetParent
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the parent field set<br>
 *          And a field in its parent object is a known value<br>
 *          When I call fpgaPropertiesGetParent with a pointer to a token
 *          object<br>
 *          Then the return value is FPGA_OK<br>
 *          And the field in the token object is set to the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_010) {
  fpga_properties prop = NULL;
  struct _fpga_token a_parent ;

  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // set the token to a known value
  token_for_afu0(&a_parent);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);
  _prop->parent = &a_parent;

  // now get the parent token from the prop structure
  fpga_token token;
  result = fpgaPropertiesGetParent(prop, &token);
  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  EXPECT_TRUE(token_is_afu0(token));
  EXPECT_EQ(FPGA_OK, fpgaDestroyToken(&token));

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    nodrv_prop_011
 * @brief   Tests: fpgaPropertiesGetParent
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the parent field set<br>
 *          When I call fpgaPropertiesGetParent with a pointer to a token
 *          object<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_011) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);

  ASSERT_EQ(result, FPGA_OK);
  ASSERT_TRUE(NULL != prop);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  // make sure the FPGA_PROPERTY_PARENT bit is zero
  EXPECT_EQ((_prop->valid_fields >> FPGA_PROPERTY_PARENT) & 1, 0);

  fpga_token token;
  result = fpgaPropertiesGetParent(_prop, &token);
  EXPECT_EQ(FPGA_NOT_FOUND, result);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    nodrv_prop_012
 * @brief   Tests: fpgaPropertiesSetParent
 * @details Given a non-null fpga_properties* object<br>
 *          And a fpga_token* object with a known value<br>
 *          When I call fpgaPropertiesSetParent with the property and the
 *          token<br>
 *          Then the parent object in the properties object is the token<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_012) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  // create a token object on the stack
  struct _fpga_token _tok;
  fpga_token token = &_tok;
  // set a field in the token to a known value
  token_for_afu0(&_tok);

  // Call the API to set the token on the property
  result = fpgaPropertiesSetParent(prop, &token);

  EXPECT_EQ(result, FPGA_OK);
  // Assert it is set to what we set it to above
  EXPECT_TRUE(token_is_afu0(token));

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    nodrv_prop_013
 * @brief   Tests: fpgaPropertiesGetParent
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetParent with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_013) {
  fpga_properties prop = NULL;

  fpga_token token;
  fpga_result result = fpgaPropertiesGetParent(prop, &token);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_014
 * @brief   Tests: fpgaPropertiesSetParent
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetParent with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_014) {
  fpga_properties prop = NULL;

  // Call the API to set the token on the property
  fpga_token token;
  fpga_result result = fpgaPropertiesSetParent(prop, &token);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * ObjectType *//
/**
 * @test    nodrv_prop_015
 * @brief   Tests: fpgaPropertiesGetObjectType
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the objtype field set<br>
 *          And its objtype field is a known value<br>
 *          When I call fpgaPropertiesGetObjectType with a pointer to an
 *          fpga_objtype<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_015) {
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
 * @test    nodrv_prop_016
 * @brief   Tests: fpgaPropertiesGetObjectType
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the objtype field set<br>
 *          When I call fpgaPropertiesGetObjectType with a pointer to an
 *          fpga_objtype<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_016) {
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
 * @test    nodrv_prop_017
 * @brief   Tests: fpgaPropertiesSetObjectType
 * @details Given a non-null fpga_properties* object<br>
 *          And a fpga_objtype object set to a known value<br>
 *          When I call fpgaPropertiesSetObjectType with the properties object
 *          and the objtype<br>
 *          Then the objtype in the properties object is the known value<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_017) {
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
 * @test    nodrv_prop_018
 * @brief   Tests: fpgaPropertiesGetObjectType
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetObjectType with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_018) {
  fpga_properties prop = NULL;

  fpga_objtype objtype;
  fpga_result result = fpgaPropertiesGetObjectType(prop, &objtype);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_019
 * @brief   Tests: fpgaPropertiesSetObjectType
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetObjectType with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_019) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_objtype objtype = FPGA_DEVICE;
  fpga_result result = fpgaPropertiesSetObjectType(prop, objtype);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * Segment field tests *//
/**
 * @test    nodrv_prop_225
 * @brief   Tests: fpgaPropertiesGetSegment
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the bus field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetSegment with a pointer to an integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_225) {
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
 * @test    nodrv_prop_226
 * @brief   Tests: fpgaPropertiesGetSegment
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the bus field set<br>
 *          When I call fpgaPropertiesGetSegment with a pointer to an integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_226) {
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
 * @test    nodrv_prop_227
 * @brief   Tests: fpgaPropertiesSetSegment
 * @details Given a non-null fpga_properties* object<br>
 *          And segment variable set to a known value<br>
 *          When I call fpgaPropertiesSetSegment with the properties object and
 *          the segment variable<br>
 *          Then the segment field in the properties object is the known value<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_227) {
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
 * @test    nodrv_prop_228
 * @brief   Tests: fpgaPropertiesGetSegment
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetSegment with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_228) {
  fpga_properties prop = NULL;

  uint16_t segment;
  fpga_result result = fpgaPropertiesGetSegment(prop, &segment);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_229
 * @brief   Tests: fpgaPropertiesSetSegment
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetSegment with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_229) {
  fpga_properties prop = NULL;

  // Call the API to set the segment on the property
  fpga_result result = fpgaPropertiesSetSegment(prop, 0xc001);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * Bus field tests *//
/**
 * @test    nodrv_prop_020
 * @brief   Tests: fpgaPropertiesGetBus
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the bus field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetBus with a pointer to an integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_020) {
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
 * @test    nodrv_prop_021
 * @brief   Tests: fpgaPropertiesGetBus
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the bus field set<br>
 *          When I call fpgaPropertiesGetBus with a pointer to an integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_021) {
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
 * @test    nodrv_prop_022
 * @brief   Tests: fpgaPropertiesSetBus
 * @details Given a non-null fpga_properties* object<br>
 *          And bus variable set to a known value<br>
 *          When I call fpgaPropertiesSetBus with the properties object and
 *          the bus variable<br>
 *          Then the bus field in the properties object is the known value<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_022) {
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
 * @test    nodrv_prop_023
 * @brief   Tests: fpgaPropertiesGetBus
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetBus with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_023) {
  fpga_properties prop = NULL;

  uint8_t bus;
  fpga_result result = fpgaPropertiesGetBus(prop, &bus);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_024
 * @brief   Tests: fpgaPropertiesSetBus
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetBus with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_024) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_result result = fpgaPropertiesSetBus(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * Device field tests *//
/**
 * @test    nodrv_prop_025
 * @brief   Tests: fpgaPropertiesGetDevice
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the device field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetDevice with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_025) {
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
 * @test    nodrv_prop_026
 * @brief   Tests: fpgaPropertiesGetDevice
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the device field set<br>
 *          When I call fpgaPropertiesGetDevice with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_026) {
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
 * @test    nodrv_prop_027
 * @brief   Tests: fpgaPropertiesSetDevice
 * @details Given a non-null fpga_properties* object<br>
 *          And device variable set to a known value<br>
 *          When I call fpgaPropertiesSetDevice with the properties object
 *          and the device variable<br>
 *          Then the device field in the properties object is the known
 *          value<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_027) {
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
 * @test    nodrv_prop_028
 * @brief   Tests: fpgaPropertiesGetDevice
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetDevice with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_028) {
  fpga_properties prop = NULL;

  uint8_t device;
  fpga_result result = fpgaPropertiesGetDevice(prop, &device);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_029
 * @brief   Tests: fpgaPropertiesSetDevice
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetDevice with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_029) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_result result = fpgaPropertiesSetDevice(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * Function field tests *//
/**
 * @test    nodrv_prop_030
 * @brief   Tests: fpgaPropertiesGetFunction
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the function field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetFunction with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_030) {
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
 * @test    nodrv_prop_031
 * @brief   Tests: fpgaPropertiesGetFunction
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the function field set<br>
 *          When I call fpgaPropertiesGetFunction with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_031) {
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
 * @test    nodrv_prop_032
 * @brief   Tests: fpgaPropertiesSetFunction
 * @details Given a non-null fpga_properties* object<br>
 *          And function variable set to a known value<br>
 *          When I call fpgaPropertiesSetFunction with the properties object
 *          and the function variable<br>
 *          Then the function field in the properties object is the known
 *          value<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_032) {
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
 * @test    nodrv_prop_033
 * @brief   Tests: fpgaPropertiesGetFunction
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetFunction with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_033) {
  fpga_properties prop = NULL;

  uint8_t function;
  fpga_result result = fpgaPropertiesGetFunction(prop, &function);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_034
 * @brief   Tests: fpgaPropertiesSetFunction
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetFunction with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_034) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_result result = fpgaPropertiesSetFunction(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

// * SocketID field tests *//
/**
 * @test    nodrv_prop_040
 * @brief   Tests: fpgaPropertiesGetSocketID
 * @details Given a non-null fpga_properties* object<br>
 *          And it has the socket_id field set<br>
 *          And it is set to a known value<br>
 *          When I call fpgaPropertiesGetSocketID with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_040) {
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
 * @test    nodrv_prop_041
 * @brief   Tests: fpgaPropertiesGetSocketID
 * @details Given a non-null fpga_properties* object<br>
 *          And it does NOT have the socket_id field set<br>
 *          When I call fpgaPropertiesGetSocketID with a pointer to an
 *          integer<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_041) {
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
 * @test    nodrv_prop_042
 * @brief   Tests: fpgaPropertiesSetSocketID
 * @details Given a non-null fpga_properties* object<br>
 *          And socket_id variable set to a known value<br>
 *          When I call fpgaPropertiesSetSocketID with the properties object
 *          and the socket_id variable<br>
 *          Then the socket_id field in the properties object is the known
 *          value<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_042) {
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
 * @test    nodrv_prop_043
 * @brief   Tests: fpgaPropertiesGetSocketID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetSocketID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_043) {
  fpga_properties prop = NULL;

  uint8_t socket_id;
  fpga_result result = fpgaPropertiesGetSocketID(prop, &socket_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_044
 * @brief   Tests: fpgaPropertiesSetSocketID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetSocketID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_044) {
  fpga_properties prop = NULL;

  // Call the API to set the objtype on the property
  fpga_result result = fpgaPropertiesSetSocketID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_203
 * @brief   Tests: fpgaPropertiesSetDeviceID
 * @details fpgaPropertiesSetDeviceID is not currently supported.
 *
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_203) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesSetDeviceID(NULL, 0));
}

/**
 * @test    nodrv_prop_204
 * @brief   Tests: fpgaPropertiesGetDeviceID
 * @details fpgaPropertiesGetDeviceID is not currently supported.
 *
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_204) {
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesGetDeviceID(NULL, NULL));
#endif
}

/** fpga.num_slots field tests **/
/**
 * @test    nodrv_prop_061
 * @brief   Tests: fpgaPropertiesGetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the num_slots field set to a known value<br>
 *          When I call fpgaPropertiesGetNumSlots with a pointer to an integer
 *          variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_061) {
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
 * @test    nodrv_prop_062
 * @brief   Tests: fpgaPropertiesGetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call fpgaPropertiesGetNumSlots with a pointer to an integer
 *          variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_062) {
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
 * @test    nodrv_prop_063
 * @brief   Tests: fpgaPropertiesGetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_DEVICE
 *          And it does NOT have the num_slots field set<br>
 *          When I call fpgaPropertiesGetNumSlots with the property object
 *          and a pointer to bool variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_063) {
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
 * @test    nodrv_prop_064
 * @brief   Tests: fpgaPropertiesSetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetNumSlots with the properties object
 *          and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the num_slots in the properties object is the known value<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_064) {
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
 * @test    nodrv_prop_065
 * @brief   Tests: fpgaPropertiesSetNumSlots
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_DEVICE<br>
 *          When I call fpgaPropertiesSetNumSlots with the properties object
 *          and a num_slots variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_065) {
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
 * @test    nodrv_prop_066
 * @brief   Tests: fpgaPropertiesGetNumSlots
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetNumSlots with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_066) {
  fpga_properties prop = NULL;

  uint32_t num_slots;
  fpga_result result = fpgaPropertiesGetNumSlots(prop, &num_slots);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_067
 * @brief   Tests: fpgaPropertiesSetNumSlots
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetNumSlots with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_067) {
  fpga_properties prop = NULL;

  // Call the API to set the num_slots on the property
  fpga_result result = fpgaPropertiesSetNumSlots(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** fpga.bbs_id field tests **/
/**
 * @test    nodrv_prop_068
 * @brief   Tests: fpgaPropertiesGetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the bbs_id field set to a known value<br>
 *          When I call fpgaPropertiesGetBBSID with a pointer to an integer
 *          variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_068) {
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
 * @test    nodrv_prop_069
 * @brief   Tests: fpgaPropertiesGetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call fpgaPropertiesGetBBSID with a pointer to an integer
 *          variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_069) {
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
 * @test    nodrv_prop_070
 * @brief   Tests: fpgaPropertiesGetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_DEVICE
 *          And it does NOT have the bbs_id field set<br>
 *          When I call fpgaPropertiesGetBBSID with the property object
 *          and a pointer to bool variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_070) {
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
 * @test    nodrv_prop_071
 * @brief   Tests: fpgaPropertiesSetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an integer variable set to a known value<br>
 *          When I call fpgaPropertiesSetBBSID with the properties object
 *          and the integer variable<br>
 *          Then the return value is FPGA_OK
 *          And the bbs_id in the properties object is the known value<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_071) {
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
 * @test    nodrv_prop_072
 * @brief   Tests: fpgaPropertiesSetBBSID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_DEVICE<br>
 *          When I call fpgaPropertiesSetBBSID with the properties object
 *          and a bbs_id variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_072) {
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
 * @test    nodrv_prop_073
 * @brief   Tests: fpgaPropertiesGetBBSID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetBBSID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_073) {
  fpga_properties prop = NULL;

  uint64_t bbs_id;
  fpga_result result = fpgaPropertiesGetBBSID(prop, &bbs_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_074
 * @brief   Tests: fpgaPropertiesSetBBSID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetBBSID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_074) {
  fpga_properties prop = NULL;

  // Call the API to set the bbs_id on the property
  fpga_result result = fpgaPropertiesSetBBSID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** fpga.bbs_version field tests **/
/**
 * @test    nodrv_prop_075
 * @brief   Tests: fpgaPropertiesGetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA
 *          And it has the bbs_version field set to a known value<br>
 *          When I call fpgaPropertiesGetBBSVersion with a pointer to an
 *          fpga_version variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_075) {
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
 * @test    nodrv_prop_076
 * @brief   Tests: fpgaPropertiesGetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA
 *          When I call fpgaPropertiesGetBBSVersion with a pointer to an
 *          fpga_version variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_076) {
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
 * @test    nodrv_prop_077
 * @brief   Tests: fpgaPropertiesGetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_DEVICE
 *          And it does NOT have the bbs_version field set<br>
 *          When I call fpgaPropertiesGetBBSVersion with the property object
 *          and a pointer to an fpga_version variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_077) {
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
 * @test    nodrv_prop_078
 * @brief   Tests: fpgaPropertiesSetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA<br>
 *          And an fpga_version variable set to a known value<br>
 *          When I call fpgaPropertiesSetBBSVersion with the properties object
 *          and the fpga_version variable<br>
 *          Then the return value is FPGA_OK
 *          And the bbs_version in the properties object is the known value<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_078) {
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
 * @test    nodrv_prop_079
 * @brief   Tests: fpgaPropertiesSetBBSVersion
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_DEVICE<br>
 *          When I call fpgaPropertiesSetBBSVersion with the properties object
 *          and a bbs_version variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_079) {
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
 * @test    nodrv_prop_080
 * @brief   Tests: fpgaPropertiesGetBBSVersion
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetBBSVersion with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_080) {
  fpga_properties prop = NULL;

  fpga_version bbs_version;
  fpga_result result = fpgaPropertiesGetBBSVersion(prop, &bbs_version);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_081
 * @brief   Tests: fpgaPropertiesSetBBSVersion
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetBBSVersion with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_081) {
  fpga_properties prop = NULL;

  // Call the API to set the bbs_version on the property
  fpga_version v = {0, 0, 0};
  fpga_result result = fpgaPropertiesSetBBSVersion(prop, v);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_082
 * @brief   Tests: fpgaClonePoperties
 * @details Given a fpga_properties object cloned with fpgaCloneProperties<br>
 *          When I call fpgaDestroyProperties with the cloned object<br>
 *          Then the result is FPGA_OK<br>
 *          And the properties object is destroyed appropriately<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_082) {
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
 * @test    nodrv_prop_207
 * @brief   Tests: fpgaPropertiesSetModel
 * @details fpgaPropertiesSetModel is not currently supported.
 *
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_207) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesSetModel(NULL, 0));
}

/**
 * @test    nodrv_prop_208
 * @brief   Tests: fpgaPropertiesGetModel
 * @details fpgaPropertiesGetModel is not currently supported.
 *
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_208) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesGetModel(NULL, NULL));
}

/**
 * @test    nodrv_prop_211
 * @brief   Tests: fpgaPropertiesSetCapabilities
 * @details fpgaPropertiesSetCapabilities is not currently supported.
 *
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_211) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesSetCapabilities(NULL, 0));
}

/**
 * @test    nodrv_prop_212
 * @brief   Tests: fpgaPropertiesGetCapabilities
 * @details fpgaPropertiesGetCapabilities is not currently supported.
 *
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_212) {
  EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesGetCapabilities(NULL, NULL));
}

/**
 * @test    drv_get_prop_w_token_1
 * @brief   Tests: fpgaGetProperties
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaGetProperties with a valid token
 *          expected result is FPGA_OK<br>
 */
TEST(LibopaecPropertiesCommonALL, drv_get_prop_w_token_1) {
  fpga_properties prop = NULL;
  struct _fpga_token _tok;
  fpga_token token = &_tok;

  fpga_result result = FPGA_OK;

  token_for_fme0(&_tok);

  result = fpgaGetProperties(token, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(FPGA_OK, result);
  ASSERT_EQ(NULL, prop);
}

/**
 * @test    drv_get_prop_w_token_2
 * @brief   Tests: fpgaGetProperties
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaGetProperties with an invalid token,
 *          expected result is FPGA_INVALID_PARAM.<br>
 */
TEST(LibopaecPropertiesCommonALL, drv_get_prop_w_token_2) {
  fpga_properties prop = NULL;
  struct _fpga_token _tok;
  fpga_token token = &_tok;

  fpga_result result = FPGA_OK;

  token_for_invalid(&_tok);

  result = fpgaGetProperties(token, &prop);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    drv_get_prop_w_token_4
 * @brief   Tests: fpgaGetProperties
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaGetProperties with a valid token (AFU)
 *          expected result is FPGA_OK<br>
 */
TEST(LibopaecPropertiesCommonALL, drv_get_prop_w_token_4) {
  fpga_properties prop = NULL;
  struct _fpga_token _tok;
  fpga_token token = &_tok;

  fpga_result result = FPGA_OK;

  token_for_afu0(&_tok);

  result = fpgaGetProperties(token, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  result = fpgaDestroyProperties(&prop);
  ASSERT_EQ(FPGA_OK, result);
  ASSERT_EQ(NULL, prop);
}

/** (afu | accelerator).guid field tests **/
/**
 * @test    nodrv_prop_131
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR
 *          And it has the guid field set to a known value<br>
 *          When I call fpgaPropertiesGetGUID with a pointer to an
 *          guid variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_131) {
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
  DECLARE_GUID(known_guid, 0xc5, 0x14, 0x92, 0x82, 0xe3, 0x4f, 0x11, 0xe6, 0x8e,
               0x3a, 0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28);

  memcpy_s(_prop->guid, sizeof(fpga_guid), known_guid, 16);

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
 * @test    nodrv_prop_132
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR
 *          And it has the guid field set to a known value<br>
 *          When I call fpgaPropertiesGetGUID with a pointer to an
 *          guid variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_132) {
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
  DECLARE_GUID(known_guid, 0xc5, 0x14, 0x92, 0x82, 0xe3, 0x4f, 0x11, 0xe6, 0x8e,
               0x3a, 0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28);

  memcpy_s(_prop->guid, sizeof(fpga_guid), known_guid, 16);

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
 * @test    nodrv_prop_133
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_DEVICE<br>
 *          And it does NOT have the guid field set<br>
 *          When I call fpgaPropertiesGetGUID with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_133) {
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
 * @test    nodrv_prop_134
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the guid field set<br>
 *          When I call fpgaPropertiesGetGUID with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_134) {
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
 * @test    nodrv_prop_135
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the guid field set<br>
 *          When I call fpgaPropertiesGetGUID with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_135) {
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
 * @test    nodrv_prop_136
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
TEST(LibopaecPropertiesCommonALL, nodrv_prop_136) {
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

  DECLARE_GUID(guid, 0xc5, 0x14, 0x92, 0x82, 0xe3, 0x4f, 0x11, 0xe6, 0x8e, 0x3a,
               0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28);
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
 * @test    nodrv_prop_137
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
TEST(LibopaecPropertiesCommonALL, nodrv_prop_137) {
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

  DECLARE_GUID(guid, 0xc5, 0x14, 0x92, 0x82, 0xe3, 0x4f, 0x11, 0xe6, 0x8e, 0x3a,
               0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28);
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
 * @test    nodrv_prop_138
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
TEST(LibopaecPropertiesCommonALL, nodrv_prop_138) {
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

  DECLARE_GUID(guid, 0xc5, 0x14, 0x92, 0x82, 0xe3, 0x4f, 0x11, 0xe6, 0x8e, 0x3a,
               0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28);
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
 * @test    nodrv_prop_139
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetGUID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_139) {
  fpga_properties prop = NULL;

  fpga_guid guid;
  fpga_result result = fpgaPropertiesGetGUID(prop, &guid);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_140
 * @brief   Tests: fpgaPropertiesGetGUID
 * @details Given a non-null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetGUID with a null guid parameter<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_140) {
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
 * @test    nodrv_prop_141
 * @brief   Tests: fpgaPropertiesSetGUID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetGUID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_141) {
  fpga_properties prop = NULL;
  fpga_guid guid;
  // Call the API to set the guid on the property
  fpga_result result = fpgaPropertiesSetGUID(prop, guid);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** accelerator.mmio_spaces field tests **/
/**
 * @test    nodrv_prop_156
 * @brief   Tests: fpgaPropertiesGetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR
 *          And it has the mmio_spaces field set to a known value<br>
 *          When I call fpgaPropertiesGetNumMMIO with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_156) {
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
 * @test    nodrv_prop_157
 * @brief   Tests: fpgaPropertiesGetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesGetNumMMIO with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_157) {
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
 * @test    nodrv_prop_158
 * @brief   Tests: fpgaPropertiesGetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the mmio_spaces field set<br>
 *          When I call fpgaPropertiesGetNumMMIO with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_158) {
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
 * @test    nodrv_prop_159
 * @brief   Tests: fpgaPropertiesSetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesSetNumMMIO with the properties
 *          object and a known value for mmio_spaces parameter<br>
 *          Then the return value is FPGA_OK<br>
 *          And the mmio_spaces in the properties object is the known
 *          value<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_159) {
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
 * @test    nodrv_prop_160
 * @brief   Tests: fpgaPropertiesSetNumMMIO
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesSetNumMMIO with the properties
 *          object<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_160) {
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
 * @test    nodrv_prop_161
 * @brief   Tests: fpgaPropertiesGetNumMMIO
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetNumMMIO with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_161) {
  fpga_properties prop = NULL;

  uint32_t mmio_spaces;
  fpga_result result = fpgaPropertiesGetNumMMIO(prop, &mmio_spaces);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_162
 * @brief   Tests: fpgaPropertiesSetNumMMIO
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetNumMMIO with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_162) {
  fpga_properties prop = NULL;
  // Call the API to set the mmio_spaces on the property
  fpga_result result = fpgaPropertiesSetNumMMIO(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** accelerator.state field tests **/
/**
 * @test    nodrv_prop_172
 * @brief   Tests: fpgaPropertiesGetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR
 *          And it has the state field set to a known value<br>
 *          When I call fpgaPropertiesGetAcceleratorState with a pointer to an
 *          fpga_accelerator_state variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_172) {
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
 * @test    nodrv_prop_173
 * @brief   Tests: fpgaPropertiesGetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesGetAcceleratorState with a pointer to an
 *          fpga_accelerator_state variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_173) {
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
 * @test    nodrv_prop_174
 * @brief   Tests: fpgaPropertiesGetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the state field set<br>
 *          When I call fpgaPropertiesGetAcceleratorState with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_174) {
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
 * @test    nodrv_prop_175
 * @brief   Tests: fpgaPropertiesSetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesSetAcceleratorState with the properties
 *          object and a known accelerator state variable<br>
 *          Then the return value is FPGA_OK
 *          And the state in the properties object is the known
 *          value<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_175) {
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
 * @test    nodrv_prop_176
 * @brief   Tests: fpgaPropertiesSetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesSetAcceleratorState with the properties
 *          object and a state variable<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_176) {
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
 * @test    nodrv_prop_177
 * @brief   Tests: fpgaPropertiesGetAcceleratorState
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetAcceleratorState with the null
 * object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_177) {
  fpga_properties prop = NULL;

  fpga_accelerator_state state;
  fpga_result result = fpgaPropertiesGetAcceleratorState(prop, &state);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_178
 * @brief   Tests: fpgaPropertiesGetAcceleratorState
 * @details Given a non-null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetAcceleratorState with a null state
 * pointer<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_178) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  ASSERT_EQ(result, FPGA_OK);
  ASSERT_FALSE(NULL == prop);

  result = fpgaPropertiesGetAcceleratorState(prop, NULL);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_179
 * @brief   Tests: fpgaPropertiesSetAcceleratorState
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetAcceleratorState with the null
 * object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_179) {
  fpga_properties prop = NULL;
  // Call the API to set the state on the property
  fpga_result result =
      fpgaPropertiesSetAcceleratorState(prop, FPGA_ACCELERATOR_UNASSIGNED);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/** accelerator.num_interrupts field tests **/
/**
 * @test    nodrv_prop_196
 * @brief   Tests: fpgaPropertiesGetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          And it has the num_interrupts field set to a known value<br>
 *          When I call fpgaPropertiesGetNumInterrupts with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_196) {
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
 * @test    nodrv_prop_197
 * @brief   Tests: fpgaPropertiesGetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesGetNumInterrupts with a pointer to an
 *          integer variable<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_197) {
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
 * @test    nodrv_prop_198
 * @brief   Tests: fpgaPropertiesGetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its type is FPGA_ACCELERATOR<br>
 *          And it does NOT have the num_interrupts field set<br>
 *          When I call fpgaPropertiesGetNumInterrupts with the property
 *          object and a pointer to an integer variable<br>
 *          Then the return value is FPGA_NOT_FOUND<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_198) {
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
 * @test    nodrv_prop_199
 * @brief   Tests: fpgaPropertiesSetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesSetNumInterrupts with the properties
 *          object and a known value for num_interrupts parameter<br>
 *          Then the return value is FPGA_OK<br>
 *          And the num_interrupts in the properties object is the known
 *          value<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_199) {
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
 * @test    nodrv_prop_200
 * @brief   Tests: fpgaPropertiesSetNumInterrupts
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is NOT of type FPGA_ACCELERATOR<br>
 *          When I call fpgaPropertiesSetNumInterrupts with the properties
 *          object<br>
 *          Then the return value is FPGA_INVALID_PARAM
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_200) {
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
 * @test    nodrv_prop_201
 * @brief   Tests: fpgaPropertiesGetNumInterrupts
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetNumInterrupts with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_201) {
  fpga_properties prop = NULL;

  uint32_t num_interrupts;
  fpga_result result = fpgaPropertiesGetNumInterrupts(prop, &num_interrupts);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_202
 * @brief   Tests: fpgaPropertiesSetNumInterrupts
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetNumInterrupts with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_202) {
  fpga_properties prop = NULL;
  // Call the API to set the num_interrupts on the property
  fpga_result result = fpgaPropertiesSetNumInterrupts(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_213
 * @brief   Tests: fpgaGetProperties
 * @details When creating a properties object<br>
 *          Then the internal magic should be set to FPGA_PROPERTY_MAGIC<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_213) {
  fpga_properties prop = NULL;
  fpga_result result = fpgaGetProperties(NULL, &prop);
  EXPECT_EQ(FPGA_OK, result);

  struct _fpga_properties* _prop = (struct _fpga_properties*)prop;

  EXPECT_EQ(FPGA_PROPERTY_MAGIC, _prop->magic);
}

/**
 * @test    nodrv_prop_214
 * @brief   Tests: fpgaGetProperties
 * @details When creating a properties object with a null properties
 * argument<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_214) {
  fpga_result result = FPGA_OK;
  ASSERT_NO_THROW(result = fpgaGetProperties(NULL, NULL));
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_215
 * @brief   Tests: fpgaPropertiesGetVendorID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetVendorID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_215) {
  fpga_properties prop = NULL;

  uint16_t vendor_id;
  fpga_result result = fpgaPropertiesGetVendorID(prop, &vendor_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_216
 * @brief   Tests: fpgaPropertiesSetVendorID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetVendorID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_216) {
  fpga_properties prop = NULL;
  // Call the API to set the vendor_id on the property
  fpga_result result = fpgaPropertiesSetVendorID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_217
 * @brief   Tests: fpgaPropertiesGetVendorID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          And it has the vendor_id field set to a known value<br>
 *          When I call fpgaPropertiesGetVendorID with a pointer to an
 *          16-bit integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_217) {
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
 * @test    nodrv_prop_218
 * @brief   Tests: fpgaPropertiesGetDeviceID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetDeviceID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_218) {
  fpga_properties prop = NULL;

  uint16_t device_id;
  fpga_result result = fpgaPropertiesGetDeviceID(prop, &device_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_219
 * @brief   Tests: fpgaPropertiesSetDeviceID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetDeviceID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_219) {
#ifndef BUILD_ASE
  fpga_properties prop = NULL;
  // Call the API to set the device_id on the property
  fpga_result result = fpgaPropertiesSetDeviceID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
#endif
}

/**
 * @test    nodrv_prop_220
 * @brief   Tests: fpgaPropertiesGetDeviceID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          And it has the device_id field set to a known value<br>
 *          When I call fpgaPropertiesGetDeviceID with a pointer to an
 *          16-bit integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_220) {
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
 * @test    nodrv_prop_221
 * @brief   Tests: fpgaPropertiesGetObjectID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesGetObjectID with the null object<br>
 *          Then the return value is FPGA_INVALID_PARAM<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_221) {
  fpga_properties prop = NULL;

  uint64_t object_id;
  fpga_result result = fpgaPropertiesGetObjectID(prop, &object_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_222
 * @brief   Tests: fpgaPropertiesSetObjectID
 * @details Given a null fpga_properties* object<br>
 *          When I call fpgaPropertiesSetObjectID with the null object<br>
 *          Then the result is FPGA_INVALID_PARAM<br>
 */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_222) {
  fpga_properties prop = NULL;
  // Call the API to set the object_id on the property
  fpga_result result = fpgaPropertiesSetObjectID(prop, 0);

  EXPECT_EQ(FPGA_INVALID_PARAM, result);
}

/**
 * @test    nodrv_prop_223
 * @brief   Tests: fpgaPropertiesGetObjectID
 * @details Given a non-null fpga_properties* object<br>
 *          And its object type is FPGA_ACCELERATOR<br>
 *          And it has the object_id field set to a known value<br>
 *          When I call fpgaPropertiesGetObjectID with a pointer to an
 *          64-bit integer variable<br>
 *          Then the return value is FPGA_OK<br>
 *          And the output value is the known value<br>
 * */
TEST(LibopaecPropertiesCommonALL, nodrv_prop_223) {
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
 * @test    prop_224
 * @brief   Tests: fpgaDestroyProperties
 * @details When the fpga_properties* object<br>
 *          to fpgaDestroyProperties is NULL<br>
 *          Then the function returns FPGA_INVALID_PARAM<br>
 *
 */
TEST(LibopaecPropertiesCommonALL, prop_224) {
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaDestroyProperties(NULL));
#endif
}
