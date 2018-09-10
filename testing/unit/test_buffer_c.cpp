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
#include <opae/fpga.h>

#ifdef __cplusplus

extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#include <tuple>
#include "gtest/gtest.h"
#include "test_system.h"
#include "intel-fpga.h"
#include <linux/ioctl.h>

struct buffer_params {
  fpga_result result;
  size_t size;
  int flags;
};

using namespace opae::testing;

class buffer_prepare
    : public ::testing::TestWithParam<std::tuple<std::string, buffer_params>> {
 protected:
  buffer_prepare() : tmpsysfs_("mocksys-XXXXXX"), handle_(nullptr) {}

  virtual void SetUp() override {
    auto tpl = GetParam();
    std::string platform_key = std::get<0>(tpl);
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
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (handle_ != nullptr) EXPECT_EQ(fpgaClose(handle_), FPGA_OK);
    if (!tmpsysfs_.empty() && tmpsysfs_.size() > 1) {
      std::string cmd = "rm -rf " + tmpsysfs_;
      std::system(cmd.c_str());
    }
    system_->finalize();
  }

  std::string tmpsysfs_;
  fpga_properties filter_;
  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};

// TEST_P(buffer_prepare, buffer_allocate) {
//  void **addr = 0;
//  uint64_t len = 0;
//  int flags = 0;
//  auto res = buffer_allocate(addr, len, flags);
//  EXPECT_EQ(res, FPGA_OK);
//}
//
// TEST_P(buffer_prepare, buffer_release) {
//  void *addr = 0;
//  uint64_t len = 0;
//  auto res = buffer_release(addr, len);
//  EXPECT_EQ(res, FPGA_OK);
//}

TEST_P(buffer_prepare, prepare_buf_err) {
  uint64_t buf_len = 1024;
  uint64_t* buf_addr;
  uint64_t wsid = 1;
  int flags = 0;
  uint64_t *ioaddr = NULL;
  uint64_t* invalid_buf_addr = NULL;
  
  // NULL Handle
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(NULL, 0, (void**) &buf_addr, &wsid, 0));

  // NULL wsid 
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(handle_, 0, (void**) &buf_addr, NULL, flags));


  // Invlaid Flags
  flags = 0x100;
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(handle_, buf_len, (void**) &buf_addr, &wsid, flags));
  
  // Buffer lenth is zero
  flags = FPGA_BUF_PREALLOCATED;
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(handle_, 0, (void**) &buf_addr, &wsid, flags));

  // Not Page aligned buffer
  buf_len = 11247;
  flags = FPGA_BUF_PREALLOCATED;
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(handle_, buf_len, (void**) &buf_addr, &wsid, flags));
  
  // Invalid input buffer pointer
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(handle_, buf_len, (void**) &invalid_buf_addr, &wsid, flags));

  // special test case
  EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(handle_, 0, (void**) NULL, &wsid, flags));
  
  // Buffer lenth is zero
  flags = FPGA_BUF_QUIET;
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(handle_, 0, (void**) NULL, &wsid, flags));
  
  // Invalid Handle
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaGetIOAddress(NULL, wsid, ioaddr));
  
  // Invalid workspace id
  EXPECT_NE(FPGA_OK, fpgaGetIOAddress(handle_, 0x10000, ioaddr));
  
  // NULL Handle
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReleaseBuffer(NULL, wsid));
  
  // Invalid workspace id
  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaReleaseBuffer(handle_, 0x10001));

}


//TEST_P(buffer_prepare, port_dma_map_err) {
//  uint64_t buf_len = 1024;
//  uint64_t* buf_addr;
//  uint64_t wsid = 1;
//  int flags = 0;
// 
//  system_->register_ioctl_handler(FPGA_PORT_DMA_MAP,dummy_ioctl<-1,EINVAL>);
//  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(handle_, 0, (void**)&buf_addr, &wsid, flags));
//
//  system_->register_ioctl_handler(FPGA_PORT_DMA_MAP,dummy_ioctl<-1,EFAULT>);
//  EXPECT_EQ(FPGA_INVALID_PARAM, fpgaPrepareBuffer(handle_, 0, (void**)&buf_addr, &wsid, flags));
//
//  system_->register_ioctl_handler(FPGA_PORT_DMA_MAP,dummy_ioctl<-1,ENOTSUP>);
//  EXPECT_EQ(FPGA_EXCEPTION, fpgaPrepareBuffer(handle_, 0, (void**)&buf_addr, &wsid, flags));
// 
//}


//TEST_P(buffer_prepare, port_dma_unmap_err) {
//  uint64_t buf_len = 1024;
//  uint64_t* buf_addr;
//  uint64_t wsid = 1;
//  int flags = 0;
// 
//  EXPECT_EQ(FPGA_OK, fpgaPrepareBuffer(handle_, 0, (void**)&buf_addr, &wsid, flags));
//  system_->register_ioctl_handler(FPGA_PORT_DMA_MAP,dummy_ioctl<-1,EINVAL>);
//  EXPECT_EQ(FPGA_INVALID_PARAM,fpgaReleaseBuffer(handle_, wsid));
//
//  system_->register_ioctl_handler(FPGA_PORT_DMA_MAP,dummy_ioctl<-1,EFAULT>);
//  EXPECT_EQ(FPGA_INVALID_PARAM,fpgaReleaseBuffer(handle_, wsid));
//
//}



TEST_P(buffer_prepare, fpgaPrepareBuffer) {
  buffer_params params = std::get<1>(GetParam());
  void *buf_addr = 0;
  uint64_t wsid = 0;
  uint64_t ioaddr = 0;
  auto res =
      fpgaPrepareBuffer(handle_, params.size, &buf_addr, &wsid, params.flags);

  EXPECT_EQ(res, params.result) << "result is " << fpgaErrStr(res);
  if (params.size > 0 && params.result == FPGA_OK) {
    EXPECT_EQ(res = fpgaGetIOAddress(handle_, wsid, &ioaddr), FPGA_OK)
        << "result is " << fpgaErrStr(res);
    EXPECT_EQ(res = fpgaReleaseBuffer(handle_, wsid), FPGA_OK)
        << "result is " << fpgaErrStr(res);
  }
}

namespace {
std::vector<buffer_params> params{
    buffer_params{FPGA_INVALID_PARAM, 0, 0},
    buffer_params{FPGA_OK, KiB(1), 0},
    buffer_params{FPGA_OK, KiB(4), 0},
    buffer_params{FPGA_OK, MiB(1), 0},
    buffer_params{FPGA_OK, MiB(2), 0},
    buffer_params{FPGA_INVALID_PARAM, 11247, FPGA_BUF_PREALLOCATED}};
}

INSTANTIATE_TEST_CASE_P(
    buffer_c, buffer_prepare,
    ::testing::Combine(::testing::ValuesIn(test_platform::keys()),
                       ::testing::ValuesIn(params)));
