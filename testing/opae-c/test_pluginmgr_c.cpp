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
int process_cfg_buffer(const char *buffer, const char *filename);
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

const char *plugin_cfg_1 = R"plug(
{
    "configurations": {
        "plugin1": {
            "configuration": {
                "key1a": 10,
                "key1b": "hello"
            },
            "enabled": true,
            "plugin": "libplugin1.so"
        },
        "plugin2": {
            "configuration": {
                "key1a": 20,
                "key1b": "goodbye"
            },
            "enabled": false,
            "plugin": "libplugin2.so"
        }
    },
    "plugins": [
        "plugin1",
        "plugin2"
    ]
}
)plug";

// missing comma (,) on line 272
const char *plugin_cfg_2 = R"plug(
{
    "configurations": {
        "plugin1": {
            "configuration": {
                "key1a": 10,
                "key1b": "hello"
            },
            "enabled": true,
            "plugin": "libplugin1.so"
        }
        "plugin2": {
            "configuration": {
                "key1a": 20,
                "key1b": "goodbye"
            },
            "enabled": false,
            "plugin": "libplugin2.so"
        }
    },
    "plugins": [
        "plugin1",
        "plugin2
    ]
}
)plug";

// keyword enabled misspelled on line 298
const char *plugin_cfg_3 = R"plug(
{
    "configurations": {
        "plugin1": {
            "configuration": {
                "key1a": 10,
                "key1b": "hello"
            },
            "enable": true,
            "plugin": "libplugin1.so"
        },
        "plugin2": {
            "configuration": {
                "key1a": 20,
                "key1b": "goodbye"
            },
            "enabled": false,
            "plugin": "libplugin2.so"
        }
    },
    "plugins": [
        "plugin1",
        "plugin2"
    ]
}
)plug";

// plugin name different on line 321
const char *plugin_cfg_4 = R"plug(
{
    "configurations": {
        "plugin10": {
            "configuration": {
                "key1a": 10,
                "key1b": "hello"
            },
            "enabled": true,
            "plugin": "libplugin1.so"
        },
        "plugin2": {
            "configuration": {
                "key1a": 20,
                "key1b": "goodbye"
            },
            "enabled": false,
            "plugin": "libplugin2.so"
        }
    },
    "plugins": [
        "plugin1",
        "plugin2"
    ]
}
)plug";

// plugins not array type
const char *plugin_cfg_5 = R"plug(
{
    "configurations": {
        "plugin1": {
            "configuration": {
                "key1a": 10,
                "key1b": "hello"
            },
            "enabled": true,
            "plugin": "libplugin1.so"
        },
        "plugin2": {
            "configuration": {
                "key1a": 20,
                "key1b": "goodbye"
            },
            "enabled": false,
            "plugin": "libplugin2.so"
        }
    },
    "plugins": 0
}
)plug";


extern "C" {
  void opae_plugin_mgr_reset_cfg(void);
  int opae_plugin_mgr_load_cfg_plugins(void);
  extern plugin_cfg *opae_plugin_mgr_config_list;
  extern int opae_plugin_mgr_plugin_count;
}

TEST(pluginmgr_c_p, process_cfg_buffer) {
  opae_plugin_mgr_reset_cfg();
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 0);
  ASSERT_EQ(process_cfg_buffer(plugin_cfg_1, "plugin1.json"), 0);
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 2);
  auto p1 = opae_plugin_mgr_config_list;
  ASSERT_NE(p1, nullptr);
  auto p2 = p1->next;
  ASSERT_NE(p2, nullptr);
  EXPECT_TRUE(p1->enabled);
  EXPECT_FALSE(p2->enabled);
  ASSERT_EQ(p2->next, nullptr);
}

TEST(pluginmgr_c_p, process_cfg_buffer_err) {
  opae_plugin_mgr_reset_cfg();
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 0);
  ASSERT_NE(process_cfg_buffer(plugin_cfg_2, "plugin2.json"), 0);

  opae_plugin_mgr_reset_cfg();
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 0);
  ASSERT_NE(process_cfg_buffer(plugin_cfg_3, "plugin3.json"), 0);
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 1);

  opae_plugin_mgr_reset_cfg();
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 0);
  ASSERT_NE(process_cfg_buffer(plugin_cfg_4, "plugin4.json"), 0);
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 1);

  opae_plugin_mgr_reset_cfg();
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 0);
  ASSERT_NE(process_cfg_buffer(plugin_cfg_5, "plugin5.json"), 0);
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 0);
}

const char *dummy_cfg = R"plug(
{
    "configurations": {
        "dummy": {
            "configuration": {
                "key1": "hello",
                "key2": "plugin",
                "fake_tokens": 99
            },
            "enabled": true,
            "plugin": "libdummy_plugin.so"
        }
    },
    "plugins": [
        "dummy"
    ]
}
)plug";

const char* err_contains = "wrapped_handle->adapter_table->fpgaReset is NULL";

TEST(pluginmgr_c_p, dummy_plugin) {
  opae_plugin_mgr_reset_cfg();
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 0);
  ASSERT_EQ(process_cfg_buffer(dummy_cfg, "dummy.json"), 0);
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 1);
  auto p1 = opae_plugin_mgr_config_list;
  ASSERT_NE(p1, nullptr);
  auto p2 = p1->next;
  ASSERT_EQ(p2, nullptr);
  EXPECT_TRUE(p1->enabled);
  testing::internal::CaptureStdout();
  ASSERT_EQ(opae_plugin_mgr_load_cfg_plugins(), 0);
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_STREQ(output.c_str(), "hello plugin!\n");

  uint32_t matches = 0;
  EXPECT_EQ(fpgaEnumerate(nullptr, 0, nullptr, 0, &matches), FPGA_OK);
  EXPECT_EQ(matches, 99);
  std::array<fpga_token, 99> tokens = { 0 };
  std::array<fpga_handle, 99> handles = { 0 };
  EXPECT_EQ(fpgaEnumerate(nullptr, 0, tokens.data(), tokens.size(), &matches), FPGA_OK);
  int i = 0;
  for (auto t : tokens) {
    EXPECT_EQ(fpgaOpen(t, &handles[i], i), FPGA_OK);
    testing::internal::CaptureStderr();
    EXPECT_EQ(fpgaReset(handles[i]), FPGA_NOT_SUPPORTED);
    std::string err = testing::internal::GetCapturedStderr();
    EXPECT_NE(err.find(err_contains), std::string::npos);
    EXPECT_EQ(fpgaClose(handles[i++]), FPGA_OK);
    EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
  }
  unlink("opae_log.log");
}

