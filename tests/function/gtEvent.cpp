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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <poll.h>

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

using namespace common_test;
using namespace std;

class LibopaecEventFCommonMOCKHW : public BaseFixture, public ::testing::Test {
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
TEST(LibopaecEventCommonMOCKHW, event_01) {
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
TEST(LibopaecEventCommonMOCKHW, event_02) {
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
TEST(LibopaecEventCommonMOCKHW, event_03) {
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
TEST(LibopaecEventCommonMOCKHW, event_04) {
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
TEST_F(LibopaecEventFCommonMOCKHW, event_drv_05) {
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

/**
 * @test       event_drv_07
 *
 * @brief      Unregister events in a different order
 *             than they were registered in order to
 *             stress the list code in fpgad.
 *
 */
TEST_F(LibopaecEventFCommonMOCKHW, event_drv_07) {
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


  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle, FPGA_EVENT_POWER_THERMAL,
                                         m_EventHandles[2]));

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_FMEHandle, FPGA_EVENT_ERROR,
                                         m_EventHandles[3]));

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_FMEHandle, FPGA_EVENT_POWER_THERMAL,
                                         m_EventHandles[4]));

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle, FPGA_EVENT_INTERRUPT,
                                         m_EventHandles[0]));

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle, FPGA_EVENT_ERROR,
                                         m_EventHandles[1]));
}

class LibopaecEventFCommonMOCK : public LibopaecEventFCommonMOCKHW {};

/**
 * @test       event_drv_06
 *
 * @brief      Test that a power event can be received.
 *
 */
TEST_F(LibopaecEventFCommonMOCK, event_drv_06) {
  int fd = -1;
  uint64_t error_csr = 1UL << 50; // Ap6Event
  struct pollfd poll_fd;
  int res;
  int maxpolls = 100;

  EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_AFUHandle, FPGA_EVENT_POWER_THERMAL,
                                       m_EventHandles[0], 0));

  // get the OS-specific event object, here (Linux) a file descriptor.
  EXPECT_EQ(FPGA_OK, fpgaGetOSObjectFromEventHandle(m_EventHandles[0], &fd));

  EXPECT_GE(fd, 0);

  // Write to the mock sysfs node to generate the event.
  sysfs_write_64("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0/errors/errors", error_csr, HEX);

  poll_fd.fd      = fd;
  poll_fd.events  = POLLIN | POLLPRI;
  poll_fd.revents = 0;

  do
  {
    res = poll(&poll_fd, 1, 1000);
    ASSERT_GE(res, 0);
    --maxpolls;
    ASSERT_GT(maxpolls, 0);
  } while(res == 0);

  EXPECT_EQ(res, 1);
  EXPECT_NE(poll_fd.revents, 0);

  // Write to the mock sysfs node to clear the event.
  sysfs_write_64("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0/errors/errors", 0, DEC);

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle, FPGA_EVENT_POWER_THERMAL,
                                         m_EventHandles[0]));
}

/**
 * @test       event_drv_08
 *
 * @brief      When passed an event handle with an invalid magic
 *             fpgaDestroyEventHandle() should return FPGA_INVALID_PARAM
 *
 */
TEST_F(LibopaecEventFCommonMOCKHW, event_drv_08) {
  fpga_event_handle bad_handle;
  EXPECT_EQ(FPGA_OK, fpgaCreateEventHandle(&bad_handle));
  struct _fpga_event_handle *h = (struct _fpga_event_handle *) bad_handle;
  h->magic=0x0;

  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaDestroyEventHandle(&bad_handle));
}

/**
 * @test       event_drv_09
 *
 * @brief      When passed an event handle with an invalid fd
 *             fpgaDestroyEventHandle() should return FPGA_INVALID_PARAM
 *
 */
TEST_F(LibopaecEventFCommonMOCKHW, event_drv_09) {
  fpga_event_handle bad_handle;
  EXPECT_EQ(FPGA_OK, fpgaCreateEventHandle(&bad_handle));
  struct _fpga_event_handle *h = (struct _fpga_event_handle *) bad_handle;
  h->fd=-1;

  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaDestroyEventHandle(&bad_handle));
}

/**
 * @test       event_drv_10
 *
 * @brief      When passed an event handle with an invalid magic
 *             fpgaGetOSObjectFromEventHandle() should return FPGA_INVALID_PARAM
 *
 */
TEST_F(LibopaecEventFCommonMOCKHW, event_drv_10) {
  fpga_event_handle bad_handle;
  int fd;
  EXPECT_EQ(FPGA_OK, fpgaCreateEventHandle(&bad_handle));
  struct _fpga_event_handle *h = (struct _fpga_event_handle *) bad_handle;
  h->magic=0x0;

  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaGetOSObjectFromEventHandle(bad_handle, &fd));
}

/**
 * @test       event_drv_11
 *
 * @brief      When passed an event handle with an invalid magic
 *             fpgaRegisterEvent() should return FPGA_INVALID_PARAM
 *
 */
TEST_F(LibopaecEventFCommonMOCKHW, event_drv_11) {
  fpga_event_handle bad_handle;
  EXPECT_EQ(FPGA_OK, fpgaCreateEventHandle(&bad_handle));
  struct _fpga_event_handle *h = (struct _fpga_event_handle *) bad_handle;
  h->magic=0x0;

  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(m_AFUHandle, FPGA_EVENT_INTERRUPT, bad_handle, 0));
}

/**
 * @test       event_drv_12
 *
 * @brief      When passed an event handle with an invalid magic
 *             fpgaUnregisterEvent() should return FPGA_INVALID_PARAM
 *
 */
TEST_F(LibopaecEventFCommonMOCKHW, event_drv_12) {
  fpga_event_handle bad_handle;
  EXPECT_EQ(FPGA_OK, fpgaCreateEventHandle(&bad_handle));
  struct _fpga_event_handle *h = (struct _fpga_event_handle *) bad_handle;

  h->magic=0x0;
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(m_AFUHandle, FPGA_EVENT_INTERRUPT, bad_handle));
}


class LibopaecEventFCommonMOCKIRQ : public LibopaecEventFCommonMOCK {
 protected:
  virtual void SetUp() {
    MOCK_enable_irq(true);
    LibopaecEventFCommonMOCKHW::SetUp();
  }

  virtual void TearDown() {
    LibopaecEventFCommonMOCKHW::TearDown();
    MOCK_enable_irq(false);
  }
};

/**
 * @test       irq_event_01
 *
 * @brief      Given a driver with IRQ support<br>
 *             when fpgaRegisterEvent is called for<br>
 *             an FPGA_DEVICE and FPGA_EVENT_ERROR<br>
 *             then the call is successful and<br>
 *             we can receive interrupt events on<br>
 *             the OS-specific object from the event handle.<br>
 *
 */
TEST_F(LibopaecEventFCommonMOCKIRQ, irq_event_01) {

  ASSERT_EQ(FPGA_OK, fpgaRegisterEvent(m_FMEHandle, FPGA_EVENT_ERROR,
                                       m_EventHandles[0], 0));

  int res;
  int fd = -1;

  EXPECT_EQ(FPGA_OK, fpgaGetOSObjectFromEventHandle(m_EventHandles[0], &fd));
  EXPECT_GE(fd, 0);

  struct pollfd poll_fd;
  int maxpolls = 100;

  poll_fd.fd      = fd;
  poll_fd.events  = POLLIN | POLLPRI;
  poll_fd.revents = 0;

  do
  {
    res = poll(&poll_fd, 1, 1000);
    ASSERT_GE(res, 0);
    --maxpolls;
    ASSERT_GT(maxpolls, 0);
  } while(res == 0);

  EXPECT_EQ(res, 1);
  EXPECT_NE(poll_fd.revents, 0);

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_FMEHandle, FPGA_EVENT_ERROR,
                                         m_EventHandles[0]));
}

/**
 * @test       irq_event_02
 *
 * @brief      Given a driver with IRQ support<br>
 *             when fpgaRegisterEvent is called for<br>
 *             an FPGA_ACCELERATOR and FPGA_EVENT_ERROR<br>
 *             then the call is successful and<br>
 *             we can receive interrupt events on<br>
 *             the OS-specific object from the event handle.<br>
 *
 */
TEST_F(LibopaecEventFCommonMOCKIRQ, irq_event_02) {

  ASSERT_EQ(FPGA_OK, fpgaRegisterEvent(m_AFUHandle, FPGA_EVENT_ERROR,
                                       m_EventHandles[1], 0));

  int res;
  int fd = -1;

  EXPECT_EQ(FPGA_OK, fpgaGetOSObjectFromEventHandle(m_EventHandles[1], &fd));
  EXPECT_GE(fd, 0);

  struct pollfd poll_fd;
  int maxpolls = 100;

  poll_fd.fd      = fd;
  poll_fd.events  = POLLIN | POLLPRI;
  poll_fd.revents = 0;

  do
  {
    res = poll(&poll_fd, 1, 1000);
    ASSERT_GE(res, 0);
    --maxpolls;
    ASSERT_GT(maxpolls, 0);
  } while(res == 0);

  EXPECT_EQ(res, 1);
  EXPECT_NE(poll_fd.revents, 0);

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle, FPGA_EVENT_ERROR,
                                         m_EventHandles[1]));
}

/**
 * @test       irq_event_03
 *
 * @brief      Given a driver with IRQ support<br>
 *             when fpgaRegisterEvent is called for<br>
 *             an FPGA_ACCELERATOR and FPGA_EVENT_INTERRUPT<br>
 *             then the call is successful and<br>
 *             we can receive interrupt events on<br>
 *             the OS-specific object from the event handle.<br>
 *
 */
TEST_F(LibopaecEventFCommonMOCKIRQ, irq_event_03) {

  ASSERT_EQ(FPGA_OK, fpgaRegisterEvent(m_AFUHandle, FPGA_EVENT_INTERRUPT,
                                       m_EventHandles[2], 0));

  int res;
  int fd = -1;

  EXPECT_EQ(FPGA_OK, fpgaGetOSObjectFromEventHandle(m_EventHandles[2], &fd));
  EXPECT_GE(fd, 0);

  struct pollfd poll_fd;
  int maxpolls = 100;

  poll_fd.fd      = fd;
  poll_fd.events  = POLLIN | POLLPRI;
  poll_fd.revents = 0;

  do
  {
    res = poll(&poll_fd, 1, 1000);
    ASSERT_GE(res, 0);
    --maxpolls;
    ASSERT_GT(maxpolls, 0);
  } while(res == 0);

  EXPECT_EQ(res, 1);
  EXPECT_NE(poll_fd.revents, 0);

  EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle, FPGA_EVENT_INTERRUPT,
                                         m_EventHandles[2]));
}

/**
 * @test       irq_event_04
 *
 * @brief      Given a driver with IRQ support<br>
 *             when fpgaRegisterEvent is called with<br>
 *             an invalid handle<br>
 *             then the call fails with FPGA_INVALID_PARAM.<br>
 *             Repeat for fpgaUnregisterEvent.<br>
 *
 */
TEST_F(LibopaecEventFCommonMOCKIRQ, irq_event_04) {

  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(NULL, FPGA_EVENT_INTERRUPT,
                                       m_EventHandles[0], 0));
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(NULL, FPGA_EVENT_INTERRUPT,
                                         m_EventHandles[0]));
}

/**
 * @test       irq_event_05
 *
 * @brief      Given a driver with IRQ support<br>
 *             when fpgaRegisterEvent is called with<br>
 *             an invalid event handle<br>
 *             then the call fails with FPGA_INVALID_PARAM.<br>
 *             Repeat for fpgaUnregisterEvent.<br>
 *             Repeat for fpgaDestroyEventHandle.<br>
 *
 */
TEST_F(LibopaecEventFCommonMOCKIRQ, irq_event_05) {

  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(m_AFUHandle, FPGA_EVENT_INTERRUPT,
                                       NULL, 0));
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(m_AFUHandle, FPGA_EVENT_INTERRUPT,
                                         NULL));
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaDestroyEventHandle(NULL));
}

/**
 * @test       irq_event_06
 *
 * @brief      Given a driver with IRQ support<br>
 *             when fpgaRegisterEvent is called for<br>
 *             an FPGA_DEVICE and FPGA_EVENT_INTERRUPT<br>
 *             then the call fails with FPGA_INVALID_PARAM.<br>
 *             Repeat for fpgaUnregisterEvent.<br>
 */
TEST_F(LibopaecEventFCommonMOCKIRQ, irq_event_06) {
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(m_FMEHandle, FPGA_EVENT_INTERRUPT,
                                       m_EventHandles[0], 0));
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(m_FMEHandle, FPGA_EVENT_INTERRUPT,
                                       m_EventHandles[0]));
}

/**
 * @test       irq_event_07
 *
 * @brief      Given a driver with IRQ support<br>
 *             when fpgaRegisterEvent is called for<br>
 *             FPGA_EVENT_POWER_THERMAL<br>
 *             then the call fails with FPGA_NO_DAEMON.<br>
 *             Repeat for fpgaUnregisterEvent (FPGA_INVALID_PARAM).<br>
 *             Repeat for FPGA_DEVICE and FPGA_ACCELERATOR.<br>
 */
/* This test must be run with mock, but without fpgad.
TEST_F(LibopaecEventFCommonMOCKIRQ, irq_event_07) {
  EXPECT_EQ(FPGA_NO_DAEMON, fpgaRegisterEvent(m_FMEHandle, FPGA_EVENT_POWER_THERMAL,
                                       m_EventHandles[0], 0));
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(m_FMEHandle, FPGA_EVENT_POWER_THERMAL,
                                       m_EventHandles[0]));

  EXPECT_EQ(FPGA_NO_DAEMON, fpgaRegisterEvent(m_AFUHandle, FPGA_EVENT_POWER_THERMAL,
                                       m_EventHandles[0], 0));
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(m_AFUHandle, FPGA_EVENT_POWER_THERMAL,
                                       m_EventHandles[0]));
}
*/
