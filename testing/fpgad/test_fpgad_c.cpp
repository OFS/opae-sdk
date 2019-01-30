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

#include "fpgad/command_line.h"
#include "fpgad/api/logging.h"

extern struct fpgad_config global_config;

void sig_handler(int sig, siginfo_t *info, void *unused);

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
#include <thread>
#include <chrono>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class fpgad_fpgad_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_fpgad_c_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);

    log_set(stdout);

    strcpy(tmpnull_gbs_, "tmpnull-XXXXXX.gbs");
    close(mkstemps(tmpnull_gbs_, 4));

    std::vector<uint8_t> gbs_hdr =
      system_->assemble_gbs_header(platform_.devices[0]);

    std::ofstream gbs;
    gbs.open(tmpnull_gbs_, std::ios::out|std::ios::binary);
    gbs.write((const char *) gbs_hdr.data(), gbs_hdr.size());
    gbs.close();

    memset_s(&config_, sizeof(config_), 0);
    config_.poll_interval_usec = 100 * 1000;
    config_.running = true;
    config_.api_socket = "/tmp/fpga_event_socket";

    global_config.poll_interval_usec = 100 * 1000;
    global_config.running = true;
    global_config.api_socket = "/tmp/fpga_event_socket";

    optind = 0;
  }

  virtual void TearDown() override {
    cmd_destroy(&config_);
    log_close();

    fpgaFinalize();
    system_->finalize();

    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(tmpnull_gbs_);
    }
  }

  char tmpnull_gbs_[20];
  struct fpgad_config config_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       sigint
 * @brief      Test: sig_handler
 * @details    When sig_handler is called with SIGINT,<br>
 *             it sets config.running to false to end the app.<br>
 */
TEST_P(fpgad_fpgad_c_p, sigint) {
  ASSERT_TRUE(global_config.running);
  sig_handler(SIGINT, nullptr, nullptr);
  EXPECT_FALSE(global_config.running);
}

/**
 * @test       sigterm
 * @brief      Test: sig_handler
 * @details    When sig_handler is called with SIGTERM,<br>
 *             it sets config.running to false to end the app.<br>
 */
TEST_P(fpgad_fpgad_c_p, sigterm) {
  ASSERT_TRUE(global_config.running);
  sig_handler(SIGTERM, nullptr, nullptr);
  EXPECT_FALSE(global_config.running);
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
  EXPECT_TRUE(global_config.daemon);
  EXPECT_STREQ(global_config.logfile, "log");
  EXPECT_STREQ(global_config.pidfile, "pid");
  EXPECT_STREQ(global_config.api_socket, "sock");
  // because main goes to out_destroy:
  EXPECT_EQ(global_config.num_null_gbs, 0);
}

/**
 * @test       main_invalid0
 * @brief      Test: fpgad_main
 * @details    When fpgad_main is called with an invalid command option,<br>
 *             it returns non-zero.<br>
 */
TEST_P(fpgad_fpgad_c_p, main_invalid0) {
  char zero[20];
  char one[20];
  strcpy(zero, "fpgad");
  strcpy(one, "-Y");

  char *argv[] = { zero, one };

  EXPECT_NE(fpgad_main(2, argv), 0);
}

/**
 * @test       main_invalid1
 * @brief      Test: fpgad_main
 * @details    When fpgad_main is called with an invalid command option,<br>
 *             it returns 1.<br>
 */
TEST_P(fpgad_fpgad_c_p, main_invalid1) {
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

static int main_returned;
void * call_main(void *unused)
{
  UNUSED_PARAM(unused);

  char zero[20];
  strcpy(zero, "fpgad");

  char *argv[] = { zero };

  main_returned = fpgad_main(1, argv);

  return NULL;
}

/**
 * @test       main_valid
 * @brief      Test: fpgad_main
 * @details    When fpgad_main is called with a valid command line,<br>
 *             it runs until signaled to stop and returns zero.<br>
 */
TEST_P(fpgad_fpgad_c_p, main_valid) {
  main_returned = -1;
  ASSERT_TRUE(global_config.running);
  std::thread main_thr = std::thread(call_main, nullptr);
  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  global_config.running = false;
  main_thr.join();
  EXPECT_EQ(main_returned, 0);
}

INSTANTIATE_TEST_CASE_P(fpgad_fpgad_c, fpgad_fpgad_c_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p","skx-p-dfl0" })));
