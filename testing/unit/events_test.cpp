// Copyright(c) 2017-2018, Intel Corporation
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

#ifdef __cplusplus

extern "C" {
#endif
#include "fpgad/config_int.h"
#include "fpgad/log.h"
#include "fpgad/srv.h"

extern fpga_result send_event_request(int conn_socket, int fd,
                                      struct event_request *req);
extern fpga_result send_fme_event_request(fpga_handle handle,
                                          fpga_event_handle event_handle,
                                          int fme_operation);
extern fpga_result send_port_event_request(fpga_handle handle,
                                           fpga_event_handle event_handle,
                                           int port_operation);
extern fpga_result send_uafu_event_request(fpga_handle handle,
                                           fpga_event_handle event_handle,
                                           uint32_t flags, int uafu_operation);
extern fpga_result check_interrupts_supported(fpga_handle handle,
                                              fpga_objtype *objtype);
extern fpga_result driver_register_event(fpga_handle handle,
                                         fpga_event_type event_type,
                                         fpga_event_handle event_handle,
                                         uint32_t flags);
extern fpga_result driver_unregister_event(fpga_handle handle,
                                           fpga_event_type event_type,
                                           fpga_event_handle event_handle);
extern fpga_result daemon_register_event(fpga_handle handle,
                                         fpga_event_type event_type,
                                         fpga_event_handle event_handle,
                                         uint32_t flags);
extern fpga_result daemon_unregister_event(fpga_handle handle,
                                           fpga_event_type event_type);
extern fpga_result fpgaCreateEventHandle(fpga_event_handle *event_handle);
extern fpga_result fpgaDestroyEventHandle(fpga_event_handle *event_handle);
extern fpga_result fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh,
                                                  int *fd);
extern fpga_result fpgaRegisterEvent(fpga_handle handle,
                                     fpga_event_type event_type,
                                     fpga_event_handle event_handle,
                                     uint32_t flags);
extern fpga_result fpgaUnregisterEvent(fpga_handle handle,
                                       fpga_event_type event_type,
                                       fpga_event_handle event_handle);

#ifdef __cplusplus
}
#endif
#include <opae/fpga.h>
#include <sys/epoll.h>
#include <thread>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class events_p : public ::testing::TestWithParam<std::string> {
 protected:
  events_p()
      : tmpsysfs_("mocksys-XXXXXX"),
        tmplog_("tmpfpgadlog-XXXXXX"),
        tmppid_("tmpfpgadpid-XXXXXX"),
        handle_(nullptr) {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    tmpsysfs_ = system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_),
              FPGA_OK);
    ASSERT_EQ(fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);

    tmplog_ = mkstemp(const_cast<char *>(tmplog_.c_str()));
    tmppid_ = mkstemp(const_cast<char *>(tmppid_.c_str()));

    config_ = {
        .verbosity = 0,
        .poll_interval_usec = 100 * 1000,
        .daemon = 0,
        .directory = ".",
        .logfile = tmplog_.c_str(),
        .pidfile = tmppid_.c_str(),
        .filemode = 0,
        .running = true,
        .socket = "/tmp/fpga_event_socket",
        .null_gbs = {0},
        .num_null_gbs = 0,
    };
    open_log(tmplog_.c_str());
    fpgad_ = std::thread(server_thread, &config_);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (handle_ != nullptr) EXPECT_EQ(fpgaClose(handle_), FPGA_OK);
    if (!tmpsysfs_.empty() && tmpsysfs_.size() > 1) {
      std::string cmd = "rm -rf " + tmpsysfs_;
      std::system(cmd.c_str());
    }
    system_->finalize();
    config_.running = false;
    fpgad_.join();
  }

  struct config config_;
  std::string tmpsysfs_;
  std::string tmplog_;
  std::string tmppid_;
  std::string tmpsocket_;
  fpga_properties filter_;
  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
  std::thread fpgad_;
  int epollfd_;
};

TEST_P(events_p, register_events) {
  fpga_event_type e = FPGA_EVENT_ERROR;
  fpga_event_handle eh;
  fpga_result res;
  ASSERT_EQ(res = fpgaCreateEventHandle(&eh), FPGA_OK) << "\tRESULT: "
                                                       << fpgaErrStr(res);
  ASSERT_EQ(res = fpgaRegisterEvent(handle_, FPGA_EVENT_ERROR, eh, 0), FPGA_OK)
      << "\tRESULT: " << fpgaErrStr(res);

  epollfd_ = epoll_create(1);
  struct epoll_event ev;
  ev.events = EPOLLIN;
  EXPECT_EQ(epoll_ctl(epollfd_, EPOLL_CTL_ADD, ev.data.fd, &ev), 0);
  ASSERT_EQ(res = fpgaGetOSObjectFromEventHandle(eh, &ev.data.fd), FPGA_OK)
      << "\tRESULT: " << fpgaErrStr(res);


  struct epoll_event events[1];
  auto num_events = epoll_wait(epollfd_, events, 1, 1000);

  EXPECT_EQ(res = fpgaUnregisterEvent(handle_, FPGA_EVENT_ERROR, eh), FPGA_OK)
      << "\tRESULT: " << fpgaErrStr(res);
  EXPECT_EQ(res = fpgaDestroyEventHandle(&eh), FPGA_OK) << "\tRESULT: "
                                                        << fpgaErrStr(res);
}

// TEST(event_c, send_event_request) {
//   int conn_socket = 0;
//   int fd = 0;
//   struct event_request *req = 0;
//   auto res = send_event_request(conn_socket, fd, req);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST(event_c, send_fme_event_request) {
//   fpga_handle handle = 0;
//   fpga_event_handle event_handle = 0;
//   int fme_operation = 0;
//   auto res = send_fme_event_request(handle, event_handle, fme_operation);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST(event_c, send_port_event_request) {
//   fpga_handle handle = 0;
//   fpga_event_handle event_handle = 0;
//   int port_operation = 0;
//   auto res = send_port_event_request(handle, event_handle, port_operation);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST(event_c, send_uafu_event_request) {
//   fpga_handle handle = 0;
//   fpga_event_handle event_handle = 0;
//   uint32_t flags = 0;
//   int uafu_operation = 0;
//   auto res =
//       send_uafu_event_request(handle, event_handle, flags, uafu_operation);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST(event_c, check_interrupts_supported) {
//   fpga_handle handle = 0;
//   fpga_objtype *objtype = 0;
//   auto res = check_interrupts_supported(handle, objtype);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST(event_c, driver_register_event) {
//   fpga_handle handle = 0;
//   fpga_event_type event_type = 0;
//   fpga_event_handle event_handle = 0;
//   uint32_t flags = 0;
//   auto res = driver_register_event(handle, event_type, event_handle, flags);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST(event_c, driver_unregister_event) {
//   fpga_handle handle = 0;
//   fpga_event_type event_type = 0;
//   fpga_event_handle event_handle = 0;
//   auto res = driver_unregister_event(handle, event_type, event_handle);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST(event_c, daemon_register_event) {
//   fpga_handle handle = 0;
//   fpga_event_type event_type = 0;
//   fpga_event_handle event_handle = 0;
//   uint32_t flags = 0;
//   auto res = daemon_register_event(handle, event_type, event_handle, flags);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST(event_c, daemon_unregister_event) {
//   fpga_handle handle = 0;
//   fpga_event_type event_type = 0;
//   auto res = daemon_unregister_event(handle, event_type);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST(event_c, fpgaCreateEventHandle) {
//   fpga_event_handle *event_handle = 0;
//   auto res = fpgaCreateEventHandle(event_handle);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST(event_c, fpgaDestroyEventHandle) {
//   fpga_event_handle *event_handle = 0;
//   auto res = fpgaDestroyEventHandle(event_handle);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST(event_c, fpgaGetOSObjectFromEventHandle) {
//   const fpga_event_handle eh = 0;
//   int *fd = 0;
//   auto res = fpgaGetOSObjectFromEventHandle(eh, fd);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST(event_c, fpgaRegisterEvent) {
//   fpga_handle handle = 0;
//   fpga_event_type event_type = 0;
//   fpga_event_handle event_handle = 0;
//   uint32_t flags = 0;
//   auto res = fpgaRegisterEvent(handle, event_type, event_handle, flags);
//   EXPECT_EQ(res, FPGA_OK);
// }
//
// TEST(event_c, fpgaUnregisterEvent) {
//   fpga_handle handle = 0;
//   fpga_event_type event_type = 0;
//   fpga_event_handle event_handle = 0;
//   auto res = fpgaUnregisterEvent(handle, event_type, event_handle);
//   EXPECT_EQ(res, FPGA_OK);
// }

INSTANTIATE_TEST_CASE_P(test_system, events_p,
                        ::testing::ValuesIn(test_platform::keys()));
