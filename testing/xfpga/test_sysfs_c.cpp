// Copyright(c) 2017-2018, Intel Corporation
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
#include <opae/utils.h>
fpga_result cat_token_sysfs_path(char *, fpga_token, const char *);
fpga_result get_port_sysfs(fpga_handle, char *);
//    fpga_result get_fpga_deviceid(fpga_handle,uint64_t*);
fpga_result sysfs_get_pr_id(int, int, fpga_guid);
fpga_result sysfs_get_slots(int, int, uint32_t *);
fpga_result sysfs_get_bitstream_id(int, int, uint64_t *);
fpga_result sysfs_sbdf_from_path(const char *, int *, int *, int *, int *);
fpga_result opae_glob_path(char *);
fpga_result make_sysfs_group(char *, const char *, fpga_object *, int,
                             fpga_handle);
ssize_t eintr_write(int, void *, size_t);
char* cstr_dup(const char *str);
}

#include <opae/enum.h>
#include <opae/fpga.h>
#include <opae/properties.h>
#include <sys/types.h>
#include <uuid/uuid.h>
#include <string>
#include <vector>
#include "sysfs_int.h"
#include "types_int.h"
#include "xfpga.h"
#include <fcntl.h>

#include "gtest/gtest.h"
#include "test_system.h"

const std::string single_sysfs_fme =
    "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0";
const std::string single_sysfs_port =
    "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-port.0";
const std::string single_dev_fme = "/dev/intel-fpga-fme.0";
const std::string single_dev_port = "/dev/intel-fpga-port.0";

using namespace opae::testing;

class sysfs_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  sysfs_c_p()
  : tokens_{{nullptr, nullptr}},
    handle_(nullptr){}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, 
                                        platform_.devices[0].device_id), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                                  &num_matches_),
              FPGA_OK);
    ASSERT_EQ(xfpga_fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (handle_) { 
        EXPECT_EQ(xfpga_fpgaClose(handle_), FPGA_OK); 
        handle_ = nullptr;
    }

    for (auto &t : tokens_) {
      if (t) {
          EXPECT_EQ(FPGA_OK, xfpga_fpgaDestroyToken(&t));
          t = nullptr;
      }
    }

    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};

/**
* @test    eintr_write_tests
* @details Given a valid fd but invalid buffer, eintr_writes
*          returns -1 on error.
*/
TEST(sysfs_c, eintr_write_tests) {
  void * data = nullptr;
  std::string filename = "empty_file.txt";
  EXPECT_EQ(std::system("touch empty_file.txt"), 0);

  int fd = open(filename.c_str(), O_RDWR);
  EXPECT_NE(fd, -1);
  size_t count = 1024;
  EXPECT_EQ(-1, eintr_write(fd, data, count));
  EXPECT_EQ(close(fd), 0);
  EXPECT_EQ(std::system("rm empty_file.txt"), 0);
}

/**
* @test    sysfs_invalid_tests
* @details When calling get_port_sysfs with invalid params
*          the functino returns FPGA_INVALID_PARAM
*/
TEST_P(sysfs_c_p, sysfs_invalid_tests) {
  const std::string sysfs_fme = "/sys/class/fpga/intel-fpga-dev/intel-fpga-fme";
  auto h = (struct _fpga_handle *)handle_;
  auto t = (struct _fpga_token *)h->token;

  char spath[SYSFS_PATH_MAX];
  fpga_result res;

  char invalid_string[] = "...";
  strncpy(t->sysfspath, invalid_string, sizeof(t->sysfspath));
  res = get_port_sysfs(handle_, spath);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);

  h->token = NULL;
  res = get_port_sysfs(handle_, spath);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);
}

/**
* @test    device_invalid_test
* @details
*/
TEST_P(sysfs_c_p, deviceid_invalid_tests) {
  const std::string sysfs_port =
      "/sys/class/fpga/intel-fpga-dev/intel-fpga-port";
  auto h = (struct _fpga_handle *)handle_;
  auto t = (struct _fpga_token *)h->token;
  uint64_t device_id;
  fpga_token tok;

  auto res = get_fpga_deviceid(handle_, NULL);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);

  tok = h->token;
  h->token = NULL;
  res = get_fpga_deviceid(handle_, &device_id);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);

  h->token = tok;
  res = get_fpga_deviceid(handle_, &device_id);
  EXPECT_EQ(FPGA_OK, res);

  strncpy(t->sysfspath, sysfs_port.c_str(), sizeof(t->sysfspath));
  res = get_fpga_deviceid(handle_, &device_id);
  EXPECT_EQ(FPGA_NOT_SUPPORTED, res);
}

/**
* @test    glob_test
* @details
*/
TEST_P(sysfs_c_p, glob_tests) {
  std::string invalid_filename = "opae";

  auto res = opae_glob_path(nullptr);
  EXPECT_EQ(FPGA_EXCEPTION, res);

  res = opae_glob_path(const_cast<char *>(invalid_filename.c_str()));
  EXPECT_EQ(FPGA_NOT_FOUND, res);
}

/**
* @test    cat_sysfs_path_errors
* @details
*/
TEST(sysfs_c, cat_sysfs_path_errors) {
  std::vector<char> buffer(256);
  std::string emptystring = "";
  EXPECT_EQ(FPGA_OK, cat_sysfs_path(buffer.data(), single_sysfs_port.c_str()));
  EXPECT_EQ(FPGA_INVALID_PARAM, cat_sysfs_path(buffer.data(), nullptr));
  EXPECT_EQ(FPGA_INVALID_PARAM,
            cat_sysfs_path(nullptr, single_sysfs_port.c_str()));
  EXPECT_EQ(FPGA_INVALID_PARAM, cat_sysfs_path(nullptr, nullptr));
}

/**
* @test   cat_token_sysfs_path
* @details
*/
TEST(sysfs_c, cat_token_sysfs_path) {
  _fpga_token tok;
  std::copy(single_sysfs_fme.begin(), single_sysfs_fme.end(),
            &tok.sysfspath[0]);
  tok.sysfspath[single_sysfs_fme.size()] = '\0';
  std::copy(single_dev_fme.begin(), single_dev_fme.end(), &tok.devpath[0]);
  tok.devpath[single_dev_fme.size()] = '\0';
  std::vector<char> buffer(256);
  EXPECT_EQ(cat_token_sysfs_path(buffer.data(), &tok, "bitstream_id"), FPGA_OK);
  EXPECT_STREQ(buffer.data(),
               std::string(single_sysfs_fme + "/bitstream_id").c_str());

  // null destination
  EXPECT_EQ(cat_token_sysfs_path(nullptr, &tok, "bitstream_id"),
            FPGA_EXCEPTION);
}

/**
* @test    cat_handle_sysfs_path
* @details
*/
TEST(sysfs_c, cat_handle_sysfs_path) {
  _fpga_token tok;
  _fpga_handle hnd;
  std::copy(single_sysfs_fme.begin(), single_sysfs_fme.end(),
            &tok.sysfspath[0]);
  tok.sysfspath[single_sysfs_fme.size()] = '\0';
  std::copy(single_dev_fme.begin(), single_dev_fme.end(), &tok.devpath[0]);
  tok.devpath[single_dev_fme.size()] = '\0';
  hnd.token = &tok;
  std::vector<char> buffer(256);
  EXPECT_EQ(cat_handle_sysfs_path(buffer.data(), &hnd, "bitstream_id"),
            FPGA_OK);
  EXPECT_STREQ(buffer.data(),
               std::string(single_sysfs_fme + "/bitstream_id").c_str());

  // null destination
  EXPECT_EQ(cat_handle_sysfs_path(nullptr, &hnd, "bitstream_id"),
            FPGA_EXCEPTION);
}

/**
* @test    make_object
* @details
*/
TEST_P(sysfs_c_p, make_object) {
  _fpga_token *tok = static_cast<_fpga_token *>(tokens_[0]);
  fpga_object object;
  // errors is a sysfs directory - this should call make_sysfs_group()
  ASSERT_EQ(make_sysfs_object(tok->sysfspath, "errors", &object, 0, 0),
            FPGA_OK);
  EXPECT_EQ(xfpga_fpgaDestroyObject(&object), FPGA_OK);
}

/**
* @test    fpga_sysfs_01
* @brief   Tests: sysfs_deviceid_from_path
* @details sysfs_deviceid_from_path giver device id
*          Then the return device id
*/

TEST_P(sysfs_c_p, fpga_sysfs_01) {
  uint64_t deviceid;
  fpga_result result;

  // Valid path
  result = sysfs_deviceid_from_path(
      "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0", &deviceid);
  ASSERT_EQ(result, FPGA_OK);

  // NULL input
  result = sysfs_deviceid_from_path(
      "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0", NULL);
  ASSERT_NE(result, FPGA_OK);

  // NULL input path
  result = sysfs_deviceid_from_path(NULL, NULL);
  ASSERT_NE(result, FPGA_OK);

  // Invalid path to get device id
  result = sysfs_deviceid_from_path(
      "/sys/class/fpga/intel-fpga-dev.0/intel-fpga.0", &deviceid);
  ASSERT_NE(result, FPGA_OK);

  result = sysfs_deviceid_from_path(
      "/sys/class/fpga/intel-fpga-dev.20/intel-fpga-fme", &deviceid);
  ASSERT_NE(result, FPGA_OK);

  result = sysfs_deviceid_from_path(
      "/sys/class/fpga/intel-fpga-dev/intel-fpga-fme", &deviceid);
  ASSERT_NE(result, FPGA_OK);
}

/**
* @test    fpga_sysfs_02
* @brief   Tests: sysfs_read_int,sysfs_read_u32
*          sysfs_read_u32_pair,sysfs_read_u64
*          sysfs_read_u64,sysfs_write_u64
*..........get_port_sysfs,sysfs_read_guid
*..........get_fpga_deviceid
*/
TEST_P(sysfs_c_p, fpga_sysfs_02) {
  fpga_result result;
  int i;
  uint32_t u32;
  uint32_t u1;
  uint32_t u2;
  uint64_t u64;

  // Empty input path string
  result = sysfs_read_int("", NULL);
  EXPECT_NE(result, FPGA_OK);

  // NULL input parameters
  result = sysfs_read_int(NULL, NULL);
  EXPECT_NE(result, FPGA_OK);

  // Invalid input path
  result = sysfs_read_int("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.10",
                          NULL);
  EXPECT_NE(result, FPGA_OK);

  result =
      sysfs_read_int("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0", NULL);
  EXPECT_NE(result, FPGA_OK);

  // Valid input path
  result = sysfs_read_int(
      "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/socket_id", &i);
  EXPECT_EQ(result, FPGA_OK);

  // Empty input path string
  result = sysfs_read_int("", NULL);
  EXPECT_NE(result, FPGA_OK);

  // Invalid input parameters
  result = sysfs_read_u32(NULL, NULL);
  EXPECT_NE(result, FPGA_OK);

  // Invalid input path
  result = sysfs_read_u32("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.10",
                          NULL);
  EXPECT_NE(result, FPGA_OK);

  result =
      sysfs_read_u32("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0", NULL);
  EXPECT_NE(result, FPGA_OK);

  // Valid input path
  result = sysfs_read_u32(
      "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/socket_id", &u32);
  EXPECT_EQ(result, FPGA_OK);

  // Invalid input parameters
  result = sysfs_read_u32_pair(NULL, NULL, NULL, '\0');
  EXPECT_NE(result, FPGA_OK);

  // Invalid input parameters
  result = sysfs_read_u32_pair(NULL, NULL, NULL, 'a');
  EXPECT_NE(result, FPGA_OK);

  // Invalid input 'sep' character
  result = sysfs_read_u32_pair(
      "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/socket_id", &u1, &u2,
      '\0');
  EXPECT_NE(result, FPGA_OK);

  // Invalid input path value
  result = sysfs_read_u32_pair(
      "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/socket_id", &u1, &u2,
      'a');
  EXPECT_NE(result, FPGA_OK);

  // Invalid input path type
  result = sysfs_read_u32_pair(
      "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0", &u1, &u2, 'a');
  EXPECT_NE(result, FPGA_OK);

  // Invalid input path
  result = sysfs_read_u32_pair(
      "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.10", &u1, &u2, 'a');
  EXPECT_NE(result, FPGA_OK);

  // Empty input path string
  result = sysfs_read_u64("", NULL);
  EXPECT_NE(result, FPGA_OK);

  // NULL input parameters
  result = sysfs_read_u64(NULL, NULL);
  EXPECT_NE(result, FPGA_OK);

  // Invalid input path
  result = sysfs_read_u64("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.10",
                          NULL);
  EXPECT_NE(result, FPGA_OK);

  // Valid input path
  result = sysfs_read_u64(
      "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/socket_id", &u64);
  EXPECT_EQ(result, FPGA_OK);

  // Invalid input parameters
  result = sysfs_write_u64(NULL, 0);
  EXPECT_NE(result, FPGA_OK);

  result = sysfs_write_u64("/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0",
                           0x100);
  EXPECT_NE(result, FPGA_OK);

  // Invalid input parameters
  fpga_guid guid;
  result = sysfs_read_guid(NULL, NULL);
  EXPECT_NE(result, FPGA_OK);

  result = sysfs_read_guid(
      "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.10/", guid);
  EXPECT_NE(result, FPGA_OK);

  // NULL input parameters
  result = get_port_sysfs(NULL, NULL);
  EXPECT_NE(result, FPGA_OK);

  // NULL handle
  result = get_port_sysfs(
      NULL,
      (char *)"/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/socket_id");
  EXPECT_NE(result, FPGA_OK);

  // NULL handle
  result = get_fpga_deviceid(NULL, NULL);
  EXPECT_NE(result, FPGA_OK);
}

/**
* @test    sysfs_sbdf_invalid_tests
* @details When calling sysfs_sbdf_from path with invalid params
*          the function returns FPGA_NO_DRIVER
*/
TEST(sysfs_c, sysfs_sbdf_invalid_tests) {
  std::string sysfs_dev =
      "/sys/devices/pci0000:5e/0000:5e:00.0/fpga/intel-fpga-dev.0";

  int s = 0, b = 0, d = 0, f = 0;
  auto res = sysfs_sbdf_from_path(sysfs_dev.c_str(), &s, &b, &d, &f);
  EXPECT_EQ(FPGA_NO_DRIVER, res);
}

/**
* @test    get_fpga_deviceid
* @details get_fpga_device given a valid parameters
*          return FPGA_OK
*/
TEST_P(sysfs_c_p, get_fpga_deviceid) {
  uint64_t deviceid;
  uint64_t real_deviceid = platform_.devices[0].device_id;
  auto res = get_fpga_deviceid(handle_, &deviceid);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(real_deviceid, deviceid);
}

/**
* @test    cstr_dup
* @details Duplicate an input string
*/
TEST(sysfs_c, cstr_dup) {
  std::string inp("this is an input string");
  char *dup = cstr_dup(inp.c_str());
  EXPECT_STREQ(dup, inp.c_str());
  free(dup);
}

INSTANTIATE_TEST_CASE_P(sysfs_c, sysfs_c_p,
                        ::testing::ValuesIn(test_platform::platforms({})));

class sysfs_c_hw_p : public sysfs_c_p {
  protected:
    sysfs_c_hw_p() {}
};

/**
 * @test    make_sysfs_group
 * @details
 */
TEST_P(sysfs_c_hw_p, make_sysfs) {
  const std::string invalid_path =
      "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme";
  _fpga_token *tok = static_cast<_fpga_token *>(tokens_[0]);
  fpga_object obj;
  auto res = make_sysfs_group(tok->sysfspath, "errors", &obj, 0, handle_);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(xfpga_fpgaDestroyObject(&obj), FPGA_OK);

  res = make_sysfs_group(tok->sysfspath, "errors", &obj, FPGA_OBJECT_GLOB,
                         handle_);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(xfpga_fpgaDestroyObject(&obj), FPGA_OK);

  res = make_sysfs_group(const_cast<char *>(invalid_path.c_str()), "errors",
                         &obj, 0, handle_);
  EXPECT_EQ(res, FPGA_NOT_FOUND);

  res = make_sysfs_group(tok->sysfspath, "errors", &obj,
                         FPGA_OBJECT_RECURSE_ONE, handle_);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(xfpga_fpgaDestroyObject(&obj), FPGA_OK);
}

/**
 * @test   make_object_glob
 * @details
 */
TEST_P(sysfs_c_hw_p, make_object_glob) {
  _fpga_token *tok = static_cast<_fpga_token *>(tokens_[0]);
  fpga_object object;
  // errors is a sysfs directory - this should call make_sysfs_group()
  ASSERT_EQ(make_sysfs_object(tok->sysfspath, "errors", &object,
                              FPGA_OBJECT_GLOB, 0),
            FPGA_OK);
  EXPECT_EQ(xfpga_fpgaDestroyObject(&object), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(sysfs_c, sysfs_c_hw_p,
                        ::testing::ValuesIn(test_platform::hw_platforms()));

class sysfs_c_mock_p : public sysfs_c_p {
 protected:
  sysfs_c_mock_p() {}
};

/**
 * @test    make_sysfs_group
 * @details
 */
TEST_P(sysfs_c_mock_p, make_sysfs) {
  const std::string invalid_path =
      "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme";
  _fpga_token *tok = static_cast<_fpga_token *>(tokens_[0]);
  fpga_object obj;

  auto res = make_sysfs_group(tok->sysfspath, "errors", &obj, 0, handle_);
  EXPECT_EQ(res, FPGA_OK);
  EXPECT_EQ(xfpga_fpgaDestroyObject(&obj), FPGA_OK);

  res = make_sysfs_group(tok->sysfspath, "errors", &obj, FPGA_OBJECT_GLOB,
                         handle_);
  EXPECT_EQ(res, FPGA_OK);

  res = make_sysfs_group(const_cast<char *>(invalid_path.c_str()), "errors",
                         &obj, 0, handle_);
  EXPECT_EQ(res, FPGA_NOT_FOUND);

  res = make_sysfs_group(tok->sysfspath, "errors", &obj,
                         FPGA_OBJECT_RECURSE_ONE, handle_);
  EXPECT_EQ(res, FPGA_OK);

  EXPECT_EQ(xfpga_fpgaDestroyObject(&obj), FPGA_OK);
}

/**
 * @test   make_object_glob
 * @details
 */
TEST_P(sysfs_c_mock_p, make_object_glob) {
  _fpga_token *tok = static_cast<_fpga_token *>(tokens_[0]);
  fpga_object object;
  // errors is a sysfs directory - this should call make_sysfs_group()
  ASSERT_EQ(make_sysfs_object(tok->sysfspath, "errors", &object, 
                              FPGA_OBJECT_GLOB, 0),
            FPGA_OK);
}

/**
 * @test    fpga_sysfs_02
 *          sysfs_write_u64
 */
TEST_P(sysfs_c_mock_p, fpga_sysfs_02) {
  fpga_result result;
  // valid path
  result = sysfs_write_u64(
           "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/socket_id", 0);
  EXPECT_EQ(result, FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(sysfs_c, sysfs_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms()));

class sysfs_c_mock_no_drv_p : public ::testing::TestWithParam<std::string> {
 protected:
  sysfs_c_mock_no_drv_p() {}
};

/**
 * @test    sysfs_get_pr_id
 * @details sysfs_get_pr_id given a valid bitstream
 *          return FPGA_NOT_FOUND from sysfs_read_guid
 */
TEST_P(sysfs_c_mock_no_drv_p, sysfs_get_pr_id) {
  int dev = 0;
  int subdev = 0;
  fpga_guid guid;
  auto res = sysfs_get_pr_id(dev, subdev, guid);
  EXPECT_NE(res, FPGA_OK);
}

/**
 * @test    sysfs_get_slots
 * @details sysfs_get_slots given a valid parameters
 *          return FPGA_NOT_FOUND from sysfs_read_u32
 */
TEST_P(sysfs_c_mock_no_drv_p, sysfs_get_slots) {
  int dev = 0;
  int subdev = 0;
  uint32_t u32;
  auto res = sysfs_get_slots(dev, subdev, &u32);
  EXPECT_NE(res, FPGA_OK);
}

/**
 * @test    sysfs_get_bitstream_id
 * @details sysfs_get_bitstream_id given a valid parameters
 *          return FPGA_NOT_FOUND from sysfs_read_u64
 */
TEST_P(sysfs_c_mock_no_drv_p, sysfs_get_bitstream_id) {
  int dev = 0;
  int subdev = 0;
  uint64_t u64;
  auto res = sysfs_get_bitstream_id(dev, subdev, &u64);
  EXPECT_NE(res, FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(sysfs_c, sysfs_c_mock_no_drv_p,
                        ::testing::ValuesIn(test_platform::mock_platforms()));
