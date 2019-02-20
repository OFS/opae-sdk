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

#include "fpgad/api/sysfs.h"

}

#include <config.h>
#include <opae/fpga.h>

#include <fstream>
#include <array>
#include <cstdlib>
#include <cstring>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class fpgad_sysfs_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_sysfs_c_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
  }

  virtual void TearDown() override {
    system_->finalize();
  }

  test_platform platform_;
  test_system *system_;
};

/**
 * @test       write01
 * @brief      Test: file_write_string
 * @details    file_write_string writes the given string to the file.<br>
 */
TEST_P(fpgad_sysfs_c_p, write01) {
  char tmp_file[20];
  strcpy(tmp_file, "tmp-XXXXXX.fil");
  close(mkstemps(tmp_file, 4));

  EXPECT_EQ(file_write_string(tmp_file, "hello", 5), 0);

  std::ifstream f;
  f.open(tmp_file, std::ios::in);
  std::string s;
  f >> s;
  f.close();
  unlink(tmp_file);

  EXPECT_STREQ(s.c_str(), "hello");
}

/**
 * @test       write02
 * @brief      Test: file_write_string
 * @details    When given a length of zero, the fn returns non-zero.<br>
 */
TEST_P(fpgad_sysfs_c_p, write02) {
  char tmp_file[20];
  strcpy(tmp_file, "tmp-XXXXXX.fil");
  close(mkstemps(tmp_file, 4));

  EXPECT_NE(file_write_string(tmp_file, "hello", 0), 0);
  unlink(tmp_file);
}

/**
 * @test       dup01
 * @brief      Test: cstr_dup
 * @details    When malloc returns NULL, the fn fails with NULL.<br>
 */
TEST_P(fpgad_sysfs_c_p, dup01) {
  system_->invalidate_malloc(0, "cstr_dup");
  EXPECT_EQ(cstr_dup("blah"), (void *)NULL);
}

/**
 * @test       dup02
 * @brief      Test: cstr_dup
 * @details    When successful, the fn returns a duplicate<br>
 *             the given string.<br>
 */
TEST_P(fpgad_sysfs_c_p, dup02) {
  char *s;
  s = cstr_dup("blah");
  ASSERT_NE(s, (void *)NULL);
  EXPECT_STREQ(s, "blah");
  free(s);
}

INSTANTIATE_TEST_CASE_P(fpgad_c, fpgad_sysfs_c_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p"/*,"skx-p-dfl0"*/ })));
