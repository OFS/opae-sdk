
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
#include "sysfs_int.h"

#ifdef __cplusplus
}
#endif

#include "gtest/gtest.h"


TEST(sysfs_int_h, get_interface_id)
{
  fpga_handle handle = 0;
  uint64_t * id_l = 0;
  uint64_t * id_h = 0;
  auto res = get_interface_id(handle,id_l,id_h);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_sbdf_from_path)
{
  const char * sysfspath = 0;
  int * s = 0;
  int * b = 0;
  int * d = 0;
  int * f = 0;
  auto res = sysfs_sbdf_from_path(sysfspath,s,b,d,f);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_read_int)
{
  const char * path = 0;
  int * i = 0;
  auto res = sysfs_read_int(path,i);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_read_u32)
{
  const char * path = 0;
  uint32_t * u = 0;
  auto res = sysfs_read_u32(path,u);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_read_u32_pair)
{
  const char * path = 0;
  uint32_t * u1 = 0;
  uint32_t * u2 = 0;
  char sep = 0;
  auto res = sysfs_read_u32_pair(path,u1,u2,sep);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_read_u64)
{
  const char * path = 0;
  uint64_t * u = 0;
  auto res = sysfs_read_u64(path,u);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_write_u64)
{
  const char * path = 0;
  uint64_t u = 0;
  auto res = sysfs_write_u64(path,u);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_read_guid)
{
  const char * path = 0;
  fpga_guid guid;
  auto res = sysfs_read_guid(path,guid);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_get_socket_id)
{
  int dev = 0;
  uint8_t * socket_id = 0;
  auto res = sysfs_get_socket_id(dev,socket_id);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_get_afu_id)
{
  int dev = 0;
  fpga_guid guid;
  auto res = sysfs_get_afu_id(dev,guid);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_get_pr_id)
{
  int dev = 0;
  fpga_guid guid;
  auto res = sysfs_get_pr_id(dev,guid);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_get_slots)
{
  int dev = 0;
  uint32_t * slots = 0;
  auto res = sysfs_get_slots(dev,slots);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_get_bitstream_id)
{
  int dev = 0;
  uint64_t * id = 0;
  auto res = sysfs_get_bitstream_id(dev,id);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, get_port_sysfs)
{
  fpga_handle handle = 0;
  char * sysfs_port = 0;
  auto res = get_port_sysfs(handle,sysfs_port);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, get_fpga_deviceid)
{
  fpga_handle handle = 0;
  uint64_t * deviceid = 0;
  auto res = get_fpga_deviceid(handle,deviceid);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_deviceid_from_path)
{
  const char * sysfspath = 0;
  uint64_t * deviceid = 0;
  auto res = sysfs_deviceid_from_path(sysfspath,deviceid);
  EXPECT_EQ(res, FPGA_OK);
}


TEST(sysfs_int_h, sysfs_objectid_from_path)
{
  const char * sysfspath = 0;
  uint64_t * object_id = 0;
  auto res = sysfs_objectid_from_path(sysfspath,object_id);
  EXPECT_EQ(res, FPGA_OK);
}
