
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
#include "token_list_int.h"

#ifdef __cplusplus
}
#endif

#include "test_system.h"
#include "gtest/gtest.h"

using namespace opae::testing;

TEST(token_list_c, simple_case)
{
  const char *sysfs_fme = "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0";
  const char *dev_fme = "/dev/intel-fpga-fme.0";
  const char *sysfs_port = "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0";
  const char *dev_port = "/dev/intel-fpga-port.0";
  auto fme = token_add(sysfs_fme, dev_fme);
  ASSERT_NE(fme, nullptr);
  auto port = token_add(sysfs_port, dev_port);
  ASSERT_NE(port, nullptr);
  auto parent = token_get_parent(port);
  EXPECT_EQ(parent, fme);

  parent = token_get_parent(fme);
  EXPECT_EQ(nullptr, parent);

  token_cleanup();

  parent = token_get_parent(port);
  EXPECT_EQ(nullptr, parent);
}


TEST(token_list_c, invalid_paths)
{
  // paths missing dot
  std::string sysfs_fme = "/sys/class/fpga/intel-fpga-dev/intel-fpga-fme";
  std::string dev_fme = "/dev/intel-fpga-fme";
  std::string sysfs_port = "/sys/class/fpga/intel-fpga-dev/intel-fpga-port";
  std::string dev_port = "/dev/intel-fpga-port";
  auto fme = token_add(sysfs_fme.c_str(), dev_fme.c_str());
  EXPECT_EQ(fme, nullptr);

  // paths with dot, but non-decimal character afterwards
  sysfs_fme += ".z";
  sysfs_port += ".z";
  fme = token_add(sysfs_fme.c_str(), dev_fme.c_str());
  EXPECT_EQ(fme, nullptr);

  // get a parent of a bogus token
  auto port = new _fpga_token;
  std::copy(sysfs_port.begin(), sysfs_port.end(), &port->sysfspath[0]);
  auto parent = token_get_parent(port);
  EXPECT_EQ(parent, nullptr);

  // invalidate malloc

  sysfs_fme = "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0";
  dev_fme = "/dev/intel-fpga-fme.0";

  test_system::instance()->invalidate_malloc();
  fme = token_add(sysfs_fme.c_str(), dev_fme.c_str());
  ASSERT_EQ(fme, nullptr);



  token_cleanup();
}
