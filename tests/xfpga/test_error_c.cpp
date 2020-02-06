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
#include "error_int.h"
#include "safe_string/safe_string.h"
#include "token_list_int.h"
}

#include <opae/error.h>
#include <props.h>
#include <fstream>
#include <string>
#include "gtest/gtest.h"
#include "mock/test_system.h"
#include "types_int.h"
#include "sysfs_int.h"
#include "xfpga.h"


extern "C" {
int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}

using namespace opae::testing;
const std::string sysfs_fme =
    "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0";
const std::string dev_fme = "/dev/intel-fpga-fme.0";
const std::string sysfs_port =
    "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0";
const std::string dev_port = "/dev/intel-fpga-port.0";

class error_c_mock_p : public ::testing::TestWithParam<std::string> {
 public:
  int delete_errors(std::string, std::string);

 protected:
  error_c_mock_p() : filter_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    tmpsysfs_ = system_->get_root();
    ASSERT_EQ(FPGA_OK, xfpga_plugin_initialize());
    if (sysfs_device_count() > 0) {
      const sysfs_fpga_device *device = sysfs_get_device(0);
      ASSERT_NE(device, nullptr);
      if (device->fme) {
        sysfs_fme = std::string(device->fme->sysfs_path);
        dev_fme = std::string("/dev/") + std::string(device->fme->sysfs_name);
      }
      if (device->port) {
        sysfs_port = std::string(device->port->sysfs_path);
        dev_port = std::string("/dev/") + std::string(device->port->sysfs_name);
      }
    }
    strncpy_s(fake_port_token_.sysfspath, sizeof(fake_port_token_.sysfspath),
              sysfs_port.c_str(), sysfs_port.size());
    strncpy_s(fake_port_token_.devpath, sizeof(fake_port_token_.devpath),
              dev_port.c_str(), dev_port.size());
    fake_port_token_.magic = FPGA_TOKEN_MAGIC;
    fake_port_token_.device_instance = 0;
    fake_port_token_.subdev_instance = 0;
    fake_port_token_.errors = nullptr;

    strncpy_s(fake_fme_token_.sysfspath, sizeof(fake_fme_token_.sysfspath),
              sysfs_fme.c_str(), sysfs_fme.size());
    strncpy_s(fake_fme_token_.devpath, sizeof(fake_fme_token_.devpath),
              dev_fme.c_str(), dev_fme.size());
    fake_fme_token_.magic = FPGA_TOKEN_MAGIC;
    fake_fme_token_.device_instance = 0;
    fake_fme_token_.subdev_instance = 0;
    fake_fme_token_.errors = nullptr;
  }

  virtual void TearDown() override {
    if (fake_fme_token_.errors) {
      free_error_list(fake_fme_token_.errors);
    }
    if (fake_port_token_.errors) {
      free_error_list(fake_port_token_.errors);
    }
    if (filter_) {
      EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
      filter_ = nullptr;
    }
    token_cleanup();
    tmpsysfs_ = "";
	xfpga_plugin_finalize();
    system_->finalize();
  }

  void free_error_list(struct error_list *p) {
    while (p) {
      struct error_list *q = p->next;
      free(p);
      p = q;
    }
  }

  fpga_properties filter_;
  std::string tmpsysfs_;
  test_platform platform_;
  test_system *system_;
  _fpga_token fake_fme_token_;
  _fpga_token fake_port_token_;
  std::string sysfs_fme;
  std::string dev_fme;
  std::string sysfs_port;
  std::string dev_port;
};

int error_c_mock_p::delete_errors(std::string fpga_type, std::string filename) {
  int result;
  std::string fme_sysfspath;
  std::string port_sysfspath;
  std::string cmd;

  if (tmpsysfs_.length() < 2) {
    fme_sysfspath = tmpsysfs_ + sysfs_fme + "/" + filename;
    port_sysfspath = tmpsysfs_ + sysfs_port + "/" + filename;
  } else {
    fme_sysfspath = tmpsysfs_ + "/" + sysfs_fme + "/" + filename;
    port_sysfspath = tmpsysfs_ + "/" + sysfs_port + "/" + filename;
  }

  if (fpga_type.compare("fme") == 0) {
    cmd = "rm -rf " + fme_sysfspath;
    goto remove;
  } else if (fpga_type.compare("port") == 0) {
    cmd = "rm -rf " + port_sysfspath;
    goto remove;
  } else {
    return -1;
  }

remove:
  result = std::system(cmd.c_str());
  (void)result;
  return 1;
}

/**
 * @test       error_01
 *
 * @brief      When passed a valid AFU token, the combination of
 * fpgaGetProperties()
 *             fpgaPropertiesGetNumErrors(), fpgaPropertiesGetErrorInfo() and
 *             fpgaReadError() is able to print the status of all error
 * registers.
 *
 */
TEST_P(error_c_mock_p, error_01) {
#ifndef BUILD_ASE
  fpga_error_info info;
  unsigned int n = 0;
  unsigned int i = 0;
  uint64_t val = 0;
  fpga_token t = &fake_port_token_;

  std::string errpath = sysfs_port + "/errors";
  build_error_list(errpath.c_str(), &fake_port_token_.errors);

  // get number of error registers
  ASSERT_EQ(FPGA_OK, xfpga_fpgaGetProperties(t, &filter_));
  auto _prop = (_fpga_properties *)filter_;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_ERRORS);
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetNumErrors(filter_, &n));
  printf("Found %d PORT error registers\n", n);

  // for each error register, get info and read the current value
  for (i = 0; i < n; i++) {
    // get info struct for error register
    ASSERT_EQ(FPGA_OK, xfpga_fpgaGetErrorInfo(t, i, &info));
    EXPECT_EQ(FPGA_OK, xfpga_fpgaReadError(t, i, &val));
    printf("[%u] %s: 0x%016lX%s\n", i, info.name, val,
           info.can_clear ? " (can clear)" : "");
  }

  auto result = delete_errors("port", "errors");
  (void)result;
  // for each error register, get info and read the current value
  for (i = 0; i < n; i++) {
    // get info struct for error register
    ASSERT_EQ(FPGA_OK, xfpga_fpgaGetErrorInfo(t, i, &info));
    EXPECT_EQ(FPGA_EXCEPTION, xfpga_fpgaReadError(t, i, &val));
    printf("[%u] %s: 0x%016lX%s\n", i, info.name, val,
           info.can_clear ? " (can clear)" : "");
  }
#endif
}

/**
 * @test       error_02
 *
 * @brief      When passed a valid FME token, the combination of
 * fpgaGetProperties()
 *             fpgaPropertiesGetNumErrors(), fpgaPropertiesGetErrorInfo() and
 *             fpgaReadError() is able to print the status of all error
 * registers.
 *
 */
TEST_P(error_c_mock_p, error_02) {
#ifndef BUILD_ASE
  fpga_error_info info;
  unsigned int n = 0;
  unsigned int i = 0;
  uint64_t val = 0;
  fpga_token t = &fake_fme_token_;

  std::string errpath = sysfs_fme + "/errors";
  build_error_list(errpath.c_str(), &fake_fme_token_.errors);

  // get number of error registers
  ASSERT_EQ(FPGA_OK, xfpga_fpgaGetProperties(t, &filter_));
  auto _prop = (_fpga_properties *)filter_;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_ERRORS);
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetNumErrors(filter_, &n));
  printf("Found %d FME error registers\n", n);

  // for each error register, get info and read the current value
  for (i = 0; i < n; i++) {
    // get info struct for error register
    ASSERT_EQ(FPGA_OK, xfpga_fpgaGetErrorInfo(t, i, &info));
    EXPECT_EQ(FPGA_OK, xfpga_fpgaReadError(t, i, &val));
    printf("[%u] %s: 0x%016lX%s\n", i, info.name, val,
           info.can_clear ? " (can clear)" : "");
  }

  auto result = delete_errors("fme", "errors");
  (void)result;
  for (i = 0; i < n; i++) {
    // get info struct for error register
    ASSERT_EQ(FPGA_OK, xfpga_fpgaGetErrorInfo(t, i, &info));
    EXPECT_NE(FPGA_OK, xfpga_fpgaReadError(t, i, &val));
    printf("[%u] %s: 0x%016lX%s\n", i, info.name, val,
           info.can_clear ? " (can clear)" : "");
  }
#endif
}

/**
 * @test       error_03
 *
 * @brief      When passed a valid AFU token for an AFU with PORT errors,
 *             fpgaReadError() will report the correct error, and
 *             fpgaClearError() will clear it.
 *
 */
TEST_P(error_c_mock_p, error_03) {
  std::fstream clear_file;
  std::ofstream error_file;
  std::string clear_name = tmpsysfs_ + sysfs_port + "/errors/clear";
  std::string error_name = tmpsysfs_ + sysfs_port + "/errors/errors";
  uint64_t clear_val;

  fpga_error_info info;
  unsigned int n = 0;
  unsigned int i = 0;
  uint64_t val = 0;
  fpga_token t = &fake_port_token_;

  std::string errpath = sysfs_port + "/errors";
  build_error_list(errpath.c_str(), &fake_port_token_.errors);

  // get number of error registers
  ASSERT_EQ(FPGA_OK, xfpga_fpgaGetProperties(t, &filter_));
  auto _prop = (_fpga_properties *)filter_;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_ERRORS);
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetNumErrors(filter_, &n));
  printf("Found %d PORT error registers\n", n);

  // for each error register, get info and read the current value
  for (i = 0; i < n; i++) {
    // get info struct for error register
    ASSERT_EQ(FPGA_OK, xfpga_fpgaGetErrorInfo(t, i, &info));
    EXPECT_EQ(FPGA_OK, xfpga_fpgaReadError(t, i, &val));
    ASSERT_EQ(val, 0);
  }

  // ------------- MAKE SURE CLEAR FILE IS 0 ------------
  clear_file.open(clear_name);
  ASSERT_EQ(1, clear_file.is_open());
  clear_file >> clear_val;
  clear_file.close();
  ASSERT_EQ(clear_val, 0);

  // ------------- INJECT PORT ERROR --------------------
  error_file.open(error_name);
  ASSERT_EQ(1, error_file.is_open());
  error_file << "0x42" << std::endl;
  error_file.close();

  // for each error register, get info and read the current value
  for (i = 0; i < n; i++) {
    // get info struct for error register
    ASSERT_EQ(FPGA_OK, xfpga_fpgaGetErrorInfo(t, i, &info));
    EXPECT_EQ(FPGA_OK, xfpga_fpgaReadError(t, i, &val));
    // if error, try to clear it (and check result)
    if (val != 0) {
      printf("[%u] %s: 0x%016lX%s\n", i, info.name, val,
             info.can_clear ? " (can clear)" : "");
      EXPECT_EQ(FPGA_OK, xfpga_fpgaClearError(t, i));
      // check if value was written to clear file
      clear_file.open(clear_name.c_str());
      clear_file >> std::hex >> clear_val;
      clear_file.close();
      ASSERT_EQ(clear_val, val);
    }
  }

  // --------------- WRITE 0 TO CLEAR AND ERROR FILES (CLEAN UP) -------------
  error_file.open(error_name);
  error_file << "0x0" << std::endl;
  error_file.close();
  clear_file.open(clear_name);
  clear_file << "0x0" << std::endl;
  clear_file.close();
}

/**
 * @test       error_04
 *
 * @brief      When passed a valid AFU token for an AFU with PORT errors,
 *             fpgaReadError() will report the correct error, and
 *             fpgaClearAllErrors() will clear it.
 *
 */
TEST_P(error_c_mock_p, error_04) {
  std::fstream clear_file;
  std::ofstream error_file;
  std::string clear_name = tmpsysfs_ + sysfs_port + "/errors/clear";
  std::string error_name = tmpsysfs_ + sysfs_port + "/errors/errors";
  uint64_t clear_val;

  fpga_error_info info;
  unsigned int n = 0;
  unsigned int i = 0;
  uint64_t val = 0;
  fpga_token t = &fake_port_token_;

  std::string errpath = sysfs_port + "/errors";
  build_error_list(errpath.c_str(), &fake_port_token_.errors);

  // get number of error registers
  ASSERT_EQ(FPGA_OK, xfpga_fpgaGetProperties(t, &filter_));
  auto _prop = (_fpga_properties *)filter_;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_ERRORS);
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetNumErrors(filter_, &n));
  printf("Found %d PORT error registers\n", n);

  // for each error register, get info and read the current value
  for (i = 0; i < n; i++) {
    // get info struct for error register
    ASSERT_EQ(FPGA_OK, xfpga_fpgaGetErrorInfo(t, i, &info));
    EXPECT_EQ(FPGA_OK, xfpga_fpgaReadError(t, i, &val));
    ASSERT_EQ(val, 0);
  }

  // ------------- MAKE SURE CLEAR FILE IS 0 ------------
  clear_file.open(clear_name);
  ASSERT_EQ(1, clear_file.is_open());
  clear_file >> clear_val;
  clear_file.close();
  ASSERT_EQ(clear_val, 0);

  // ------------- INJECT PORT ERROR --------------------
  error_file.open(error_name);
  ASSERT_EQ(1, error_file.is_open());
  error_file << "0x42" << std::endl;
  error_file.close();

  // for each error register, get info and read the current value
  for (i = 0; i < n; i++) {
    // get info struct for error register
    EXPECT_EQ(FPGA_OK, xfpga_fpgaGetErrorInfo(t, i, &info));
    EXPECT_EQ(FPGA_OK, xfpga_fpgaReadError(t, i, &val));
    // if error, try to clear it (and check result)
    if (val != 0) {
      printf("[%u] %s: 0x%016lX%s\n", i, info.name, val,
             info.can_clear ? " (can clear)" : "");
      EXPECT_EQ(FPGA_OK, xfpga_fpgaClearAllErrors(t));
      // check if value was written to clear file
      clear_file.open(clear_name);
      clear_file >> std::hex >> clear_val;
      clear_file.close();
      EXPECT_EQ(clear_val, val);
    }
  }

  // --------------- WRITE 0 TO CLEAR AND ERROR FILES (CLEAN UP) -------------
  error_file.open(error_name);
  error_file << "0x0" << std::endl;
  error_file.close();
  clear_file.open(clear_name);
  clear_file << "0x0" << std::endl;
  clear_file.close();
}

/**
 * @test       error_05
 *
 * @brief      When passed a valid AFU token for an AFU with PORT errors,
 *             fpgaReadError() will report the correct error, and
 *             fpgaClearError() will clear it.
 *
 */
TEST_P(error_c_mock_p, error_05) {
  unsigned int n = 0;
  fpga_token t = &fake_port_token_;

  std::string errpath = sysfs_port + "/errors";
  build_error_list(errpath.c_str(), &fake_port_token_.errors);

  // get number of error registers
  ASSERT_EQ(FPGA_OK, xfpga_fpgaGetProperties(t, &filter_));
  auto _prop = (_fpga_properties *)filter_;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_ERRORS);
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetNumErrors(filter_, &n));
  printf("Found %d PORT error registers\n", n);

  struct error_list *p = fake_port_token_.errors;
  p->info.can_clear = false;
  EXPECT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaClearError(t, 0));
}

/**
 * @test       error_06
 *
 * @brief      When passed a valid FME token,
 *             fpgaReadError() will report the correct error, and
 *             fpgaClearError() will clear it.
 *
 */
TEST_P(error_c_mock_p, error_06) {
  fpga_error_info info;
  unsigned int n = 0;
  unsigned int i = 0;
  fpga_token t = &fake_fme_token_;

  std::string errpath = sysfs_fme + "/errors";
  build_error_list(errpath.c_str(), &fake_fme_token_.errors);

  // get number of error registers
  ASSERT_EQ(FPGA_OK, xfpga_fpgaGetProperties(t, &filter_));
  auto _prop = (_fpga_properties *)filter_;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_ERRORS);
  // ASSERT_EQ(FPGA_OK, xfpga_fpgaPropertiesGetNumErrors(filter_, &n));
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetNumErrors(filter_, &n));
  printf("Found %d FME error registers\n", n);

  // for each error register, get info and read the current value
  for (i = 0; i < n; i++) {
    // get info struct for error register
    ASSERT_EQ(FPGA_OK, xfpga_fpgaGetErrorInfo(t, i, &info));
    // if error, try to clear it (and check result)
    if (info.can_clear) {
      EXPECT_EQ(FPGA_OK, xfpga_fpgaClearError(t, i));
    }
  }

  free_error_list(fake_fme_token_.errors);

  // set error list to null
  fake_fme_token_.errors = nullptr;
  EXPECT_EQ(FPGA_NOT_FOUND, xfpga_fpgaClearError(t, 0));
}
/**
 * @test       error_07
 *
 * @brief      When passed a valid FME token,
 *             and delete error removes errors dir
 *             fpgaReadError() and fpgaClearError will
 *             returns FPGA_EXCEPTION
 *
 */
TEST_P(error_c_mock_p, error_07) {
  fpga_error_info info;
  fpga_token t = &fake_fme_token_;
  uint32_t num_errors = 0, i = 0;

  std::string errpath = sysfs_fme + "/errors";
  // build errors and immediately remove errors dir
  build_error_list(errpath.c_str(), &fake_fme_token_.errors);

  ASSERT_EQ(FPGA_OK, xfpga_fpgaGetProperties(t, &filter_));
  ASSERT_EQ(fpgaPropertiesGetNumErrors(filter_, &num_errors), FPGA_OK);
  ASSERT_NE(num_errors, 0) << "No errors to clear";
  for (i = 0; i < num_errors; i++) {
    ASSERT_EQ(xfpga_fpgaGetErrorInfo(t, i, &info), FPGA_OK);
    if (info.can_clear) {
      auto ret = delete_errors("fme", "errors");
      // get the clearable error
      if (ret) {
        EXPECT_EQ(FPGA_EXCEPTION, xfpga_fpgaClearError(t, i));
      }
      break;
    }
  }
  EXPECT_NE(i, num_errors) << "Did not attempt to clear errors";
}

/**
 * @test       error_08

 *             fpgaReadError() will report the correct error, and
 *             fpgaClearError() will clear it.
 *
 */
TEST_P(error_c_mock_p, error_08) {
  unsigned int n = 0;
  fpga_token t = &fake_port_token_;

  std::string errpath = sysfs_port + "/errors";
  build_error_list(errpath.c_str(), &fake_port_token_.errors);

  // get number of error registers
  ASSERT_EQ(FPGA_OK, xfpga_fpgaGetProperties(t, &filter_));
  auto _prop = (_fpga_properties *)filter_;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_ERRORS);
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetNumErrors(filter_, &n));
  printf("Found %d PORT error registers\n", n);

  EXPECT_EQ(FPGA_OK, xfpga_fpgaClearAllErrors(t));
}

/**
 * @test       error_09
 *
 * @brief      When passed a valid FME token,
 *             fpgaReadError() will report the correct error, and
 *             fpgaClearError() will clear it.
 *
 */
TEST_P(error_c_mock_p, error_09) {
  unsigned int n = 0;
  fpga_token t = &fake_fme_token_;

  std::string errpath = sysfs_fme + "/errors";
  build_error_list(errpath.c_str(), &fake_fme_token_.errors);

  // get number of error registers
  ASSERT_EQ(FPGA_OK, xfpga_fpgaGetProperties(t, &filter_));
  auto _prop = (_fpga_properties *)filter_;
  SET_FIELD_VALID(_prop, FPGA_PROPERTY_NUM_ERRORS);
  ASSERT_EQ(FPGA_OK, fpgaPropertiesGetNumErrors(filter_, &n));
  printf("Found %d PORT error registers\n", n);

  EXPECT_EQ(FPGA_OK, xfpga_fpgaClearAllErrors(t));
}

/**
 * @test       error_12
 * @brief      When passed an invalid token magic,
 *             xfpga_fpgaClearAllErrors() should return FPGA_INVALID_PARAM.
 */
TEST_P(error_c_mock_p, error_12) {
  auto fme = token_add(sysfs_fme.c_str(), dev_fme.c_str());
  ASSERT_NE(fme, nullptr);
  auto port = token_add(sysfs_port.c_str(), dev_port.c_str());
  ASSERT_NE(port, nullptr);
  auto parent = token_get_parent(port);
  EXPECT_EQ(parent, fme);
  auto tok = (struct _fpga_token *)parent;

  tok->magic = FPGA_TOKEN_MAGIC;
  EXPECT_EQ(FPGA_OK, xfpga_fpgaClearAllErrors(parent));
  tok->magic = 0x123;
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaClearAllErrors(parent));
}

INSTANTIATE_TEST_CASE_P(error_c, error_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p","dcp-rc","dcp-vc" })));

class error_c_p : public error_c_mock_p {};

/**
 * @test       error_10
 *
 * @brief      When passed an invalid token magic,
 *             xfpga_fpgaReadError() should return FPGA_INVALID_PARAM.
 *             when token doesn't have errpath
 *             xfpga_fpgaReadError() should return FPGA_NOT_FOUND.
 *
 */
TEST_P(error_c_p, error_10) {
  auto fme = token_add(sysfs_fme.c_str(), dev_fme.c_str());
  ASSERT_NE(fme, nullptr);
  auto port = token_add(sysfs_port.c_str(), dev_port.c_str());
  ASSERT_NE(port, nullptr);
  auto parent = token_get_parent(port);
  EXPECT_EQ(parent, fme);
  auto tok = (struct _fpga_token *)parent;

  uint64_t val = 0;
  tok->magic = 0x123;
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaReadError(parent, 0, &val));

  tok->magic = FPGA_TOKEN_MAGIC;
  EXPECT_EQ(FPGA_OK, xfpga_fpgaReadError(parent, 0, &val));
  EXPECT_EQ(FPGA_NOT_FOUND, xfpga_fpgaReadError(parent, 1000, &val));
}

/**
 * @test       error_11
 *
 * @brief      When passed an invalid token magic,
 *             xfpga_fpgaClearError() should return FPGA_INVALID_PARAM.
 *             when token doesn't have errpath
 *             xfpga_fpgaClearError() should return FPGA_NOT_FOUND.
 *
 */
TEST_P(error_c_p, error_11) {
  auto fme = token_add(sysfs_fme.c_str(), dev_fme.c_str());
  ASSERT_NE(fme, nullptr);
  auto port = token_add(sysfs_port.c_str(), dev_port.c_str());
  ASSERT_NE(port, nullptr);
  auto parent = token_get_parent(port);
  EXPECT_EQ(parent, fme);
  auto tok = (struct _fpga_token *)parent;

  EXPECT_EQ(FPGA_NOT_FOUND, xfpga_fpgaClearError(parent, 1000));
  tok->magic = 0x123;
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaClearError(parent, 0));
}

/**
 * @test       error_13
 * @brief      When passed an invalid token magic,
 *             xfpga_fpgaClearAllErrors() should return FPGA_INVALID_PARAM.
 *             when token doesn't have errpath
 *             xfpga_fpgaClearAllErrors() should return FPGA_NOT_FOUND.
 */
TEST_P(error_c_p, error_13) {
  auto fme = token_add(sysfs_fme.c_str(), dev_fme.c_str());
  ASSERT_NE(fme, nullptr);
  auto port = token_add(sysfs_port.c_str(), dev_port.c_str());
  ASSERT_NE(port, nullptr);
  auto parent = token_get_parent(port);
  EXPECT_EQ(parent, fme);
  auto tok = (struct _fpga_token *)parent;

  struct fpga_error_info info;
  tok->magic = FPGA_TOKEN_MAGIC;
  EXPECT_EQ(FPGA_NOT_FOUND, xfpga_fpgaGetErrorInfo(parent, 1000, &info));
  tok->magic = 0x123;
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetErrorInfo(parent, 0, &info));
}

INSTANTIATE_TEST_CASE_P(error_c, error_c_p,
                        ::testing::ValuesIn(test_platform::platforms({ "skx-p","dcp-rc","dcp-vc"  })));

/**
 * @test       error_01
 *
 * @brief      When passed an NULL token
 *             xfpga_fpgaReadError() should return FPGA_INVALID_PARAM.
 *             xfpga_fpgaClearError() should return FPGA_INVALID_PARAM.
 *             xfpga_fpgaClearAllErrors() should return FPGA_INVALID_PARAM.
 *
 */
TEST(error_c, error_01) {
  fpga_token tok = NULL;
  uint64_t val = 0;

  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaReadError(tok, 0, &val));
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaClearError(tok, 0));
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaClearAllErrors(tok));
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaGetErrorInfo(tok, 0, NULL));
}

/**
 * @test       error_06
 *
 * @brief      When passed in invalid errors path to build_error_list,
 *             the function returns 0 for file doesn't exist.
 *
 */
TEST(error_c, error_06) {
  struct _fpga_token _t;
  strncpy_s(_t.sysfspath, sizeof(_t.sysfspath), sysfs_port.c_str(),
            sysfs_port.size());
  strncpy_s(_t.devpath, sizeof(_t.devpath), dev_port.c_str(), dev_port.size());
  _t.magic = FPGA_TOKEN_MAGIC;
  _t.errors = nullptr;

  std::string invalid_errpath = sysfs_port + "/errorss";
  auto result = build_error_list(invalid_errpath.c_str(), &_t.errors);
  EXPECT_EQ(result, 0);
}

/**
 * @test       error_07
 *
 * @brief      When passed pathname longer than FILENAME_MAX
 *             build_error_list() should return and not build anything
 *
 *@note        Must set env-variable LIBOPAE_LOG=1 to run this test.
 *
 */
TEST(error_c, error_07) {
  struct error_list *el = NULL;
  std::string lpn(FILENAME_MAX + 1, 'a');
  std::string exptout("path too long");

  char *loglv = secure_getenv("LIBOPAE_LOG");
  if (loglv && atoi(loglv) > 0) {
    testing::internal::CaptureStdout();

    build_error_list(lpn.c_str(), &el);

    std::string actout = testing::internal::GetCapturedStdout();
    EXPECT_NE(std::string::npos, actout.find(exptout));
  }

  EXPECT_EQ(NULL, el);
}
