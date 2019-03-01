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

#include "bits_utils.h"

#include <config.h>
#include <opae/fpga.h>

#include <fstream>
#include <vector>

#include "gtest/gtest.h"
#include "test_system.h"
#include "safe_string/safe_string.h"

using namespace opae::testing;

class bits_utils_c_p : public ::testing::TestWithParam<std::string> {
 protected:

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    j_root_ = nullptr;
  }

  virtual void TearDown() override {

    if (j_root_)
      json_object_put(j_root_);

    system_->finalize();
  }

  json_object *parse(const char *json_str)
  {
    enum json_tokener_error j_err = json_tokener_success;
    return j_root_ = json_tokener_parse_verbose(json_str, &j_err);
  }

  json_object *j_root_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       string_err0
 * @brief      Test: opae_bits_get_json_string
 * @details    If the given name doesn't exist,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(bits_utils_c_p, string_err0) {
    const char *mdata =
    R"mdata({
"a": "foo"
})mdata";
  json_object *root;
  char *value = nullptr;

  root = parse(mdata);
  ASSERT_NE(root, nullptr);

  EXPECT_EQ(opae_bits_get_json_string(root,
                                      "b",
                                      &value),
            FPGA_EXCEPTION);
  EXPECT_EQ(value, nullptr);
}

/**
 * @test       string_err1
 * @brief      Test: opae_bits_get_json_string
 * @details    If the given name exists,<br>
 *             but isn't a string,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(bits_utils_c_p, string_err1) {
    const char *mdata =
    R"mdata({
"a": 3
})mdata";
  json_object *root;
  char *value = nullptr;

  root = parse(mdata);
  ASSERT_NE(root, nullptr);

  EXPECT_EQ(opae_bits_get_json_string(root,
                                      "a",
                                      &value),
            FPGA_EXCEPTION);
  EXPECT_EQ(value, nullptr);
}

/**
 * @test       int_err0
 * @brief      Test: opae_bits_get_json_int
 * @details    If the given name doesn't exist,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(bits_utils_c_p, int_err0) {
    const char *mdata =
    R"mdata({
"a": 3
})mdata";
  json_object *root;
  int value = 0;

  root = parse(mdata);
  ASSERT_NE(root, nullptr);

  EXPECT_EQ(opae_bits_get_json_int(root,
                                   "b",
                                   &value),
            FPGA_EXCEPTION);
  EXPECT_EQ(value, 0);
}

/**
 * @test       int_err1
 * @brief      Test: opae_bits_get_json_int
 * @details    If the given name exists,<br>
 *             but isn't of type integer,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(bits_utils_c_p, int_err1) {
    const char *mdata =
    R"mdata({
"a": "str"
})mdata";
  json_object *root;
  int value = 0;

  root = parse(mdata);
  ASSERT_NE(root, nullptr);

  EXPECT_EQ(opae_bits_get_json_int(root,
                                   "a",
                                   &value),
            FPGA_EXCEPTION);
  EXPECT_EQ(value, 0);
}

INSTANTIATE_TEST_CASE_P(bits_utils_c, bits_utils_c_p,
    ::testing::ValuesIn(test_platform::platforms({})));


class mock_bits_utils_c_p : public bits_utils_c_p {};

/**
 * @test       string_err2
 * @brief      Test: opae_bits_get_json_string
 * @details    If malloc fails,<br>
 *             the fn returns FPGA_NO_MEMORY.<br>
 */
TEST_P(mock_bits_utils_c_p, string_err2) {
    const char *mdata =
    R"mdata({
"a": "str"
})mdata";
  json_object *root;
  char *value = nullptr;

  root = parse(mdata);
  ASSERT_NE(root, nullptr);

  system_->invalidate_malloc(0, "opae_bits_get_json_string");
  EXPECT_EQ(opae_bits_get_json_string(root,
                                      "a",
                                      &value),
            FPGA_NO_MEMORY);
  EXPECT_EQ(value, nullptr);
}

INSTANTIATE_TEST_CASE_P(bits_utils_c, mock_bits_utils_c_p,
    ::testing::ValuesIn(test_platform::mock_platforms({})));
