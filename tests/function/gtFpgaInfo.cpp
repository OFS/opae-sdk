// Copyright(c) 2018, Intel Corporation
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
#include "fpgainfo.h"
#include "fmeinfo.h"
#include "portinfo.h"

#include "common_test.h"
#include "gtest/gtest.h"

using namespace common_test;

class fpgainfo : public BaseFixture, public ::testing::Test
{
      protected:
	virtual void SetUp()
	{
		m_Properties = NULL;
		EXPECT_EQ(FPGA_OK, fpgaGetProperties(NULL, &m_Properties));
		m_NumMatches = 0;
		m_Matches = NULL;
	}

	virtual void TearDown()
	{
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
 * @test       fme
 *
 * @brief      Show FME info
 */
TEST_F(fpgainfo, fme)
{
	// Should select one FME and one AFU.
	const uint8_t bus = 0x5e;
	const uint8_t device = 0x00;
	const uint8_t function = 0x00;
	const char *argv[] = {"fme"};
	const char *expected =
		"//****** FME ******//\n"
		"Object Id                : 0xF500000\n"
		"Bus                      : 0x5E\n"
		"Device                   : 0x00\n"
		"Function                 : 0x00\n"
		"Socket Id                : 0x00\n"
		"Ports Num                : 01\n"
		"Bitstream Id             : 0x63000023B637277\n"
		"Bitstream Metadata       : 0x1612156\n"
		"Pr Interface Id          : "
		"e993f64a-7d56-4b53-870c-3bcb1a3a7f02\n";


	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBus(m_Properties, bus));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetDevice(m_Properties, device));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetFunction(m_Properties, function));

	ASSERT_EQ(FPGA_OK,
		  fme_filter(&m_Properties, 1, const_cast<char **>(argv)));
	EXPECT_EQ(FPGA_OK,
		  fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(1, m_NumMatches);

	m_Matches = (fpga_token *)calloc((m_NumMatches * sizeof(fpga_token)),
					 sizeof(char));
	ASSERT_NE((void *)NULL, m_Matches);

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches,
					 m_NumMatches, &m_NumMatches));
	testing::internal::CaptureStdout();
	ASSERT_EQ(FPGA_OK, fme_command(m_Matches, m_NumMatches, 1,
				       const_cast<char **>(argv)));
	std::string output = testing::internal::GetCapturedStdout();
	(void)expected;
	// TODO: Add this last check once everything is implemented
	// ASSERT_STREQ(expected, output.c_str());
}

/**
 * @test       port
 *
 * @brief      Show FME info
 */
TEST_F(fpgainfo, port)
{
	// Should select one FME and one AFU.
	const uint8_t bus = 0x5e;
	const uint8_t device = 0x00;
	const uint8_t function = 0x00;
	const char *argv[] = {"port"};
	const char *expected =
		"//****** PORT ******//\n"
		"Object ID                : 0xF400000\n"
		"Bus                      : 0x5E\n"
		"Device                   : 0x00\n"
		"Function                 : 0x00\n"
		"Socket ID                : 0x00\n"
		"Accelerator GUID         : "
		"d8424dc4-a4a3-c413-f89e-433683f9040b\n";


	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetBus(m_Properties, bus));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetDevice(m_Properties, device));
	EXPECT_EQ(FPGA_OK, fpgaPropertiesSetFunction(m_Properties, function));

	ASSERT_EQ(FPGA_OK,
		  port_filter(&m_Properties, 1, const_cast<char **>(argv)));
	EXPECT_EQ(FPGA_OK,
		  fpgaEnumerate(&m_Properties, 1, NULL, 0, &m_NumMatches));
	EXPECT_EQ(1, m_NumMatches);

	m_Matches = (fpga_token *)calloc((m_NumMatches * sizeof(fpga_token)),
					 sizeof(char));
	ASSERT_NE((void *)NULL, m_Matches);

	EXPECT_EQ(FPGA_OK, fpgaEnumerate(&m_Properties, 1, m_Matches,
					 m_NumMatches, &m_NumMatches));
	testing::internal::CaptureStdout();
	ASSERT_EQ(FPGA_OK, port_command(m_Matches, m_NumMatches, 1,
					const_cast<char **>(argv)));
	std::string output = testing::internal::GetCapturedStdout();
	ASSERT_STREQ(expected, output.c_str());
}
