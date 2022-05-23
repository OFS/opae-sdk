// Copyright(c) 2018-2022, Intel Corporation
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

#include <signal.h>
#include <unistd.h>

#define NO_OPAE_C
#include "mock/opae_fixtures.h"

extern "C" {
int daemonize(void (*hndlr)(int, siginfo_t *, void *), mode_t mask, const char *dir);

void test_sig_handler(int sig, siginfo_t *info, void *unused)
{
  UNUSED_PARAM(sig);
  UNUSED_PARAM(info);
  UNUSED_PARAM(unused);
}
}

using namespace opae::testing;

class fpgad_daemonize_c_p : public opae_base_p<> {
 protected:

  virtual void SetUp() override {
    opae_base_p<>::SetUp();

    strcpy(daemonize_result_, "daem-XXXXXX.pid");
    opae_close(mkstemps(daemonize_result_, 4));
  }

  virtual void TearDown() override {
    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(daemonize_result_);
    }

    opae_base_p<>::TearDown();
  }

  char daemonize_result_[20];
};

/**
 * @test       test
 * @brief      Test: daemonize
 * @details    daemonize places the process in daemon mode.<br>
 */
TEST_P(fpgad_daemonize_c_p, test) {
  pid_t pid = fork();

  ASSERT_NE(-1, pid);

  if (!pid) {
    // child
    int res;
    char cwd[PATH_MAX];

    res = daemonize(test_sig_handler, 0, getcwd(cwd, sizeof(cwd)));

    // pass the result of daemonize to the parent proc via the tmp file.
    FILE *fp = opae_fopen(daemonize_result_, "w");
    if (fp) {
      fprintf(fp, "%d\n", res);
      opae_fclose(fp);
    }

    exit(0);

  } else {
    // parent
    int res = -1;
    int timeout = 15;

    FILE *fp = opae_fopen(daemonize_result_, "r");
    ASSERT_NE(nullptr, fp);

    while (fscanf(fp, "%d", &res) != 1) {
      std::this_thread::sleep_for(std::chrono::milliseconds(250));
      rewind(fp);
      timeout--;
      if (!timeout)
	      opae_fclose(fp);
      ASSERT_GT(timeout, 0);
    }
    opae_fclose(fp);

    EXPECT_EQ(res, 0);
  }

}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(fpgad_daemonize_c_p);
INSTANTIATE_TEST_SUITE_P(fpgad_daemonize_c, fpgad_daemonize_c_p,
                         ::testing::ValuesIn(test_platform::platforms({ "skx-p" })));
