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

#include "test_system.h"

extern "C" {

#include "fpgad/api/logging.h"
#include "fpgad/command_line.h"

bool cmd_register_null_gbs(struct fpgad_config *c, char *null_gbs_path);

}

#include <config.h>
#include <opae/fpga.h>

#include <fstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "gtest/gtest.h"

using namespace opae::testing;

class fpgad_command_line_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_command_line_c_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    log_set(stdout);

    strcpy(tmpnull_gbs_, "tmpnull-XXXXXX.gbs");
    close(mkstemps(tmpnull_gbs_, 4));

    strcpy(invalid_gbs_, "invalid-XXXXXX.gbs");
    close(mkstemps(invalid_gbs_, 4));

    strcpy(cfg_file_, "fpgad-XXXXXX.cfg");
    close(mkstemps(cfg_file_, 4));

    std::vector<uint8_t> gbs_hdr =
      system_->assemble_gbs_header(platform_.devices[0]);

    std::ofstream gbs;
    gbs.open(tmpnull_gbs_, std::ios::out|std::ios::binary);
    gbs.write((const char *) gbs_hdr.data(), gbs_hdr.size());
    gbs.close();

    std::ofstream cfg;
    cfg.open(cfg_file_, std::ios::out);
    cfg.write("{}\n", 3);
    cfg.close();

    memset_s(&config_, sizeof(config_), 0);
    config_.poll_interval_usec = 100 * 1000;
    config_.running = true;
    config_.api_socket = "/tmp/fpga_event_socket";
  }

  virtual void TearDown() override {
    cmd_destroy(&config_);
    log_close();

    system_->finalize();

    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(tmpnull_gbs_);
      unlink(cfg_file_);
    }
    unlink(invalid_gbs_);
  }

  char tmpnull_gbs_[20];
  char invalid_gbs_[20];
  char cfg_file_[20];
  struct fpgad_config config_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       help
 * @brief      Test: cmd_show_help
 * @details    cmd_show_help sends the app help message<br>
 *             to the given file pointer.<br>
 */
TEST_P(fpgad_command_line_c_p, help) {
  cmd_show_help(stdout);
}

/**
 * @test       register_err0
 * @brief      Test: cmd_register_null_gbs
 * @details    When the maximum number of NULL GBS's have been<br>
 *             registered, the fn logs an error and returns true.<br>
 */
TEST_P(fpgad_command_line_c_p, register_err0) {
  config_.num_null_gbs = MAX_NULL_GBS;
  EXPECT_TRUE(cmd_register_null_gbs(&config_, (char *)"blah"));
  config_.num_null_gbs = 0;
}

/**
 * @test       register_err1
 * @brief      Test: cmd_register_null_gbs
 * @details    When given an invalid NULL GBS path,<br>
 *             the fn logs an error and returns false.<br>
 */
TEST_P(fpgad_command_line_c_p, register_err1) {
  EXPECT_FALSE(cmd_register_null_gbs(&config_, (char *)"blah"));
  EXPECT_EQ(config_.num_null_gbs, 0);
}

/**
 * @test       register_err2
 * @brief      Test: cmd_register_null_gbs
 * @details    When given a valid NULL GBS path,<br>
 *             but that GBS path is to an invalid GBS,<br>
 *             the fn logs an error and returns false.<br>
 */
TEST_P(fpgad_command_line_c_p, register_err2) {
  EXPECT_FALSE(cmd_register_null_gbs(&config_, invalid_gbs_));
  EXPECT_EQ(config_.num_null_gbs, 0);
}

/**
 * @test       register_null
 * @brief      Test: cmd_register_null_gbs
 * @details    When given a path to a valid NULL GBS,<br>
 *             the fn loads the GBS and returns true.<br>
 */
TEST_P(fpgad_command_line_c_p, register_null) {
  EXPECT_TRUE(cmd_register_null_gbs(&config_, tmpnull_gbs_));
  EXPECT_EQ(config_.num_null_gbs, 1);
}

/**
 * @test       canonicalize0
 * @brief      Test: cmd_canonicalize_paths
 * @details    When the .logfile and .pidfile members of<br>
 *             the input config don't contain ".." nor "/",<br>
 *             then the fn prepends a suitable prefix.<br>
 */
TEST_P(fpgad_command_line_c_p, canonicalize0) {

  strcpy(config_.logfile, "log");
  strcpy(config_.pidfile, "pid");

  mode_t filemode;

  std::string dir;

  if (!geteuid()) {
    dir = "/var/lib/opae";
    filemode = 0026;
  } else {
    dir = std::string(getenv("HOME")) + std::string("/.opae");
    filemode = 0022;
  }

  std::string log = dir + std::string("/log");
  std::string pid = dir + std::string("/pid");

  cmd_canonicalize_paths(&config_);

  EXPECT_STREQ(config_.directory, dir.c_str());
  EXPECT_STREQ(config_.logfile, log.c_str());
  EXPECT_STREQ(config_.pidfile, pid.c_str());
  EXPECT_EQ(config_.filemode, filemode);
}

/**
 * @test       canonicalize1
 * @brief      Test: cmd_canonicalize_paths
 * @details    If the .logfile and .pidfile members of<br>
 *             the input config contain ".." or "/",<br>
 *             then the fn prepends a suitable prefix<br>
 *             and uses the default names fpgad.log and fpgad.pid.<br>
 */
TEST_P(fpgad_command_line_c_p, canonicalize1) {

  strcpy(config_.logfile, "../../etc/something_im_not_supposed_to_access");
  strcpy(config_.pidfile, "..");

  mode_t filemode;

  std::string dir;

  if (!geteuid()) {
    dir = "/var/lib/opae";
    filemode = 0026;
  } else {
    dir = std::string(getenv("HOME")) + std::string("/.opae");
    filemode = 0022;
  }

  std::string log = dir + std::string("/fpgad.log");
  std::string pid = dir + std::string("/fpgad.pid");

  cmd_canonicalize_paths(&config_);

  EXPECT_STREQ(config_.directory, dir.c_str());
  EXPECT_STREQ(config_.logfile, log.c_str());
  EXPECT_STREQ(config_.pidfile, pid.c_str());
  EXPECT_EQ(config_.filemode, filemode);
  config_.daemon = true;
}

/**
 * @test       canonicalize2
 * @brief      Test: cmd_canonicalize_paths
 * @details    If the .cfgfile member of<br>
 *             the input config contains "..",<br>
 *             then the fn falls back to searching<br>
 *             for a config file.<br>
 */
TEST_P(fpgad_command_line_c_p, canonicalize2) {
  strcpy(config_.cfgfile, "../some.cfg");

  cmd_canonicalize_paths(&config_);

  EXPECT_EQ(config_.cfgfile[0], '\0');
}

/**
 * @test       canonicalize3
 * @brief      Test: cmd_canonicalize_paths
 * @details    If the .cfgfile member of<br>
 *             the input config is a valid file,<br>
 *             but it has invalid configuration contents,<br>
 *             then the fn attempts to use it to load<br>
 *             the configuration, but fails over to searching.<br>
 */
TEST_P(fpgad_command_line_c_p, canonicalize3) {
  strcpy(config_.cfgfile, cfg_file_);

  cmd_canonicalize_paths(&config_);

  EXPECT_STREQ(config_.cfgfile, "");
}

/**
 * @test       canonicalize4
 * @brief      Test: cmd_canonicalize_paths
 * @details    If the .cfgfile member of<br>
 *             the input config is a valid file,<br>
 *             and it has valid configuration contents,<br>
 *             then the fn attempts to use it to load<br>
 *             the configuration and succeeds.<br>
 */
TEST_P(fpgad_command_line_c_p, canonicalize4) {
  const char *valid_cfg = R"cfg(
{
  "configurations": {
    "a": {
      "configuration": {},
      "enabled": true,
      "plugin": "liba.so",
      "devices": [
        [ "0x8086", "0x0b30" ]
      ]
    }
  },
  "plugins": [
    "a"
  ]
}
)cfg";

  strcpy(config_.cfgfile, cfg_file_);
  std::ofstream cfg;
  cfg.open(cfg_file_, std::ios::out);
  cfg.write(valid_cfg, strlen(valid_cfg));
  cfg.close();

  cmd_canonicalize_paths(&config_);

  char *d = get_current_dir_name();
  std::string cfg_file = std::string(d) + std::string("/") + std::string(cfg_file_);

  EXPECT_STREQ(config_.cfgfile, cfg_file.c_str());
  free(d);
}

/**
 * @test       symlink0
 * @brief      Test: cmd_path_is_symlink
 * @details    If the given path string is empty,<br>
 *             then the fn returns false.<br>
 */
TEST_P(fpgad_command_line_c_p, symlink0) {
  EXPECT_FALSE(cmd_path_is_symlink(""));
}

/**
 * @test       symlink1
 * @brief      Test: cmd_path_is_symlink
 * @details    If the given file name doesn't exist,<br>
 *             then the fn returns false.<br>
 */
TEST_P(fpgad_command_line_c_p, symlink1) {
  EXPECT_FALSE(cmd_path_is_symlink("doesntexist"));
}

/**
 * @test       symlink2
 * @brief      Test: cmd_path_is_symlink
 * @details    If the given file name exists,<br>
 *             and it does not contain any / characters,<br>
 *             and it is a symlink,<br>
 *             then the fn returns true.<br>
 */
TEST_P(fpgad_command_line_c_p, symlink2) {
  ASSERT_EQ(symlink(cfg_file_, "mylink"), 0);
  EXPECT_TRUE(cmd_path_is_symlink("mylink"));
  unlink("mylink");
}

/**
 * @test       symlink3
 * @brief      Test: cmd_path_is_symlink
 * @details    If the given file name exists,<br>
 *             and it does not contain a / character in position 0,<br>
 *             and there is a symlink in any of the path components,<br>
 *             then the fn returns true.<br>
 */
TEST_P(fpgad_command_line_c_p, symlink3) {
  std::string s;

  // bar/baz/foo -> cfg_file_
  ASSERT_EQ(mkdir("bar", 0755), 0);
  ASSERT_EQ(mkdir("bar/baz", 0755), 0);
  s = std::string("../../") + std::string(cfg_file_);
  ASSERT_EQ(symlink(s.c_str(), "bar/baz/foo"), 0);
  EXPECT_TRUE(cmd_path_is_symlink("bar/baz/foo"));
  ASSERT_EQ(unlink("bar/baz/foo"), 0);
  ASSERT_EQ(rmdir("bar/baz"), 0);
  ASSERT_EQ(rmdir("bar"), 0);

  // bar/baz -> ../
  ASSERT_EQ(mkdir("bar", 0755), 0);
  ASSERT_EQ(symlink("..", "bar/baz"), 0);
  s = std::string("bar/baz/") + std::string(cfg_file_);
  EXPECT_TRUE(cmd_path_is_symlink(s.c_str()));
  ASSERT_EQ(unlink("bar/baz"), 0);
  ASSERT_EQ(rmdir("bar"), 0);

  // bar -> blah which contains baz, which contains the config file
  ASSERT_EQ(mkdir("blah", 0755), 0); 
  ASSERT_EQ(mkdir("blah/baz", 0755), 0);
  s = std::string("blah/baz/") + std::string(cfg_file_);
  ASSERT_EQ(rename(cfg_file_, s.c_str()), 0);
  ASSERT_EQ(symlink("blah", "bar"), 0);
  s = std::string("bar/baz/") + std::string(cfg_file_);
  EXPECT_TRUE(cmd_path_is_symlink(s.c_str()));
  ASSERT_EQ(rename(s.c_str(), cfg_file_), 0);
  ASSERT_EQ(rmdir("blah/baz"), 0);
  ASSERT_EQ(rmdir("blah"), 0);
  ASSERT_EQ(unlink("bar"), 0);
}

/**
 * @test       symlink4
 * @brief      Test: cmd_path_is_symlink
 * @details    If the given file name exists,<br>
 *             and it contains a / character in position 0,<br>
 *             and there is a symlink in any of the path components,<br>
 *             then the fn returns true.<br>
 */
TEST_P(fpgad_command_line_c_p, symlink4) {
  std::string s;
  char *d = get_current_dir_name();

  ASSERT_NE(d, nullptr);

  // /current/dir/foo -> cfg file
  ASSERT_EQ(symlink(cfg_file_, "foo"), 0);
  s = std::string(d) + std::string("/foo");
  EXPECT_TRUE(cmd_path_is_symlink(s.c_str()));
  ASSERT_EQ(unlink("foo"), 0);

  free(d);
}

INSTANTIATE_TEST_CASE_P(fpgad_command_line_c, fpgad_command_line_c_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p","skx-p-dfl0" })));
