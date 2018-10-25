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
#include "safe_string/safe_string.h"
#include "opae_int.h"
#include "feature_pluginmgr.h"

int feature_plugin_mgr_initialize_all(void);
opae_feature_adapter_table *feature_plugin_mgr_alloc_adapter(const char *lib_path);
int feature_plugin_mgr_free_adapter(opae_feature_adapter_table *adapter);
int feature_plugin_mgr_register_adapter(opae_feature_adapter_table *adapter);
opae_feature_adapter_table *get_feature_plugin_adapter(fpga_guid guid);
int feature_plugin_mgr_configure_plugin(opae_feature_adapter_table *adapter,
				     const char *config);
int feature_plugin_mgr_finalize_all(void);

extern opae_feature_adapter_table *feature_adapter_list;
}

#include <config.h>
#include <opae/fpga.h>
#include "intel-fpga.h"

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
TEST(feature_pluginmgr, alloc_adapter01) {
  EXPECT_EQ(NULL, feature_plugin_mgr_alloc_adapter("libthatdoesntexist.so"));
}

/**
 * @test       alloc_adapter02
 * @brief      Test: opae_plugin_mgr_alloc_adapter
 * @details    When calloc fails,<br>
 *             opae_plugin_mgr_alloc_adapter returns NULL.<br>
 */
TEST(feature_pluginmgr, alloc_adapter02) {
  test_system::instance()->invalidate_calloc(0, "feature_plugin_mgr_alloc_adapter");
  EXPECT_EQ(NULL, feature_plugin_mgr_alloc_adapter("libintel-dma.so"));
}

/**
 * @test       free_adapter01
 * @brief      Test: opae_plugin_mgr_free_adapter
 * @details    opae_plugin_mgr_free_adapter frees the given adapter table<br>
 *             and returns 0 on success.<br>
 */
TEST(feature_pluginmgr, free_adapter) {
  opae_feature_adapter_table *at;
  at = feature_plugin_mgr_alloc_adapter("libintel-dma.so");
  ASSERT_NE(nullptr, at);
  EXPECT_EQ(0, feature_plugin_mgr_free_adapter(at));
}

/**
 * @test       config_err
 * @brief      Test: opae_plugin_mgr_configure_plugin
 * @details    When opae_plugin_mgr_configure_plugin is called on a load library<br>
 *             that has no opae_plugin_configure symbol,<br>
 *             then the fn returns non-zero.<br>
 */
TEST(feature_pluginmgr, config_err) {
  opae_feature_adapter_table *at;
  at = feature_plugin_mgr_alloc_adapter("libopae-c.so");  // TODO: checking
  ASSERT_NE(nullptr, at);
  EXPECT_NE(0, feature_plugin_mgr_configure_plugin(at, ""));
  EXPECT_EQ(0, feature_plugin_mgr_free_adapter(at));
}

extern "C" {

static int test_feature_plugin_initialize_called;
static int test_feature_plugin_initialize(void)
{
  ++test_feature_plugin_initialize_called;
  return 0;
}

static int test_feature_plugin_bad_initialize(void)
{
  ++test_feature_plugin_initialize_called;
  return 1;
}

static int test_feature_plugin_finalize_called;
static int test_feature_plugin_finalize(void)
{
  ++test_feature_plugin_finalize_called;
  return 0;
}

static int test_feature_plugin_bad_finalize(void)
{
  ++test_feature_plugin_finalize_called;
  return 1;
}

}

class feature_pluginmgr_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  feature_pluginmgr_c_p() : tokens_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    invalid_device_ = test_device::unknown();

    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    num_matches_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_),
              FPGA_OK);
    EXPECT_EQ(num_matches_, platform_.devices.size());
    accel_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_[0], &accel_, 0), FPGA_OK);
	num_matches_ = 0;
	feature_filter_.type = DMA;
	memset_s(feature_filter_.guid, sizeof(fpga_guid), 0);   // TODO: Fill in DMA guid id here
	ASSERT_EQ(fpgaFeatureEnumerate(accel_, &feature_filter_, ftokens_.data(), ftokens_.size(), &num_matches_), FPGA_OK);
	
    // save the global adapter list.
    feature_adapter_list_ = feature_adapter_list;
    feature_adapter_list = nullptr;

    test_feature_plugin_initialize_called = 0;
    test_feature_plugin_finalize_called = 0;

    faux_adapter0_ = feature_plugin_mgr_alloc_adapter("libintel-dma.so");
    ASSERT_NE(nullptr, faux_adapter0_);

    faux_adapter0_->initialize = test_feature_plugin_initialize;
    faux_adapter0_->finalize = test_feature_plugin_finalize;
    EXPECT_EQ(0, feature_plugin_mgr_register_adapter(faux_adapter0_));

    faux_adapter1_ = feature_plugin_mgr_alloc_adapter("libintel-dma.so");
    ASSERT_NE(nullptr, faux_adapter1_);

    faux_adapter1_->initialize = test_feature_plugin_initialize;
    faux_adapter1_->finalize = test_feature_plugin_finalize;
    EXPECT_EQ(0, feature_plugin_mgr_register_adapter(faux_adapter1_));
  }

  virtual void TearDown() override {
    // restore the global adapter list.
    feature_adapter_list = feature_adapter_list_;
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (accel_) {
        EXPECT_EQ(fpgaClose(accel_), FPGA_OK);
        accel_ = nullptr;
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
  std::array<fpga_feature_token, 2> ftokens_;
  fpga_properties filter_;  
  fpga_handle accel_;
  fpga_feature_properties feature_filter_;
  opae_feature_adapter_table *feature_adapter_list_;
  opae_feature_adapter_table *faux_adapter0_;
  opae_feature_adapter_table *faux_adapter1_;
  test_platform platform_;
  uint32_t num_matches_;
  test_device invalid_device_;
  test_system *system_;
};

/**
 * @test       foreach_err
 * @brief      Test: opae_plugin_mgr_for_each_adapter
 * @details    When opae_plugin_mgr_for_each_adapter is passed a NULL callback,<br>
 *             then the fn returns OPAE_ENUM_STOP.<br>
 */
/*TEST_P(feature_pluginmgr_c_p, get_adapter_err) {
  EXPECT_EQ(OPAE_ENUM_STOP, get_dma_plugin_adapter( guid_dma));    //TODO: check
 } */

/**
 * @test       bad_init_all
 * @brief      Test: opae_plugin_mgr_initialize_all
 * @details    When any of the registered adapters' initialize fn returns non-zero,<br>
 *             then opae_plugin_mgr_initialize_all returns non-zero.<br>
 */
TEST_P(feature_pluginmgr_c_p, bad_init_all) {
  faux_adapter1_->initialize = test_feature_plugin_bad_initialize;
  EXPECT_NE(0, feature_plugin_mgr_initialize_all());
  EXPECT_EQ(2, test_feature_plugin_initialize_called); //TODO: checking
 }

/**
 * @test       bad_final_all
 * @brief      Test: opae_plugin_mgr_finalize_all
 * @details    When any of the registered adapters' finalize fn returns non-zero,<br>
 *             then opae_plugin_mgr_finalize_all returns non-zero.<br>
 */
TEST_P(feature_pluginmgr_c_p, bad_final_all) {
  faux_adapter1_->finalize = test_feature_plugin_bad_finalize;

  EXPECT_NE(0, feature_plugin_mgr_finalize_all());
  EXPECT_EQ(nullptr, feature_adapter_list);
  EXPECT_EQ(2, test_feature_plugin_finalize_called); //TODO: checking
}

INSTANTIATE_TEST_CASE_P(feature_pluginmgr_c, feature_pluginmgr_c_p, ::testing::ValuesIn(test_platform::keys(true)));
