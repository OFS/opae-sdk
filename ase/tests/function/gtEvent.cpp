#include "gtest/gtest.h"
#include "fpga/fpga.h"
#include "globals.h"
#include "types_int.h"

/**
 * @test event_01
 * When the fpga_event_handle pointer to fpgaCreateEventHandle()
 * is NULL, the function returns FPGA_INVALID_PARAM.
 */
TEST(Event, event_01)
{
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaCreateEventHandle(NULL));
}

/**
 * @test event_02
 * When the fpga_event_handle pointer to fpgaDestroyEventHandle()
 * is NULL, the function returns FPGA_INVALID_PARAM.
 */
TEST(Event, event_02)
{
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaDestroyEventHandle(NULL));
}

/**
 * @test event_03
 * Tests fpgaRegisterEvent()'s ability to detect invalid arguments.
 * When the handle is NULL or otherwise invalid, FPGA_INVALID_PARAM.
 * When the handle has an invalid token, FPGA_INVALID_PARAM.
 * When the handle's token describes a device for which the given
 * event does't apply, FPGA_INVALID_PARAM.
 */
TEST(Event, event_03)
{
   fpga_event_type e = FPGA_EVENT_PORT_ERROR;
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
   token_for_afu0(&_t);
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(&_h, FPGA_EVENT_FME_ERROR, eh, 0));

   token_for_fme0(&_t);
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(&_h, FPGA_EVENT_AFC_INTERRUPT, eh, 0));
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(&_h, FPGA_EVENT_PORT_ERROR, eh, 0));

   EXPECT_EQ(FPGA_OK, fpgaDestroyEventHandle(&eh));
}

/**
 * @test event_04
 * Tests fpgaUnregisterEvent()'s ability to detect invalid arguments.
 * When the handle is NULL or otherwise invalid, FPGA_INVALID_PARAM.
 * When the handle has an invalid token, FPGA_INVALID_PARAM.
 * When the handle's token describes a device for which the given
 * event does't apply, FPGA_INVALID_PARAM.
 */
TEST(Event, event_04)
{
   fpga_event_type e = FPGA_EVENT_PORT_ERROR;

   // NULL handle.
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(NULL, e));

   // handle with bad magic.
   struct _fpga_handle _h;
   struct _fpga_token _t;

   token_for_afu0(&_t);

   memset(&_h, 0, sizeof(_h));
   _h.token = &_t;
   _h.magic = FPGA_INVALID_MAGIC;
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(&_h, e));

   // handle with bad token.
   _t.magic = FPGA_INVALID_MAGIC;
   _h.magic = FPGA_HANDLE_MAGIC;
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(&_h, e));

   // token/event mismatch.
   token_for_afu0(&_t);
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(&_h, FPGA_EVENT_FME_ERROR));

   token_for_fme0(&_t);
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(&_h, FPGA_EVENT_AFC_INTERRUPT));
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(&_h, FPGA_EVENT_PORT_ERROR));
}

////////////////////////////////////////////////////////////////////////////////

class Event_f0 : public ::testing::Test
{
protected:
   Event_f0() {}

   virtual void SetUp()
   {
      token_for_afu0(&m_AFUToken);
      EXPECT_EQ(FPGA_OK, fpgaOpen(&m_AFUToken, &m_AFUHandle, 0));

      token_for_fme0(&m_FMEToken);
      EXPECT_EQ(FPGA_OK, fpgaOpen(&m_FMEToken, &m_FMEHandle, 0));

      unsigned i;
      for (i = 0 ; i < sizeof(m_EventHandles) / sizeof(m_EventHandles[0]) ; ++i) {
         EXPECT_EQ(FPGA_OK, fpgaCreateEventHandle(&m_EventHandles[i]));
      }
   }
   virtual void TearDown()
   {
      unsigned i;

      for (i = 0 ; i < sizeof(m_EventHandles) / sizeof(m_EventHandles[0]) ; ++i) {
         EXPECT_EQ(FPGA_OK, fpgaDestroyEventHandle(&m_EventHandles[i]));
      }

      EXPECT_EQ(FPGA_OK, fpgaClose(m_FMEHandle));
      EXPECT_EQ(FPGA_OK, fpgaClose(m_AFUHandle));
   }

   struct _fpga_token m_AFUToken;
   struct _fpga_token m_FMEToken;
   fpga_handle        m_AFUHandle;
   fpga_handle        m_FMEHandle;
   fpga_event_handle  m_EventHandles[5];
};

/**
 * @test event_drv_05
 * Test registration and unregistration of events.
 */
TEST_F(Event_f0, event_drv_05)
{
   EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_AFUHandle,
			                FPGA_EVENT_AFC_INTERRUPT,
					m_EventHandles[0],
					0));

   EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_AFUHandle,
			                FPGA_EVENT_PORT_ERROR,
					m_EventHandles[1],
					0));

   EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_AFUHandle,
			                FPGA_EVENT_AP,
					m_EventHandles[2],
					0));

   EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_FMEHandle,
			                FPGA_EVENT_FME_ERROR,
					m_EventHandles[3],
					0));

   EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_FMEHandle,
			                FPGA_EVENT_AP,
					m_EventHandles[4],
					0));

   EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_FMEHandle,
			                  FPGA_EVENT_AP));

   EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_FMEHandle,
			                  FPGA_EVENT_FME_ERROR));

   EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle,
			                  FPGA_EVENT_AP));

   EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle,
			                  FPGA_EVENT_PORT_ERROR));

   EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle,
			                  FPGA_EVENT_AFC_INTERRUPT));
}


