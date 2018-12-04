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

#define __STDC_FORMAT_MACROS

extern "C" {

#include <json-c/json.h>
#include <uuid/uuid.h>

#include "fpgad/log.h"
#include "fpgad/srv.h"
#include "fpgad/errtable.h"
#include "fpgad/ap6.h"

struct client_event_registry *register_event(int conn_socket, int fd,
	                                     fpga_event_type e, uint64_t object_id);

void unregister_all_events(void);

void *logger_thread(void *thread_context);

int log_fpga_error(monitored_device *d, struct fpga_err *e);

int poll_error(monitored_device *d, struct fpga_err *e);

}

#include <config.h>
#include <opae/fpga.h>

#include <array>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <inttypes.h>
#include <sys/eventfd.h>
#include <poll.h>
#include <dlfcn.h>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class fpgad_errtable_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_errtable_c_p()
   : port0_("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0"),
     fme0_("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0") {}

  virtual void SetUp() override {
    strcpy(tmpfpgad_log_, "tmpfpgad-XXXXXX.log");
    strcpy(tmpfpgad_pid_, "tmpfpgad-XXXXXX.pid");
    close(mkstemps(tmpfpgad_log_, 4));
    close(mkstemps(tmpfpgad_pid_, 4));
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(FPGA_OK, fpgaInitialize(NULL));
    port0_obj_id_ = 0;
    fme0_obj_id_ = 0;

    open_log(tmpfpgad_log_);
    int i;
    for (i = 0 ; i < MAX_SOCKETS ; ++i)
      sem_init(&ap6_sem[i], 0, 0);

    config_ = {
      .verbosity = 0,
      .poll_interval_usec = 1000 * 1000,
      .daemon = 0,
      .directory = { 0, },
      .logfile = { 0, },
      .pidfile = { 0, },
      .filemode = 0,
      .running = true,
      .socket = "/tmp/fpga_event_socket",
      .null_gbs = {0},
      .num_null_gbs = 0,
    };
    strcpy(config_.logfile, tmpfpgad_log_);
    strcpy(config_.pidfile, tmpfpgad_pid_);

    logger_thread_ = std::thread(logger_thread, &config_);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  bool do_poll(int fd)
  {
    struct pollfd pollfd;

    pollfd.fd = fd;
    pollfd.events = POLLIN;

    int res = poll(&pollfd, 1, 1000);

    return (res == 1) && ((pollfd.revents & POLLIN) != 0);
  }

  void cause_ap6_port0()
  {
    std::string fname = port0_ + "/errors/errors";
    uint64_t ap6_bit = ((uint64_t)1 << 50);
    FILE *fp = fopen(fname.c_str(), "w");    
    ASSERT_NE(nullptr, fp);
    EXPECT_GT(fprintf(fp, "0x%lx\n", ap6_bit), 0);
    fclose(fp);
  }

  void clear_errors_port0()
  {
    std::string fname = port0_ + "/errors/errors";
    FILE *fp = fopen(fname.c_str(), "w");    
    ASSERT_NE(nullptr, fp);
    EXPECT_GT(fprintf(fp, "0\n"), 0);
    fclose(fp);
  }

  void cause_ktilinkfatal_fme0()
  {
    std::string fname = fme0_ + "/errors/fatal_errors";
    std::ofstream err(system_->get_sysfs_path(fname));
    ASSERT_TRUE(err.is_open());
    err << "0x1\n";
    err.close();
  }

  void clear_errors_fme0()
  {
    std::string fname = fme0_ + "/errors/fatal_errors";
    FILE *fp = fopen(fname.c_str(), "w");    
    ASSERT_NE(nullptr, fp);
    EXPECT_GT(fprintf(fp, "0\n"), 0);
    fclose(fp);
  }

  virtual void TearDown() override {
    config_.running = false;
    logger_thread_.join();

    unregister_all_events();

    int i;
    for (i = 0 ; i < MAX_SOCKETS ; ++i)
      sem_destroy(&ap6_sem[i]);

    close_log();

    system_->finalize();

    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(tmpfpgad_log_);
      unlink(tmpfpgad_pid_);
    }
  }

  void GetPort0ObjID();
  void GetFME0ObjID();

  std::string port0_;
  std::string fme0_;
  uint64_t port0_obj_id_;
  uint64_t fme0_obj_id_;
  struct config config_;
  char tmpfpgad_log_[20];
  char tmpfpgad_pid_[20];
  std::thread logger_thread_;
  test_platform platform_;
  test_system *system_;
};

void fpgad_errtable_c_p::GetPort0ObjID()
{
  fpga_token tok = nullptr;
  fpga_properties prop = nullptr;
  uint32_t matches = 0;

  ASSERT_EQ(FPGA_OK, fpgaGetProperties(nullptr, &prop));

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(prop, FPGA_ACCELERATOR));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetSocketID(prop, 0));

  ASSERT_EQ(FPGA_OK, fpgaEnumerate(&prop, 1, &tok, 1, &matches));
  EXPECT_EQ(1, matches);
  ASSERT_NE(nullptr, tok);

  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&prop));
  ASSERT_EQ(FPGA_OK, fpgaGetProperties(tok, &prop));

  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetObjectID(prop, &port0_obj_id_));

  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&prop));
  EXPECT_EQ(FPGA_OK, fpgaDestroyToken(&tok));
}

void fpgad_errtable_c_p::GetFME0ObjID()
{
  fpga_token tok = nullptr;
  fpga_properties prop = nullptr;
  uint32_t matches = 0;

  ASSERT_EQ(FPGA_OK, fpgaGetProperties(nullptr, &prop));

  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetObjectType(prop, FPGA_DEVICE));
  EXPECT_EQ(FPGA_OK, fpgaPropertiesSetSocketID(prop, 0));

  ASSERT_EQ(FPGA_OK, fpgaEnumerate(&prop, 1, &tok, 1, &matches));
  EXPECT_EQ(1, matches);
  ASSERT_NE(nullptr, tok);

  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&prop));
  ASSERT_EQ(FPGA_OK, fpgaGetProperties(tok, &prop));

  EXPECT_EQ(FPGA_OK, fpgaPropertiesGetObjectID(prop, &fme0_obj_id_));

  EXPECT_EQ(FPGA_OK, fpgaDestroyProperties(&prop));
  EXPECT_EQ(FPGA_OK, fpgaDestroyToken(&tok));
}

/**
 * @test       logger_ap6_ktilinkfatal
 * @brief      Test: logger_thread
 * @details    Verify logger_thread's ability to signal an eventfd<br>
 *             for an AP6 and for a KtiLinkFatalErr.<br>
 */
TEST_P(fpgad_errtable_c_p, logger_ap6_ktilinkfatal) {
  int conn_sockets[2] = { 0, 1 };
  int evt_fds[2]      = { eventfd(0, 0), eventfd(0, 0) };

  GetPort0ObjID();
  GetFME0ObjID();

  uint64_t obj_ids[2] = { port0_obj_id_,  // for AP6
	                  fme0_obj_id_ }; // for KtiLinkFatalErr 

  EXPECT_NE(nullptr, register_event(conn_sockets[0], evt_fds[0], FPGA_EVENT_POWER_THERMAL, obj_ids[0]));
  EXPECT_NE(nullptr, register_event(conn_sockets[1], evt_fds[1], FPGA_EVENT_ERROR, obj_ids[1]));

  cause_ap6_port0();
  ASSERT_TRUE(do_poll(evt_fds[0]));
  clear_errors_port0();

#if 0
  cause_ktilinkfatal_fme0();
  ASSERT_TRUE(do_poll(evt_fds[1]));
  clear_errors_fme0();
#endif

  close(evt_fds[0]);
  close(evt_fds[1]);
}

/**
 * @test       sysfs_read_err03
 * @brief      Test: poll_error
 * @details    When fpgad_sysfs_read_u64 fails,<br>
 *             poll_error returns -1.<br>
 */
TEST_P(fpgad_errtable_c_p, sysfs_read_err03) {
#if 0
  std::string fname = port0_ + "/errors/errors";
  struct fpga_err err = {
    .socket = -1,
    .sysfsfile = fname.c_str(),
    .reg_field = "reg_field",
    .lowbit = 0,
    .highbit = 1,
    .occurred = true,
    .callback = nullptr
  };
  system_->invalidate_read(0, "fpgad_sysfs_read_u64");
  EXPECT_EQ(-1, poll_error(&err));
#endif
}

INSTANTIATE_TEST_CASE_P(fpgad_errtable_c, fpgad_errtable_c_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({"skx-p"})));

/**
 * @test       log_fpga_err
 * @brief      Test: log_fpga_error
 * @details    When the input error struct's occurred field is true,<br>
 *             log_fpga_error returns 0.<br>
 */
TEST(errtable_c, log_fpga_err) {
#if 0
  struct fpga_err err = {
    .sysfsfile = "devc",
    .reg_field = "reg_field",
    .lowbit = 0,
    .highbit = 1,
    .callback = nullptr
  };

  EXPECT_EQ(log_fpga_error(&err), 0);
#endif
}
