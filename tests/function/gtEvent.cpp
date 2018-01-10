/*++

INTEL CONFIDENTIAL
Copyright 2016 - 2017 Intel Corporation

The source code contained or described  herein and all documents related to
the  source  code  ("Material")  are  owned  by  Intel  Corporation  or its
suppliers  or  licensors.  Title   to  the  Material   remains  with  Intel
Corporation or  its suppliers  and licensors.  The Material  contains trade
secrets  and  proprietary  and  confidential  information  of Intel  or its
suppliers and licensors.  The Material is protected  by worldwide copyright
and trade secret laws and treaty provisions. No part of the Material may be
used,   copied,   reproduced,   modified,   published,   uploaded,  posted,
transmitted,  distributed, or  disclosed in  any way  without Intel's prior
express written permission.

No license under any patent, copyright,  trade secret or other intellectual
property  right  is  granted to  or conferred  upon  you by  disclosure  or
delivery of the  Materials, either  expressly, by  implication, inducement,
estoppel or otherwise. Any license  under such intellectual property rights
must be express and approved by Intel in writing.

--*/

#include <opae/fpga.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

using namespace common_test;
using namespace std;

class LibopaecEventFCommonHW : public BaseFixture, public ::testing::Test {
 protected:
  virtual void SetUp() {
    m_AFUHandle = NULL;
    m_FMEHandle = NULL;

    token_for_afu0(&m_AFUToken);
    ASSERT_EQ(FPGA_OK, fpgaOpen(&m_AFUToken, &m_AFUHandle, 0));

    token_for_fme0(&m_FMEToken);
    ASSERT_EQ(FPGA_OK, fpgaOpen(&m_FMEToken, &m_FMEHandle, 0));

    unsigned i;
    for (i = 0; i < sizeof(m_EventHandles) / sizeof(m_EventHandles[0]); ++i) {
      EXPECT_EQ(FPGA_OK, fpgaCreateEventHandle(&m_EventHandles[i]));
    }
  }

  virtual void TearDown() {
    unsigned i;

    for (i = 0; i < sizeof(m_EventHandles) / sizeof(m_EventHandles[0]); ++i) {
      EXPECT_EQ(FPGA_OK, fpgaDestroyEventHandle(&m_EventHandles[i]));
    }

    EXPECT_EQ(FPGA_OK, fpgaClose(m_FMEHandle));
    EXPECT_EQ(FPGA_OK, fpgaClose(m_AFUHandle));
  }

  struct _fpga_token m_AFUToken;
  struct _fpga_token m_FMEToken;
  fpga_handle m_AFUHandle;
  fpga_handle m_FMEHandle;
  fpga_event_handle m_EventHandles[5];
};

/**
 * @test       event_01
 *
 * @brief      When the fpga_event_handle pointer to
 *             fpgaCreateEventHandle() is NULL, the function returns
 *             FPGA_INVALID_PARAM.
 *
 */
TEST(LibopaecEventCommonHW, event_01) {
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaCreateEventHandle(NULL));
}

/**
 * @test       event_02
 *
 * @brief      When the fpga_event_handle pointer to
 *             fpgaDestroyEventHandle() is NULL, the function returns
 *             FPGA_INVALID_PARAM.
 *
 */
TEST(LibopaecEventCommonHW, event_02) {
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaDestroyEventHandle(NULL));
}

/**
 * @test       event_03
 *
 * @brief      Tests fpgaRegisterEvent()'s ability to detect invalid
 *             arguments. When the handle is NULL or otherwise invalid,
 *             FPGA_INVALID_PARAM. When the handle has an invalid token,
 *             FPGA_INVALID_PARAM. When the handle's token describes a
 *             device for which the given event does't apply,
 *             FPGA_INVALID_PARAM.
 *
 */
TEST(LibopaecEventCommonHW, event_03) {
  fpga_event_type e = FPGA_EVENT_ERROR;
  fpga_event_handle eh;

  EXPECT_EQ(FPGA_OK, fpgaCreateEventHandle(&eh));

  // NULL handle.
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(NULL, e, eh, 0));

  // handle with bad magic.
  struct _fpga_handle _h;
  struct _fpga_token _t;

  token_for_afu0(&_t);

  memset(&_h, 0, sizeof(_h));
  _h.token = &_t;
  _h.magic = FPGA_INVALID_MAGIC;
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(&_h, e, eh, 0));

  // handle with bad token.
  _t.magic = FPGA_INVALID_MAGIC;
  _h.magic = FPGA_HANDLE_MAGIC;
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(&_h, e, eh, 0));

  // token/event mismatch.
  token_for_fme0(&_t);
  EXPECT_EQ(FPGA_INVALID_PARAM,
            fpgaRegisterEvent(&_h, FPGA_EVENT_INTERRUPT, eh, 0));

  EXPECT_EQ(FPGA_OK, fpgaDestroyEventHandle(&eh));
}

/**
 * @test       event_04
 *
 * @brief      Tests fpgaUnregisterEvent()'s ability to detect invalid
 *             arguments. When the handle is NULL or otherwise invalid,
 *             FPGA_INVALID_PARAM. When the handle has an invalid token,
 *             FPGA_INVALID_PARAM. When the handle's token describes a
 *             device for which the given event does't apply,
 *             FPGA_INVALID_PARAM.
 *
 */
TEST(LibopaecEventCommonHW, event_04) {
  fpga_event_type e = FPGA_EVENT_ERROR;
  fpga_event_handle eh;

  EXPECT_EQ(FPGA_OK, fpgaCreateEventHandle(&eh));

  // NULL handle.
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(NULL, e, eh));

  // handle with bad magic.
  struct _fpga_handle _h;
  struct _fpga_token _t;

  token_for_afu0(&_t);

  memset(&_h, 0, sizeof(_h));
  _h.token = &_t;
  _h.magic = FPGA_INVALID_MAGIC;
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(&_h, e, eh));

  // handle with bad token.
  _t.magic = FPGA_INVALID_MAGIC;
  _h.magic = FPGA_HANDLE_MAGIC;
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(&_h, e, eh));

  // token/event mismatch.
  token_for_fme0(&_t);
  EXPECT_EQ(FPGA_INVALID_PARAM,
            fpgaUnregisterEvent(&_h, FPGA_EVENT_INTERRUPT, eh));
}

/**
 * @test       event_drv_05
 *
 * @brief      Test registration and unregistration of events.
 *
 */
TEST_F(LibopaecEventFCommonHW, event_drv_05) {
  EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_AFUHandle, FPGA_EVENT_INTERRUPT,
                                       m_EventHandles[0], 0));

  EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_AFUHandle, FPGA_EVENT_ERROR,
                                       m_EventHandles[1], 0));

  EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_AFUHandle, FPGA_EVENT_POWER_THERMAL,
                                       m_EventHandles[2], 0));

  EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_FMEHandle, FPGA_EVENT_ERROR,
                                       m_EventHandles[3], 0));

  EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_FMEHandle, FPGA_EVENT_POWER_THERMAL,
                                       m_EventHandles[4], 0));

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_FMEHandle, FPGA_EVENT_POWER_THERMAL,
                                         m_EventHandles[4]));

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_FMEHandle, FPGA_EVENT_ERROR,
                                         m_EventHandles[3]));

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle, FPGA_EVENT_POWER_THERMAL,
                                         m_EventHandles[2]));

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle, FPGA_EVENT_ERROR,
                                         m_EventHandles[1]));

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle, FPGA_EVENT_INTERRUPT,
                                         m_EventHandles[0]));
}
