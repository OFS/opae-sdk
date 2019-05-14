// Copyright(c) 2018-2019, Intel Corporation
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
#include <libgen.h>

char *find_ase_cfg();
void opae_init(void);
void opae_release(void);

#define HOME_CFG_PATHS 3
extern const char *_ase_home_cfg_files[HOME_CFG_PATHS];
}

#include <config.h>
#include <opae/fpga.h>

#include <array>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stack>
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

  opae_release();
  EXPECT_EQ(0, unsetenv("LIBOPAE_LOG"));
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

  opae_release();
  EXPECT_EQ(0, unsetenv("LIBOPAE_LOG"));
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

  opae_release();
  EXPECT_EQ(0, unsetenv("LIBOPAE_LOG"));
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

  opae_release();
  EXPECT_EQ(0, unsetenv("LIBOPAE_LOGFILE"));
  unlink("opae_log.log");
}

/**
 * @test       find_ase_cfg
 *
 * @brief      When WITH_ASE is specified, opae_ase.cfg will
 *             be searched from OPAE source directory, OPAE
 *             installation directory or home/system config directory.
 *             
 */
TEST(init, find_ase_cfg) {
  char *cfg_path = nullptr;

  ASSERT_EQ(0, putenv((char*)"WITH_ASE=1"));
  cfg_path = find_ase_cfg();
  EXPECT_NE(cfg_path, nullptr);

  EXPECT_EQ(0, unsetenv("WITH_ASE"));
}

class init_ase_cfg_p : public ::testing::TestWithParam<const char*> {
 protected:
  init_ase_cfg_p() : buffer_ {0} {}

  virtual void SetUp() override {
    // let's rename the opae_ase.cfg in OPAE_ASE_CFG_SRC_PATH and OPAE_ASE_CFG_INST_PATH

    // copy it to a temporary buffer that we can use dirname with
    std::string src_cfg_path = OPAE_ASE_CFG_SRC_PATH;
    std::copy(src_cfg_path.begin(), src_cfg_path.end(), &buffer_[0]);
    char *src_cfg_dir = dirname(buffer_);
    char *src_cfg_file = basename(buffer_);

    std::string cfg_dir = src_cfg_dir;
    src_cfg_file_ = cfg_dir + std::string(src_cfg_file) + std::string(".backup");
    rename(OPAE_ASE_CFG_SRC_PATH, src_cfg_file_.c_str());

    // This parameterized test iterates over the possible config file paths
    // relative to a user's home directory

    // let's build the full path by prepending the parameter with $HOME
    char *home_cstr = getenv("HOME");
    ASSERT_NE(home_cstr, nullptr) << "No home environment found";
    std::string home = home_cstr;
    // the parameter paths start with a '/'
    cfg_file_ = home + std::string(GetParam());
    // copy it to a temporary buffer that we can use dirname with
    std::copy(cfg_file_.begin(), cfg_file_.end(), &buffer_[0]);
    // get the directory name of the file
    cfg_dir_ = dirname(buffer_);
    struct stat st;
    // if the directory doesn't exist, create the entire path
    if (stat(cfg_dir_, &st)) {
      std::string dir = cfg_dir_;
      // find the first '/' after $HOME
      int pos = dir.find('/', home.size());
      while (pos != std::string::npos) {
        std::string sub = dir.substr(0, pos);
        // sub is $HOME/<dir1>, then $HOME/<dir1>/<dir2>, ...
        // if this directory doesn't exist, create it
        if (stat(sub.c_str(), &st) && sub != "") {
          ASSERT_EQ(mkdir(sub.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH),
                    0)
              << "Error creating subdirectory (" << sub
              << "}: " << strerror(errno);
          // keep track of directories created
          dirs_.push(sub);
        }
        pos = pos < dir.size() ? dir.find('/', pos + 1) : std::string::npos;
      }
      // finally, we know the entire path didn't exist, create the last
      // directory
      ASSERT_EQ(mkdir(cfg_dir_, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0)
          << "Error creating subdirectory (" << cfg_dir_
          << "}: " << strerror(errno);
      dirs_.push(cfg_dir_);
    }

    if (stat(cfg_file_.c_str(), &st) == 0) {
      unlink(cfg_file_.c_str());
    }

    setenv("WITH_ASE", "1", 0);
  }

  virtual void TearDown() override {
    unsetenv("WITH_ASE");
    unlink(cfg_file_.c_str());
    // remove any directories we created in SetUp
    while (!dirs_.empty()) {
      unlink(dirs_.top().c_str());
      dirs_.pop();
    }
    // restore the opae_ase.cfg file at OPAE_ASE_CFG_SRC_PATH
    rename(src_cfg_file_.c_str(), (char *)OPAE_ASE_CFG_SRC_PATH);
  }

  char buffer_[PATH_MAX];
  std::string cfg_file_;
  char *cfg_dir_;
  std::stack<std::string> dirs_;
  std::string src_cfg_file_;
};


/**
 * @test       find_ase_cfg_2
 * @brief      Test: find_ase_cfg at OPAE_ASE_CFG_INST_PATH and OPAE_PLATFORM_ROOT
 *             release location <br>
 * @details    After renaming a configuration file located in the OPAE_ASE_CFG_SRC_PATH
 *             and OPAE_ASE_CFG_INST_PATH<br>
 *             When I call find_ase_cfg
 *             Then the call is successful<br>
 */
TEST_P(init_ase_cfg_p, find_ase_cfg_2) {
    char *cfg_file = nullptr;
    std::string inst_cfg_file_;

    // find_ase_cfg at OPAE_ASE_CFG_INST_PATH
    cfg_file = find_ase_cfg();
    EXPECT_NE(cfg_file, nullptr);

    // copy it to a temporary buffer that we can use dirname with
    std::string inst_cfg_path = OPAE_ASE_CFG_INST_PATH;
    std::copy(inst_cfg_path.begin(), inst_cfg_path.end(), &buffer_[0]);
    char *inst_cfg_dir = dirname(buffer_);
    char *inst_cfg_file = basename(buffer_);

   // rename opae_ase.cfg under releae directory
    std::string cfg_dir = inst_cfg_dir;
    inst_cfg_file_ = cfg_dir + std::string(inst_cfg_file) + std::string(".backup");
    rename(OPAE_ASE_CFG_INST_PATH, inst_cfg_file_.c_str());

    // find_ase_cfg at release directory
    cfg_file = find_ase_cfg();
    EXPECT_NE(cfg_file, nullptr);
    rename(inst_cfg_file_.c_str(), OPAE_ASE_CFG_INST_PATH);
}

/**
 * @test       find_ase_cfg_3
 * @brief      Test: find_ase_cfg at HOME location
 * @details    After renaming a configuration file located in the OPAE_ASE_CFG_SRC_PATH,
 *             OPAE_ASE_CFG_INST_PATH and OPAE release directory<br>
 *             When I call find_ase_cfg
 *             Then the call is successful<br>
 */
TEST_P(init_ase_cfg_p, find_ase_cfg_3) {
    char *cfg_file = nullptr;
    char *opae_path;
    std::string  inst_cfg_file_, opae_cfg_file_;
    std::string  rel_cfg_file_, rel_cfg_file2_;

    // copy it to a temporary buffer that we can use dirname with
    std::string inst_cfg_path = OPAE_ASE_CFG_INST_PATH;
    std::copy(inst_cfg_path.begin(), inst_cfg_path.end(), &buffer_[0]);
    char *inst_cfg_dir = dirname(buffer_);
    char *inst_cfg_file = basename(buffer_);
    std::string cfg_dir = inst_cfg_dir;
    inst_cfg_file_ = cfg_dir + std::string(inst_cfg_file) + std::string(".backup");
    rename(OPAE_ASE_CFG_INST_PATH, inst_cfg_file_.c_str());

    // rename opae_ase.cfg under releae directory
    opae_path = getenv("OPAE_PLATFORM_ROOT");
    std::string rel_cfg_path = opae_path;
    rel_cfg_file_ = rel_cfg_path + std::string("/share/opae/ase/opae_ase.cfg");
    rel_cfg_file2_ = rel_cfg_path + std::string("/share/opae/ase/opae_ase.cfg.backup");
    rename(rel_cfg_file_.c_str(), rel_cfg_file2_.c_str());

    // find_ase_cfg at HOME directory
    cfg_file = find_ase_cfg();
    EXPECT_NE(cfg_file, nullptr);

    rename(rel_cfg_file2_.c_str(), rel_cfg_file_.c_str());  // restore file
    rename(inst_cfg_file_.c_str(), OPAE_ASE_CFG_INST_PATH);  // restore file
}

/**
 * @test       find_ase_cfg_4
 * @brief      Test: find_ase_cfg at OPAE_PLATFORM_ROOT release location
 * @details    After renaming a configuration file located in the OPAE_ASE_CFG_SRC_PATH,
 *             OPAE_ASE_CFG_INST_PATH, OPAE release and HOME directories<br>
 *             When I call find_ase_cfg
 *             Then the call is successful<br>
 */
TEST_P(init_ase_cfg_p, find_ase_cfg_4) {
    char *cfg_file = nullptr;
    char *opae_path;
    std::string  inst_cfg_file_, opae_cfg_file_;
    std::string  rel_cfg_file_, rel_cfg_file2_;

    // copy it to a temporary buffer that we can use dirname with
    std::string inst_cfg_path = OPAE_ASE_CFG_INST_PATH;
    std::copy(inst_cfg_path.begin(), inst_cfg_path.end(), &buffer_[0]);
    char *inst_cfg_dir = dirname(buffer_);
    char *inst_cfg_file = basename(buffer_);
    std::string cfg_dir = inst_cfg_dir;
    inst_cfg_file_ = cfg_dir + std::string(inst_cfg_file) + std::string(".backup");
    rename(OPAE_ASE_CFG_INST_PATH, inst_cfg_file_.c_str());

    // rename the opae_ase.cfg under release directory 
    opae_path = getenv("OPAE_PLATFORM_ROOT");
    std::string rel_cfg_path = opae_path;
    rel_cfg_file_ = rel_cfg_path + std::string("/share/opae/ase/opae_ase.cfg");
    rel_cfg_file2_ = rel_cfg_path + std::string("/share/opae/ase/opae_ase.cfg.backup");
    rename(rel_cfg_file_.c_str(), rel_cfg_file2_.c_str());

    // find_ase_cfg at HOME directory
    cfg_file = find_ase_cfg();
    EXPECT_NE(cfg_file, nullptr);

    opae_init();

    rename(rel_cfg_file2_.c_str(), rel_cfg_file_.c_str());  // restore file
    rename(inst_cfg_file_.c_str(), OPAE_ASE_CFG_INST_PATH);  // restore file
}

INSTANTIATE_TEST_CASE_P(init_ase_cfg, init_ase_cfg_p,
                        ::testing::ValuesIn(_ase_home_cfg_files));