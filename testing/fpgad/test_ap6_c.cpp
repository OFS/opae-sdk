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

#include "fpgad/log.h"
#include "fpgad/ap6.h"

struct bitstream_info {
  const char *filename;
  uint8_t *data;
  size_t data_len;
  uint8_t *rbf_data;
  size_t rbf_len;
  fpga_guid interface_id;
};

int parse_metadata(struct bitstream_info *info);

fpga_result get_bitstream_ifc_id(const uint8_t *bitstream, fpga_guid *guid);

int read_bitstream(const char *filename, struct bitstream_info *info);

}

#include <config.h>
#include <opae/fpga.h>

#include <array>
#include <vector>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "gtest/gtest.h"
#include "test_system.h"
#include "safe_string/safe_string.h"

using namespace opae::testing;

class fpgad_ap6_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_ap6_c_p()
   : junk_gbs_(nullptr),
     port0_("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0"),
     fme0_("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0") {}

  virtual void SetUp() override {
    strcpy(tmpfpgad_log_, "tmpfpgad-XXXXXX.log");
    strcpy(tmpnull_gbs_, "tmpnull-XXXXXX.gbs");
    close(mkstemps(tmpfpgad_log_, 4));
    close(mkstemps(tmpnull_gbs_, 4));
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    fpgaInitialize(NULL);

    open_log(tmpfpgad_log_);
    int i;
    for (i = 0 ; i < MAX_SOCKETS ; ++i)
      sem_init(&ap6_sem[i], 0, 0);

    null_gbs_ = system_->assemble_gbs_header(platform_.devices[0]);

    std::ofstream gbs;
    gbs.open(tmpnull_gbs_, std::ios::out|std::ios::binary);
    gbs.write((const char *) null_gbs_.data(), null_gbs_.size());
    gbs.close();

    junk_gbs_ = (char*)malloc(9);
    ASSERT_TRUE(junk_gbs_) << "Error allocating buffer for junk.gbs";
    ASSERT_EQ(strncpy_s(junk_gbs_, 9, "junk.gbs", 9), 0) << "error copying string: junk.gbs";
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
      .null_gbs = { junk_gbs_, tmpnull_gbs_ },
      .num_null_gbs = 2,
    };
    strcpy(config_.logfile, tmpfpgad_log_);

    context_.config = &config_;
    context_.socket = 0;

    ap6_thread_ = std::thread(ap6_thread, &context_);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  virtual void TearDown() override {
    config_.running = false;
    ap6_thread_.join();

    int i;
    for (i = 0 ; i < MAX_SOCKETS ; ++i)
      sem_destroy(&ap6_sem[i]);

    close_log();

    system_->finalize();

    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(tmpfpgad_log_);
      unlink(tmpnull_gbs_);
    }
    if (junk_gbs_) {
      free(junk_gbs_);
      junk_gbs_ = nullptr;
    }
  }

  char *junk_gbs_;
  std::string port0_;
  std::string fme0_;
  struct config config_;
  struct ap6_context context_;
  char tmpfpgad_log_[20];
  char tmpnull_gbs_[20];
  std::vector<uint8_t> null_gbs_;
  std::thread ap6_thread_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       parse_err0
 * @brief      Test: parse_metadata
 * @details    When parse_metadata is given a null pointer,<br>
 *             the fn returns a negative value.<br>
 */
TEST_P(fpgad_ap6_c_p, parse_err0) {
  EXPECT_LT(parse_metadata(nullptr), 0);
}

/**
 * @test       parse_err1
 * @brief      Test: parse_metadata
 * @details    When the parameter to parse_metadata has a data_len field,<br>
 *             that is less than the size of a gbs header,<br>
 *             the fn returns a negative value.<br>
 */
TEST_P(fpgad_ap6_c_p, parse_err1) {
  struct bitstream_info info;
  info.data_len = 3;
  EXPECT_LT(parse_metadata(&info), 0);
}

/**
 * @test       parse_err2
 * @brief      Test: parse_metadata
 * @details    When the parameter to parse_metadata has a data field<br>
 *             with an invalid gbs magic value in the first 32 bits,<br>
 *             the fn returns a negative value.<br>
 */
TEST_P(fpgad_ap6_c_p, parse_err2) {
  struct bitstream_info info;
  uint32_t blah = 0;
  info.data_len = 20;
  info.data = (uint8_t *) &blah;
  EXPECT_LT(parse_metadata(&info), 0);
}

/**
 * @test       parse
 * @brief      Test: parse_metadata
 * @details    When parse_metadata is called with valid input<br>
 *             the fn returns zero.<br>
 */
TEST_P(fpgad_ap6_c_p, parse) {
  struct bitstream_info info;
  EXPECT_EQ(read_bitstream(tmpnull_gbs_, &info), 0);
  *(uint32_t *) info.data = 0x1d1f8680;
  EXPECT_EQ(parse_metadata(&info), 0);
  free(info.data);
}

/**
 * @test       get_bits_err0
 * @brief      Test: get_bitstream_ifc_id
 * @details    When get_bitstream_ifc_id is passed a NULL bitstream buffer,<br>
 *             the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(fpgad_ap6_c_p, get_bits_err0) {
  EXPECT_EQ(get_bitstream_ifc_id(nullptr, nullptr), FPGA_EXCEPTION);
}

/**
 * @test       get_bits_err1
 * @brief      Test: get_bitstream_ifc_id
 * @details    When malloc fails,<br>
 *             get_bitstream_ifc_id returns FPGA_NO_MEMORY.<br>
 */
TEST_P(fpgad_ap6_c_p, get_bits_err1) {
  struct bitstream_info info;
  EXPECT_EQ(read_bitstream(tmpnull_gbs_, &info), 0);

  fpga_guid guid;
  system_->invalidate_malloc(0, "get_bitstream_ifc_id");
  EXPECT_EQ(get_bitstream_ifc_id(info.data, &guid), FPGA_NO_MEMORY);
  free(info.data);
}

/**
 * @test       get_bits_err2
 * @brief      Test: get_bitstream_ifc_id
 * @details    When get_bitstream_ifc_id is passed a bitstream buffer,<br>
 *             and that buffer has a json data length field of zero,
 *             then the fn returns FPGA_OK.<br>
 */
TEST_P(fpgad_ap6_c_p, get_bits_err2) {
  struct bitstream_info info;
  EXPECT_EQ(read_bitstream(tmpnull_gbs_, &info), 0);

  fpga_guid guid;
  *(uint32_t *) (info.data + 16) = 0;
  EXPECT_EQ(get_bitstream_ifc_id(info.data, &guid), FPGA_OK);
  free(info.data);
}

/**
 * @test       get_bits_err3
 * @brief      Test: get_bitstream_ifc_id
 * @details    When get_bitstream_ifc_id is passed a bitstream buffer,<br>
 *             and that buffer has a json data length field of that is invalid,
 *             then the fn returns FPGA_EXCEPTION.<br>
 */
TEST_P(fpgad_ap6_c_p, get_bits_err3) {
  struct bitstream_info info;
  EXPECT_EQ(read_bitstream(tmpnull_gbs_, &info), 0);

  // TODO: Consolidate bitstream related functions into a library (internal
  // API)
  //  This test is disabled because get_bitstream_ifc_id needs the
  //  bitstream length to get a maximum lson metadata length.
  //  This function is also cloned so fixing it here would not be the
  //  right thing to do

  // ********************** DISABLED ********************************//
  //fpga_guid guid;
  //*(uint32_t *) (info.data + 16) = 65535;
  //EXPECT_EQ(get_bitstream_ifc_id(info.data, &guid), FPGA_EXCEPTION);
  free(info.data);
  // ********************** DISABLED ********************************//
}

/**
 * @test       read_bits_err0
 * @brief      Test: read_bitstream
 * @details    When given a NULL filename,<br>
 *             read_bitstream returns a negative value.<br>
 */
TEST_P(fpgad_ap6_c_p, read_bits_err0) {
  struct bitstream_info info;
  EXPECT_LT(read_bitstream(nullptr, &info), 0);
}

/**
 * @test       read_bits_err1
 * @brief      Test: read_bitstream
 * @details    When the filename is not found,<br>
 *             read_bitstream returns a negative value.<br>
 */
TEST_P(fpgad_ap6_c_p, read_bits_err1) {
  struct bitstream_info info;
  EXPECT_LT(read_bitstream("/doesnt/exist", &info), 0);
}

/**
 * @test       read_bits_err2
 * @brief      Test: read_bitstream
 * @details    When malloc fails,<br>
 *             read_bitstream returns a negative value.<br>
 */
TEST_P(fpgad_ap6_c_p, read_bits_err2) {
  struct bitstream_info info;
  system_->invalidate_malloc(0, "read_bitstream");
  EXPECT_LT(read_bitstream(tmpnull_gbs_, &info), 0);
}

/**
 * @test       read_bits
 * @brief      Test: read_bitstream
 * @details    When the parameters to read_bitstream are valid,<br>
 *             the function loads the given gbs file into the bitstream_info.<br>
 */
TEST_P(fpgad_ap6_c_p, read_bits) {
  struct bitstream_info info;
  EXPECT_EQ(read_bitstream(tmpnull_gbs_, &info), 0);
  free(info.data);
}

/**
 * @test       ap6
 * @brief      Test: ap6_thread
 * @details    When the ap6_sem corresponding to the context socket is signaled<br>
 *             and a valid NULL gbs has been provided,<br>
 *             ap6_thread PR's the socket.<br>
 */
TEST_P(fpgad_ap6_c_p, ap6) {
  sem_post(&ap6_sem[0]);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

INSTANTIATE_TEST_CASE_P(fpgad_ap6_c, fpgad_ap6_c_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p","skx-p-dfl0" })));
