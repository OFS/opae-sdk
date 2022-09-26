// Copyright(c) 2019-2022, Intel Corporation
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

#define NO_OPAE_C
#include "mock/opae_fixtures.h"

extern "C" {
#include "fpgad/api/logging.h"
#include "fpgad/config_file.h"

extern fpgad_config_data default_fpgad_config_table[];
}

#include <linux/limits.h>
#include <sys/types.h>
#include <sys/wait.h>

using namespace opae::testing;

class fpgad_config_file_c_p : public opae_base_p<> {
 protected:

  virtual void SetUp() override {
    opae_base_p<>::SetUp();

    log_set(stdout);

    strcpy(cfg_file_, "fpgad-XXXXXX.cfg");
    opae_close(mkstemps(cfg_file_, 4));

    memset(&config_, 0, sizeof(config_));
    config_.poll_interval_usec = 100 * 1000;
    config_.running = true;
    config_.api_socket = "/tmp/fpga_event_socket";
    config_.cfgfile = cfg_file_;
  }

  virtual void TearDown() override {
    cmd_destroy(&config_);
    log_close();

    if (!::testing::Test::HasFatalFailure() &&
        !::testing::Test::HasNonfatalFailure()) {
      unlink(cfg_file_);
    }

    opae_base_p<>::TearDown();
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
};

/**
 * @test       load0
 * @brief      Test: cfg_load_config
 * @details    When the given fpgad_config has a NULL cfgfile member,<br>
 *             then the fn populates the supported_devices field with<br>
 *             default_fpgad_config_table.
 */
TEST_P(fpgad_config_file_c_p, load0) {
  config_.cfgfile = NULL;
  EXPECT_EQ(cfg_load_config(&config_), 0);
  EXPECT_EQ(default_fpgad_config_table, config_.supported_devices);
}

/**
 * @test       load1
 * @brief      Test: cfg_load_config
 * @details    When the given fpgad_config has a cfgfile member<br>
 *             that points to a valid config file,<br>
 *             then the function populates the supported_devices<br>
 *             member with a newly-allocated data table.
 */
TEST_P(fpgad_config_file_c_p, load1) {
  const char *json = R"json(
{
  "configurations": {

    "ofs": {
      "enabled": true,
      "devices": [
        { "name": "ofs0_pf", "id": [ "0x8086", "0xaf00", "0x8086", "0" ] },
        { "name": "ofs0_vf", "id": [ "0x8086", "0xaf01", "0x8086", "0" ] },
        { "name": "ofs1_pf", "id": [ "0x8086", "0xbcce", "0x8086", "0" ] },
        { "name": "ofs1_vf", "id": [ "0x8086", "0xbccf", "0x8086", "0" ] }
      ],

      "opae": {
        "fpgad": [
          {
            "enabled": true,
            "module": "libfpgad-vc.so",
            "devices": [ "ofs0_pf", "ofs1_pf" ],
            "configuration": {}
          }
        ]
      }
    }

  },

  "configs": [
    "ofs"
  ]
}
)json";

  write_cfg(json);
  EXPECT_EQ(cfg_load_config(&config_), 0);
  EXPECT_NE((fpgad_config_data *)NULL, config_.supported_devices);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(fpgad_config_file_c_p);
INSTANTIATE_TEST_SUITE_P(fpgad_config_file_c, fpgad_config_file_c_p,
                         ::testing::ValuesIn(test_platform::platforms({ "dfl-n3000" })));
