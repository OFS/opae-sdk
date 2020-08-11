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

#include <stdarg.h>
#include <linux/ioctl.h>
#include <config.h>
#include "fpga-dfl.h"

#include "gtest/gtest.h"
#include "mock/test_system.h"
#include "mock/fpgad_control.h"
#include <opae/fpga.h>

#include <CLI/CLI.hpp>

#include "mmio.h"
#include "lpbk.h"
#include "ddr.h"

#include "test_afu.h"

const char *AFU_ID = "f7df405c-bd7a-cf72-22f1-44b0b93acd18";

using namespace opae::app;
using namespace opae::testing;

int mmio_ioctl(mock_object * m, int request, va_list argp){
    UNUSED_PARAM(m);
    UNUSED_PARAM(request);
    dfl_fpga_port_region_info *a = va_arg(argp, dfl_fpga_port_region_info*);
    a->flags = DFL_PORT_REGION_READ | DFL_PORT_REGION_WRITE | DFL_PORT_REGION_MMAP;
    a->size =0x4000;
    a->offset = 0;
    errno = 0;
    return 0;
}

class sleep_test : public test_command
{
public:
  sleep_test() : sleep_msec_(1000){}
  virtual ~sleep_test(){}
  virtual const char *name() const
  {
    return "sleep";
  }

  virtual const char *description() const
  {
    return "none";
  }

  virtual int run(test_afu *afu, CLI::App *app) override
  {
    (void)afu;
    (void)app;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_msec_));
    return 0;
  }

  virtual void add_options(CLI::App *app) override
  {
    app->add_option("-s,--sleep",
                    sleep_msec_, "sleep time (msec)")->default_val(sleep_msec_);

  }

private:
  uint32_t sleep_msec_;

};

class dummy_afu_p : public ::testing::TestWithParam<std::string>,
                    public fpgad_control {
 protected:
  dummy_afu_p()
  : app_(0)
  {
  }

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    EXPECT_EQ(fpgaInitialize(NULL), FPGA_OK);
    system_->register_ioctl_handler(DFL_FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
    app_ = new test_afu("dummy_afu", AFU_ID);
    app_->register_command<mmio_test>();
    app_->register_command<ddr_test>();
    app_->register_command<lpbk_test>();
    app_->register_command<sleep_test>();
    fpgad_start();
  }

  virtual void TearDown() override {
    fpgad_stop();
    delete app_;
    app_ = 0;
    fpgaFinalize();
    system_->finalize();
    clear_args();
  }

  void clear_args() {
    for (auto p : args_) free(p);
    args_.clear();
  }

  test_platform platform_;
  test_system *system_;
  test_afu *app_;
  std::vector<char*> args_;
};

/**
 * @test       main_noargs
 * @brief      Test: test main without argumenets
 * @details    Test the main entry point for dummy_afu
 */
TEST_P(dummy_afu_p, main_noargs) {
  args_.push_back(strdup("dummy_afu"));
  EXPECT_NE(0, app_->main(args_.size(), const_cast<char**>(args_.data())));
}

/*
 * @test       main_mmio
 * @brief      Test: test main with mmio subcommand
 * @details    Test the main entry point for dummy_afu
 */
TEST_P(dummy_afu_p, main_mmio) {
  args_.push_back(strdup("dummy_afu"));
  args_.push_back(strdup("mmio"));
  EXPECT_EQ(0, app_->main(args_.size(), const_cast<char**>(args_.data())));
}

/*
 * @test       main_mmio_count
 * @brief      Test: test main with mmio subcommand and count > 1
 * @details    Test the main entry point for dummy_afu
 */
TEST_P(dummy_afu_p, main_mmio_count) {
  args_.push_back(strdup("dummy_afu"));
  args_.push_back(strdup("-c"));
  args_.push_back(strdup("100"));
  args_.push_back(strdup("mmio"));
  testing::internal::CaptureStdout();
  EXPECT_EQ(0, app_->main(args_.size(), const_cast<char**>(args_.data())));
  auto s_out = testing::internal::GetCapturedStdout();
  EXPECT_NE(s_out.find("Test mmio(100): PASS"), std::string::npos);
}

/*
 * @test       main_sleep_timeout
 * @brief      Test: test main with sleep subcommand and timeout 100msec
 * @details    Test the main entry point for dummy_afu
 */
TEST_P(dummy_afu_p, main_sleep_timeout) {
  args_.push_back(strdup("dummy_afu"));
  args_.push_back(strdup("-t"));
  args_.push_back(strdup("100"));
  args_.push_back(strdup("sleep"));
  EXPECT_NE(0, app_->main(args_.size(), const_cast<char**>(args_.data())));
}

/*
 * @test       main_sleep_notimeout
 * @brief      Test: test main with sleep subcommand and timeout 100msec and
 *                   sleep time of 99msec
 * @details    Test the main entry point for dummy_afu
 */
TEST_P(dummy_afu_p, main_sleep_notimeout) {
  args_.push_back(strdup("dummy_afu"));
  args_.push_back(strdup("-t"));
  args_.push_back(strdup("1000"));
  args_.push_back(strdup("sleep"));
  args_.push_back(strdup("-s"));
  args_.push_back(strdup("100"));
  EXPECT_EQ(0, app_->main(args_.size(), const_cast<char**>(args_.data())));
}

/*
 * @test       main_ddr
 * @brief      Test: test main with ddr subcommand
 * @details    Test the main entry point for dummy_afu
 */
TEST_P(dummy_afu_p, main_ddr) {
  args_.push_back(strdup("dummy_afu"));
  args_.push_back(strdup("ddr"));
  EXPECT_EQ(4, app_->main(args_.size(), const_cast<char**>(args_.data())));
}

/*
 * @test       main_lpbk
 * @brief      Test: test main with lpbk command
 * @details    Test the main entry point for dummy_afu
 */
TEST_P(dummy_afu_p, main_lpbk) {
  args_.push_back(strdup("dummy_afu"));
  args_.push_back(strdup("lpbk"));
  EXPECT_EQ(4, app_->main(args_.size(), const_cast<char**>(args_.data())));
}

/**
 * @test       main_invalid_pci_addr
 * @brief      Test: main
 * @details    Given and invalid pcie_address
 *             When I run dummy_afu with this
 *             pcie_address in the command line
 *             I get not_found exit code
 */
TEST_P(dummy_afu_p, main_invalid_pci_addr) {
  args_.push_back(strdup("dummy_afu"));
  args_.push_back(strdup("-p"));
  args_.push_back(strdup("00:00.1"));
  args_.push_back(strdup("mmio"));
  EXPECT_EQ(test_afu::exit_codes::not_found,
            app_->main(args_.size(), const_cast<char**>(args_.data())));
}

/**
 * @test       main_invalid_guid
 * @brief      Test: main
 * @details    Given and invalid guid
 *             When I run dummy_afu with this
 *             guid in the command line
 *             I get not_found exit code
 */
TEST_P(dummy_afu_p, main_invalid_guid) {
  const char *guid = "8a38d6c4-d29c-11ea-9b96-005056ac13c8";
  args_.push_back(strdup("dummy_afu"));
  args_.push_back(strdup("-g"));
  args_.push_back(strdup(guid));
  args_.push_back(strdup("mmio"));
  EXPECT_EQ(test_afu::exit_codes::not_found,
            app_->main(args_.size(), const_cast<char**>(args_.data())));
}

INSTANTIATE_TEST_CASE_P(dummy_afu, dummy_afu_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({"dfl-d5005"})));

TEST(dummy_afu, parse_pcie_address)
{
  auto p = pcie_address::parse("1111:3b:19.4");
  EXPECT_EQ(p.fields.domain, 0x1111);
  EXPECT_EQ(p.fields.bus, 0x3b);
  EXPECT_EQ(p.fields.device, 0x19);
  EXPECT_EQ(p.fields.function, 4);
  p = pcie_address::parse("3b:00.1");
  EXPECT_EQ(p.fields.domain, 0x0000);
  EXPECT_EQ(p.fields.bus, 0x3b);
  EXPECT_EQ(p.fields.device, 0x0);
  EXPECT_EQ(p.fields.function, 1);
  EXPECT_THROW(pcie_address::parse("xy:11.g"), std::runtime_error);
}
