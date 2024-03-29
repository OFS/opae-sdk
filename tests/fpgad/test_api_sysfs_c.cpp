// Copyright(c) 2019-2022, Intel Corporation
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

extern "C" {
#include "fpgad/api/sysfs.h"
}

#define NO_OPAE_C
#include "mock/opae_fixtures.h"

using namespace opae::testing;

class fpgad_sysfs_c_p : public opae_base_p<> {};

/**
 * @test       write01
 * @brief      Test: file_write_string
 * @details    file_write_string writes the given string to the file.<br>
 */
TEST_P(fpgad_sysfs_c_p, write01) {
  char tmp_file[20];
  strcpy(tmp_file, "tmp-XXXXXX.fil");
  opae_close(mkstemps(tmp_file, 4));

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
  opae_close(mkstemps(tmp_file, 4));

  EXPECT_NE(file_write_string(tmp_file, "hello", 0), 0);
  unlink(tmp_file);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(fpgad_sysfs_c_p);
INSTANTIATE_TEST_SUITE_P(fpgad_c, fpgad_sysfs_c_p,
                         ::testing::ValuesIn(test_platform::platforms({ "skx-p" })));
