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

void resolve_dirs(struct config *c);

int fpgad_main(int argc, char *argv[]);

}

#include <config.h>
#include <opae/fpga.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <fstream>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class fpgad_fpgad_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_fpgad_c_p() {}

  virtual void SetUp() override {
    strcpy(tmpnull_gbs_, "tmpnull-XXXXXX.gbs");
    close(mkstemps(tmpnull_gbs_, 4));
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    null_gbs_ = system_->assemble_gbs_header(platform_.devices[0]);

    std::ofstream gbs;
    gbs.open(tmpnull_gbs_, std::ios::out|std::ios::binary);
    gbs.write((const char *) null_gbs_.data(), null_gbs_.size());
    gbs.close();

    fLog = stdout;
    optind = 0;
    config_ = config;
  }

  virtual void TearDown() override {
    config = config_;

    system_->finalize();
  }

  std::vector<uint8_t> null_gbs_;
  char tmpnull_gbs_[20];
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
 * @test       resolve0
 * @brief      Test: resolve_dirs
 * @details    When the .logfile and .pidfile members of<br>
 *             the input config don't contain ".." nor "/",<br>
 *             then resolve_dirs prepends a suitable prefix.<br>
 */
TEST_P(fpgad_fpgad_c_p, resolve0) {

  strcpy(config.logfile, "log");
  strcpy(config.pidfile, "pid");

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

  resolve_dirs(&config);

  EXPECT_STREQ(config.directory, dir.c_str());
  EXPECT_STREQ(config.logfile, log.c_str());
  EXPECT_STREQ(config.pidfile, pid.c_str());
  EXPECT_EQ(config.filemode, filemode);
}

/**
 * @test       resolve1
 * @brief      Test: resolve_dirs
 * @details    If the .logfile and .pidfile members of<br>
 *             the input config contain ".." or "/",<br>
 *             then resolve_dirs prepends a suitable prefix
 *             and uses the default names fpgad.log and fpgad.pid.<br>
 */
TEST_P(fpgad_fpgad_c_p, resolve1) {

  strcpy(config.logfile, "../../etc/something_im_not_supposed_to_access");
  strcpy(config.pidfile, "..");

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

  resolve_dirs(&config);

  EXPECT_STREQ(config.directory, dir.c_str());
  EXPECT_STREQ(config.logfile, log.c_str());
  EXPECT_STREQ(config.pidfile, pid.c_str());
  EXPECT_EQ(config.filemode, filemode);
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
  strcpy(zero, "fpgad");
  strcpy(one, "-d");
  strcpy(two, "-l");
  strcpy(three, "log");
  strcpy(four, "-p");
  strcpy(five, "pid");
  strcpy(six, "-s");
  strcpy(seven, "sock");
  strcpy(eight, "-n");
  strcpy(nine, tmpnull_gbs_);
  strcpy(ten, "-h");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten };

  EXPECT_EQ(fpgad_main(11, argv), 0);
  EXPECT_NE(config.daemon, 0);
  EXPECT_STREQ(config.logfile, "log");
  EXPECT_STREQ(config.pidfile, "pid");
  EXPECT_STREQ(config.socket, "sock");
  EXPECT_STREQ(basename(config.null_gbs[0]), tmpnull_gbs_);
  EXPECT_EQ(config.num_null_gbs, 1);
  free(config.null_gbs[0]);
}

/**
 * @test       invalid_nullgbs
 * @brief      Test: fpgad_main
 * @details    When fpgad_main is called with a null_gbs that is invalid<br>
 *             it fails to add the null gbs to the config struct<br>
 */
TEST_P(fpgad_fpgad_c_p, invalid_nullgbs) {
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
  strcpy(zero, "fpgad");
  strcpy(one, "-d");
  strcpy(two, "-l");
  strcpy(three, "log");
  strcpy(four, "-p");
  strcpy(five, "pid");
  strcpy(six, "-s");
  strcpy(seven, "sock");
  strcpy(eight, "-n");
  strcpy(nine, "no-file");
  strcpy(ten, "-h");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten };

  EXPECT_NE(fpgad_main(11, argv), 0);
  EXPECT_NE(config.daemon, 0);
  EXPECT_STREQ(config.logfile, "log");
  EXPECT_STREQ(config.pidfile, "pid");
  EXPECT_STREQ(config.socket, "sock");
  EXPECT_FALSE(config.null_gbs[0]);
  EXPECT_EQ(config.num_null_gbs, 0);
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
 * @test       main_invalid
 * @brief      Test: fpgad_main
 * @details    When fpgad_main is called with an invalid command option,<br>
 *             it returns 1.<br>
 */
TEST_P(fpgad_fpgad_c_p, main_invalid_01) {
  char zero[32];
  char one[32];
  char two[32];
  char three[32];
  char four[32];
  char five[32];
  char six[32];
  char seven[32];
  char eight[32];
  char nine[32];
  char ten[32];
  char eleven[32];
  char twelve[32];
  char thirteen[32];
  char fourteen[32];
  strcpy(zero, "  fpgad  ");
  strcpy(one, "--daemon\x00\x08\x09\x0B\x0D");
  strcpy(two, "-D");
  strcpy(three, "dir \t\n\v");
  strcpy(four, "-l");
  strcpy(five, "log~`1$#%#&%&*&$^&$&*");
  strcpy(six, "-p");
  strcpy(seven, "     231478937024**%^%$&pid");
  strcpy(eight, "-m");
  strcpy(nine, "\000");
  strcpy(ten, "-s");
  strcpy(eleven, "sock \x00\x00\x08\x09\x0B\x0D");
  strcpy(twelve, "-n");
  strcpy(thirteen, "null_gbs");
  strcpy(fourteen, "-hhelp");

  char *argv[] = { zero, one, two, three, four,
                   five, six, seven, eight, nine,
                   ten, eleven, twelve, thirteen, fourteen };

  EXPECT_EQ(fpgad_main(15, argv), 1);
}

TEST_P(fpgad_fpgad_c_p, gbsarg_lead_nullchar) {
  const char* argv[] = { "fpgad", "-n", "\0null.gbs"};
  EXPECT_NE(fpgad_main(3, (char**)argv), 0);
}

TEST_P(fpgad_fpgad_c_p, gbsarg_inner_nullchar) {
  const char* argv[] = { "fpgad", "-n", "null\0.gbs"};
  EXPECT_NE(fpgad_main(3, (char**)argv), 0);
}

TEST_P(fpgad_fpgad_c_p, gbsarg_carriage_return) {
  const char* argv[] = { "fpgad", "-n", "n\15ll.gbs"};
  EXPECT_NE(fpgad_main(3, (char**)argv), 0);
}

TEST_P(fpgad_fpgad_c_p, gbsarg_backtick) {
  const char* argv[] = { "fpgad", "-n", "/tmp/`rm dummy.txt`/three/four/five/null.gbs"};
  EXPECT_NE(fpgad_main(3, (char**)argv), 0);
}

TEST_P(fpgad_fpgad_c_p, gbsarg_non_print) {
  const char* argv[] = { "fpgad", "-n", "/one/\07/three/\21/four/five\32/null.gbs"};
  EXPECT_NE(fpgad_main(3, (char**)argv), 0);
}

INSTANTIATE_TEST_CASE_P(fpgad_fpgad_c, fpgad_fpgad_c_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p","skx-p-dfl0" })));
