// Copyright(c) 2017-2020, Intel Corporation
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

extern "C" {
    fpga_result buffer_allocate(void*,uint64_t,int);
    fpga_result buffer_release(void*,uint64_t);
    int xfpga_plugin_initialize(void);
    int xfpga_plugin_finalize(void);
}

#include "error_int.h"
#include "common_int.h"
#include <tuple>
#include "xfpga.h"
#include "gtest/gtest.h"
#include "mock/test_system.h"
#include "intel-fpga.h"
#include "fpga-dfl.h"
#include <linux/ioctl.h>
#include <cstdarg>
#include "types_int.h"
#include <sys/mman.h>
#include <opae/buffer.h>
#include <opae/mmio.h>
#include <string>
#include <algorithm>


#define NLB_DSM_SIZE (2 * 1024 * 1024)
#define KB 1024
#define MB (1024 * KB)
#define GB (1024UL * MB)
#define FPGA_MOCK_IOVA 0xDECAFBADDEADBEEF
#undef FPGA_MSG
#define FPGA_MSG(fmt, ...) \
	printf("MOCK " fmt "\n", ## __VA_ARGS__)

#pragma pack(push, 1)
struct buffer_params {
  fpga_result result;
  size_t size;
  int flags;
};
#pragma pack(pop)



using namespace opae::testing;

int dma_map_ioctl(mock_object * m, int request, va_list argp){
    UNUSED_PARAM(m);
    UNUSED_PARAM(request);
    int retval = -1;
    errno = EINVAL;
    struct fpga_port_dma_map *dma_map = va_arg(argp, struct fpga_port_dma_map *);
    if (!dma_map) {
    	FPGA_MSG("dma_map is NULL");
    	goto out_EINVAL;
    }
    if (dma_map->argsz != sizeof(*dma_map)) {
    	FPGA_MSG("wrong structure size");
    	goto out_EINVAL;
    }
    if (!dma_map->user_addr) {
    	FPGA_MSG("mapping address is NULL");
    	goto out_EINVAL;
    }
    /* TODO: check alignment */
    if (dma_map->length == 0) {
    	FPGA_MSG("mapping size is 0");
    	goto out_EINVAL;
    }
    dma_map->iova = FPGA_MOCK_IOVA; /* return something */
out:
    return retval;

out_EINVAL:
    retval = -1;
    errno = EINVAL;
    goto out;
}

int dma_unmap_ioctl(mock_object * m, int request, va_list argp){
    UNUSED_PARAM(m);
    UNUSED_PARAM(request);
    int retval = -1;
    errno = EINVAL;
    struct fpga_port_dma_unmap *dma_unmap = va_arg(argp, struct fpga_port_dma_unmap *);
    if (!dma_unmap) {
    	FPGA_MSG("dma_unmap is NULL");
    	goto out_EINVAL;
    }
    if (dma_unmap->argsz != sizeof(*dma_unmap)) {
    	FPGA_MSG("wrong structure size");
    	goto out_EINVAL;
    }
    if (dma_unmap->iova != FPGA_MOCK_IOVA) {
    	FPGA_MSG("unexpected IOVA (0x%llx)", dma_unmap->iova);
    	goto out_EINVAL;
    }
    retval = 0;
    errno = 0;
out:
    return retval;

out_EINVAL:
    retval = -1;
    errno = EINVAL;
    goto out;
}


class buffer_prepare : public ::testing::TestWithParam<std::tuple<std::string, buffer_params>> {
 protected:
  buffer_prepare()
  : tokens_{{nullptr, nullptr}},
    handle_(nullptr) {}

  virtual void SetUp() override {
    auto tpl = GetParam();
    std::string platform_key = std::get<0>(tpl);
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    ASSERT_EQ(xfpga_plugin_initialize(), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_), FPGA_OK);
    ASSERT_GT(num_matches_, 0);
    ASSERT_EQ(xfpga_fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);

    for (auto &t : tokens_) {
        if (t) {
            EXPECT_EQ(FPGA_OK,xfpga_fpgaDestroyToken(&t));
            t = nullptr;
        }
    }

    if (handle_ != nullptr) { 
      EXPECT_EQ(xfpga_fpgaClose(handle_), FPGA_OK); 
      handle_ = nullptr;
    }

    xfpga_plugin_finalize();
    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       PrepPre2MB01
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             with pre-allocated buffer, fpgaPrepareBuffer must
 *             allocate a shared memory buffer. fpgaReleaseBuffer must
 *             release a shared memory buffer.
 *
 */
TEST_P(buffer_prepare, PrepPre2MB01) {
  uint64_t buf_len;
  uint64_t* buf_addr = nullptr;
  uint64_t wsid;

  // Allocate buffer in MB range
  buf_len = 2 * 1024 * 1024;
  buf_addr = (uint64_t*)mmap(ADDR, buf_len, PROTECTION, FLAGS_2M, 0, 0);
  EXPECT_EQ(FPGA_OK, xfpga_fpgaPrepareBuffer(handle_, buf_len, (void**)&buf_addr, &wsid,
                                       FPGA_BUF_PREALLOCATED));

  // Release buffer in MB range
  EXPECT_EQ(FPGA_OK, xfpga_fpgaReleaseBuffer(handle_, wsid));

  // buf_addr was preallocated, do not touch it
  ASSERT_NE(buf_addr, (void*)nullptr);
  munmap(buf_addr, buf_len);
}

TEST_P(buffer_prepare, prepare_buf_err) {
  uint64_t buf_len = 1024;
  uint64_t* buf_addr = nullptr;
  uint64_t wsid;
  int flags = 0;
  uint64_t *ioaddr = nullptr;
  uint64_t* invalid_buf_addr = nullptr;

  // NULL Handle
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaPrepareBuffer(nullptr, 0, (void**) &buf_addr, &wsid, 0));

  // NULL wsid
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaPrepareBuffer(handle_, 0, (void**) &buf_addr, nullptr, flags));

  // Invlaid Flags
  flags = 0x100;
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaPrepareBuffer(handle_, buf_len, (void**) &buf_addr, &wsid, flags));

  // Buffer lenth is zero
  flags = FPGA_BUF_PREALLOCATED;
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaPrepareBuffer(handle_, 0, (void**) &buf_addr, &wsid, flags));

  // Not Page aligned buffer
  buf_len = 11247;
  flags = FPGA_BUF_PREALLOCATED;
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaPrepareBuffer(handle_, buf_len, (void**) &buf_addr, &wsid, flags));

  // Invalid input buffer pointer
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaPrepareBuffer(handle_, buf_len, (void**) &invalid_buf_addr, &wsid, flags));

  // special test case
  EXPECT_EQ(FPGA_OK, xfpga_fpgaPrepareBuffer(handle_, 0, (void**) nullptr, &wsid, flags));

  // Buffer lenth is zero
  flags = FPGA_BUF_QUIET;
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaPrepareBuffer(handle_, 0, (void**) nullptr, &wsid, flags));

  // Invalid Handle
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetIOAddress(nullptr, wsid, ioaddr));

  // Invalid workspace id
  EXPECT_NE(FPGA_OK, xfpga_fpgaGetIOAddress(handle_, 0x10000, ioaddr));

  // NULL Handle
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaReleaseBuffer(nullptr, wsid));

  // Invalid workspace id
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaReleaseBuffer(handle_, 0x10001));
}

TEST_P(buffer_prepare, xfpga_fpgaPrepareBuffer) {
  buffer_params params = std::get<1>(GetParam());
  void *buf_addr = nullptr;
  uint64_t wsid = 0;
  uint64_t ioaddr = 0;
  auto res = xfpga_fpgaPrepareBuffer(handle_, params.size, (void **)&buf_addr, &wsid, params.flags);

  EXPECT_EQ(res, params.result) << "result is " << fpgaErrStr(res);
  if (params.size > 0 && params.result == FPGA_OK) {
    EXPECT_EQ(res = xfpga_fpgaGetIOAddress(handle_, wsid, &ioaddr), FPGA_OK)
        << "result is " << fpgaErrStr(res);
    EXPECT_EQ(res = xfpga_fpgaReleaseBuffer(handle_, wsid), FPGA_OK)
        << "result is " << fpgaErrStr(res);
  }
}

/**
 * @test       release_neg
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             fpgaReleaseBuffer must fail if fpga_buffer was not
 *             prepared.
 *
 */
TEST_P(buffer_prepare, release_neg) {
  uint64_t wsid= 1;

  EXPECT_EQ(xfpga_fpgaReleaseBuffer(handle_, wsid), FPGA_INVALID_PARAM);
}

/**
 * @test       not_aligned
 *
 * @brief      When FPGA_BUF_PREALLOCATED is not given and the buffer
 *             len is not a multiple of the page size, fpgaPrepareBuffer
 *             allocates the next multiple of page size and returns
 *             FPGA_OK.
 *
 */
TEST_P(buffer_prepare, not_aligned) {
  uint64_t buf_len = (4 * 1024) - 1;
  void *buf_addr = nullptr;
  uint64_t wsid = 1;
  int flags = 0;

  EXPECT_EQ(xfpga_fpgaPrepareBuffer(handle_, buf_len, &buf_addr, &wsid, flags),
            FPGA_OK);

  EXPECT_EQ(xfpga_fpgaReleaseBuffer(handle_, wsid), FPGA_OK);
}

/**
 * @test       write_read
 *
 * @brief      When the parameters are valid and the drivers are loaded:
 *             Test writing and reading to/from a shared memory buffer.
 *
 */
TEST_P(buffer_prepare, write_read) {
  uint64_t buf_len = NLB_DSM_SIZE;
  void *buf_addr = nullptr;
  uint64_t wsid = 2;
  int flags = 0;
  uint64_t offset;
  uint64_t value;

  // Allocate buffer
  ASSERT_EQ(xfpga_fpgaPrepareBuffer(handle_, buf_len, &buf_addr, &wsid, flags),
            FPGA_OK);

  // Write test
  memset(buf_addr, 0, buf_len);

  for (offset = 0; offset < buf_len - sizeof(uint64_t);
       offset += sizeof(uint64_t)) {
    value = offset;
    *((volatile uint64_t*)((uint64_t)buf_addr + offset)) = value;
    EXPECT_EQ(*((volatile uint64_t*)((uint64_t)buf_addr + offset)), offset);
  }

  // Release buffer
  EXPECT_EQ(xfpga_fpgaReleaseBuffer(handle_, wsid), FPGA_OK);
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

INSTANTIATE_TEST_CASE_P(buffer_c, buffer_prepare,
                        ::testing::Combine(::testing::ValuesIn(test_platform::keys()),
                                           ::testing::ValuesIn(params)));

class buffer_c_mock_p : public ::testing::TestWithParam<std::string> {
 protected:
  buffer_c_mock_p()
  : tokens_{{nullptr, nullptr}},
    handle_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    ASSERT_EQ(xfpga_plugin_initialize(), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_), FPGA_OK);
    ASSERT_GT(num_matches_, 0);
    ASSERT_EQ(xfpga_fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);

    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(FPGA_OK,xfpga_fpgaDestroyToken(&t));
        t = nullptr;
      }
    }

    if (handle_ != nullptr) { 
      EXPECT_EQ(xfpga_fpgaClose(handle_), FPGA_OK); 
      handle_ = nullptr;
    }

    xfpga_plugin_finalize();
    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};

TEST_P(buffer_c_mock_p, port_dma_unmap) {
  void *buf_addr = nullptr;
  uint64_t wsid = 0;
  uint64_t buf_len = KiB(1);
  auto res = xfpga_fpgaPrepareBuffer(handle_, buf_len, (void **)&buf_addr, &wsid, 0);
  EXPECT_EQ(res, FPGA_OK);

  system_->register_ioctl_handler(FPGA_PORT_DMA_UNMAP, dummy_ioctl<-1,EINVAL>);
  system_->register_ioctl_handler(DFL_FPGA_PORT_DMA_UNMAP, dummy_ioctl<-1, EINVAL>);
  EXPECT_EQ(res = xfpga_fpgaReleaseBuffer(handle_, wsid), FPGA_INVALID_PARAM)
        << "result is " << fpgaErrStr(res);

  buf_addr = nullptr;
}

TEST_P(buffer_c_mock_p, port_dma_map) {
  void *buf_addr = nullptr;
  uint64_t wsid = 0;
  uint64_t buf_len = KiB(1);

  system_->register_ioctl_handler(FPGA_PORT_DMA_MAP, dummy_ioctl<-1,EINVAL>);
  system_->register_ioctl_handler(DFL_FPGA_PORT_DMA_MAP, dummy_ioctl<-1, EINVAL>);
  auto res = xfpga_fpgaPrepareBuffer(handle_, buf_len, (void **)&buf_addr, &wsid, 0);
  EXPECT_EQ(res, FPGA_INVALID_PARAM) << "result is " << fpgaErrStr(res);
}

INSTANTIATE_TEST_CASE_P(buffer_c, buffer_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms()));
