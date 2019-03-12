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
#include "ase_common.h"
#include "common_int.h"
#include <pthread.h>
}

#include <opae/fpga.h>

#include "gtest/gtest.h"
#include "test_system.h"

/**
* @test       ase_common_01
*
* @brief      When the property's magic is invalid and libopae-ase-c is loaded:
*             prop_check_and_lock() function should return FPGA_INVALID_PARAM
*
*/
TEST(sim_sw_ase, ase_common_01) {
	struct _fpga_properties prop;

	prop.lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
	prop.magic = FPGA_PROPERTY_MAGIC;
	EXPECT_EQ(FPGA_OK, prop_check_and_lock(&prop));

	prop.magic = 0xFFFFFFFF;
	EXPECT_EQ(FPGA_INVALID_PARAM, prop_check_and_lock(&prop));


}

/**
* @test       ase_common_02
*
* @brief      When the property's magic is invalid and libopae-ase-c is loaded:
*             prop_check_and_lock() function should return FPGA_INVALID_PARAM
*
*/
TEST(sim_sw_ase, ase_common_02) {
	struct _fpga_handle handle;

	handle.magic = 0xFFFFFFFF;
	handle.lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
	EXPECT_EQ(FPGA_INVALID_PARAM, handle_check_and_lock(&handle));
}

/**
* @test       ase_common_03
*
* @brief      When the property's magic is invalid and libopae-ase-c is loaded:
*             prop_check_and_lock() function should return FPGA_INVALID_PARAM
*
*/
TEST(sim_sw_ase, ase_common_03) {
	struct _fpga_event_handle ehandle;

	ehandle.magic = 0xFFFFFFFF;
	ehandle.lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
	EXPECT_EQ(FPGA_INVALID_PARAM, event_handle_check_and_lock(&ehandle));
}

/**
 * @test       fpga_version
 *
 * @brief      When the parameter fpga_version* to fpgaGetOPAECVersion() is nullptr, 
 *             the function returns FPGA_INVALID_PARAM.
 *             If the parameter fpga_version* to fpgaGetOPAECVersion() is valid,
 *             the function returns FPGA_OK.
 *             When the parameter version_str* to fpgaGetOPAECVersionString() is nullptr, 
 *             the function returns FPGA_INVALID_PARAM.
 *             When the parameter build_str* to fpgaGetOPAECBuildString() is nullptr, 
 *             the function returns FPGA_INVALID_PARAM.
 */
TEST(open_c_ase, fpga_version) {
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaGetOPAECVersion(nullptr));
	fpga_version version;
	EXPECT_EQ(FPGA_OK, fpgaGetOPAECVersion(&version));

	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaGetOPAECVersionString(nullptr, 10));
	EXPECT_EQ(FPGA_INVALID_PARAM, fpgaGetOPAECBuildString(nullptr, 10));
}

/**
 * @test       ase_wsid_1
 *
 * @brief      When the parameter n_hash_buckets is zero or greater than 16384, 
 *             the function returns NULL. 
 */
TEST(open_c_ase, ase_wsid_1) {
	EXPECT_EQ(NULL, wsid_tracker_init(0));
	EXPECT_EQ(NULL, wsid_tracker_init(16385));

}

/**
* @test       ase_wsid_2
*
* @brief      When memory allocation function ase_malloc failed,
*             wsid_tracker_init() function should return NULL and 
*             wsid_add() returns false 
*
*/
using namespace opae::testing;
TEST(sim_sw_ase, ase_wsid_2) {
	test_system *system_;
	system_ = test_system::instance();
	system_->initialize();

    // Invalidate the memory allocation .
    system_->invalidate_malloc(0, "wsid_tracker_init");
    system_->invalidate_malloc(0, "ase_malloc");
    EXPECT_EQ(wsid_tracker_init(1000), (struct wsid_tracker*)NULL);

	system_->finalize();
}

/**
 * @test       ase_umsg_1
 *
 * @brief      When ase_capability's umsg feature is zero, 
 *             fpgaGetNumUmsg function returns FPGA_NOT_SUPPORTED. 
 *             When ase_capability's umsg feature is nonzero,
 *             fpgaGetNumUmsg function returns FPGA_OK.
 */
TEST(open_c_ase, ase_umsg_1) {
	ase_capability.umsg_feature = 2;
	struct _fpga_handle handle_;
	fpga_handle accel_ = &handle_;

	uint64_t value;

	EXPECT_EQ(FPGA_OK, fpgaGetNumUmsg(accel_, &value));

	ase_capability.umsg_feature = 0;
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaGetNumUmsg(accel_, &value));
}

/**
 * @test       ase_umsg_2
 *
 * @brief      When ase_capability's umsg feature is zero, 
 *             fpgaSetUmsgAttributes function returns FPGA_NOT_SUPPORTED. 
 *             When ase_capability's umsg feature is nonzero,
 *             fpgaSetUmsgAttributes function returns FPGA_OK.
 */
TEST(open_c_ase, ase_umsg_2) {
	struct _fpga_handle handle_;
	fpga_handle accel_ = &handle_;

	uint64_t value = 0x10;

	ase_capability.umsg_feature = 0;
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaSetUmsgAttributes(accel_, value));
}

/**
 * @test       ase_umsg_3
 *
 * @brief      When ase_capability's umsg feature is zero, 
 *             fpgaGetUmsgPtr function returns FPGA_NOT_SUPPORTED. 
 *             When ase_capability's umsg feature is nonzero,
 *             fpgaGetUmsgPtr function returns FPGA_OK.
 */
TEST(open_c_ase, ase_umsg_3) {
	ase_capability.umsg_feature = 2;

	struct _fpga_handle handle_;
	fpga_handle accel_ = &handle_;

	uint64_t umsg[10];
	uint64_t **umsg_ptr = (uint64_t **)&umsg;

	EXPECT_EQ(FPGA_NO_MEMORY, fpgaGetUmsgPtr(accel_, umsg_ptr));

	umsg_umas_vbase = (uint64_t*)0x100;
	EXPECT_EQ(FPGA_OK, fpgaGetUmsgPtr(accel_, umsg_ptr));

	ase_capability.umsg_feature = 0;
	EXPECT_EQ(FPGA_NOT_SUPPORTED, fpgaGetUmsgPtr(accel_, umsg_ptr));
}




