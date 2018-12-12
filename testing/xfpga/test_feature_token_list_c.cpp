// Copyright(c) 2018-2019, Intel Corporation
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
#include "safe_string/safe_string.h"
#include "feature_token_list_int.h"

#ifdef __cplusplus
}
#endif

#include "test_system.h"
#include "gtest/gtest.h"

extern pthread_mutex_t global_lock;

using namespace opae::testing;

/**
 * @test       simple_case
 * @brief      Test: feature_token_add
 * @details    When feature_token_add is called,<br>
 *             feature_token_add returns a valid feature token.<br>
 */
TEST(feature_token_list_c, simple_case)
{
	fpga_guid feature_guid = {0xE7, 0xE3, 0xE9, 0x58, 0xF2, 0xE8, 0x73, 0x9D, 
					0xE0, 0x4C, 0x48, 0xC1, 0x58, 0x69, 0x81, 0x87};

	struct _fpga_feature_token *ftoken = 
		feature_token_add(FPGA_DMA_FEATURE, 0, feature_guid, 0x100, nullptr); 

	ASSERT_NE(ftoken, nullptr);
  
	feature_token_cleanup();
}

/**
 * @test       invalid_mutex
 * @brief      Test: feature_token_add
 * @details    When feature_token_add is called,<br>
 *             feature_token_add returns NULL.<br>
 */
TEST(feature_token_list_c, invalid_mutex)
{
	pthread_mutex_destroy(&global_lock);
	fpga_guid feature_guid = {0xE7, 0xE3, 0xE9, 0x58, 0xF2, 0xE8, 0x73, 0x9D, 
					0xE0, 0x4C, 0x48, 0xC1, 0x58, 0x69, 0x81, 0x87};
	struct _fpga_feature_token *ftoken = 
		feature_token_add(FPGA_DMA_FEATURE, 0, feature_guid, 0x100, nullptr );
	EXPECT_EQ(ftoken, nullptr);

	pthread_mutex_init(&global_lock, NULL);
 
	feature_token_cleanup();
}


