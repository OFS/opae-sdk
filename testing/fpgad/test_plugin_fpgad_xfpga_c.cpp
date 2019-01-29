// Copyright(c) 2019, Intel Corporation
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

#include "fpgad/api/opae_events_api.h"
#include "fpgad/api/device_monitoring.h"

enum fpga_power_state {
  FPGAD_NORMAL_PWR = 0,
  FPGAD_AP1_STATE,
  FPGAD_AP2_STATE,
  FPGAD_AP6_STATE
};

typedef struct _fpgad_xfpga_AP_context {
  const char *sysfs_file;
  const char *message;
  int low_bit;
  int high_bit;
} fpgad_xfpga_AP_context;

fpgad_detection_status
fpgad_xfpga_detect_AP1_or_AP2(fpgad_monitored_device *d,
                              void *context);

void fpgad_xfpga_respond_AP1_or_AP2(fpgad_monitored_device *d,
                                    void *context);

fpgad_detection_status
fpgad_xfpga_detect_PowerStateChange(fpgad_monitored_device *d,
                                    void *context);

void fpgad_xfpga_respond_PowerStateChange(fpgad_monitored_device *d,
                                          void *context);

typedef struct _fpgad_xfpga_Error_context {
  const char *sysfs_file;
  const char *message;
  int low_bit;
  int high_bit;
} fpgad_xfpga_Error_context;

fpgad_detection_status
fpgad_xfpga_detect_Error(fpgad_monitored_device *d,
                         void *context);

fpgad_detection_status
fpgad_xfpga_detect_High_Priority_Error(fpgad_monitored_device *d,
                                       void *context);

void fpgad_xfpga_respond_LogError(fpgad_monitored_device *d,
                                  void *context);

void fpgad_xfpga_respond_AP6_and_Null_GBS(fpgad_monitored_device *d,
                                          void *context);

void fpgad_xfpga_respond_AP6(fpgad_monitored_device *d,
                             void *context);

int fpgad_plugin_configure(fpgad_monitored_device *d,
                           const char *cfg);

void fpgad_plugin_destroy(fpgad_monitored_device *d);

extern fpgad_detect_event_t fpgad_xfpga_port_detections[1];
extern void *fpgad_xfpga_port_detection_contexts[1];
extern fpgad_respond_event_t fpgad_xfpga_port_responses[1];
extern void *fpgad_xfpga_port_response_contexts[1];

extern fpgad_detect_event_t fpgad_xfpga_fme_detections[1];
extern void *fpgad_xfpga_fme_detection_contexts[1];
extern fpgad_respond_event_t fpgad_xfpga_fme_responses[1];
extern void *fpgad_xfpga_fme_response_contexts[1];

}

#include <config.h>
#include <opae/fpga.h>
#include "safe_string/safe_string.h"

#include <array>
#include <vector>
#include <cstdlib>
#include <cstring>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

class mock_port_fpgad_xfpga_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  mock_port_fpgad_xfpga_c_p()
    : tokens_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
    std::string platform_key = GetParam();
    ASSERT_TRUE(test_platform::exists(platform_key));
    platform_ = test_platform::get(platform_key);
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    bitstream_ = system_->assemble_gbs_header(platform_.devices[0]);

    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    num_matches_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_),
              FPGA_OK);
    EXPECT_GT(num_matches_, 0);
    accel_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_[0], &accel_, 0), FPGA_OK);

    log_set(stdout);
  }

  virtual void TearDown() override {
    log_close();

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
    fpgaFinalize();
    system_->finalize();
  }

  void init_monitored_device(fpgad_monitored_device *d,
                             fpgad_supported_device *s)
  {
    memset_s(d, sizeof(fpgad_monitored_device), 0);
    d->token = tokens_[0];
    d->object_type = FPGA_ACCELERATOR;
    d->num_error_occurrences = 0;
    d->supported = s;

    s->vendor_id = 0x8086;
    s->device_id = 0x0b30;
    s->library_path = "libfpgad-xfpga.so";
    s->flags = FPGAD_DEV_DETECTED|FPGAD_DEV_LOADED;
    s->dl_handle = NULL;
  }

  void init_AP1_context(fpgad_xfpga_AP_context *c)
  {
    c->sysfs_file = "ap1_event";
    c->message = "AP1 triggered!";
    c->low_bit = 0;
    c->high_bit = 0;
  }

  void set_AP1_state(bool state)
  {
    fpga_object obj = nullptr;

    ASSERT_EQ(fpgaHandleGetObject(accel_,
                                  "ap1_event",
                                  &obj,
                                  0), FPGA_OK);

    EXPECT_EQ(fpgaObjectWrite64(obj,
                                state ? 1 : 0,
                                0), FPGA_OK);

    EXPECT_EQ(fpgaDestroyObject(&obj), FPGA_OK);
  }

  void init_AP2_context(fpgad_xfpga_AP_context *c)
  {
    c->sysfs_file = "ap2_event";
    c->message = "AP2 triggered!";
    c->low_bit = 0;
    c->high_bit = 0;
  }

  void set_AP2_state(bool state)
  {
    fpga_object obj = nullptr;

    ASSERT_EQ(fpgaHandleGetObject(accel_,
                                  "ap2_event",
                                  &obj,
                                  0), FPGA_OK);

    EXPECT_EQ(fpgaObjectWrite64(obj,
                                state ? 1 : 0,
                                0), FPGA_OK);

    EXPECT_EQ(fpgaDestroyObject(&obj), FPGA_OK);
  }

  void init_power_state_context(fpgad_xfpga_AP_context *c)
  {
    c->sysfs_file = "power_state";
    c->message = "Power state changed to";
    c->low_bit = 0;
    c->high_bit = 1;
  }

  void set_power_state(enum fpga_power_state s)
  {
    fpga_object obj = nullptr;

    ASSERT_EQ(fpgaHandleGetObject(accel_,
                                  "power_state",
                                  &obj,
                                  0), FPGA_OK);

    EXPECT_EQ(fpgaObjectWrite64(obj,
                                (uint64_t)s,
                                0), FPGA_OK);

    EXPECT_EQ(fpgaDestroyObject(&obj), FPGA_OK);
  }

  void init_AP6_context(fpgad_xfpga_Error_context *c)
  {
    c->sysfs_file = "errors/errors";
    c->message = "PORT_ERROR[0x1010].Ap6Event";
    c->low_bit = 50;
    c->high_bit = 50;
  }

  void set_AP6_state(bool state)
  {
    fpga_object obj = nullptr;
    uint64_t value = 0x0004000000000000ULL;

    ASSERT_EQ(fpgaHandleGetObject(accel_,
                                  "errors/errors",
                                  &obj,
                                  0), FPGA_OK);

    EXPECT_EQ(fpgaObjectWrite64(obj,
                                state ? value : 0ULL,
                                0), FPGA_OK);

    EXPECT_EQ(fpgaDestroyObject(&obj), FPGA_OK);
  }

  std::array<fpga_token, 2> tokens_;
  fpga_properties filter_;
  fpga_handle accel_;
  uint32_t num_matches_;
  std::vector<uint8_t> bitstream_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       AP1
 * @brief      Test: fpgad_xfpga_detect_AP1_or_AP2, fpgad_xfpga_respond_AP1_or_AP2
 * @details    Test the plugin's ability to detect/respond to AP1.<br>
 */
TEST_P(mock_port_fpgad_xfpga_c_p, AP1) {
  fpgad_monitored_device d;
  fpgad_supported_device s;
  init_monitored_device(&d, &s);

  fpgad_xfpga_AP_context context;
  init_AP1_context(&context);

  set_AP1_state(true);

  EXPECT_EQ(fpgad_xfpga_detect_AP1_or_AP2(&d, &context),
            FPGAD_STATUS_DETECTED);
  EXPECT_EQ(d.num_error_occurrences, 1);
  EXPECT_EQ(d.error_occurrences[0], &context);

  fpgad_xfpga_respond_AP1_or_AP2(&d, &context);

  set_AP1_state(false);

  EXPECT_EQ(fpgad_xfpga_detect_AP1_or_AP2(&d, &context),
            FPGAD_STATUS_NOT_DETECTED);
  EXPECT_EQ(d.num_error_occurrences, 0);
}

/**
 * @test       AP2
 * @brief      Test: fpgad_xfpga_detect_AP1_or_AP2, fpgad_xfpga_respond_AP1_or_AP2
 * @details    Test the plugin's ability to detect/respond to AP2.<br>
 */
TEST_P(mock_port_fpgad_xfpga_c_p, AP2) {
  fpgad_monitored_device d;
  fpgad_supported_device s;
  init_monitored_device(&d, &s);

  fpgad_xfpga_AP_context context;
  init_AP2_context(&context);

  set_AP2_state(true);

  EXPECT_EQ(fpgad_xfpga_detect_AP1_or_AP2(&d, &context),
            FPGAD_STATUS_DETECTED);
  EXPECT_EQ(d.num_error_occurrences, 1);
  EXPECT_EQ(d.error_occurrences[0], &context);

  fpgad_xfpga_respond_AP1_or_AP2(&d, &context);

  set_AP2_state(false);

  EXPECT_EQ(fpgad_xfpga_detect_AP1_or_AP2(&d, &context),
            FPGAD_STATUS_NOT_DETECTED);
  EXPECT_EQ(d.num_error_occurrences, 0);
}

/**
 * @test       power_state
 * @brief      Test: fpgad_xfpga_detect_PowerStateChange, fpgad_xfpga_respond_PowerStateChange
 * @details    Test the plugin's ability to detect/respond to power_state changes.<br>
 */
TEST_P(mock_port_fpgad_xfpga_c_p, power_state) {
  fpgad_monitored_device d;
  fpgad_supported_device s;
  init_monitored_device(&d, &s);

  fpgad_xfpga_AP_context context;
  init_power_state_context(&context);

  set_power_state(FPGAD_AP1_STATE);

  EXPECT_EQ(fpgad_xfpga_detect_PowerStateChange(&d, &context),
            FPGAD_STATUS_DETECTED);

  ::testing::internal::CaptureStdout();
  fpgad_xfpga_respond_PowerStateChange(&d, &context);
  std::string out = ::testing::internal::GetCapturedStdout();
  EXPECT_STREQ(out.c_str(), "fpgad-xfpga: Power state changed to AP1 Power State\n");


  set_power_state(FPGAD_AP2_STATE);

  EXPECT_EQ(fpgad_xfpga_detect_PowerStateChange(&d, &context),
            FPGAD_STATUS_DETECTED);

  ::testing::internal::CaptureStdout();
  fpgad_xfpga_respond_PowerStateChange(&d, &context);
  out = ::testing::internal::GetCapturedStdout();
  EXPECT_STREQ(out.c_str(), "fpgad-xfpga: Power state changed to AP2 Power State\n");


  set_power_state(FPGAD_AP6_STATE);

  EXPECT_EQ(fpgad_xfpga_detect_PowerStateChange(&d, &context),
            FPGAD_STATUS_DETECTED);

  ::testing::internal::CaptureStdout();
  fpgad_xfpga_respond_PowerStateChange(&d, &context);
  out = ::testing::internal::GetCapturedStdout();
  EXPECT_STREQ(out.c_str(), "fpgad-xfpga: Power state changed to AP6 Power State\n");


  set_power_state(FPGAD_NORMAL_PWR);

  EXPECT_EQ(fpgad_xfpga_detect_PowerStateChange(&d, &context),
            FPGAD_STATUS_DETECTED);

  ::testing::internal::CaptureStdout();
  fpgad_xfpga_respond_PowerStateChange(&d, &context);
  out = ::testing::internal::GetCapturedStdout();
  EXPECT_STREQ(out.c_str(), "fpgad-xfpga: Power state changed to Normal Power\n");
}

/**
 * @test       AP6
 * @brief      Test: fpgad_xfpga_detect_Error, fpgad_xfpga_detect_High_Priority_Error,
 *                   fpgad_xfpga_respond_AP6_and_Null_GBS.
 * @details    Test the plugin's ability to detect/respond to AP6.<br>
 */
TEST_P(mock_port_fpgad_xfpga_c_p, AP6) {
  fpgad_monitored_device d;
  fpgad_supported_device s;
  init_monitored_device(&d, &s);

  struct bitstream_info binfo;
  memset_s(&binfo, sizeof(bitstream_info), 0);
  binfo.filename = (char *)"null.gbs";
  binfo.data = bitstream_.data();
  binfo.data_len = bitstream_.size();

  d.bitstr = &binfo;

  fpgad_xfpga_Error_context context;
  init_AP6_context(&context);

  set_AP6_state(true);

  EXPECT_EQ(fpgad_xfpga_detect_High_Priority_Error(&d, &context),
            FPGAD_STATUS_DETECTED_HIGH);
  EXPECT_EQ(d.num_error_occurrences, 1);
  EXPECT_EQ(d.error_occurrences[0], &context);

  ::testing::internal::CaptureStdout();
  fpgad_xfpga_respond_AP6_and_Null_GBS(&d, &context);
  std::string out = ::testing::internal::GetCapturedStdout();

  EXPECT_STREQ(out.c_str(),
               "fpgad-xfpga: PORT_ERROR[0x1010].Ap6Event\n"
               "fpgad-xfpga: programming \"null.gbs\": fpgad-xfpga: SUCCESS\n");

  set_AP6_state(false);

  EXPECT_EQ(fpgad_xfpga_detect_High_Priority_Error(&d, &context),
            FPGAD_STATUS_NOT_DETECTED);
  EXPECT_EQ(d.num_error_occurrences, 0);
}

/**
 * @test       configure
 * @brief      Test: fpgad_plugin_configure, fpgad_plugin_destroy
 * @details    Test the plugin's configure and destroy fn's. (Port)<br>
 */
TEST_P(mock_port_fpgad_xfpga_c_p, configure) {
  fpgad_monitored_device d;
  fpgad_supported_device s;
  init_monitored_device(&d, &s);

  EXPECT_EQ(fpgad_plugin_configure(&d, NULL), 0);

  EXPECT_EQ(d.type, FPGAD_PLUGIN_TYPE_CALLBACK);
  EXPECT_EQ(d.detections, fpgad_xfpga_port_detections);
  EXPECT_EQ(d.detection_contexts, fpgad_xfpga_port_detection_contexts);
  EXPECT_EQ(d.responses, fpgad_xfpga_port_responses);
  EXPECT_EQ(d.response_contexts, fpgad_xfpga_port_response_contexts);

  fpgad_plugin_destroy(&d);
}

INSTANTIATE_TEST_CASE_P(fpgad_c, mock_port_fpgad_xfpga_c_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p" })));


class mock_fme_fpgad_xfpga_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  mock_fme_fpgad_xfpga_c_p()
    : tokens_{{nullptr, nullptr}} {}

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
                            &num_matches_),
              FPGA_OK);
    EXPECT_GT(num_matches_, 0);
    device_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_[0], &device_, 0), FPGA_OK);

    log_set(stdout);
  }

  virtual void TearDown() override {
    log_close();

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
    fpgaFinalize();
    system_->finalize();
  }

  void init_monitored_device(fpgad_monitored_device *d,
                             fpgad_supported_device *s)
  {
    memset_s(d, sizeof(fpgad_monitored_device), 0);
    d->token = tokens_[0];
    d->object_type = FPGA_DEVICE;
    d->num_error_occurrences = 0;
    d->supported = s;

    s->vendor_id = 0x8086;
    s->device_id = 0x0b30;
    s->library_path = "libfpgad-xfpga.so";
    s->flags = FPGAD_DEV_DETECTED|FPGAD_DEV_LOADED;
    s->dl_handle = NULL;
  }

  void init_AP6_context(fpgad_xfpga_Error_context *c)
  {
    c->sysfs_file = "errors/nonfatal_errors";
    c->message = "RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP6";
    c->low_bit = 9;
    c->high_bit = 9;
  }

  void set_AP6_state(bool state)
  {
    fpga_object obj = nullptr;
    uint64_t value = 0x200ULL;

    ASSERT_EQ(fpgaHandleGetObject(device_,
                                  "errors/nonfatal_errors",
                                  &obj,
                                  0), FPGA_OK);

    EXPECT_EQ(fpgaObjectWrite64(obj,
                                state ? value : 0ULL,
                                0), FPGA_OK);

    EXPECT_EQ(fpgaDestroyObject(&obj), FPGA_OK);
  }

  void init_KtiLinkFatal_context(fpgad_xfpga_Error_context *c)
  {
    c->sysfs_file = "errors/catfatal_errors";
    c->message = "RAS_CATFAT_ERROR_STAT[0x4060].KtiLinkFatalErr";
    c->low_bit = 0;
    c->high_bit = 0;
  }

  void set_KtiLinkFatal_state(bool state)
  {
    fpga_object obj = nullptr;
    uint64_t value = 1ULL;

    ASSERT_EQ(fpgaHandleGetObject(device_,
                                  "errors/catfatal_errors",
                                  &obj,
                                  0), FPGA_OK);

    EXPECT_EQ(fpgaObjectWrite64(obj,
                                state ? value : 0ULL,
                                0), FPGA_OK);

    EXPECT_EQ(fpgaDestroyObject(&obj), FPGA_OK);
  }

  std::array<fpga_token, 2> tokens_;
  fpga_handle device_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};

/**
 * @test       AP6
 * @brief      Test: fpgad_xfpga_detect_Error, fpgad_xfpga_respond_AP6
 * @details    Test the plugin's ability to detect/respond to AP6 (FME).<br>
 */
TEST_P(mock_fme_fpgad_xfpga_c_p, AP6) {
  fpgad_monitored_device d;
  fpgad_supported_device s;

  init_monitored_device(&d, &s);

  fpgad_xfpga_Error_context context;
  init_AP6_context(&context);

  set_AP6_state(true);

  EXPECT_EQ(fpgad_xfpga_detect_Error(&d, &context),
            FPGAD_STATUS_DETECTED);
  EXPECT_EQ(d.num_error_occurrences, 1);
  EXPECT_EQ(d.error_occurrences[0], &context);

  ::testing::internal::CaptureStdout();
  fpgad_xfpga_respond_AP6(&d, &context);
  std::string out = ::testing::internal::GetCapturedStdout();

  EXPECT_STREQ(out.c_str(), "fpgad-xfpga: RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP6\n");

  set_AP6_state(false);

  EXPECT_EQ(fpgad_xfpga_detect_Error(&d, &context),
            FPGAD_STATUS_NOT_DETECTED);
  EXPECT_EQ(d.num_error_occurrences, 0);
}

/**
 * @test       KtiLinkFatal
 * @brief      Test: fpgad_xfpga_detect_Error, fpgad_xfpga_respond_LogError
 * @details    Test the plugin's ability to detect/respond to KtiLinkFatal.<br>
 */
TEST_P(mock_fme_fpgad_xfpga_c_p, KtiLinkFatal) {
  fpgad_monitored_device d;
  fpgad_supported_device s;
  init_monitored_device(&d, &s);

  fpgad_xfpga_Error_context context;
  init_KtiLinkFatal_context(&context);

  set_KtiLinkFatal_state(true);

  EXPECT_EQ(fpgad_xfpga_detect_Error(&d, &context),
            FPGAD_STATUS_DETECTED);
  EXPECT_EQ(d.num_error_occurrences, 1);
  EXPECT_EQ(d.error_occurrences[0], &context);

  ::testing::internal::CaptureStdout();
  fpgad_xfpga_respond_LogError(&d, &context);
  std::string out = ::testing::internal::GetCapturedStdout();

  EXPECT_STREQ(out.c_str(), "fpgad-xfpga: RAS_CATFAT_ERROR_STAT[0x4060].KtiLinkFatalErr\n");

  set_KtiLinkFatal_state(false);

  EXPECT_EQ(fpgad_xfpga_detect_Error(&d, &context),
            FPGAD_STATUS_NOT_DETECTED);
  EXPECT_EQ(d.num_error_occurrences, 0);
}

/**
 * @test       configure
 * @brief      Test: fpgad_plugin_configure, fpgad_plugin_destroy
 * @details    Test the plugin's configure and destroy fn's. (FME)<br>
 */
TEST_P(mock_fme_fpgad_xfpga_c_p, configure) {
  fpgad_monitored_device d;
  fpgad_supported_device s;
  init_monitored_device(&d, &s);

  EXPECT_EQ(fpgad_plugin_configure(&d, NULL), 0);

  EXPECT_EQ(d.type, FPGAD_PLUGIN_TYPE_CALLBACK);
  EXPECT_EQ(d.detections, fpgad_xfpga_fme_detections);
  EXPECT_EQ(d.detection_contexts, fpgad_xfpga_fme_detection_contexts);
  EXPECT_EQ(d.responses, fpgad_xfpga_fme_responses);
  EXPECT_EQ(d.response_contexts, fpgad_xfpga_fme_response_contexts);

  fpgad_plugin_destroy(&d);
}

INSTANTIATE_TEST_CASE_P(fpgad_c, mock_fme_fpgad_xfpga_c_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p" })));
