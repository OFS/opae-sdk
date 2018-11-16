// Copyright(c) 2018, Intel Corporation
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

#include <signal.h>

extern "C" {

#include <json-c/json.h>
#include <uuid/uuid.h>

#include "fpgad/config_int.h"
#include "fpgad/log.h"

extern struct config config;

void show_help(void);

void sig_handler(int sig, siginfo_t *info, void *unused);

int fpgad_main(int argc, char *argv[]);

}

#include <config.h>
#include <opae/fpga.h>

#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class fpgad_fpgad_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_fpgad_c_p() {}

  virtual void SetUp() override {
    strcpy(tmp_gbs_, "tmp-XXXXXX.gbs");
    close(mkstemps(tmp_gbs_, 4));
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    bitstream_valid_ = system_->assemble_gbs_header(platform_.devices[0]);

    std::ofstream gbs;
    gbs.open(tmp_gbs_, std::ios::out|std::ios::binary);
    gbs.write((const char *) bitstream_valid_.data(), bitstream_valid_.size());
    gbs.close();

    fLog = stdout;
    optind = 0;
    config_ = config;
  }

  virtual void TearDown() override {
    config = config_;

    system_->finalize();

    unlink(tmp_gbs_);
  }

  char tmp_gbs_[20];
  std::vector<uint8_t> bitstream_valid_;
  struct config config_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       help
 * @brief      Test: show_help
 * @details    show_help displays the application help message.<br>
 */
TEST_P(fpgad_fpgad_c_p, help) {
  show_help();
}

/**
 * @test       sig
 * @brief      Test: sig_handler
 * @details    When sig_handler is called with SIGINT,<br>
 *             it sets config.running to false to end the app.<br>
 */
TEST_P(fpgad_fpgad_c_p, sig) {
  ASSERT_NE(config.running, 0);
  sig_handler(SIGINT, nullptr, nullptr);
  EXPECT_EQ(config.running, 0);
}

/**
 * @test       main_help
 * @brief      Test: fpgad_main
 * @details    When fpgad_main is called with -h,<br>
 *             it displays the app help and returns 0.<br>
 */
TEST_P(fpgad_fpgad_c_p, main_help) {
  char zero[20];
  char one[20];
  strcpy(zero, "fpgad");
  strcpy(one, "-h");
  char *argv[] = { zero, one };
  EXPECT_EQ(fpgad_main(2, argv), 0);
}

/**
 * @test       main_params
 * @brief      Test: fpgad_main
 * @details    When fpgad_main is called with valid params,<br>
 *             it initializes the global config struct correctly.<br>
 */
TEST_P(fpgad_fpgad_c_p, main_params) {
  char zero[20];
  char one[20];
  char two[20];
  char three[20];
  char four[20];
  char five[20];
  char six[20];
  char seven[20];
  char eight[20];
  char nine[20];
  char ten[20];
  char eleven[20];
  char twelve[20];
  char thirteen[20];
  char fourteen[20];
  strcpy(zero, "fpgad");
  strcpy(one, "-d");
  strcpy(two, "-D");
  strcpy(three, "dir");
  strcpy(four, "-l");
  strcpy(five, "log");
  strcpy(six, "-p");
  strcpy(seven, "pid");
  strcpy(eight, "-m");
  strcpy(nine, "0x777");
  strcpy(ten, "-s");
  strcpy(eleven, "sock");
  strcpy(twelve, "-n");
  strcpy(thirteen, "null_gbs");
  strcpy(fourteen, "-h");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve, thirteen, fourteen };

  EXPECT_EQ(fpgad_main(15, argv), 0);
  EXPECT_NE(config.daemon, 0);
  EXPECT_STREQ(config.directory, "dir");
  EXPECT_STREQ(config.logfile, "log");
  EXPECT_STREQ(config.pidfile, "pid");
  EXPECT_EQ(config.filemode, 0x777);
  EXPECT_STREQ(config.socket, "sock");
  EXPECT_STREQ(config.null_gbs[0], "null_gbs");
  EXPECT_EQ(config.num_null_gbs, 1);
}

/**
 * @test       main_invalid
 * @brief      Test: fpgad_main
 * @details    When fpgad_main is called with an invalid command option,<br>
 *             it returns non-zero.<br>
 */
TEST_P(fpgad_fpgad_c_p, main_invalid) {
  char zero[20];
  char one[20];
  strcpy(zero, "fpgad");
  strcpy(one, "-Y");

  char *argv[] = { zero, one };

  EXPECT_NE(fpgad_main(2, argv), 0);
}

/**
 * @test       main_symlink_gbs
 * @brief      Test: fpgad_main
 * @details    When fpgad_main is called with valid params<br>
 *             but the input bitstream file is a symlink<br>
 *             to a bitstream, fpgad returns non-zero.<br>
 */
TEST_P(fpgad_fpgad_c_p, main_symlink_gbs) {
  char zero[20];
  char one[20];
  char two[20];
  char three[20];
  strcpy(zero, "fpgad");
  strcpy(one, "-n");
  strcpy(two, "test_symlink");
  strcpy(three, "-h");

  char *argv[] = { zero, one, two, three };

  ASSERT_EQ(symlink(tmp_gbs_, "test_symlink"), 0);
  EXPECT_NE(fpgad_main(4, argv), 0);
  EXPECT_EQ(unlink("test_symlink"), 0);
}

/**
 * @test       main_symlink_log
 * @brief      Test: fpgad_main
 * @details    When fpgad_main is called with valid params<br>
 *             but the output log file is a symlink<br>
 *             to a file, fpgad returns non-zero.<br>
 */
TEST_P(fpgad_fpgad_c_p, main_symlink_log) {
  char zero[20];
  char one[20];
  char two[20];
  char three[20];
  strcpy(zero, "fpgad");
  strcpy(one, "-l");
  strcpy(two, "test_symlink");
  strcpy(three, "-h");

  char *argv[] = { zero, one, two, three };

  std::string dir;
  std::string test_symlink;

  if (!geteuid()) {
    dir = "/var/lib/opae";
  } else {
    dir = std::string(getenv("HOME")) +
               std::string("/.opae");
  }

  test_symlink = dir + "/test_symlink";

  struct stat buffer;

  if (stat(dir.c_str(), &buffer)) {
    ASSERT_EQ(mkdir(dir.c_str(), 0770), 0);
  }

  if (stat(test_symlink.c_str(), &buffer) == 0) {
    ASSERT_EQ(unlink(test_symlink.c_str()), 0);
  }

  ASSERT_EQ(symlink("/tmp/fpgad.log", test_symlink.c_str()), 0);
  EXPECT_NE(fpgad_main(4, argv), 0);
  EXPECT_EQ(unlink(test_symlink.c_str()), 0);
}

/**
 * @test       main_symlink_pid
 * @brief      Test: fpgad_main
 * @details    When fpgad_main is called with valid params<br>
 *             but the output log file is a symlink<br>
 *             to a file, fpgad returns non-zero.<br>
 */
TEST_P(fpgad_fpgad_c_p, main_symlink_pid) {
  char zero[20];
  char one[20];
  char two[20];
  char three[20];
  strcpy(zero, "fpgad");
  strcpy(one, "-p");
  strcpy(two, "test_symlink");
  strcpy(three, "-h");

  char *argv[] = { zero, one, two, three };

  std::string dir;
  std::string test_symlink;

  if (!geteuid()) {
    dir = "/var/lib/opae";
  } else {
    dir = std::string(getenv("HOME")) +
               std::string("/.opae");
  }

  test_symlink = dir + "/test_symlink";

  struct stat buffer;

  if (stat(dir.c_str(), &buffer)) {
    ASSERT_EQ(mkdir(dir.c_str(), 0770), 0);
  }

  if (stat(test_symlink.c_str(), &buffer) == 0) {
    ASSERT_EQ(unlink(test_symlink.c_str()), 0);
  }

  ASSERT_EQ(symlink("/tmp/fpgad.pid", test_symlink.c_str()), 0);
  EXPECT_NE(fpgad_main(4, argv), 0);
  EXPECT_EQ(unlink(test_symlink.c_str()), 0);
}

INSTANTIATE_TEST_CASE_P(fpgad_fpgad_c, fpgad_fpgad_c_p,
                        ::testing::Values(std::string("skx-p")));
