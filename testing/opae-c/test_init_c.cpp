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

extern "C" {

#include <json-c/json.h>
#include <uuid/uuid.h>
#include "opae_int.h"

void opae_init(void);
void opae_release(void);
}

#include <config.h>
#include <opae/fpga.h>

#include <array>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

/**
 * @test       opae_init
 * @brief      Test: opae_init, opae_release, opae_print
 */
TEST(init, opae_init_rel) {
  opae_init();
  opae_print(OPAE_LOG_ERROR, "OPAE_LOG_ERROR from test opae_init_rel\n");
  opae_print(OPAE_LOG_MESSAGE, "OPAE_LOG_MESSAGE from test opae_init_rel\n");
  opae_print(OPAE_LOG_DEBUG, "OPAE_LOG_DEBUG from test opae_init_rel\n");
  opae_release();
}

#ifdef LIBOPAE_DEBUG

/**
 * @test       log_debug
 *
 * @brief      When the log level is set to debug, then all errors,
 *             messages, and debug info are logged.
 */
TEST(init, log_debug) {
  ASSERT_EQ(0, putenv((char*)"LIBOPAE_LOG=2"));
  opae_init();
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();

  OPAE_ERR("Error log.");
  OPAE_MSG("Message log.");
  OPAE_DBG("Debug log.");

  std::string log_stdout = testing::internal::GetCapturedStdout();
  std::string log_stderr = testing::internal::GetCapturedStderr();

  EXPECT_TRUE(log_stderr.find("Error log.") != std::string::npos);
  EXPECT_TRUE(log_stdout.find("Message log.") != std::string::npos);
  EXPECT_TRUE(log_stdout.find("Debug log.") != std::string::npos);
}

#endif

/**
 * @test       log_message
 *
 * @brief      When the log level is set to message, then all errors
 *             and messages are logged.
 */
TEST(init, log_message) {
  ASSERT_EQ(0, putenv((char*)"LIBOPAE_LOG=1"));
  opae_init();
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();

  OPAE_ERR("Error log.");
  OPAE_MSG("Message log.");
  OPAE_DBG("Debug log.");

  std::string log_stdout = testing::internal::GetCapturedStdout();
  std::string log_stderr = testing::internal::GetCapturedStderr();

  EXPECT_TRUE(log_stderr.find("Error log.") != std::string::npos);
  EXPECT_TRUE(log_stdout.find("Message log.") != std::string::npos);
  EXPECT_FALSE(log_stdout.find("Debug log.") != std::string::npos);
}

/**
 * @test       log_error
 *
 * @brief      When the log level is set to error, then only errors
 *             are logged.
 */
TEST(init, log_error) {
  ASSERT_EQ(0, putenv((char*)"LIBOPAE_LOG=0"));
  opae_init();
  testing::internal::CaptureStdout();
  testing::internal::CaptureStderr();

  OPAE_ERR("Error log.");
  OPAE_MSG("Message log.");
  OPAE_DBG("Debug log.");

  std::string log_stdout = testing::internal::GetCapturedStdout();
  std::string log_stderr = testing::internal::GetCapturedStderr();

  EXPECT_TRUE(log_stderr.find("Error log.") != std::string::npos);
  EXPECT_FALSE(log_stdout.find("Message log.") != std::string::npos);
  EXPECT_FALSE(log_stdout.find("Debug log.") != std::string::npos);
}

/**
 * @test       log_file
 *
 * @brief      When LIBOPAE_LOGFILE is specified, then the logger
 *             will log to the specified file.
 */
TEST(init, log_file) {
  struct stat buf;

  EXPECT_NE(0, stat("opae_log.log", &buf));

  ASSERT_EQ(0, putenv((char*)"LIBOPAE_LOGFILE=opae_log.log"));
  opae_init();

  EXPECT_EQ(0, stat("opae_log.log", &buf));
}

