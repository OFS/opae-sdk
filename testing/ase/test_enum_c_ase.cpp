// Copyright(c) 2019, Intel Corporation
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

extern "C" {
#include <json-c/json.h>
#include <uuid/uuid.h>
#include <opae/types.h>
#include "types_int.h"
#include "properties_int.h"
#include "common_int.h"
}

#include <opae/fpga.h>

#include <cstdlib>
#include <memory>
#include "gtest/gtest.h"

// ASE ID
#define ASE_TOKEN_MAGIC    0x46504741544f4b40

static const fpga_guid ASE_GUID = {
    0xd8, 0x42, 0x4d, 0xc4, 0xa4,  0xa3, 0xc4, 0x13, 0xf8,0x9e,
    0x43, 0x36, 0x83, 0xf9, 0x04, 0x0b
};

inline void token_for_fme(struct _fpga_token* tok_)
{
    std::copy(&FPGA_FME_GUID[0], &FPGA_FME_GUID[16], tok_->accelerator_id);
    tok_->magic = ASE_TOKEN_MAGIC;
    tok_->ase_objtype = FPGA_DEVICE;
}

inline void token_for_afu0(struct _fpga_token* tok_)
{
    std::copy(&ASE_GUID[0], &ASE_GUID[16], tok_->accelerator_id);
    tok_->magic = ASE_TOKEN_MAGIC;
    tok_->ase_objtype = FPGA_ACCELERATOR;
}

fpga_guid known_guid = {0xc5, 0x14, 0x92, 0x82, 0xe3, 0x4f, 0x11, 0xe6,
                        0x8e, 0x3a, 0x13, 0xcc, 0x9d, 0x38, 0xca, 0x28};

class enum_c_ase_p : public testing::Test {
  protected:
    enum_c_ase_p() : tok(nullptr) {}

    virtual void SetUp() override {
        token_for_afu0(&tok_);
        tok = &tok_;

        filter_ = nullptr;
        ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    }

    virtual void TearDown() override {
        if (filter_ != nullptr) {
            EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
        }
    }

    fpga_properties filter_;
    struct _fpga_token tok_;
    fpga_token tok;
};

/**
 * @test       nullfilter
 *
 * @brief      When the fpga_properties * parameter to fpgaEnumerate is nullptr, the
 *             function returns FPGA_INVALID_PARAM.
 */
TEST_F(enum_c_ase_p, nullfilter) {
  uint32_t matches = 0;
  EXPECT_EQ(fpgaEnumerate(nullptr, 1, &tok, 1, &matches),
            FPGA_INVALID_PARAM);
}

/**
 * @test       nullmatches
 *
 * @brief      When the uint32_t *num_matches parameter to fpgaEnumerate is nullptr, the
 *             function returns FPGA_INVALID_PARAM.
 */
TEST_F(enum_c_ase_p, nullmatches) {
  uint32_t matches = 0;
  EXPECT_EQ(fpgaEnumerate(&filter_, 1, &tok, 1, NULL),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(
      fpgaEnumerate(&filter_, 0, &tok, 1, &matches),
      FPGA_INVALID_PARAM);
}

/**
 * @test       nulltokens
 *
 * @brief      When the fpga_token* parameter to fpgaEnumerate is nullptr, the
 *             function returns FPGA_INVALID_PARAM.
 *
 */
TEST_F(enum_c_ase_p, nulltokens) {
  uint32_t matches = 0;
  EXPECT_EQ(fpgaEnumerate(&filter_, 0, NULL, 1, &matches),
            FPGA_INVALID_PARAM);
}

/**
 * @test       api_guid_to_fpga
 *
 * @brief      Test internal function api_guid_to_fpga in enum.c,
 *             api_guid_to_fpga returns the expected guid.
 *
 */
TEST(enum_c_ase, api_guid_to_fpga) {
    fpga_guid guid;
    uint8_t *guid_;
    uint64_t guidh = 0x4041424344454647;
    uint64_t guidl = 0x3031323334353637;;
    api_guid_to_fpga(guidh, guidl, guid);
    guid_ = (uint8_t*)guid;
    for (int i=0; i<8; i++)
        EXPECT_EQ(guid_[i], (0x40 + i));
    for (int i=0; i<8; i++)
        EXPECT_EQ(guid_[i+8], (0x30 + i));
}

/**
 * @test       matches_filters_1
 *
 * @brief      Test internal function matches_filters(), the function returns
 *             true.
 */
TEST_F(enum_c_ase_p, matches_filters_1) {
  uint64_t j = 0;
  struct _fpga_properties* _prop = (struct _fpga_properties*)filter_;

  // Set the property valid field
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_FUNCTION);
  _prop->function = 0xAE;

  EXPECT_EQ(matches_filters(&filter_, 1, &tok,  &j), true);  
}

/**
 * @test       matches_filters_2
 *
 * @brief      Test internal function matches_filters(), the function returns
 *             true.
 */
TEST_F(enum_c_ase_p, matches_filters_2) {
  uint64_t j = 0;
  struct _fpga_properties* _prop = (struct _fpga_properties*)filter_;

  SET_FIELD_VALID(_prop, FPGA_PROPERTY_DEVICE);
  _prop->device = 0xAE;
  
  EXPECT_EQ(matches_filters(&filter_, 1, &tok,  &j), true);  
}

/**
 * @test       matches_filters_3
 *
 * @brief      Test internal function matches_filters(), the function returns
 *             true.
 */
TEST_F(enum_c_ase_p, matches_filters_3) {
  uint64_t j = 0;
  struct _fpga_properties* _prop = (struct _fpga_properties*)filter_;

  _prop->valid_fields = 0;
  EXPECT_EQ(matches_filters(&filter_, 1, &tok,  &j), true);
}

/**
 * @test       matches_filters_4
 *
 * @brief      Test internal function matches_filters(), the function returns
 *             false.
 */
TEST_F(enum_c_ase_p, matches_filters_4) {
  uint64_t j = 0;
  struct _fpga_properties* _prop = (struct _fpga_properties*)filter_;

  SET_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);
  _prop->parent = NULL;
  
  EXPECT_EQ(matches_filters(&filter_, 1, &tok,  &j), false);  
}

/**
 * @test       matches_filters_5
 *
 * @brief      Test internal function matches_filters(), the function returns
 *             false.
 */
TEST_F(enum_c_ase_p, matches_filters_5) {
  uint64_t j = 0;
  struct _fpga_token tok1_;
  fpga_token tok_dev_;
  tok_dev_ = &tok1_;
  token_for_afu0(&tok1_);

  struct _fpga_properties* _prop = (struct _fpga_properties*)filter_;

  SET_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);
  _prop->parent = tok_dev_;
  
  EXPECT_EQ(matches_filters(&filter_, 1, &tok,  &j), false);  
}

/**
 * @test       matches_filters_6
 *
 * @brief      Test internal function matches_filters(), the function returns
 *             false.
 */
TEST_F(enum_c_ase_p, matches_filters_6) {
  uint64_t j = 0;
  struct _fpga_token tok1_;
  fpga_token tok_dev_;

  tok_dev_ = &tok1_;
  token_for_fme(&tok1_);

  struct _fpga_properties* _prop = (struct _fpga_properties*)filter_;

  SET_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);
  _prop->parent = tok_dev_;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJECTID);
  _prop->object_id = ASE_OBJID + 1;
  
  EXPECT_EQ(matches_filters(&filter_, 1, &tok,  &j), false);
}

/**
 * @test       matches_filters_7
 *
 * @brief      Test internal function matches_filters(), the function returns
 *             false.
 */
TEST_F(enum_c_ase_p, matches_filters_7) {
  uint64_t j = 0;
  struct _fpga_token tok1_;
  fpga_token tok_dev_;

  tok_dev_ = &tok1_;
  token_for_fme(&tok1_);

  struct _fpga_properties* _prop = (struct _fpga_properties*)filter_;

  SET_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);
  _prop->parent = tok_dev_;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJECTID);
  _prop->object_id = ASE_OBJID;

  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  _prop->objtype = FPGA_DEVICE;
  
  EXPECT_EQ(matches_filters(&filter_, 1, &tok,  &j), false);
}

/**
 * @test       matches_filters_8
 *
 * @brief      Test internal function matches_filters(), the function returns
 *             false.
 */
TEST_F(enum_c_ase_p, matches_filters_8) {
  uint64_t j = 0;
  struct _fpga_token tok1_;
  fpga_token tok_dev_;

  tok_dev_ = &tok1_;
  token_for_fme(&tok1_);

  struct _fpga_properties* _prop = (struct _fpga_properties*)filter_;

  SET_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);
  _prop->parent = tok_dev_;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJECTID);
  _prop->object_id = ASE_OBJID;

  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_GUID);
  _prop->objtype = FPGA_ACCELERATOR;
  
 // set the guid to a known value
  std::copy(&known_guid[0], &known_guid[16], _prop->guid);
  
  EXPECT_EQ(matches_filters(&filter_, 1, &tok,  &j), false);
}

/**
 * @test       matches_filters_9
 *
 * @brief      Test internal function matches_filters(), the function returns
 *             true.
 */
TEST_F(enum_c_ase_p, matches_filters_9) {
  uint64_t j = 0;
  struct _fpga_token tok1_;
  fpga_token tok_dev_;

  token_for_fme(&tok1_);
  tok_dev_ = &tok1_;
  
  struct _fpga_properties* _prop = (struct _fpga_properties*)filter_;

  SET_FIELD_VALID(_prop, FPGA_PROPERTY_PARENT);
  _prop->parent = tok_dev_;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJECTID);
  _prop->object_id = ASE_OBJID;

  SET_FIELD_VALID(_prop, FPGA_PROPERTY_OBJTYPE);
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_GUID);
  _prop->objtype = FPGA_ACCELERATOR;

  std::copy(&ASE_GUID[0], &ASE_GUID[16], _prop->guid);
   
  EXPECT_EQ(matches_filters(&filter_, 1, &tok,  &j), true);
}

/**
 * @test       matches_filters_10
 *
 * @brief      Test internal function matches_filters(), the function returns
 *             true if the filter is nullptr.
 */
TEST_F(enum_c_ase_p, matches_filters_10) {
  uint64_t j = 0;
  fpga_properties * filter = nullptr;

  EXPECT_EQ(matches_filters(filter, 1, &tok,  &j), true);  
}

/**
 * @test       matches_filters_11
 *
 * @brief      Test internal function matches_filters(), the function returns
 *             true if the num_filter is zero.
 */
TEST_F(enum_c_ase_p, matches_filters_11) {
  uint64_t j = 0; 
  EXPECT_EQ(matches_filters(&filter_, 0, &tok,  &j), true);  
}

/**
 * @test       destroy_token_1
 *
 * @brief      Test fpgaDestroyToken, if the token is nullptr
 *             the function returns FPGA_INVALID_PARAM.
 */
TEST(enum_c_ase, destroy_token_1) {
  EXPECT_EQ(fpgaDestroyToken(nullptr), FPGA_INVALID_PARAM);
  fpga_token token = nullptr;
  EXPECT_EQ(fpgaDestroyToken(&token), FPGA_INVALID_PARAM);
}
