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

#include <unistd.h>
#include "mock/opae_fixtures.h"

using namespace opae::testing;

class buffer_c_p : public opae_p<> {
 public:

  virtual void SetUp() override {
    opae_p::SetUp();
    pg_size_ = (size_t) sysconf(_SC_PAGE_SIZE);
  }

  virtual fpga_properties accelerator_filter(fpga_token parent) const override {
    fpga_properties filter = opae_p<>::accelerator_filter(parent);

    if (!filter)
      return nullptr;

    auto device_id = platform_.devices[0].device_id;
    if (platform_.devices[0].num_vfs) {
      device_id++;
    }

    if (fpgaPropertiesSetDeviceID(filter, device_id) != FPGA_OK) {
      fpgaDestroyProperties(&filter);
      return nullptr;
    }

    return filter;
  }

 protected:
  size_t pg_size_;
};

/**
 * @test       prep_rel
 * @brief      Test: fpgaPrepareBuffer, fpgaReleaseBuffer
 * @details    When fpgaPrepareBuffer retrieves a valid buffer pointer and wsid,<br>
 *             then a subsequent call to fpgaReleaseBuffer with the wsid,<br>
 *             also returns FPGA_OK.<br>
 */
TEST_P(buffer_c_p, prep_rel) {
  void *buf_addr = nullptr;
  uint64_t wsid = 0xabadbeef;
  ASSERT_EQ(fpgaPrepareBuffer(accel_, (uint64_t) pg_size_,
                              &buf_addr, &wsid, 0), FPGA_OK);
  EXPECT_NE(buf_addr, nullptr);
  EXPECT_NE(wsid, 0xabadbeef);
  EXPECT_EQ(fpgaReleaseBuffer(accel_, wsid), FPGA_OK);
}

/**
 * @test       ioaddr
 * @brief      Test: fpgaGetIOAddress
 * @details    When called with a valid wsid,<br>
 *             fpgaGetIOAddress retrieves the IO address for the wsid<br>
 *             and returns FPGA_OK.<br>
 */
TEST_P(buffer_c_p, ioaddr) {
  void *buf_addr = nullptr;
  uint64_t wsid = 0;
  uint64_t io = 0xdecafbadbeefdead;
  ASSERT_EQ(fpgaPrepareBuffer(accel_, (uint64_t) pg_size_,
                              &buf_addr, &wsid, 0), FPGA_OK);
  EXPECT_EQ(fpgaGetIOAddress(accel_, wsid, &io), FPGA_OK);
  EXPECT_NE(io, 0xdecafbadbeefdead);
  EXPECT_EQ(fpgaReleaseBuffer(accel_, wsid), FPGA_OK);
}

/**
 * @test       fpgaGetIOAddress
 * @brief      Test: fpgaGetIOAddress_neg_test
 * @details    When called with a invalid fpga handle,<br>
 *             the method returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(buffer_c_p, ioaddr_neg_test) {
  void *buf_addr = nullptr;
  uint64_t wsid = 0;
  uint64_t io = 0xdecafbadbeefdead;
  ASSERT_EQ(fpgaPrepareBuffer(accel_, (uint64_t) pg_size_,
                              &buf_addr, &wsid, 0), FPGA_OK);
  EXPECT_EQ(fpgaGetIOAddress(NULL, wsid, &io), FPGA_INVALID_PARAM);
}

/**
 * @test       fpgaPrepareBuffer_neg_test
 * @brief      Test: fpgaPrepareBuffer
 * @details    When fpgaPrepareBuffer called with null fpga handle,<br>
 *             it returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(buffer_c_p, neg_test1) {
  void *buf_addr = nullptr;
  uint64_t wsid = 0;
  ASSERT_EQ(fpgaPrepareBuffer(NULL, (uint64_t) pg_size_,
                              &buf_addr, &wsid, 0), FPGA_INVALID_PARAM);
}

/**
 * @test       fpgaReleaseBuffer_neg_test
 * @brief      Test: fpgaReleaseBuffer
 * @details    When fpgaReleaseBuffer called with null fpga handle,<br>
 *             it returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(buffer_c_p, neg_test2) {
  uint64_t wsid = 0;
  EXPECT_EQ(fpgaReleaseBuffer(NULL, wsid), FPGA_INVALID_PARAM);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(buffer_c_p);
INSTANTIATE_TEST_SUITE_P(buffer_c, buffer_c_p,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-d5005",
                                                                        "dfl-n3000",
                                                                        "dfl-n6000-sku0",
                                                                        "dfl-n6000-sku1",
                                                                        "dfl-c6100"
                                                                      })));
