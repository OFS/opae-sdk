#include <opae/fpga.h>

#include "gtest/gtest.h"
#include "globals.h"
#include "types_int.h"

#if 0
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
   token_for_afu0(&_t);
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(&_h, FPGA_EVENT_ERROR, eh, 0));

   token_for_fme0(&_t);
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(&_h, FPGA_EVENT_INTERRUPT, eh, 0));
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaRegisterEvent(&_h, FPGA_EVENT_ERROR, eh, 0));

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
   fpga_event_type e = FPGA_EVENT_ERROR;

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
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(&_h, FPGA_EVENT_ERROR));

   token_for_fme0(&_t);
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(&_h, FPGA_EVENT_INTERRUPT));
   EXPECT_EQ(FPGA_INVALID_PARAM, fpgaUnregisterEvent(&_h, FPGA_EVENT_ERROR));
}
#endif
////////////////////////////////////////////////////////////////////////////////

class Event_f0 : public ::testing::Test
{
protected:
   Event_f0() {}

   virtual void SetUp()
   {
      m_AFUHandle = NULL;
      m_FMEHandle = NULL;

      token_for_afu0(&m_AFUToken);
      ASSERT_EQ(FPGA_OK, fpgaOpen(&m_AFUToken, &m_AFUHandle, 0));

      token_for_fme0(&m_FMEToken);
      ASSERT_EQ(FPGA_OK, fpgaOpen(&m_FMEToken, &m_FMEHandle, 0));

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
			                FPGA_EVENT_INTERRUPT,
					m_EventHandles[0],
					0));

   EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_AFUHandle,
			                FPGA_EVENT_ERROR,
					m_EventHandles[1],
					0));

   EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_AFUHandle,
			                FPGA_EVENT_POWER_THERMAL,
					m_EventHandles[2],
					0));

   EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_FMEHandle,
			                FPGA_EVENT_ERROR,
					m_EventHandles[3],
					0));

   EXPECT_EQ(FPGA_OK, fpgaRegisterEvent(m_FMEHandle,
			                FPGA_EVENT_POWER_THERMAL,
					m_EventHandles[4],
					0));

   EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_FMEHandle,
			                  FPGA_EVENT_POWER_THERMAL));

   EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_FMEHandle,
			                  FPGA_EVENT_ERROR));

   EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle,
			                  FPGA_EVENT_POWER_THERMAL));

   EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle,
			                  FPGA_EVENT_ERROR));

   EXPECT_EQ(FPGA_OK, fpgaUnregisterEvent(m_AFUHandle,
			                  FPGA_EVENT_INTERRUPT));
}


/**
 * @test event_ap6_mock_06
 * Tests proper handling of AP6 events
 * Requires fpgad to be running with proper NULL bitstream specified
 * Will trigger an AP6 event on the first FPGA's FME via the mocked up driver
 * The mock up driver will write a checksum of the FPGA bitstream data to file
 * last_gbs_chksum of the mocked up driver
 * This test checks for the expected value in that file
 * NOTE: This test needs to be updated if the NULL bitstream changes
 */
/* hash for 121516_skxp_630_pr_7277_sd00_skxnullafu.gbs */
#if 0
#define NULL_HASH 0x14702383
#define AP6_PORT_ERROR_MASK "0x4000000000000"
#define NO_PORT_ERROR_MASK "0"
#define ERROR_FILENAME "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-port.0/errors/errors"
#define HASH_FILENAME "/tmp/intel-fpga-fme.0.gbshash"
TEST(Event, event_ap6_mock_06)
{
	FILE *hashfile;
	uint32_t hash;

	/* "clear" error */
	ASSERT_EQ(0, system("echo " NO_PORT_ERROR_MASK " > " ERROR_FILENAME));

	/* wait a second */
	EXPECT_EQ(0, sleep(1));

	/* remove hash file */
	if (unlink(HASH_FILENAME) != 0) {
		perror("unlink");
	}

	/* trigger AP6 */
	ASSERT_EQ(0, system("echo " AP6_PORT_ERROR_MASK " > " ERROR_FILENAME));

	/* wait a second */
	EXPECT_EQ(0, sleep(1));

	/* read hash file */
	hashfile = fopen(HASH_FILENAME, "r");
	if (!hashfile)
		perror("fopen");
	ASSERT_TRUE(hashfile != NULL);
	EXPECT_EQ(fread(&hash, sizeof(hash), 1, hashfile), 1);
	fclose(hashfile);

	/* compare hash */
	EXPECT_EQ(hash, NULL_HASH);
}
#endif
