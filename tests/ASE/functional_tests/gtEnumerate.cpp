#include <opae/fpga.h>

#include "gtest/gtest.h"
#include "globals.h"
#include "types_int.h"

/**
 * @test enum_01
 * When the num_match parameter to fpgaEnumerate is NULL,
 * the function returns FPGA_INVALID_PARAM.
 */
TEST(Enum, enum_01)
{
	fpga_properties prop;
	fpga_token      tok;

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaEnumerate(&prop, 1, &tok, 0, NULL));
}

/**
 * @test enum_02
 * When the max_tokens parameter to fpgaEnumerate is greater than zero, but the
 * tokens parameter is NULL, the function returns FPGA_INVALID_PARAM.
 */
TEST(Enum, enum_02)
{
	fpga_properties prop;
	uint32_t        nummatch;

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaEnumerate(&prop, 1, NULL, 1, &nummatch));
}

/**
 * @test enum_03
 * When the num_filter parameter to fpgaEnumerate is
 * greater than zero, but the filter parameter is NULL,
 * the function returns FPGA_INVALID_PARAM.
 */
TEST(Enum, enum_03)
{
	fpga_token tok;
	uint32_t   nummatch;

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaEnumerate(NULL, 1, &tok, 0, &nummatch));
}

/**
 * @test enum_drv_04
 * When the num_filter parameter to fpgaEnumerate is
 * zero, the filter parameter is NULL, and the tokens
 * parameter is NULL, the function places the number
 * of devices in the system into the memory pointed
 * to by num_matches, and returns FPGA_OK.
 */
TEST(Enum, enum_drv_04)
{
	uint32_t nummatch;

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(NULL, 0, NULL, 0, &nummatch));
	// One FME and one AFU per socket.
	EXPECT_EQ(2, nummatch);
}

/**
 * @test enum_drv_05
 * fpgaEnumerate honors the input max_tokens value by
 * limiting the number of output entries written to
 * the memory at match, even though more may exist.
 */
TEST(Enum, enum_drv_05)
{
	uint32_t   nummatch;
	fpga_token match[3] = { NULL, NULL, NULL };

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(NULL, 0, match, 2, &nummatch));
	EXPECT_EQ(2 * GlobalOptions::Instance().NumSockets(), nummatch);

	EXPECT_NE((void *)NULL, match[0]);
	EXPECT_NE((void *)NULL, match[1]);
	EXPECT_EQ((void *)NULL, match[2]);
}

/**
 * @test enum_drv_06
 * fpgaEnumerate honors a "don't care" properties filter
 * by returning all available tokens.
 */

TEST(Enum, enum_drv_06)
{
	uint32_t         nummatch;
	fpga_properties props;

	EXPECT_EQ(FPGA_OK, fpgaGetProperties(NULL, &props));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&props, 1, NULL, 0, &nummatch));
	EXPECT_EQ(2 * GlobalOptions::Instance().NumSockets(), nummatch);
	EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&props));
}

////////////////////////////////////////////////////////////////////////////////

class Enum_f0 : public ::testing::Test
{
	protected:
		Enum_f0() {}

		virtual void SetUp()
		{
			m_Properties = NULL;
			EXPECT_EQ(FPGA_OK, fpgaGetProperties(NULL, &m_Properties));

			m_NumMatches = 0;
			m_Matches    = NULL;
		}
		virtual void TearDown()
		{
			EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&m_Properties));
			m_Properties = NULL;

			if ( NULL != m_Matches ) {
				free(m_Matches);
			}
		}

		fpga_properties m_Properties;
		uint32_t         m_NumMatches;
		fpga_token      *m_Matches;
};

////////////////////////////////////////////////////////////////////////////////

/**
 * @test enum_drv_07
 * fpgaEnumerate allows filtering by bus,device,function.
 */

TEST_F(Enum_f0, enum_drv_07)
{
	// Should select one FME and one AFU.
	const uint8_t bus      = 0x5e;
	const uint8_t device   = 0x00;
	const uint8_t function = 0x00;
	EXPECT_EQ(FPGA_OK, fpgaGetProperties(NULL, &m_Properties));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBus(m_Properties, bus));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetDevice(m_Properties, device));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetFunction(m_Properties, function));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(2, m_NumMatches);

	m_Matches = (fpga_token *) malloc(m_NumMatches * sizeof(fpga_token));
	ASSERT_NE((void *)NULL, m_Matches);

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches, &m_NumMatches));
	EXPECT_EQ(2, m_NumMatches);

	uint32_t i;
	bool got_fme = false;
	bool got_afu = false;
	for ( i = 0 ; i < m_NumMatches ; ++i ) {
		// one fme and one afu, in no particular order.
		if (0== memcmp(((struct _fpga_token *)m_Matches[i])->accelerator_id, FPGA_FME_ID,sizeof(fpga_guid)) ) {
			got_fme = true;
		} else if (0== memcmp(((struct _fpga_token*)m_Matches[i])->accelerator_id, ASE_GUID,sizeof(fpga_guid)) ) {
			got_afu = true;
		}
	}

	EXPECT_TRUE(got_fme);
	EXPECT_TRUE(got_afu);
}

/**
 * @test enum_drv_08
 * fpgaEnumerate allows filtering by device guid.
 */
TEST_F(Enum_f0, enum_drv_08)
{
	uint32_t         nummatch = 0;
	fpga_properties props;
	fpga_token      *matches;

	fpga_guid        guid;

	// AFU 
	guid[ 0] = 0xd8;
	guid[ 1] = 0x42;
	guid[ 2] = 0x4d;
	guid[ 3] = 0xc4;
	guid[ 4] = 0xa4;
	guid[ 5] = 0xa3;
	guid[ 6] = 0xc4;
	guid[ 7] = 0x13;
	guid[ 8] = 0xf8;
	guid[ 9] = 0x9e;
	guid[10] = 0x43;
	guid[11] = 0x36;
	guid[12] = 0x83;
	guid[13] = 0xf9;
	guid[14] = 0x04;
	guid[15] = 0x0b;

	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_ACCELERATOR));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetGUID(m_Properties, guid));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(1, m_NumMatches);// 1 for ASE , for hardware depends on GBS and Num of Sockets

	m_Matches = (fpga_token *) malloc(m_NumMatches * sizeof(fpga_token));
	ASSERT_NE((void *)NULL, m_Matches);

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches, &m_NumMatches));
	EXPECT_EQ(1, m_NumMatches);

	uint32_t i;
	for ( i = 0 ; i < m_NumMatches ; ++i ) {
		EXPECT_EQ(0,memcmp(((struct _fpga_token*)m_Matches[i])->accelerator_id, ASE_GUID,sizeof(fpga_guid))) << "at " << i;
	}

}

/**
 * @test enum_drv_09
 * fpgaEnumerate allows filtering by device object type (FPGA_ACCELERATOR).
 */
TEST_F(Enum_f0, enum_drv_09)
{
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_ACCELERATOR));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(1, m_NumMatches);

	m_Matches = (fpga_token *) malloc(m_NumMatches * sizeof(fpga_token));
	ASSERT_NE((void *)NULL, m_Matches);

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches, &m_NumMatches));
	EXPECT_EQ(1, m_NumMatches);

	uint32_t i;
	for ( i = 0 ; i < m_NumMatches ; ++i ) {
		EXPECT_EQ(0, memcmp(((struct _fpga_token*)m_Matches[i])->accelerator_id,ASE_GUID, sizeof(fpga_guid))) << "at " << i;
	}
}

/**
 * @test enum_drv_10
 * fpgaEnumerate allows filtering by device object type (FPGA_DEVICE).
 */
TEST_F(Enum_f0, enum_drv_10)
{
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_DEVICE));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

	m_Matches = (fpga_token *) malloc(m_NumMatches * sizeof(fpga_token));
	ASSERT_NE((void *)NULL, m_Matches);

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches, &m_NumMatches));
	EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

	uint32_t i;
	for ( i = 0 ; i < m_NumMatches ; ++i ) {
		EXPECT_EQ(0, memcmp(((struct _fpga_token*)m_Matches[i])->accelerator_id,FPGA_FME_ID,sizeof(fpga_guid)));
	}
}

/**
 * @test enum_drv_11
 * When the filter passed to fpgaEnumerate has a valid parent field set to
 * the FME, fpgaEnumerate gives back the corresponding AFU.
 */
TEST_F(Enum_f0, enum_drv_11)
{
	struct _fpga_token _tok;

	token_for_fme0(&_tok);

	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetParent(m_Properties, &_tok));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(1, m_NumMatches);

	m_Matches = (fpga_token *) malloc(m_NumMatches * sizeof(fpga_token));
	ASSERT_NE((void *)NULL, m_Matches);

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches, &m_NumMatches));
	ASSERT_EQ(1, m_NumMatches);

	EXPECT_TRUE(token_is_afu0(m_Matches[0]));
}

/**
 * @test enum_drv_12
 * When the filter passed to fpgaEnumerate has a valid parent field set, but
 * that parent is not found to be the parent of any device, fpgaEnumerate
 * returns zero matches.
 */
TEST_F(Enum_f0, enum_drv_12)
{
	struct _fpga_token _tok;

	token_for_afu0(&_tok); // not a parent of anything

	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetParent(m_Properties, &_tok));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(0, m_NumMatches);
}

/**
 * @test enum_drv_13
 * When the filter passed to fpgaEnumerate has a NULL parent field set,
 * fpgaEnumerate returns zero matches.
 */
TEST_F(Enum_f0, enum_drv_13)
{
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetParent(m_Properties, NULL));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(0, m_NumMatches);
}

/**
 * @test enum_drv_14
 * fpgaEnumerate allows filtering by number of MMIO regions (FPGA_ACCELERATOR).
 */
TEST_F(Enum_f0, enum_drv_14)
{
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_ACCELERATOR));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetNumMMIO(m_Properties, 2));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

	m_Matches = (fpga_token *) malloc(m_NumMatches * sizeof(fpga_token));
	ASSERT_NE((void *)NULL, m_Matches);

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches, &m_NumMatches));
	EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

	uint32_t i;
	for ( i = 0 ; i < m_NumMatches ; ++i ) {
		EXPECT_EQ(0,memcmp(((struct _fpga_token *)m_Matches[0])->accelerator_id, ASE_GUID,sizeof(fpga_guid))) << "at " << i;
	}

	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetNumMMIO(m_Properties, 3));
	//-----------------------------------------PENDING
#if 0
	m_NumMatches = 0;
	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test enum_drv_15
 * fpgaEnumerate allows filtering by number of interrupts (FPGA_ACCELERATOR).
 */
TEST_F(Enum_f0, enum_drv_15)
{
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_ACCELERATOR));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetNumInterrupts(m_Properties, 0));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

	m_Matches = (fpga_token *) malloc(m_NumMatches * sizeof(fpga_token));
	ASSERT_NE((void *)NULL, m_Matches);

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches, &m_NumMatches));
	EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

	uint32_t i;
	for ( i = 0 ; i < m_NumMatches ; ++i ) {
		EXPECT_EQ(0, memcmp(((struct _fpga_token*)m_Matches[i])->accelerator_id,ASE_GUID,sizeof(fpga_guid))) << "at " << i;
	}

	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetNumInterrupts(m_Properties, 1));
	//--------------------------------------PENDING
#if 0
	m_NumMatches = 0;
	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test enum_drv_16
 * fpgaEnumerate allows filtering by accelerator state (FPGA_ACCELERATOR).
 */
TEST_F(Enum_f0, enum_drv_16)
{
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_ACCELERATOR));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetAcceleratorState(m_Properties, FPGA_ACCELERATOR_UNASSIGNED));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

	m_Matches = (fpga_token *) malloc(m_NumMatches * sizeof(fpga_token));
	ASSERT_NE((void *)NULL, m_Matches);

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches, &m_NumMatches));
	EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

	uint32_t i;
	for ( i = 0 ; i < m_NumMatches ; ++i ) {
		EXPECT_EQ(0, memcmp(((struct _fpga_token*)m_Matches[i])->accelerator_id,ASE_GUID,sizeof(fpga_guid))) << "at " << i;
	}

	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetAcceleratorState(m_Properties, FPGA_ACCELERATOR_ASSIGNED));
	// --------------------------------------------PENDING
#if 0
	m_NumMatches = 0;
	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test enum_drv_17
 * fpgaEnumerate allows filtering by number of slots (FPGA_DEVICE).
 */
TEST_F(Enum_f0, enum_drv_17)
{
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_DEVICE));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetNumSlots(m_Properties, 1));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(1, m_NumMatches);

	m_Matches = (fpga_token *) malloc(m_NumMatches * sizeof(fpga_token));
	ASSERT_NE((void *)NULL, m_Matches);

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches, &m_NumMatches));
	EXPECT_EQ(GlobalOptions::Instance().NumSockets(), m_NumMatches);

	uint32_t i;
	for ( i = 0 ; i < m_NumMatches ; ++i ) {
		EXPECT_EQ(0, memcmp(((struct _fpga_token*)m_Matches[i])->accelerator_id,FPGA_FME_ID,sizeof(fpga_guid))) << "at " << i;
	}

	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetNumSlots(m_Properties, 2));
	//-----------------------------------------------PENDING
#if 0
	m_NumMatches = 0;
	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test enum_drv_18
 * fpgaEnumerate allows filtering by BBSID (FPGA_DEVICE).
 */
TEST_F(Enum_f0, enum_drv_18)
{
	const uint64_t bitstream_id = 0x63000023b637277UL;

	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_DEVICE));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBBSID(m_Properties, bitstream_id));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(1, m_NumMatches);

	m_Matches = (fpga_token *) malloc(m_NumMatches * sizeof(fpga_token));
	ASSERT_NE((void *)NULL, m_Matches);

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches, &m_NumMatches));
	EXPECT_EQ(1, m_NumMatches);

	uint32_t i;
	for ( i = 0 ; i < m_NumMatches ; ++i ) {
		EXPECT_EQ(0, memcmp(((struct _fpga_token*)m_Matches[i])->accelerator_id,FPGA_FME_ID,sizeof(fpga_guid)))<< "at " << i;
	}
	//------------------------------------------------PENDING........
#if 0
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBBSID(m_Properties, 0));

	m_NumMatches = 0;
	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test enum_drv_19
 * fpgaEnumerate allows filtering by BBS Version (FPGA_DEVICE).
 */
TEST_F(Enum_f0, enum_drv_19)
{
	fpga_version version;

	version.major = 6;
	version.minor = 3;
	version.patch = 0;

	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_DEVICE));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBBSVersion(m_Properties, version));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(1, m_NumMatches);

	m_Matches = (fpga_token *) malloc(m_NumMatches * sizeof(fpga_token));
	ASSERT_NE((void *)NULL, m_Matches);

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches, &m_NumMatches));
	EXPECT_EQ(1, m_NumMatches);

	uint32_t i;
	for ( i = 0 ; i < m_NumMatches ; ++i ) {
		EXPECT_EQ(0, memcmp(((struct _fpga_token*)m_Matches[i])->accelerator_id,FPGA_FME_ID,sizeof(fpga_guid))) << "at " << i;
	}
#if 0
	version.major = 9;
	version.minor = 9;
	version.patch = 9;

	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaPropertiesSetBBSVersion(m_Properties, version));

	m_NumMatches = 0;
	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(0, m_NumMatches);
#endif
}

/**
 * @test enum_drv_020
 * Given an environment with at least one fpga resource<br>
 * And fpgaEnumerate returns at least one token<br>
 * When I call fpgaCloneToken with a valid token<br>
 * Then the result is FPGA_OK<br>
 * And the cloned token is equal to the original<br>
 */

TEST(Enum, enum_drv_020)
{
	fpga_properties props;
	uint32_t nummatch;
	fpga_token  clone;
	EXPECT_EQ(FPGA_OK, fpgaGetProperties(NULL, &props));

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&props, 1, NULL, 0, &nummatch));
	EXPECT_EQ(2 * GlobalOptions::Instance().NumSockets(), nummatch);
	fpga_token  tokens[nummatch];
	uint32_t maxtokens = 2;
	ASSERT_EQ(FPGA_OK, fpgaEnumerate(&props, 1, tokens, maxtokens, &nummatch));
	ASSERT_EQ(FPGA_OK, fpgaCloneToken(tokens[0], &clone));

	// if(0 == memcmp(clone->accelerator_id,token[0]->accelerator_id,sizeof(struct _fpga_token)))
	EXPECT_EQ(0, memcmp(clone,tokens[0], 24));

	// EXPECT_EQ(0, memcmp(clone,tokens[0], sizeof(struct _fpga_token)));

	for (auto & t : tokens)
	{
		EXPECT_EQ(FPGA_OK, fpgaDestroyToken(&t));
	}

	EXPECT_EQ(FPGA_OK, fpgaDestroyToken(&clone));
	EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&props));
}



/**
 * @test enum_drv_021
 * Given an environment with at least two sockets<br>
 * And both are configured with the same AFU<br>
 * When I filter by bus-number, AFU ID<br>
 * fpgaEnumerate should return only one AFU match<br>
 */
TEST_F(Enum_f0, enum_drv_021)
{
	// Should select one AFU.
	const uint8_t bus      = 0x5e;

	fpga_guid        guid;

	// AFU
	guid[ 0] = 0xd8;
	guid[ 1] = 0x42;
	guid[ 2] = 0x4d;
	guid[ 3] = 0xc4;
	guid[ 4] = 0xa4;
	guid[ 5] = 0xa3;
	guid[ 6] = 0xc4;
	guid[ 7] = 0x13;
	guid[ 8] = 0xf8;
	guid[ 9] = 0x9e;
	guid[10] = 0x43;
	guid[11] = 0x36;
	guid[12] = 0x83;
	guid[13] = 0xf9;
	guid[14] = 0x04;
	guid[15] = 0x0b;

	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBus(m_Properties, bus));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(m_Properties, FPGA_ACCELERATOR));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetGUID(m_Properties, guid));

	ASSERT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	ASSERT_EQ(1, m_NumMatches);

	m_Matches = (fpga_token *) malloc(m_NumMatches * sizeof(fpga_token));
	ASSERT_NE((void *)NULL, m_Matches);

	ASSERT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches, m_NumMatches, &m_NumMatches));
	ASSERT_EQ(1, m_NumMatches);

	bool got_afu ;
	if(0==memcmp(((struct _fpga_token *)m_Matches[0])->accelerator_id, ASE_GUID,sizeof(fpga_guid)))got_afu= true;else got_afu= false;

	EXPECT_TRUE(got_afu);
}
