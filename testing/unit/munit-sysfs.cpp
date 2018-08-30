
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

#ifdef __cplusplus

extern "C" {
#endif

#include <opae/enum.h>
#include <opae/properties.h>
#include "sysfs_int.h"
#include <opae/fpga.h>
#include <uuid/uuid.h>
#include "types_int.h"

#ifdef __cplusplus
}
#endif


#include <array>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class sysfs_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  sysfs_c_p() : tmpsysfs("mocksys-XXXXXX") {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    tmpsysfs = system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaGetProperties(nullptr, &filter), FPGA_OK);
    num_matches = 0xc01a;
    invalid_device_ = test_device::unknown();

  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
    if (!tmpsysfs.empty() && tmpsysfs.size() > 1) {
      std::string cmd = "rm -rf " + tmpsysfs;
      std::system(cmd.c_str());
    }
    system_->finalize();
  }

  std::string tmpsysfs;
  fpga_properties filter;
  std::array<fpga_token, 2> tokens;
  uint32_t num_matches;
  test_platform platform_;
  test_device invalid_device_;
  test_system *system_;
};

/**
* @test    null_device_id
* @brief   Tests: sysfs_c_p
* @details sysfs_deviceid_from_path giver invalid device id
*          Then return FPGA_INVALID_PARAM/FPGA_NOT_FOUND
*/
TEST_P(sysfs_c_p, null_device_id){
  auto device = platform_.devices[0];
  //ASSERT_EQ(sysfs_deviceid_from_path(tmpsysfs.c_str(),device.device_id),FPGA_OK);
  ASSERT_NE(sysfs_deviceid_from_path(tmpsysfs.c_str(),NULL), FPGA_OK);
  ASSERT_NE(sysfs_deviceid_from_path(NULL,NULL), FPGA_OK);
}



/**
* @test    null_path
* @brief   Tests: sysfs_c_p
* @details sysfs_read_int,sysfs_read_u32
*          sysfs_read_u32_pair,sysfs_read_u64
*          sysfs_read_u64
*/
TEST_P(sysfs_c_p, null_path){
  fpga_result res;
  uint32_t u1;
  uint32_t u2;
  uint64_t u64;

  EXPECT_NE(res = sysfs_read_int("",NULL), FPGA_OK) << "\tRESULT: [" << fpgaErrStr(res) << "]";
  EXPECT_NE(sysfs_read_int(NULL,NULL), FPGA_OK);
  EXPECT_NE(sysfs_read_u32("",NULL), FPGA_OK);
  EXPECT_NE(sysfs_read_u32(NULL,NULL), FPGA_OK);

  res = sysfs_read_u32_pair(NULL,NULL,NULL,'\0');
  EXPECT_NE(res, FPGA_OK);
  res = sysfs_read_u32_pair(NULL,NULL,NULL,'s');
  EXPECT_NE(res, FPGA_OK);

  EXPECT_NE(sysfs_read_u64("",NULL), FPGA_OK);
  EXPECT_NE(sysfs_read_u64(NULL,NULL), FPGA_OK);


}


INSTANTIATE_TEST_CASE_P(sysfs_c, sysfs_c_p, ::testing::ValuesIn(test_platform::keys(true)));


//TEST(sysfs_int_h, get_interface_id)
//{
//  fpga_handle handle = 0;
//  uint64_t * id_l = 0;
//  uint64_t * id_h = 0;
//  auto res = get_interface_id(handle,id_l,id_h);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_sbdf_from_path)
//{
//  const char * sysfspath = 0;
//  int * s = 0;
//  int * b = 0;
//  int * d = 0;
//  int * f = 0;
//  auto res = sysfs_sbdf_from_path(sysfspath,s,b,d,f);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_read_int)
//{
//  const char * path = 0;
//  int * i = 0;
//  auto res = sysfs_read_int(path,i);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_read_u32)
//{
//  const char * path = 0;
//  uint32_t * u = 0;
//  auto res = sysfs_read_u32(path,u);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_read_u32_pair)
//{
//  const char * path = 0;
//  uint32_t * u1 = 0;
//  uint32_t * u2 = 0;
//  char sep = 0;
//  auto res = sysfs_read_u32_pair(path,u1,u2,sep);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_read_u64)
//{
//  const char * path = 0;
//  uint64_t * u = 0;
//  auto res = sysfs_read_u64(path,u);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_write_u64)
//{
//  const char * path = 0;
//  uint64_t u = 0;
//  auto res = sysfs_write_u64(path,u);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_read_guid)
//{
//  const char * path = 0;
//  fpga_guid guid;
//  auto res = sysfs_read_guid(path,guid);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_get_socket_id)
//{
//  int dev = 0;
//  uint8_t * socket_id = 0;
//  auto res = sysfs_get_socket_id(dev,socket_id);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_get_afu_id)
//{
//  int dev = 0;
//  fpga_guid guid;
//  auto res = sysfs_get_afu_id(dev,guid);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_get_pr_id)
//{
//  int dev = 0;
//  fpga_guid guid;
//  auto res = sysfs_get_pr_id(dev,guid);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_get_slots)
//{
//  int dev = 0;
//  uint32_t * slots = 0;
//  auto res = sysfs_get_slots(dev,slots);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_get_bitstream_id)
//{
//  int dev = 0;
//  uint64_t * id = 0;
//  auto res = sysfs_get_bitstream_id(dev,id);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, get_port_sysfs)
//{
//  fpga_handle handle = 0;
//  char * sysfs_port = 0;
//  auto res = get_port_sysfs(handle,sysfs_port);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, get_fpga_deviceid)
//{
//  fpga_handle handle = 0;
//  uint64_t * deviceid = 0;
//  auto res = get_fpga_deviceid(handle,deviceid);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_deviceid_from_path)
//{
//  const char * sysfspath = 0;
//  uint64_t * deviceid = 0;
//  auto res = sysfs_deviceid_from_path(sysfspath,deviceid);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
//
//TEST(sysfs_int_h, sysfs_objectid_from_path)
//{
//  const char * sysfspath = 0;
//  uint64_t * object_id = 0;
//  auto res = sysfs_objectid_from_path(sysfspath,object_id);
//  EXPECT_EQ(res, FPGA_OK);
//}
