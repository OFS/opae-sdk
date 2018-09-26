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
#include "pluginmgr.h"

int opae_plugin_mgr_initialize_all(void);
opae_api_adapter_table *opae_plugin_mgr_alloc_adapter(const char *lib_path);
int opae_plugin_mgr_free_adapter(opae_api_adapter_table *adapter);
int opae_plugin_mgr_register_adapter(opae_api_adapter_table *adapter);
int opae_plugin_mgr_for_each_adapter
	(int (*callback)(const opae_api_adapter_table *, void *), void *context);
int opae_plugin_mgr_configure_plugin(opae_api_adapter_table *adapter,
				     const char *config);

extern opae_api_adapter_table *adapter_list;
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
 * @test       alloc_adapter01
 * @brief      Test: opae_plugin_mgr_alloc_adapter
 * @details    When the given library name is not found,<br>
 *             opae_plugin_mgr_alloc_adapter returns NULL.<br>
 */
TEST(pluginmgr, alloc_adapter01) {
  EXPECT_EQ(NULL, opae_plugin_mgr_alloc_adapter("libthatdoesntexist.so"));
}

/**
 * @test       alloc_adapter02
 * @brief      Test: opae_plugin_mgr_alloc_adapter
 * @details    When calloc fails,<br>
 *             opae_plugin_mgr_alloc_adapter returns NULL.<br>
 */
TEST(pluginmgr, alloc_adapter02) {
  test_system::instance()->invalidate_calloc(0, "opae_plugin_mgr_alloc_adapter");
  EXPECT_EQ(NULL, opae_plugin_mgr_alloc_adapter("libxfpga.so"));
}

/**
 * @test       free_adapter01
 * @brief      Test: opae_plugin_mgr_free_adapter
 * @details    opae_plugin_mgr_free_adapter frees the given adapter table<br>
 *             and returns 0 on success.<br>
 */
TEST(pluginmgr, free_adapter) {
  opae_api_adapter_table *at;
  at = opae_plugin_mgr_alloc_adapter("libxfpga.so");
  ASSERT_NE(nullptr, at);
  EXPECT_EQ(0, opae_plugin_mgr_free_adapter(at));
}

/**
 * @test       config_err
 * @brief      Test: opae_plugin_mgr_configure_plugin
 * @details    When opae_plugin_mgr_configure_plugin is called on a load library<br>
 *             that has no opae_plugin_configure symbol,<br>
 *             then the fn returns non-zero.<br>
 */
TEST(pluginmgr, config_err) {
  opae_api_adapter_table *at;
  at = opae_plugin_mgr_alloc_adapter("libopae-c.so");
  ASSERT_NE(nullptr, at);
  EXPECT_NE(0, opae_plugin_mgr_configure_plugin(at, ""));
  EXPECT_EQ(0, opae_plugin_mgr_free_adapter(at));
}

extern "C" {

static int test_plugin_initialize_called;
static int test_plugin_initialize(void)
{
  ++test_plugin_initialize_called;
  return 0;
}

static int test_plugin_bad_initialize(void)
{
  ++test_plugin_initialize_called;
  return 1;
}

static int test_plugin_finalize_called;
static int test_plugin_finalize(void)
{
  ++test_plugin_finalize_called;
  return 0;
}

static int test_plugin_bad_finalize(void)
{
  ++test_plugin_finalize_called;
  return 1;
}

}

class pluginmgr_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  pluginmgr_c_p() {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    invalid_device_ = test_device::unknown();

    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    // save the global adapter list.
    adapter_list_ = adapter_list;
    adapter_list = nullptr;

    test_plugin_initialize_called = 0;
    test_plugin_finalize_called = 0;

    faux_adapter0_ = opae_plugin_mgr_alloc_adapter("libxfpga.so");
    ASSERT_NE(nullptr, faux_adapter0_);

    faux_adapter0_->initialize = test_plugin_initialize;
    faux_adapter0_->finalize = test_plugin_finalize;
    EXPECT_EQ(0, opae_plugin_mgr_register_adapter(faux_adapter0_));

    faux_adapter1_ = opae_plugin_mgr_alloc_adapter("libxfpga.so");
    ASSERT_NE(nullptr, faux_adapter1_);

    faux_adapter1_->initialize = test_plugin_initialize;
    faux_adapter1_->finalize = test_plugin_finalize;
    EXPECT_EQ(0, opae_plugin_mgr_register_adapter(faux_adapter1_));
  }

  virtual void TearDown() override {
    // restore the global adapter list.
    adapter_list = adapter_list_;
    
    system_->finalize();
  }

  opae_api_adapter_table *adapter_list_;
  opae_api_adapter_table *faux_adapter0_;
  opae_api_adapter_table *faux_adapter1_;
  test_platform platform_;
  test_device invalid_device_;
  test_system *system_;
};

/**
 * @test       foreach_err
 * @brief      Test: opae_plugin_mgr_for_each_adapter
 * @details    When opae_plugin_mgr_for_each_adapter is passed a NULL callback,<br>
 *             then the fn returns OPAE_ENUM_STOP.<br>
 */
TEST_P(pluginmgr_c_p, foreach_err) {
  EXPECT_EQ(OPAE_ENUM_STOP, opae_plugin_mgr_for_each_adapter(nullptr, nullptr));

  EXPECT_EQ(0, opae_plugin_mgr_finalize_all());
  EXPECT_EQ(nullptr, adapter_list);
  EXPECT_EQ(2, test_plugin_finalize_called);
}

/**
 * @test       bad_init_all
 * @brief      Test: opae_plugin_mgr_initialize_all
 * @details    When any of the registered adapters' initialize fn returns non-zero,<br>
 *             then opae_plugin_mgr_initialize_all returns non-zero.<br>
 */
TEST_P(pluginmgr_c_p, bad_init_all) {
  faux_adapter1_->initialize = test_plugin_bad_initialize;
  EXPECT_NE(0, opae_plugin_mgr_initialize_all());
  EXPECT_EQ(2, test_plugin_initialize_called);

  EXPECT_EQ(0, opae_plugin_mgr_finalize_all());
  EXPECT_EQ(nullptr, adapter_list);
  EXPECT_EQ(2, test_plugin_finalize_called);
}

/**
 * @test       bad_final_all
 * @brief      Test: opae_plugin_mgr_finalize_all
 * @details    When any of the registered adapters' finalize fn returns non-zero,<br>
 *             then opae_plugin_mgr_finalize_all returns non-zero.<br>
 */
TEST_P(pluginmgr_c_p, bad_final_all) {
  faux_adapter1_->finalize = test_plugin_bad_finalize;

  EXPECT_NE(0, opae_plugin_mgr_finalize_all());
  EXPECT_EQ(nullptr, adapter_list);
  EXPECT_EQ(2, test_plugin_finalize_called);
}

INSTANTIATE_TEST_CASE_P(pluginmgr_c, pluginmgr_c_p, ::testing::ValuesIn(test_platform::keys(true)));
