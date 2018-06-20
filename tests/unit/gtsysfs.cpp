// Copyright(c) 2017, Intel Corporation
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
#include <opae/enum.h>
#include <opae/properties.h>
#include "sysfs_int.h"

#ifdef __cplusplus
}
#endif

#include "common_test.h"
#include "gtest/gtest.h"
#include "types_int.h"

#define DECLARE_GUID(var, ...) uint8_t var[16] = {__VA_ARGS__};

using namespace common_test;


/**
* @test    fpga_sysfs_01
* @brief   Tests: sysfs_deviceid_from_path
* @details sysfs_deviceid_from_path giver device id
*          Then the return device id <br>

*/
TEST(LibopaecsysfsCommonMOCKHW, fpga_sysfs_01) {

	uint64_t deviceid;
	
	//Valid path
	fpga_result result = sysfs_deviceid_from_path("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0", &deviceid);
	ASSERT_EQ(result, FPGA_OK);

	//NULL input
	result = sysfs_deviceid_from_path("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0", NULL);
	ASSERT_NE(result, FPGA_OK);

	//NULL input
	result = sysfs_deviceid_from_path(NULL, NULL);
	ASSERT_NE(result, FPGA_OK);

	//Invalid Valid path
	result = sysfs_deviceid_from_path("/sys/class/fpga/intel-fpga-dev.0/intel-fpga.0", &deviceid);
	ASSERT_NE(result, FPGA_OK);

	result = sysfs_deviceid_from_path("/sys/class/fpga/intel-fpga-dev.20/intel-fpga-fme", &deviceid);
	ASSERT_NE(result, FPGA_OK);

	result = sysfs_deviceid_from_path("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.20", &deviceid);
	ASSERT_NE(result, FPGA_OK);

	result = sysfs_deviceid_from_path("/sys/class/fpga/intel-fpga-dev/intel-fpga-fme", &deviceid);
	ASSERT_NE(result, FPGA_OK);

}