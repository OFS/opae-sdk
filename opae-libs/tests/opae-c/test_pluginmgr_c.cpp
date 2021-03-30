// Copyright(c) 2018-2020, Intel Corporation
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
#include <libgen.h>
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
int opae_plugin_mgr_finalize_all(void);
}

#include <config.h>
#include <opae/fpga.h>

#include <array>
#include <cstdlib>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <stack>
#include "gtest/gtest.h"
#include "mock/test_system.h"

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
    fpgaFinalize();
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

#define HOME_CFG_PATHS 3
extern "C" {
  void opae_plugin_mgr_reset_cfg(void);
  int opae_plugin_mgr_load_cfg_plugins(void);
  int opae_plugin_mgr_finalize_all(void);
  extern plugin_cfg *opae_plugin_mgr_config_list;
  extern int opae_plugin_mgr_plugin_count;
  const char *_opae_home_configs[HOME_CFG_PATHS] = {
	"/.local/opae.cfg",
	"/.local/opae/opae.cfg",
	"/.config/opae/opae.cfg",
  };
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

const char *err_contains = "wrapped_handle->adapter_table->fpgaReset is NULL";


TEST_P(pluginmgr_c_p, dummy_plugin) {
  auto ldl_path = getenv("LD_LIBRARY_PATH");
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
  ASSERT_EQ(opae_plugin_mgr_load_cfg_plugins(), 0) << "LD_LIBRARY_PATH: '"
                                                   << ldl_path << "'";
  std::string output = testing::internal::GetCapturedStdout();
  EXPECT_STREQ(output.c_str(), "hello plugin!\n");

  uint32_t matches = 0;
  fpga_properties filter = NULL;
  uint16_t device_id = 49178;
  EXPECT_EQ(fpgaGetProperties(nullptr, &filter), FPGA_OK);
  EXPECT_EQ(fpgaPropertiesSetDeviceID(filter, device_id), FPGA_OK);
  EXPECT_EQ(fpgaEnumerate(&filter, 1, nullptr, 0, &matches), FPGA_OK);
  EXPECT_EQ(matches, 99);
  std::array<fpga_token, 99> tokens = {0};
  std::array<fpga_handle, 99> handles = {0};
  EXPECT_EQ(fpgaEnumerate(&filter, 1, tokens.data(), tokens.size(), &matches),
            FPGA_OK);
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

  EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
  unlink("opae_log.log");
  opae_plugin_mgr_finalize_all();
  opae_plugin_mgr_reset_cfg(); 
}

TEST_P(pluginmgr_c_p, no_cfg) {
  opae_plugin_mgr_reset_cfg();
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 0);
  ASSERT_EQ(opae_plugin_mgr_initialize(nullptr), 0);
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 0);
  auto p1 = opae_plugin_mgr_config_list;
  ASSERT_EQ(p1, nullptr);
  opae_plugin_mgr_finalize_all();
  opae_plugin_mgr_reset_cfg(); 
}

class pluginmgr_cfg_p : public ::testing::TestWithParam<const char*> {
 protected:
  pluginmgr_cfg_p() : buffer_ {0} {}

  virtual void SetUp() override {
    // This parameterized test iterates over the possible config file paths
    // relative to a user's home directory

    // let's build the full path by prepending the parameter with $HOME
    char *home_cstr = getenv("HOME");
    ASSERT_NE(home_cstr, nullptr) << "No home environment found";
    std::string home = home_cstr;
    // the parameter paths start with a '/'
    cfg_file_ = home + std::string(GetParam());
    // copy it to a temporary buffer that we can use dirname with
    std::copy(cfg_file_.begin(), cfg_file_.end(), &buffer_[0]);
    // get the directory name of the file
    cfg_dir_ = dirname(buffer_);
    struct stat st;
    // if the directory doesn't exist, create the entire path
    if (stat(cfg_dir_, &st)) {
      std::string dir = cfg_dir_;
      // find the first '/' after $HOME
      size_t pos = dir.find('/', home.size());
      while (pos != std::string::npos) {
        std::string sub = dir.substr(0, pos);
        // sub is $HOME/<dir1>, then $HOME/<dir1>/<dir2>, ...
        // if this directory doesn't exist, create it
        if (stat(sub.c_str(), &st) && sub != "") {
          ASSERT_EQ(mkdir(sub.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH),
                    0)
              << "Error creating subdirectory (" << sub
              << "}: " << strerror(errno);
          // keep track of directories created
          dirs_.push(sub);
        }
        pos = pos < dir.size() ? dir.find('/', pos + 1) : std::string::npos;
      }
      // finally, we know the entire path didn't exist, create the last
      // directory
      ASSERT_EQ(mkdir(cfg_dir_, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH), 0)
          << "Error creating subdirectory (" << cfg_dir_
          << "}: " << strerror(errno);
      dirs_.push(cfg_dir_);
    }

    if (stat(cfg_file_.c_str(), &st) == 0) {
      unlink(cfg_file_.c_str());
    }

    std::ofstream cfg_stream(cfg_file_);
    cfg_stream.write(dummy_cfg, strlen(dummy_cfg));
    cfg_stream.close();
  }

  virtual void TearDown() override {
    opae_plugin_mgr_finalize_all();
    unlink(cfg_file_.c_str());
    // remove any directories we created in SetUp
    while (!dirs_.empty()) {
      unlink(dirs_.top().c_str());
      dirs_.pop();
    }
  }

  char buffer_[PATH_MAX];
  std::string cfg_file_;
  char *cfg_dir_;
  std::stack<std::string> dirs_;
};


/**
 * @test       find_and_parse_cfg
 * @brief      Test: find_and_parse_cfg
 * @details    Given a valid configuration with one plugin<br>
 *             And a configuration file located in one of three possible paths
 *             in the user's home directory<br>
 *             When I call opae_plugin_mgr_initialize
 *             Then the call is successful<br>
 *             And the number of plugins in the global plugin list is 1
 */
TEST_P(pluginmgr_cfg_p, find_and_parse_cfg) {
  opae_plugin_mgr_reset_cfg();
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 0);
  ASSERT_EQ(opae_plugin_mgr_initialize(nullptr), 0);
  EXPECT_EQ(opae_plugin_mgr_plugin_count, 1);
  auto p1 = opae_plugin_mgr_config_list;
  ASSERT_NE(p1, nullptr);
  ASSERT_EQ(p1->next, nullptr);
  EXPECT_TRUE(p1->enabled);
  opae_plugin_mgr_finalize_all();
  opae_plugin_mgr_reset_cfg(); 
}

INSTANTIATE_TEST_CASE_P(pluginmgr_cfg, pluginmgr_cfg_p,
                        ::testing::ValuesIn(_opae_home_configs));
