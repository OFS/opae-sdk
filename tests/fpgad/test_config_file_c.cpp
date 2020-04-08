// Copyright(c) 2019-2020, Intel Corporation
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

#include "mock/test_system.h"

extern "C" {

#include <json-c/json.h>

#include "fpgad/api/logging.h"
#include "fpgad/config_file.h"

char *cfg_read_file(const char *file);

typedef struct _cfg_vendor_device_id {
  uint16_t vendor_id;
  uint16_t device_id;
  struct _cfg_vendor_device_id *next;
} cfg_vendor_device_id;

cfg_vendor_device_id *
cfg_process_plugin_devices(const char *name,
                           json_object *j_devices);

}

#include <config.h>
#include <opae/fpga.h>

#include <fstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "gtest/gtest.h"

using namespace opae::testing;

class fpgad_config_file_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_config_file_c_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    log_set(stdout);

    strcpy(cfg_file_, "fpgad-XXXXXX.cfg");
    close(mkstemps(cfg_file_, 4));

    memset(&config_, 0, sizeof(config_));
    config_.poll_interval_usec = 100 * 1000;
    config_.running = true;
    config_.api_socket = "/tmp/fpga_event_socket";
    strcpy(config_.cfgfile, cfg_file_);
  }

  virtual void TearDown() override {
    cmd_destroy(&config_);
    log_close();

    system_->finalize();

    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(cfg_file_);
    }
  }

  void write_cfg(const char *s)
  {
    std::ofstream cfg;
    cfg.open(cfg_file_, std::ios::out);
    cfg.write(s, strlen(s));
    cfg.close();
  }

  char cfg_file_[20];
  struct fpgad_config config_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       read_file0
 * @brief      Test: cfg_read_file
 * @details    When the given file does not exist,<br>
 *             the fn returns NULL.<br>
 */
TEST_P(fpgad_config_file_c_p, read_file0) {
  EXPECT_EQ(cfg_read_file("doesntexist"), nullptr);
}

/**
 * @test       load0
 * @brief      Test: cfg_load_config
 * @details    If the given file doesn't exist,<br>
 *             the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, load0) {
  strcpy(config_.cfgfile, "doesntexist");
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       load1
 * @brief      Test: cfg_load_config
 * @details    If the given file doesn't conform to the JSON spec,<br>
 *             the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, load1) {
  write_cfg("{");
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       load2
 * @brief      Test: cfg_load_config
 * @details    If the given file contains a "configurations" key,<br>
 *             but no "plugins" key,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, load2) {
  const char *cfg = R"cfg(
{
  "configurations" : {}
}
)cfg";

  write_cfg(cfg);
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       load3
 * @brief      Test: cfg_load_config
 * @details    If the given file contains a "configurations" key,<br>
 *             and it contains a "plugins" key,<br>
 *             but the "plugins" key is not an array,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, load3) {
  const char *cfg = R"cfg(
{
  "configurations": {},
  "plugins": 3
}
)cfg";

  write_cfg(cfg);
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       load4
 * @brief      Test: cfg_load_config, cfg_verify_supported_devices
 * @details    If the given file contains valid configuration content,<br>
 *             but a "plugin" key specifies an absolute path,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, load4) {
  const char *cfg = R"cfg(
{
  "configurations": {
    "a": {
      "configuration": {},
      "enabled": true,
      "plugin": "/my/absolute/path/liba.so",
      "devices": [
        [ "0x8086", "0xbcc0" ]
      ]
    }
  },
  "plugins": [
    "a"
  ]
}
)cfg";

  write_cfg(cfg);
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       load5
 * @brief      Test: cfg_load_config, cfg_verify_supported_devices
 * @details    If the given file contains valid configuration content,<br>
 *             but a "plugin" key specifies a path that contains ..,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, load5) {
  const char *cfg = R"cfg(
{
  "configurations": {
    "a": {
      "configuration": {},
      "enabled": true,
      "plugin": "../liba.so",
      "devices": [
        [ "0x8086", "0xbcc0" ]
      ]
    }
  },
  "plugins": [
    "a"
  ]
}
)cfg";

  write_cfg(cfg);
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       load6
 * @brief      Test: cfg_load_config, cfg_verify_supported_devices
 * @details    If the given file contains valid configuration content,<br>
 *             but a "plugin" key specifies a path that contains a symlink,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, load6) {
  const char *cfg = R"cfg(
{
  "configurations": {
    "a": {
      "configuration": {},
      "enabled": true,
      "plugin": "bar/liba.so",
      "devices": [
        [ "0x8086", "0xbcc0" ]
      ]
    }
  },
  "plugins": [
    "a"
  ]
}
)cfg";

  write_cfg(cfg);

  EXPECT_EQ(std::system("rm -rf bar"), 0);

  ASSERT_EQ(mkdir("bar", 0755), 0);
  std::string s = std::string("../") + std::string(cfg_file_);
  ASSERT_EQ(symlink(s.c_str(), "bar/liba.so"), 0);

  EXPECT_NE(cfg_load_config(&config_), 0);

  ASSERT_EQ(unlink("bar/liba.so"), 0);
  ASSERT_EQ(rmdir("bar"), 0);
}

/**
 * @test       process_plugin0
 * @brief      Test: cfg_process_plugin
 * @details    If the given file lacks a plugin configuration,<br>
 *             section for a plugin that is mentioned in the "plugins" key,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, process_plugin0) {
  const char *cfg = R"cfg(
{
  "configurations": {},
  "plugins": [
    "a"
  ]
}
)cfg";

  write_cfg(cfg);
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       process_plugin1
 * @brief      Test: cfg_process_plugin
 * @details    If the given file lacks a plugin key "configuration",<br>
 *             for a plugin that is mentioned in the "plugins" key,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, process_plugin1) {
  const char *cfg = R"cfg(
{
  "configurations": {
    "a": {}
  },
  "plugins": [
    "a"
  ]
}
)cfg";

  write_cfg(cfg);
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       process_plugin3
 * @brief      Test: cfg_process_plugin
 * @details    If a plugin section lacks the "enabled" key,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, process_plugin3) {
  const char *cfg = R"cfg(
{
  "configurations": {
    "a": {
      "configuration": {}
    }
  },
  "plugins": [
    "a"
  ]
}
)cfg";

  write_cfg(cfg);
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       process_plugin4
 * @brief      Test: cfg_process_plugin
 * @details    If a plugin section's "enabled" key is not boolean,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, process_plugin4) {
  const char *cfg = R"cfg(
{
  "configurations": {
    "a": {
      "configuration": {},
      "enabled": 3
    }
  },
  "plugins": [
    "a"
  ]
}
)cfg";

  write_cfg(cfg);
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       process_plugin5
 * @brief      Test: cfg_process_plugin
 * @details    If a plugin section lacks the "plugin" key,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, process_plugin5) {
  const char *cfg = R"cfg(
{
  "configurations": {
    "a": {
      "configuration": {},
      "enabled": true
    }
  },
  "plugins": [
    "a"
  ]
}
)cfg";

  write_cfg(cfg);
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       process_plugin6
 * @brief      Test: cfg_process_plugin
 * @details    If a plugin section's "plugin" key is not a string,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, process_plugin6) {
  const char *cfg = R"cfg(
{
  "configurations": {
    "a": {
      "configuration": {},
      "enabled": true,
      "plugin" : 3
    }
  },
  "plugins": [
    "a"
  ]
}
)cfg";

  write_cfg(cfg);
  EXPECT_NE(cfg_load_config(&config_), 0);
}


/**
 * @test       process_plugin8
 * @brief      Test: cfg_process_plugin
 * @details    If a plugin section lacks the "devices" key,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(fpgad_config_file_c_p, process_plugin8) {
  const char *cfg = R"cfg(
{
  "configurations": {
    "a": {
      "configuration": {},
      "enabled": true,
      "plugin": "liba.so"
    }
  },
  "plugins": [
    "a"
  ]
}
)cfg";

  write_cfg(cfg);
  EXPECT_NE(cfg_load_config(&config_), 0);
}

INSTANTIATE_TEST_CASE_P(fpgad_config_file_c, fpgad_config_file_c_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p","skx-p-dfl0" })));


class fpgad_config_file_devices_p : public ::testing::TestWithParam<std::string> {
 protected:
  fpgad_config_file_devices_p() {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    log_set(stdout);
    root_ = nullptr;
  }

  virtual void TearDown() override {
    log_close();

    if (root_)
	    json_object_put(root_);

    system_->finalize();
  }

  json_object *parse(const char *cfg) {
    enum json_tokener_error j_err = json_tokener_success;

    root_ = json_tokener_parse_verbose(cfg, &j_err);
    if (!root_)
	    return nullptr;

    json_object *devices = nullptr;
    json_object_object_get_ex(root_, "devices", &devices);
    return devices;
  }

  json_object *root_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       process_plugin_devices0
 * @brief      Test: cfg_process_plugin_devices
 * @details    If the "devices" key of a plugin configuration is not an array,<br>
 *             then the fn returns NULL.<br>
 */
TEST_P(fpgad_config_file_devices_p, process_plugin_devices0) {
  const char *cfg = R"cfg(
{
  "devices": 3
}
)cfg";

  json_object *devices = parse(cfg);
  ASSERT_NE(devices, nullptr);
  EXPECT_EQ(cfg_process_plugin_devices("a", devices), nullptr);
}

/**
 * @test       process_plugin_devices1
 * @brief      Test: cfg_process_plugin_devices
 * @details    If the items within the "devices" key of a plugin<br>
 *             configuration are not arrays,<br>
 *             then the fn returns NULL.<br>
 */
TEST_P(fpgad_config_file_devices_p, process_plugin_devices1) {
  const char *cfg = R"cfg(
{
  "devices": [
    true
  ]
}
)cfg";

  json_object *devices = parse(cfg);
  ASSERT_NE(devices, nullptr);
  EXPECT_EQ(cfg_process_plugin_devices("a", devices), nullptr);
}

/**
 * @test       process_plugin_devices2
 * @brief      Test: cfg_process_plugin_devices
 * @details    If the items within the "devices" key of a plugin<br>
 *             configuration are not arrays of size 2,<br>
 *             then the fn returns NULL.<br>
 */
TEST_P(fpgad_config_file_devices_p, process_plugin_devices2) {
  const char *cfg = R"cfg(
{
  "devices": [
    [ "0x8086", "0x0b30", "1" ]
  ]
}
)cfg";

  json_object *devices = parse(cfg);
  ASSERT_NE(devices, nullptr);
  EXPECT_EQ(cfg_process_plugin_devices("a", devices), nullptr);
}

/**
 * @test       process_plugin_devices3
 * @brief      Test: cfg_process_plugin_devices
 * @details    If the form of the "devices" key is correct<br>
 *             and the vendor ID is represented as a string,<br>
 *             but that string is not a valid integer,<br>
 *             then the fn returns NULL.<br>
 */
TEST_P(fpgad_config_file_devices_p, process_plugin_devices3) {
  const char *cfg = R"cfg(
{
  "devices": [
    [ "not an integer", "0x0b30" ]
  ]
}
)cfg";

  json_object *devices = parse(cfg);
  ASSERT_NE(devices, nullptr);
  EXPECT_EQ(cfg_process_plugin_devices("a", devices), nullptr);
}

/**
 * @test       process_plugin_devices4
 * @brief      Test: cfg_process_plugin_devices
 * @details    If the form of the "devices" key is correct<br>
 *             but the vendor ID is not a string nor an integer,<br>
 *             then the fn returns NULL.<br>
 */
TEST_P(fpgad_config_file_devices_p, process_plugin_devices4) {
  const char *cfg = R"cfg(
{
  "devices": [
    [ 3.14, "0x0b30" ]
  ]
}
)cfg";

  json_object *devices = parse(cfg);
  ASSERT_NE(devices, nullptr);
  EXPECT_EQ(cfg_process_plugin_devices("a", devices), nullptr);
}

/**
 * @test       process_plugin_devices5
 * @brief      Test: cfg_process_plugin_devices
 * @details    If the form of the "devices" key is correct<br>
 *             and the device ID is represented as a string,<br>
 *             but that string is not a valid integer,<br>
 *             then the fn returns NULL.<br>
 */
TEST_P(fpgad_config_file_devices_p, process_plugin_devices5) {
  const char *cfg = R"cfg(
{
  "devices": [
    [ "0x8086", "not an integer" ]
  ]
}
)cfg";

  json_object *devices = parse(cfg);
  ASSERT_NE(devices, nullptr);
  EXPECT_EQ(cfg_process_plugin_devices("a", devices), nullptr);
}

/**
 * @test       process_plugin_devices6
 * @brief      Test: cfg_process_plugin_devices
 * @details    If the form of the "devices" key is correct<br>
 *             but the device ID is not a string nor an integer,<br>
 *             then the fn returns NULL.<br>
 */
TEST_P(fpgad_config_file_devices_p, process_plugin_devices6) {
  const char *cfg = R"cfg(
{
  "devices": [
    [ "0x8086", 3.14 ]
  ]
}
)cfg";

  json_object *devices = parse(cfg);
  ASSERT_NE(devices, nullptr);
  EXPECT_EQ(cfg_process_plugin_devices("a", devices), nullptr);
}

/**
 * @test       process_plugin_devices9
 * @brief      Test: cfg_process_plugin_devices
 * @details    If the form of the "devices" key is correct<br>
 *             and the content within the keys is correct,<br>
 *             then the fn returns a linked list of id's.<br>
 */
TEST_P(fpgad_config_file_devices_p, process_plugin_devices9) {
  const char *cfg = R"cfg(
{
  "devices": [
    [ 32902, 2864 ],
    [ "0x8086", "0xbcc0" ]
  ]
}
)cfg";

  json_object *devices = parse(cfg);
  ASSERT_NE(devices, nullptr);

  cfg_vendor_device_id *ids = cfg_process_plugin_devices("a", devices);
  ASSERT_NE(ids, nullptr);

  EXPECT_EQ(ids->vendor_id, 0x8086);
  EXPECT_EQ(ids->device_id, 0x0b30);

  ASSERT_NE(ids->next, nullptr);
  EXPECT_EQ(ids->next->vendor_id, 0x8086);
  EXPECT_EQ(ids->next->device_id, 0xbcc0);

  free(ids->next);
  free(ids);
}

INSTANTIATE_TEST_CASE_P(fpgad_config_file_c, fpgad_config_file_devices_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p","skx-p-dfl0" })));


class mock_fpgad_config_file_c_p : public fpgad_config_file_c_p {};
 
/**
 * @test       read_file1
 * @brief      Test: cfg_read_file
 * @details    If malloc fails,<br>
 *             the fn returns NULL.<br>
 */
TEST_P(mock_fpgad_config_file_c_p, read_file1) {
  system_->invalidate_malloc(0, "cfg_read_file");
  write_cfg("{}");
  EXPECT_EQ(cfg_read_file(cfg_file_), nullptr);
}

/**
 * @test       process_plugin2
 * @brief      Test: cfg_process_plugin
 * @details    If cstr_dup fails to clone the "configuration" key,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(mock_fpgad_config_file_c_p, process_plugin2) {
  const char *cfg = R"cfg(
{
  "configurations": {
    "a": {
      "configuration": {}
    }
  },
  "plugins": [
    "a"
  ]
}
)cfg";

  write_cfg(cfg);
  system_->invalidate_malloc(0, "cstr_dup");
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       process_plugin7
 * @brief      Test: cfg_process_plugin
 * @details    If cstr_dup fails to clone the "plugin" key,<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(mock_fpgad_config_file_c_p, process_plugin7) {
  const char *cfg = R"cfg(
{
  "configurations": {
    "a": {
      "configuration": {},
      "enabled": true,
      "plugin": "liba.so"
    }
  },
  "plugins": [
    "a"
  ]
}
)cfg";

  write_cfg(cfg);
  system_->invalidate_malloc(1, "cstr_dup");
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       process_plugin9
 * @brief      Test: cfg_process_plugin, alloc_configuration
 * @details    If malloc fails during alloc_configuration (empty list),<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(mock_fpgad_config_file_c_p, process_plugin9) {
  const char *cfg = R"cfg(
{
  "configurations": {
    "a": {
      "configuration": {},
      "enabled": true,
      "plugin": "liba.so",
      "devices": [
        [ "0x8086", "0x0b30" ]
      ]
    }
  },
  "plugins": [
    "a"
  ]
}
)cfg";

  write_cfg(cfg);
  system_->invalidate_malloc(0, "alloc_configuration");
  EXPECT_NE(cfg_load_config(&config_), 0);
}

/**
 * @test       process_plugin10
 * @brief      Test: cfg_process_plugin, alloc_configuration
 * @details    If malloc fails during alloc_configuration (non-empty list),<br>
 *             then the fn returns non-zero.<br>
 */
TEST_P(mock_fpgad_config_file_c_p, process_plugin10) {
  const char *cfg = R"cfg(
{
  "configurations": {
    "a": {
      "configuration": {},
      "enabled": true,
      "plugin": "liba.so",
      "devices": [
        [ "0x8086", "0x0b30" ]
      ]
    },
    "b": {
      "configuration": {},
      "enabled": true,
      "plugin": "libb.so",
      "devices": [
        [ "0x8086", "0xbcc0" ]
      ]
    }
  },
  "plugins": [
    "a",
    "b"
  ]
}
)cfg";

  write_cfg(cfg);
  system_->invalidate_malloc(1, "alloc_configuration");
  EXPECT_NE(cfg_load_config(&config_), 0);
}

INSTANTIATE_TEST_CASE_P(fpgad_config_file_c, mock_fpgad_config_file_c_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p","skx-p-dfl0" })));


class mock_fpgad_config_file_devices_p : public fpgad_config_file_devices_p {};

/**
 * @test       process_plugin_devices7
 * @brief      Test: cfg_process_plugin_devices
 * @details    If the form of the "devices" key is correct<br>
 *             but malloc fails (empty list),<br>
 *             then the fn returns NULL.<br>
 */
TEST_P(mock_fpgad_config_file_devices_p, process_plugin_devices7) {
  const char *cfg = R"cfg(
{
  "devices": [
    [ 32902, 2864 ]
  ]
}
)cfg";

  json_object *devices = parse(cfg);
  ASSERT_NE(devices, nullptr);
  system_->invalidate_malloc(0, "alloc_device");
  EXPECT_EQ(cfg_process_plugin_devices("a", devices), nullptr);
}

/**
 * @test       process_plugin_devices8
 * @brief      Test: cfg_process_plugin_devices
 * @details    If the form of the "devices" key is correct<br>
 *             but malloc fails (non-empty list),<br>
 *             then the fn returns NULL.<br>
 */
TEST_P(mock_fpgad_config_file_devices_p, process_plugin_devices8) {
  const char *cfg = R"cfg(
{
  "devices": [
    [ 32902, 2864 ],
    [ "0x8086", "0xbcc0" ]
  ]
}
)cfg";

  json_object *devices = parse(cfg);
  ASSERT_NE(devices, nullptr);
  system_->invalidate_malloc(1, "alloc_device");
  EXPECT_EQ(cfg_process_plugin_devices("a", devices), nullptr);
}

INSTANTIATE_TEST_CASE_P(fpgad_config_file_c, mock_fpgad_config_file_devices_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p","skx-p-dfl0" })));
