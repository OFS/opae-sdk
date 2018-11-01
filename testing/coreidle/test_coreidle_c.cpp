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

#include <opae/fpga.h>
#include <sched.h>

extern "C" {

#include <json-c/json.h>
#include <uuid/uuid.h>

fpga_result sysfs_read_u64(const char *path, uint64_t *u);

int readmsr(int split_point, uint64_t msr, uint64_t *value);

fpga_result set_cpu_core_idle(fpga_handle handle, uint64_t gbs_power);

fpga_result get_package_power(int split_point, long double *pkg_power);

fpga_result setaffinity(cpu_set_t *idle_set, int socket,
                        int pid, int split_point);

fpga_result cpuset_setaffinity(int socket, int split_point,
                               uint64_t max_cpu_count);

}

#include <config.h>
#include <opae/fpga.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <array>
#include <string>
#include <fstream>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class coreidle_coreidle_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  coreidle_coreidle_c_p()
  : tokens_{{nullptr, nullptr}},
    fme0_("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0") {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    num_matches_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_), FPGA_OK);
    EXPECT_GT(num_matches_, 0);
    device_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_[0], &device_, 0), FPGA_OK);
  }

  uint64_t get_xeon_limit() {
    std::string fname = fme0_ + "/power_mgmt/xeon_limit";
    uint64_t xeon_limit = 0;
    std::ifstream fin(system_->get_sysfs_path(fname));
    if (fin.is_open()){
        fin >> xeon_limit;
        fin.close();
        return xeon_limit;
    }
    else {return -1;}
  }

  void set_xeon_limit(uint64_t xeon_limit) {
    std::string fname = fme0_ + "/power_mgmt/xeon_limit";
    std::ofstream fout(system_->get_sysfs_path(fname));
    ASSERT_TRUE(fout.is_open());
    fout << "0x" << std::hex << xeon_limit << std::endl;
    fout.close();
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (device_) {
      EXPECT_EQ(fpgaClose(device_), FPGA_OK);
      device_ = nullptr;
    }
    for (auto &t : tokens_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }

    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_;
  std::string fme0_;
  fpga_handle device_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       sys_read0
 * @brief      Test: sysfs_read_u64
 * @details    When given a NULL path string,<br>
 *             sysfs_read_u64 returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(coreidle_coreidle_c_p, sys_read0) {
  uint64_t u;
  EXPECT_EQ(sysfs_read_u64(NULL, &u), FPGA_INVALID_PARAM);
}

/**
 * @test       sys_read1
 * @brief      Test: sysfs_read_u64
 * @details    When given a path string to a non-existent file,<br>
 *             sysfs_read_u64 returns FPGA_NOT_FOUND.<br>
 */
TEST_P(coreidle_coreidle_c_p, sys_read1) {
  uint64_t u;
  EXPECT_EQ(sysfs_read_u64("/doesnt/exist", &u), FPGA_NOT_FOUND);
}

/**
 * @test       readmsr0
 * @brief      Test: readmsr
 * @details    When passed a NULL value pointer,<br>
 *             readmsr returns a negative value.<br>
 */
TEST_P(coreidle_coreidle_c_p, readmsr0) {
  EXPECT_LT(readmsr(0, 0, nullptr), 0);
}

/**
 * @test       set_cpu0
 * @brief      Test: set_cpu_core_idle
 * @details    When the gbs_power parameter is zero,<br>
 *             and the device is a TDP+ SKU,<br>
 *             set_cpu_core_idle calculates a value for gbs_power,<br>
 *             and the fn returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(coreidle_coreidle_c_p, set_cpu0) {
  EXPECT_EQ(set_cpu_core_idle(device_, 0), FPGA_INVALID_PARAM);
}

/**
 * @test       set_cpu1
 * @brief      Test: set_cpu_core_idle
 * @details    When the gbs_power parameter is out of range,<br>
 *             set_cpu_core_idle returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(coreidle_coreidle_c_p, set_cpu1) {
  EXPECT_EQ(set_cpu_core_idle(device_, 9999), FPGA_INVALID_PARAM);
}


/**
 * @test       pkg_pow0
 * @brief      Test: get_package_power
 * @details    When passed a NULL pkg_power pointer,<br>
 *             get_package_pwr returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(coreidle_coreidle_c_p, pkg_pow0) {
  EXPECT_EQ(get_package_power(0, nullptr), FPGA_INVALID_PARAM);
}

/**
 * @test       setaff0
 * @brief      Test: setaffinity
 * @details    When passed a NULL idle_set pointer,<br>
 *             setaffinity returns FPGA_INVALID_PARAM.<br>
 */
TEST_P(coreidle_coreidle_c_p, setaff0) {
  EXPECT_EQ(setaffinity(nullptr, 0, 0, 0), FPGA_INVALID_PARAM);
}


/**
 * @test       cpu_setaff0
 * @brief      Test: cpuset_setaffinity
 * @details    When passed an invalid socket parameter,<br>
 *             cpuset_setaffinity returns FPGA_NOT_SUPPORTED.<br>
 */
TEST_P(coreidle_coreidle_c_p, cpu_setaff0) {
  EXPECT_EQ(cpuset_setaffinity(2, 0, 0), FPGA_NOT_SUPPORTED);
}

/**
 * @test       cpu_setaff3
 * @brief      Test: cpuset_setaffinity
 * @details    When called with valid parameters,<br>
 *             cpuset_setaffinity returns FPGA_OK.<br>
 */
TEST_P(coreidle_coreidle_c_p, cpu_setaff3) {
  EXPECT_EQ(cpuset_setaffinity(0, 0, 0), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(coreidle_coreidle_c, coreidle_coreidle_c_p,
                        ::testing::ValuesIn(test_platform::platforms({"skx-p"})));

class coreidle_coreidle_c_mock_p : public coreidle_coreidle_c_p {
  protected:
    coreidle_coreidle_c_mock_p() {}
};


/**
 * @test       setaff2
 * @brief      Test: setaffinity
 * @details    When pid <= 2 and sched_setaffinity fails,<br>
 *             setaffinity returns FGPA_NOT_SUPPORTED.<br>
 */
TEST_P(coreidle_coreidle_c_mock_p, setaff2) {
  cpu_set_t idle;
  CPU_ZERO(&idle);
  system_->hijack_sched_setaffinity(-1, 0, "setaffinity");
  EXPECT_EQ(setaffinity(&idle, 0, 2, 0), FPGA_NOT_SUPPORTED);
}

/**
 * @test       setaff1
 * @brief      Test: setaffinity
 * @details    When pid > 2 and sched_setaffinity fails,<br>
 *             setaffinity returns FGPA_OK.<br>
 */
TEST_P(coreidle_coreidle_c_mock_p, setaff1) {
  cpu_set_t idle;
  CPU_ZERO(&idle);
  system_->hijack_sched_setaffinity(-1, 0, "setaffinity");
  EXPECT_EQ(setaffinity(&idle, 0, 3, 0), FPGA_OK);
}

/**
 * @test       cpu_setaff1
 * @brief      Test: cpuset_setaffinity
 * @details    When sched_setaffinity fails for pid 1,<br>
 *             cpuset_setaffinity returns FPGA_NOT_SUPPORTED.<br>
 */
TEST_P(coreidle_coreidle_c_mock_p, cpu_setaff1) {
  system_->hijack_sched_setaffinity(-1, 0, "setaffinity");
  EXPECT_EQ(cpuset_setaffinity(0, 0, 0), FPGA_NOT_SUPPORTED);
}

/**
 * @test       cpu_setaff2
 * @brief      Test: cpuset_setaffinity
 * @details    When sched_setaffinity fails for pid 2,<br>
 *             cpuset_setaffinity returns FPGA_NOT_SUPPORTED.<br>
 */
TEST_P(coreidle_coreidle_c_mock_p, cpu_setaff2) {
  system_->hijack_sched_setaffinity(-1, 1, "setaffinity");
  EXPECT_EQ(cpuset_setaffinity(0, 0, 0), FPGA_NOT_SUPPORTED);
}

/**
 * @test       sys_read2
 * @brief      Test: sysfs_read_u64
 * @details    When read fails,<br>
 *             sysfs_read_u64 returns FPGA_NOT_FOUND.<br>
 */
TEST_P(coreidle_coreidle_c_mock_p, sys_read2) {
  uint64_t u;
  system_->invalidate_read(0, "sysfs_read_u64");
  EXPECT_EQ(sysfs_read_u64("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0/errors/errors", &u), FPGA_NOT_FOUND);
}

/**
 * @test       set_cpu2
 * @brief      Test: set_cpu_core_idle
 * @details    When the xeon_pwr_limit + fpga_pwr_limit > total_power,<br>
 *             (indicating shared TDP SKU),<br>
 *             set_cpu_core_idle sets the CPU affinity appropriately,<br>
 *             and the fn returns FPGA_OK.<br>
 */
TEST_P(coreidle_coreidle_c_mock_p, set_cpu2) {
  uint64_t xeon_limit = get_xeon_limit();
  set_xeon_limit(9999);
  EXPECT_EQ(set_cpu_core_idle(device_, 25), FPGA_OK);
  set_xeon_limit(xeon_limit);
}

INSTANTIATE_TEST_CASE_P(coreidle_coreidle_c, coreidle_coreidle_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({"skx-p"})));


