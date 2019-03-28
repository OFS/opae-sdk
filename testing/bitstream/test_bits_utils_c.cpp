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

extern "C" {

bool opae_bitstream_path_invalid_chars(const char *path,
				       size_t len);

bool opae_bitstream_path_not_file(const char *path);

bool opae_bitstream_path_contains_dotdot(const char *path,
					 size_t len);

bool opae_bitstream_path_contains_symlink(const char *path,
					  size_t len);
}

#include <config.h>
#include <opae/fpga.h>

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
 * @brief      Test: opae_bitstream_get_json_string
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

  EXPECT_EQ(opae_bitstream_get_json_string(root,
					   "b",
					   &value),
            FPGA_EXCEPTION);
  EXPECT_EQ(value, nullptr);
}

/**
 * @test       string_err1
 * @brief      Test: opae_bitstream_get_json_string
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

  EXPECT_EQ(opae_bitstream_get_json_string(root,
					   "a",
					   &value),
            FPGA_EXCEPTION);
  EXPECT_EQ(value, nullptr);
}

/**
 * @test       int_err0
 * @brief      Test: opae_bitstream_get_json_int
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

  EXPECT_EQ(opae_bitstream_get_json_int(root,
					"b",
					&value),
            FPGA_EXCEPTION);
  EXPECT_EQ(value, 0);
}

/**
 * @test       int_err1
 * @brief      Test: opae_bitstream_get_json_int
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

  EXPECT_EQ(opae_bitstream_get_json_int(root,
					"a",
					&value),
            FPGA_EXCEPTION);
  EXPECT_EQ(value, 0);
}

/**
 * @test       double_err0
 * @brief      Test: opae_bitstream_get_json_double
 * @details    If the given name doesn't exist,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(bits_utils_c_p, double_err0) {
    const char *mdata =
    R"mdata({
"a": 3.14
})mdata";
  json_object *root;
  double value = 0.0;

  root = parse(mdata);
  ASSERT_NE(root, nullptr);

  EXPECT_EQ(opae_bitstream_get_json_double(root,
					   "b",
					   &value),
            FPGA_EXCEPTION);
  EXPECT_EQ(value, 0.0);
}

/**
 * @test       double_err1
 * @brief      Test: opae_bitstream_get_json_double
 * @details    If the given name exists,<br>
 *             but isn't of type double,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(bits_utils_c_p, double_err1) {
    const char *mdata =
    R"mdata({
"a": "str"
})mdata";
  json_object *root;
  double value = 0.0;

  root = parse(mdata);
  ASSERT_NE(root, nullptr);

  EXPECT_EQ(opae_bitstream_get_json_double(root,
					   "a",
					   &value),
            FPGA_EXCEPTION);
  EXPECT_EQ(value, 0.0);
}

/**
 * @test       invalid_chars0
 * @brief      Test: opae_bitstream_path_invalid_chars
 * @details    Given a path that contains non-printable chars,<br>
 *             the fn returns true.<br>
 */
TEST_P(bits_utils_c_p, invalid_chars0) {
  const char *p;
  p = "\x01\x05xyz.gbs";
  EXPECT_TRUE(opae_bitstream_path_invalid_chars(p, strlen(p)));
}

/**
 * @test       invalid_chars1
 * @brief      Test: opae_bitstream_path_invalid_chars
 * @details    Given a path that contains URL encoding,<br>
 *             the fn returns true.<br>
 */
TEST_P(bits_utils_c_p, invalid_chars1) {
  const char *p;
  p = "my%2E.gbs";
  EXPECT_TRUE(opae_bitstream_path_invalid_chars(p, strlen(p)));
}

/**
 * @test       invalid_chars2
 * @brief      Test: opae_bitstream_path_invalid_chars
 * @details    Given a path that contains no invalid chars,<br>
 *             the fn returns false.<br>
 */
TEST_P(bits_utils_c_p, invalid_chars2) {
  const char *p;
  p = "abc.gbs";
  EXPECT_FALSE(opae_bitstream_path_invalid_chars(p, strlen(p)));
}

/**
 * @test       not_file0
 * @brief      Test: opae_bitstream_path_not_file
 * @details    Given a path to a file that doesn't exist,<br>
 *             the fn returns true.<br>
 */
TEST_P(bits_utils_c_p, not_file0) {
  EXPECT_TRUE(opae_bitstream_path_not_file("doesntexist"));
}

/**
 * @test       not_file1
 * @brief      Test: opae_bitstream_path_not_file
 * @details    Given a path to a directory,<br>
 *             the fn returns true.<br>
 */
TEST_P(bits_utils_c_p, not_file1) {
  EXPECT_TRUE(opae_bitstream_path_not_file("/"));
}

/**
 * @test       not_file2
 * @brief      Test: opae_bitstream_path_not_file
 * @details    Given a path to valid file,<br>
 *             the fn returns false.<br>
 */
TEST_P(bits_utils_c_p, not_file2) {
  char tmpfile[20];

  strcpy(tmpfile, "tmp-XXXXXX.gbs");
  close(mkstemps(tmpfile, 4));

  EXPECT_FALSE(opae_bitstream_path_not_file(tmpfile));

  unlink(tmpfile);
}

/**
 * @test       dotdot0
 * @brief      Test: opae_bitstream_path_contains_dotdot
 * @details    Given a path that contains a reference to<br>
 *             the special directory designator ..<br>
 *             the fn returns true.<br>
 */
TEST_P(bits_utils_c_p, dotdot0) {
  EXPECT_TRUE(opae_bitstream_path_contains_dotdot("..", 2));
  EXPECT_TRUE(opae_bitstream_path_contains_dotdot("../", 3));
  EXPECT_TRUE(opae_bitstream_path_contains_dotdot("../abc.gbs", 10));
  EXPECT_TRUE(opae_bitstream_path_contains_dotdot("my/../abc.gbs", 13));
  EXPECT_TRUE(opae_bitstream_path_contains_dotdot("my/..", 5));
}

/**
 * @test       dotdot1
 * @brief      Test: opae_bitstream_path_contains_dotdot
 * @details    Given a path that contains the character sequence '..',<br>
 *             if that character sequence does not designate the parent dir,<br>
 *             the fn returns false.<br>
 */
TEST_P(bits_utils_c_p, dotdot1) {
  EXPECT_FALSE(opae_bitstream_path_contains_dotdot("my..gbs", 7));
}

/**
 * @test       symlink0
 * @brief      Test: opae_bitstream_path_contains_symlink
 * @details    If the given path string is empty,<br>
 *             then the fn returns true.<br>
 */
TEST_P(bits_utils_c_p, symlink0) {
  EXPECT_TRUE(opae_bitstream_path_contains_symlink("", 0));
}

/**
 * @test       symlink1
 * @brief      Test: opae_bitstream_path_contains_symlink
 * @details    If the given file name doesn't exist,<br>
 *             then the fn returns true.<br>
 */
TEST_P(bits_utils_c_p, symlink1) {
  EXPECT_TRUE(opae_bitstream_path_contains_symlink("doesntexist", 11));
}

/**
 * @test       symlink2
 * @brief      Test: opae_bitstream_path_contains_symlink
 * @details    If the given file name exists,<br>
 *             and it does not contain any / characters,<br>
 *             and it is a symlink,<br>
 *             then the fn returns true.<br>
 */
TEST_P(bits_utils_c_p, symlink2) {
  char tmpfile[20];

  strcpy(tmpfile, "tmp-XXXXXX.gbs");
  close(mkstemps(tmpfile, 4));

  ASSERT_EQ(symlink(tmpfile, "mylink"), 0);
  EXPECT_TRUE(opae_bitstream_path_contains_symlink("mylink", 6));
  unlink("mylink");
  unlink(tmpfile);
}

/**
 * @test       symlink3
 * @brief      Test: opae_bitstream_path_contains_symlink
 * @details    If the given file name exists,<br>
 *             and it does not contain a / character in position 0,<br>
 *             and there is a symlink in any of the path components,<br>
 *             then the fn returns true.<br>
 */
TEST_P(bits_utils_c_p, symlink3) {
  char tmpfile[20];

  strcpy(tmpfile, "tmp-XXXXXX.gbs");
  close(mkstemps(tmpfile, 4));
      
  std::string s;

  EXPECT_EQ(std::system("rm -rf bar"), 0);

  // bar/baz/foo -> tmpfile
  ASSERT_EQ(mkdir("bar", 0755), 0);
  ASSERT_EQ(mkdir("bar/baz", 0755), 0);
  s = std::string("../../") + std::string(tmpfile);
  ASSERT_EQ(symlink(s.c_str(), "bar/baz/foo"), 0);
  EXPECT_TRUE(opae_bitstream_path_contains_symlink("bar/baz/foo", 11));
  ASSERT_EQ(unlink("bar/baz/foo"), 0);
  ASSERT_EQ(rmdir("bar/baz"), 0);
  ASSERT_EQ(rmdir("bar"), 0);

  // bar/baz -> ../
  ASSERT_EQ(mkdir("bar", 0755), 0);
  ASSERT_EQ(symlink("..", "bar/baz"), 0);
  s = std::string("bar/baz/") + std::string(tmpfile);
  EXPECT_TRUE(opae_bitstream_path_contains_symlink(s.c_str(), strlen(s.c_str())));
  ASSERT_EQ(unlink("bar/baz"), 0);
  ASSERT_EQ(rmdir("bar"), 0);

  // bar -> blah which contains baz, which contains the config file
  ASSERT_EQ(mkdir("blah", 0755), 0); 
  ASSERT_EQ(mkdir("blah/baz", 0755), 0);
  s = std::string("blah/baz/") + std::string(tmpfile);
  ASSERT_EQ(rename(tmpfile, s.c_str()), 0);
  ASSERT_EQ(symlink("blah", "bar"), 0);
  s = std::string("bar/baz/") + std::string(tmpfile);
  EXPECT_TRUE(opae_bitstream_path_contains_symlink(s.c_str(), strlen(s.c_str())));
  ASSERT_EQ(rename(s.c_str(), tmpfile), 0);
  ASSERT_EQ(rmdir("blah/baz"), 0);
  ASSERT_EQ(rmdir("blah"), 0);
  ASSERT_EQ(unlink("bar"), 0);
  ASSERT_EQ(unlink(tmpfile), 0);
}

/**
 * @test       symlink4
 * @brief      Test: opae_bitstream_path_contains_symlink
 * @details    If the given file name exists,<br>
 *             and it contains a / character in position 0,<br>
 *             and there is a symlink in any of the path components,<br>
 *             then the fn returns true.<br>
 */
TEST_P(bits_utils_c_p, symlink4) {
  char tmpfile[20];

  strcpy(tmpfile, "tmp-XXXXXX.gbs");
  close(mkstemps(tmpfile, 4));
      
  std::string s;
  char *d = get_current_dir_name();

  ASSERT_NE(d, nullptr);

  // /current/dir/foo -> cfg file
  ASSERT_EQ(symlink(tmpfile, "foo"), 0);
  s = std::string(d) + std::string("/foo");
  EXPECT_TRUE(opae_bitstream_path_contains_symlink(s.c_str(), strlen(s.c_str())));
  ASSERT_EQ(unlink("foo"), 0);
  ASSERT_EQ(unlink(tmpfile), 0);

  free(d);
}

/**
 * @test       symlink5
 * @brief      Test: opae_bitstream_path_contains_symlink
 * @details    If the given file name exists and is a regular file,<br>
 *             then the fn returns false.<br>
 */
TEST_P(bits_utils_c_p, symlink5) {
  char tmpfile[20];

  strcpy(tmpfile, "tmp-XXXXXX.gbs");
  close(mkstemps(tmpfile, 4));

  EXPECT_FALSE(opae_bitstream_path_contains_symlink(tmpfile, strlen(tmpfile)));

  ASSERT_EQ(unlink(tmpfile), 0);
}

/**
 * @test       is_valid0
 * @brief      Test: opae_bitstream_path_is_valid
 * @details    If the given path pointer is NULL or<br>
 *             points to the empty string,<br>
 *             then the fn returns false.<br>
 */
TEST_P(bits_utils_c_p, is_valid0) {
  EXPECT_FALSE(opae_bitstream_path_is_valid(NULL, 0));
  EXPECT_FALSE(opae_bitstream_path_is_valid("", 0));
}

/**
 * @test       is_valid1
 * @brief      Test: opae_bitstream_path_is_valid
 * @details    If the given path contains non-printable characters,<br>
 *             then the fn returns false.<br>
 */
TEST_P(bits_utils_c_p, is_valid1) {
  const char *p = "\x01ijk.gbs";
  EXPECT_FALSE(opae_bitstream_path_is_valid(p, 0));
}

/**
 * @test       is_valid2
 * @brief      Test: opae_bitstream_path_is_valid
 * @details    If the given path doesn't exist,<br>
 *             then the fn returns false.<br>
 */
TEST_P(bits_utils_c_p, is_valid2) {
  EXPECT_FALSE(opae_bitstream_path_is_valid("doesntexist", 0));
}

/**
 * @test       is_valid3
 * @brief      Test: opae_bitstream_path_is_valid
 * @details    If the given flags parameter does not contain<br>
 *             OPAE_BITSTREAM_PATH_NO_PARENT,<br>
 *             and the special parent directory indicator ..<br>
 *             appears in the path, then the fn returns true.<br>
 */
TEST_P(bits_utils_c_p, is_valid3) {
  char tmpfile[32];

  EXPECT_EQ(std::system("rm -rf bar"), 0);

  ASSERT_EQ(mkdir("bar", 0755), 0); 
  strcpy(tmpfile, "tmp-XXXXXX.gbs");
  close(mkstemps(tmpfile, 4));

  std::string s = std::string("bar/../") + std::string(tmpfile);

  EXPECT_TRUE(opae_bitstream_path_is_valid(s.c_str(), 0));

  ASSERT_EQ(unlink(tmpfile), 0);
  ASSERT_EQ(rmdir("bar"), 0);
}

/**
 * @test       is_valid4
 * @brief      Test: opae_bitstream_path_is_valid
 * @details    If the given flags parameter contains<br>
 *             OPAE_BITSTREAM_PATH_NO_PARENT,<br>
 *             and the special parent directory indicator ..<br>
 *             appears in the path, then the fn returns false.<br>
 */
TEST_P(bits_utils_c_p, is_valid4) {
  char tmpfile[32];

  EXPECT_EQ(std::system("rm -rf bar"), 0);

  ASSERT_EQ(mkdir("bar", 0755), 0); 
  strcpy(tmpfile, "tmp-XXXXXX.gbs");
  close(mkstemps(tmpfile, 4));

  std::string s = std::string("bar/../") + std::string(tmpfile);

  EXPECT_FALSE(opae_bitstream_path_is_valid(s.c_str(),
                                            OPAE_BITSTREAM_PATH_NO_PARENT));

  ASSERT_EQ(unlink(tmpfile), 0);
  ASSERT_EQ(rmdir("bar"), 0);
}

/**
 * @test       is_valid5
 * @brief      Test: opae_bitstream_path_is_valid
 * @details    If the given flags parameter does not contain<br>
 *             OPAE_BITSTREAM_PATH_NO_SYMLINK,<br>
 *             and the path contains a symlink component,<br>
 *             then the fn returns true.<br>
 */
TEST_P(bits_utils_c_p, is_valid5) {
  char tmpfile[20];

  strcpy(tmpfile, "tmp-XXXXXX.gbs");
  close(mkstemps(tmpfile, 4));

  ASSERT_EQ(symlink(tmpfile, "foo"), 0);

  EXPECT_TRUE(opae_bitstream_path_is_valid("foo", 0));

  ASSERT_EQ(unlink("foo"), 0);
  ASSERT_EQ(unlink(tmpfile), 0);
}

/**
 * @test       is_valid6
 * @brief      Test: opae_bitstream_path_is_valid
 * @details    If the given flags parameter contains<br>
 *             OPAE_BITSTREAM_PATH_NO_SYMLINK,<br>
 *             and the path contains a symlink component,<br>
 *             then the fn returns false.<br>
 */
TEST_P(bits_utils_c_p, is_valid6) {
  char tmpfile[20];

  strcpy(tmpfile, "tmp-XXXXXX.gbs");
  close(mkstemps(tmpfile, 4));

  ASSERT_EQ(symlink(tmpfile, "foo"), 0);

  EXPECT_FALSE(opae_bitstream_path_is_valid("foo",
                                            OPAE_BITSTREAM_PATH_NO_SYMLINK));

  ASSERT_EQ(unlink("foo"), 0);
  ASSERT_EQ(unlink(tmpfile), 0);
}

INSTANTIATE_TEST_CASE_P(bits_utils_c, bits_utils_c_p,
    ::testing::ValuesIn(test_platform::platforms({})));


class mock_bits_utils_c_p : public bits_utils_c_p {};

/**
 * @test       string_err2
 * @brief      Test: opae_bitstream_get_json_string
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

  system_->invalidate_malloc(0, "opae_bitstream_get_json_string");
  EXPECT_EQ(opae_bitstream_get_json_string(root,
					   "a",
					   &value),
            FPGA_NO_MEMORY);
  EXPECT_EQ(value, nullptr);
}

INSTANTIATE_TEST_CASE_P(bits_utils_c, mock_bits_utils_c_p,
    ::testing::ValuesIn(test_platform::mock_platforms({})));
