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

#include "opae/fpga.h"

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

using namespace common_test;

class LibopaecEnumFCommonMOCKHW : public BaseFixture, public ::testing::Test {
 protected:
  virtual void SetUp() {
    m_Properties = NULL;
    EXPECT_EQ(FPGA_OK, fpgaGetProperties(NULL, &m_Properties));

    m_NumMatches = 0;
    m_Matches = NULL;
  }

  virtual void TearDown() {
    EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&m_Properties));
    m_Properties = NULL;

    if (NULL != m_Matches) {
      free(m_Matches);
    }
  }

  fpga_properties m_Properties;
  uint32_t m_NumMatches;
  fpga_token* m_Matches;
};

class LibopaecEnumFCommonALL : public BaseFixture, public ::testing::Test {
protected:
  virtual void SetUp() {
    m_Properties = NULL;
    EXPECT_EQ(FPGA_OK, fpgaGetProperties(NULL, &m_Properties));
    m_NumMatches = 0;
    m_Matches = NULL;
  }

  virtual void TearDown() {
    EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&m_Properties));
    m_Properties = NULL;

    if (NULL != m_Matches) {
      free(m_Matches);
    }
  }

  fpga_properties m_Properties;
  uint32_t m_NumMatches;
  fpga_token *m_Matches;
};


/**
 * @test       01
 *
 * @brief      When the num_match parameter to fpgaEnumerate is NULL,
 *             the function returns FPGA_INVALID_PARAM.
 */
TEST(LibopaecEnumCommonALL, 01) {
  fpga_properties prop;
  fpga_token tok;

  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaEnumerate(&prop, 1, &tok, 0, NULL));
}

/**
 * @test       02
 *
 * @brief      When the max_tokens parameter to fpgaEnumerate is greater
 *             than zero, but the tokens parameter is NULL, the function
 *             returns FPGA_INVALID_PARAM.
 */
TEST(LibopaecEnumCommonALL, 02) {
  fpga_properties prop;
  uint32_t nummatch;

  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaEnumerate(&prop, 1, NULL, 1, &nummatch));
}

/**
 * @test       03
 *
 * @brief      When the num_filter parameter to fpgaEnumerate is greater
 *             than zero, but the filter parameter is NULL, the function
 *             returns FPGA_INVALID_PARAM.
 */
TEST(LibopaecEnumCommonALL, 03) {
  fpga_token tok;
  uint32_t nummatch;

  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaEnumerate(NULL, 1, &tok, 0, &nummatch));
}

/**
 * @test       23
 *
 * @brief      When the num_filter parameter to fpgaEnumerate is zero,
 *             but the filter parameter is non-NULL, the function
 *             returns FPGA_INVALID_PARAM.
 */
TEST(LibopaecEnumCommonALL, 23) {
  fpga_properties prop;
  fpga_token tok;
  uint32_t nummatch;

  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaEnumerate(&prop, 0, &tok, 0, &nummatch));
}

/**
 * @test       04
 *
 * @brief      When the num_filter parameter to fpgaEnumerate is zero,
 *             the filter parameter is NULL, and the tokens parameter is
 *             NULL, the function places the number of devices in the
 *             system into the memory pointed to by num_matches, and
 *             returns FPGA_OK.
 */
TEST(LibopaecEnumCommonALL, 04) {
  uint32_t nummatch;

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(NULL, 0, NULL, 0, &nummatch));
  // One FME and one AFU per socket.
  EXPECT_EQ(2 * GlobalOptions::Instance().NumSockets(), nummatch);
}

/**
 * @test       05
 *
 * @brief      fpgaEnumerate honors the input max_tokens value by
 *             limiting the number of output entries written to the
 *             memory at match, even though more may exist.
 */
TEST(LibopaecEnumCommonALL, 05) {
  uint32_t nummatch;
  fpga_token match[3] = {NULL, NULL, NULL};

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(NULL, 0, match, 2, &nummatch));
  EXPECT_EQ(2 * GlobalOptions::Instance().NumSockets(), nummatch);

  EXPECT_NE((void*)NULL, match[0]);
  EXPECT_EQ(FPGA_OK, fpgaDestroyToken(&match[0]));

  EXPECT_NE((void*)NULL, match[1]);
  EXPECT_EQ(FPGA_OK, fpgaDestroyToken(&match[1]));

  EXPECT_EQ((void*)NULL, match[2]);
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaDestroyToken(&match[2]));
}

/**
 * @test       06
 *
 * @brief      fpgaEnumerate honors a "don't care" properties filter by
 *             returning all available tokens.
 */
TEST(LibopaecEnumCommonALL, 06) {
  uint32_t nummatch;
  fpga_properties props;

  EXPECT_EQ(FPGA_OK, fpgaGetProperties(NULL, &props));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&props, 1, NULL, 0, &nummatch));
  EXPECT_EQ(2 * GlobalOptions::Instance().NumSockets(), nummatch);
  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&props));
}

/**
 * @test       07
 *
 * @brief      fpgaEnumerate allows filtering by bus,device,function.
 */
TEST_F(LibopaecEnumFCommonALL, 07) {
  // Should select one FME and one AFU.
  const uint8_t bus = 0x5e;
  const uint8_t device = 0x00;
  const uint8_t function = 0x00;

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBus(m_Properties, bus));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetDevice(m_Properties, device));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetFunction(m_Properties, function));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(2, m_NumMatches);

  m_Matches =
      (fpga_token*)calloc((m_NumMatches * sizeof(fpga_token)), sizeof(char));
  ASSERT_NE((void*)NULL, m_Matches);

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches,
                                   &m_NumMatches));
  EXPECT_EQ(2, m_NumMatches);

  uint32_t i;
  bool got_fme = false;
  bool got_afu = false;
  for (i = 0; i < m_NumMatches; ++i) {
// one fme and one afu, in no particular order.
#ifdef BUILD_ASE
    // one fme and one afu, in no particular order.
	if (0 == memcmp(((struct _fpga_token *)m_Matches[i])->accelerator_id,
                    FPGA_FME_ID, sizeof(fpga_guid))) {
      got_fme = true;
    } else if (0 == memcmp(((struct _fpga_token *)m_Matches[i])->accelerator_id,
                           ASE_GUID, sizeof(fpga_guid))) {
      got_afu = true;
    }
#else
    if (strstr(((struct _fpga_token*)m_Matches[i])->sysfspath, "fme")) {
      got_fme = true;
    } else if (strstr(((struct _fpga_token*)m_Matches[i])->sysfspath, "port")) {
      got_afu = true;
    }
#endif
  }

  EXPECT_TRUE(got_fme);
  EXPECT_TRUE(got_afu);
}

/**
 * @test       LibopaecEnumCommonHW08
 *
 * @brief      fpgaEnumerate allows filtering by device guid.
 */
TEST_F(LibopaecEnumFCommonALL, 08) {
  fpga_guid guid;

  // AFU
  guid[0] = 0xd8;
  guid[1] = 0x42;
  guid[2] = 0x4d;
  guid[3] = 0xc4;
  guid[4] = 0xa4;
  guid[5] = 0xa3;
  guid[6] = 0xc4;
  guid[7] = 0x13;
  guid[8] = 0xf8;
  guid[9] = 0x9e;
  guid[10] = 0x43;
  guid[11] = 0x36;
  guid[12] = 0x83;
  guid[13] = 0xf9;
  guid[14] = 0x04;
  guid[15] = 0x0b;

  EXPECT_EQ(FPGA_OK,
            fpgaPropertiesSetObjectType(m_Properties, FPGA_ACCELERATOR));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetGUID(m_Properties, guid));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  m_Matches =
      (fpga_token*)calloc((m_NumMatches * sizeof(fpga_token)), sizeof(char));
  ASSERT_NE((void*)NULL, m_Matches);

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches,
                                   &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  uint32_t i;
  for (i = 0; i < m_NumMatches; ++i) {
#ifdef BUILD_ASE
    EXPECT_EQ(0, memcmp(((struct _fpga_token *)m_Matches[i])->accelerator_id,
                        ASE_GUID, sizeof(fpga_guid)))
        << "at " << i;
#else
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->sysfspath, "port"))
        << "at " << i;
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->devpath, "port"));
#endif // BUILD_ASE
  }
}

/**
 * @test       LibopaecEnumCommonHW09
 *
 * @brief      fpgaEnumerate allows filtering by device object type
 *             (FPGA_ACCELERATOR).
 */
TEST_F(LibopaecEnumFCommonMOCKHW, 09) {
  EXPECT_EQ(FPGA_OK,
            fpgaPropertiesSetObjectType(m_Properties, FPGA_ACCELERATOR));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  m_Matches =
      (fpga_token*)calloc((m_NumMatches * sizeof(fpga_token)), sizeof(char));
  ASSERT_NE((void*)NULL, m_Matches);

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches,
                                   &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  uint32_t i;
  for (i = 0; i < m_NumMatches; ++i) {
#ifdef BUILD_ASE
    EXPECT_EQ(0, memcmp(((struct _fpga_token *)m_Matches[i])->accelerator_id,
                        ASE_GUID, sizeof(fpga_guid)))
        << "at " << i;
#else
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->sysfspath, "port"))
        << "at " << i;
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->devpath, "port"));
#endif // BUILD_ASE
  }
}

/**
 * @test       LibopaecEnumCommonHW10
 *
 * @brief      fpgaEnumerate allows filtering by device object type
 *             (FPGA_DEVICE).
 */
TEST_F(LibopaecEnumFCommonMOCKHW, 10) {
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_DEVICE));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  m_Matches =
      (fpga_token*)calloc((m_NumMatches * sizeof(fpga_token)), sizeof(char));
  ASSERT_NE((void*)NULL, m_Matches);

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches,
                                   &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  uint32_t i;
  for (i = 0; i < m_NumMatches; ++i) {
#ifdef BUILD_ASE
    EXPECT_EQ(0, memcmp(((struct _fpga_token *)m_Matches[i])->accelerator_id,
                        FPGA_FME_ID, sizeof(fpga_guid)));
#else
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->sysfspath, "fme"))
        << "at " << i;
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->devpath, "fme"));
#endif // BUILD_ASE
  }
}

/**
 * @test       LibopaecEnumCommonHW11
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             parent field set to the FME, fpgaEnumerate gives back the
 *             corresponding AFU.
 */
TEST_F(LibopaecEnumFCommonALL, 11) {
  struct _fpga_token _tok;

  token_for_fme0(&_tok);

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetParent(m_Properties, &_tok));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(1, m_NumMatches);

  m_Matches =
      (fpga_token*)calloc((m_NumMatches * sizeof(fpga_token)), sizeof(char));
  ASSERT_NE((void*)NULL, m_Matches);

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches,
                                   &m_NumMatches));
  ASSERT_EQ(1, m_NumMatches);

  EXPECT_TRUE(token_is_afu0(m_Matches[0]));
}

/**
 * @test       LibopaecEnumCommonHW12
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             parent field set, but that parent is not found to be the
 *             parent of any device, fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, 12) {
  struct _fpga_token _tok;

  token_for_afu0(&_tok);  // not a parent of anything

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetParent(m_Properties, &_tok));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
}

/**
 * @test       LibopaecEnumCommonHW13
 *
 * @brief      When the filter passed to fpgaEnumerate has a NULL parent
 *             field set, fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, 13) {
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetParent(m_Properties, NULL));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
}

/**
 * @test       LibopaecEnumCommonHW14
 *
 * @brief      fpgaEnumerate allows filtering by number of MMIO regions
 *             (FPGA_ACCELERATOR).
 */
TEST_F(LibopaecEnumFCommonALL, 14) {
  EXPECT_EQ(FPGA_OK,
            fpgaPropertiesSetObjectType(m_Properties, FPGA_ACCELERATOR));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetNumMMIO(m_Properties, 2));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  m_Matches =
      (fpga_token*)calloc((m_NumMatches * sizeof(fpga_token)), sizeof(char));
  ASSERT_NE((void*)NULL, m_Matches);

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches,
                                   &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  uint32_t i;
  for (i = 0; i < m_NumMatches; ++i) {
#ifdef BUILD_ASE
    EXPECT_EQ(0, memcmp(((struct _fpga_token *)m_Matches[0])->accelerator_id,
                        ASE_GUID, sizeof(fpga_guid)))
        << "at " << i;
#else
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->sysfspath, "port"))
        << "at " << i;
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->devpath, "port"));
#endif
  }

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetNumMMIO(m_Properties, 3));
#ifndef BUILD_ASE
  m_NumMatches = 0;
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       LibopaecEnumCommonHW15
 *
 * @brief      fpgaEnumerate allows filtering by number of interrupts
 *             (FPGA_ACCELERATOR).
 */
TEST_F(LibopaecEnumFCommonALL, 15) {
  EXPECT_EQ(FPGA_OK,
            fpgaPropertiesSetObjectType(m_Properties, FPGA_ACCELERATOR));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetNumInterrupts(m_Properties, 0));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  m_Matches =
      (fpga_token*)calloc((m_NumMatches * sizeof(fpga_token)), sizeof(char));
  ASSERT_NE((void*)NULL, m_Matches);

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches,
                                   &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  uint32_t i;
  for (i = 0; i < m_NumMatches; ++i) {
#ifdef BUILD_ASE
    EXPECT_EQ(0, memcmp(((struct _fpga_token *)m_Matches[i])->accelerator_id,
                        ASE_GUID, sizeof(fpga_guid)))
        << "at " << i;
#else
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->sysfspath, "port"))
        << "at " << i;
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->devpath, "port"));
#endif
  }

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetNumInterrupts(m_Properties, 1));

#ifndef BUILD_ASE
  m_NumMatches = 0;
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       16
 *
 * @brief      fpgaEnumerate allows filtering by accelerator state
 *             (FPGA_ACCELERATOR).
 */
TEST_F(LibopaecEnumFCommonALL, 16) {
  EXPECT_EQ(FPGA_OK,
            fpgaPropertiesSetObjectType(m_Properties, FPGA_ACCELERATOR));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetAcceleratorState(
                         m_Properties, FPGA_ACCELERATOR_UNASSIGNED));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  m_Matches =
      (fpga_token*)calloc((m_NumMatches * sizeof(fpga_token)), sizeof(char));
  ASSERT_NE((void*)NULL, m_Matches);

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches,
                                   &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  uint32_t i;
  for (i = 0; i < m_NumMatches; ++i) {
#ifdef BUILD_ASE
    EXPECT_EQ(0, memcmp(((struct _fpga_token *)m_Matches[i])->accelerator_id,
                        ASE_GUID, sizeof(fpga_guid)))
        << "at " << i;
#else
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->sysfspath, "port"))
        << "at " << i;
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->devpath, "port"));
#endif
  }

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetAcceleratorState(
                         m_Properties, FPGA_ACCELERATOR_ASSIGNED));
#ifndef BUILD_ASE
  m_NumMatches = 0;
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       LibopaecEnumCommonHW17
 *
 * @brief      fpgaEnumerate allows filtering by number of slots
 *             (FPGA_DEVICE).
 */
TEST_F(LibopaecEnumFCommonALL, 17) {
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_DEVICE));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetNumSlots(m_Properties, 1));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  m_Matches =
      (fpga_token*)calloc((m_NumMatches * sizeof(fpga_token)), sizeof(char));
  ASSERT_NE((void*)NULL, m_Matches);

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches,
                                   &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  uint32_t i;
  for (i = 0; i < m_NumMatches; ++i) {
#ifdef BUILD_ASE
    EXPECT_EQ(0, memcmp(((struct _fpga_token *)m_Matches[i])->accelerator_id,
                        FPGA_FME_ID, sizeof(fpga_guid)))
        << "at " << i;
#else
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->sysfspath, "fme"))
        << "at " << i;
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->devpath, "fme"));
#endif
  }

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetNumSlots(m_Properties, 2));
#ifndef BUILD_ASE
  m_NumMatches = 0;
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       LibopaecEnumCommonHW18
 *
 * @brief      fpgaEnumerate allows filtering by BBSID (FPGA_DEVICE).
 */
TEST_F(LibopaecEnumFCommonALL, 18) {
  const uint64_t bitstream_id = 0x6400002fc614bb9UL;

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_DEVICE));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBBSID(m_Properties, bitstream_id));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  m_Matches =
      (fpga_token*)calloc((m_NumMatches * sizeof(fpga_token)), sizeof(char));
  ASSERT_NE((void*)NULL, m_Matches);

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches,
                                   &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  uint32_t i;
  for (i = 0; i < m_NumMatches; ++i) {
#ifdef BUILD_ASE
    EXPECT_EQ(0, memcmp(((struct _fpga_token *)m_Matches[i])->accelerator_id,
                        FPGA_FME_ID, sizeof(fpga_guid)))
        << "at " << i;
#else
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->sysfspath, "fme"))
        << "at " << i;
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->devpath, "fme"));
#endif
  }

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBBSID(m_Properties, 0));
#ifndef BUILD_ASE
  m_NumMatches = 0;
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       LibopaecEnumCommonHW19
 *
 * @brief      fpgaEnumerate allows filtering by BBS Version
 *             (FPGA_DEVICE).
 */
TEST_F(LibopaecEnumFCommonALL, 19) {
  fpga_version version;

  version.major = 6;
  version.minor = 4;
  version.patch = 0;

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_DEVICE));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBBSVersion(m_Properties, version));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  m_Matches =
      (fpga_token*)calloc((m_NumMatches * sizeof(fpga_token)), sizeof(char));
  ASSERT_NE((void*)NULL, m_Matches);

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches,
                                   &m_NumMatches));
  EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

  uint32_t i;
  for (i = 0; i < m_NumMatches; ++i) {
#ifdef BUILD_ASE
    EXPECT_EQ(0, memcmp(((struct _fpga_token *)m_Matches[i])->accelerator_id,
                        FPGA_FME_ID, sizeof(fpga_guid)))
        << "at " << i;
#else
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->sysfspath, "fme"))
        << "at " << i;
    EXPECT_NE((void*)NULL,
              strstr(((struct _fpga_token*)m_Matches[i])->devpath, "fme"));
#endif
  }

  version.major = 9;
  version.minor = 9;
  version.patch = 9;

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBBSVersion(m_Properties, version));

#ifndef BUILD_ASE
  m_NumMatches = 0;
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       enum_drv_020
 *
 * @brief      Given an environment with at least one fpga resource, and
 *             fpgaEnumerate returns at least one token, when one calls
 *             fpgaCloneToken with a valid token, the result is FPGA_OK,
 *             and the cloned token is equal to the original.
 *
 */
TEST(LibopaecEnumCommonALL, enum_drv_020) {
  fpga_properties props;
  uint32_t nummatch;
  fpga_token clone;
  EXPECT_EQ(FPGA_OK, fpgaGetProperties(NULL, &props));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&props, 1, NULL, 0, &nummatch));
  EXPECT_EQ(2 * GlobalOptions::Instance().NumSockets(), nummatch);
  fpga_token tokens[nummatch];
  uint32_t maxtokens = 2 * GlobalOptions::Instance().NumSockets();
  ASSERT_EQ(FPGA_OK, fpgaEnumerate(&props, 1, tokens, maxtokens, &nummatch));
  ASSERT_EQ(FPGA_OK, fpgaCloneToken(tokens[0], &clone));

#ifdef BUILD_ASE
  EXPECT_EQ(0, memcmp(clone, tokens[0], 24));
#else
  struct _fpga_token* _orig = (struct _fpga_token*)tokens[0];
  struct _fpga_token* _clone = (struct _fpga_token*)clone;

  EXPECT_EQ(_orig->magic, FPGA_TOKEN_MAGIC);
  EXPECT_EQ(_orig->magic, _clone->magic);
  EXPECT_STREQ(_orig->sysfspath, _clone->sysfspath);
  EXPECT_STREQ(_orig->devpath, _clone->devpath);
#endif

  for (auto& t : tokens) {
    EXPECT_EQ(FPGA_OK, fpgaDestroyToken(&t));
  }

  EXPECT_EQ(FPGA_OK, fpgaDestroyToken(&clone));
  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&props));
}

/**
 * @test       enum_drv_021
 *
 * @brief      Given an environment with at least two sockets, and both
 *             are configured with the same AFU, when filtered by socket
 *             ID and AFU ID, fpgaEnumerate shall return only one AFU
 *             match.
 *
 * @details
 *             Set-up instructions: 1. Make sure the target system has 2
 *             sockets. Do not run this test on single-socket systems.
 *             2. Configure the same bitstream on the 2 ports. For
 *             example,
 *
 *             sudo ./fpgaconf -v -s 0
 *             /home/lab/workspace/rpan1/rtl/121516_skxp_630_pr_hssiE100_7277_sd00_skxnlb400m0.gbs
 *             sudo ./fpgaconf -v -s 1
 *             /home/lab/workspace/rpan1/rtl/121516_skxp_630_pr_hssiE100_7277_sd00_skxnlb400m0.gbs
 *
 */
TEST_F(LibopaecEnumFCommonALL, enum_drv_021) {
  // Should select one AFU.
  const uint8_t socket = 0x0;

  fpga_guid guid;

  // AFU
  guid[0] = 0xd8;
  guid[1] = 0x42;
  guid[2] = 0x4d;
  guid[3] = 0xc4;
  guid[4] = 0xa4;
  guid[5] = 0xa3;
  guid[6] = 0xc4;
  guid[7] = 0x13;
  guid[8] = 0xf8;
  guid[9] = 0x9e;
  guid[10] = 0x43;
  guid[11] = 0x36;
  guid[12] = 0x83;
  guid[13] = 0xf9;
  guid[14] = 0x04;
  guid[15] = 0x0b;

  EXPECT_EQ(FPGA_OK,
            fpgaPropertiesSetObjectType(m_Properties, FPGA_ACCELERATOR));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetSocketID(m_Properties, socket));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetGUID(m_Properties, guid));

  ASSERT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  ASSERT_EQ(1, m_NumMatches);

  m_Matches =
      (fpga_token*)calloc((m_NumMatches * sizeof(fpga_token)), sizeof(char));
  ASSERT_NE((void*)NULL, m_Matches);

  ASSERT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches,
                                   &m_NumMatches));
  ASSERT_EQ(1, m_NumMatches);

  bool got_afu;

#ifdef BUILD_ASE
  if (0 == memcmp(((struct _fpga_token *)m_Matches[0])->accelerator_id,
                  ASE_GUID, sizeof(fpga_guid)))
    got_afu = true;
  else
    got_afu = false;
#else
  got_afu = strstr(((struct _fpga_token *)m_Matches[0])->sysfspath, "port");
#endif
  EXPECT_TRUE(got_afu);
}

/**
 * @test       enum_drv_022
 *
 * @brief      Given I have a system with at least one FPGA And I
 *             enumerate with a filter of objtype of FPGA_DEVICE When I
 *             get properties from the resulting token And I query the
 *             GUID from the properties Then the GUID is returned and
 *             the result is FPGA_OK
 *
 */
TEST(LibopaecEnumCommonALL, enum_drv_022) {
  fpga_properties prop;
  fpga_guid guid;
  fpga_token tok;

  fpga_properties filterp = NULL;

  ASSERT_EQ(FPGA_OK, fpgaGetProperties(NULL, &filterp));
  ASSERT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(filterp, FPGA_DEVICE));
  uint32_t nm = 0;
  ASSERT_EQ(FPGA_OK, fpgaEnumerate(&filterp, 1, &tok, 1, &nm));
  ASSERT_EQ(FPGA_OK, fpgaDestroyProperties(&filterp));

  ASSERT_EQ(FPGA_OK, fpgaGetProperties(tok, &prop));

  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetGUID(prop, &guid));
  ASSERT_EQ(FPGA_OK, fpgaDestroyProperties(&prop));
  ASSERT_EQ(FPGA_OK, fpgaDestroyToken(&tok));
}

/**
 * @test       enum_033
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             segment field set, but that segment does not match any device,
 *             fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, enum_033) {
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetSegment(m_Properties, 0xc001));
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       enum_023
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             bus field set, but that bus does not match any device,
 *             fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, enum_023) {
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBus(m_Properties, 3));
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       enum_024
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             device field set, but that device does not match any FPGA device,
 *             fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, enum_024) {
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetDevice(m_Properties, 3));
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       enum_025
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             function field set, but that function does not match any FPGA device,
 *             fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, enum_025) {
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetFunction(m_Properties, 3));
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       enum_026
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             guid field set, but that guid does not match any FPGA device,
 *             fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, enum_026) {
  fpga_guid guid;

  memset(&guid, 0, sizeof(fpga_guid));

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetGUID(m_Properties, guid));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
}

/**
 * @test       enum_027
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             object ID field set, but that object ID does not match any FPGA device,
 *             fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, enum_027) {
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectID(m_Properties, 0));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
}

/**
 * @test       enum_028
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             vendor ID field set, but that vendor ID does not match any FPGA device,
 *             fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, enum_028) {
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetVendorID(m_Properties, 0));
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       enum_029
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             device ID field set, but that device ID does not match any FPGA device,
 *             fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, enum_029) {
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetDeviceID(m_Properties, 0));

  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       enum_030
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             num errors field set, but that num errors does not match any FPGA device,
 *             fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, enum_030) {
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetNumErrors(m_Properties, 999));
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       enum_031
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             BBS ID field set, but that BBS ID does not match any FPGA device,
 *             fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, enum_031) {
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_DEVICE));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBBSID(m_Properties, 0));
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       enum_032
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             BBS version field set, but that BBS version does not match any FPGA device,
 *             fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, enum_032) {
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_DEVICE));

  fpga_version v;
  v.major = 9;
  v.minor = 9;
  v.patch = 99;

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBBSVersion(m_Properties, v));
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif

  v.major = 6;
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBBSVersion(m_Properties, v));
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif

  v.minor = 4;
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBBSVersion(m_Properties, v));
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test       enum_034
 *
 * @brief      When the filter passed to fpgaEnumerate has a valid
 *             socket id field set, but that socket id does not match any device,
 *             fpgaEnumerate returns zero matches.
 */
TEST_F(LibopaecEnumFCommonALL, enum_034) {
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetSocketID(m_Properties, 0xff));
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
  EXPECT_EQ(0, m_NumMatches);
#endif
}
