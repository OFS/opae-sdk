// Copyright(c) 2017-2019, Intel Corporation
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
#include "sysfs_int.h"
#include "types_int.h"
fpga_result cat_token_sysfs_path(char *, fpga_token, const char *);
fpga_result get_port_sysfs(fpga_handle, char *);
fpga_result sysfs_get_socket_id(int, int, fpga_guid);
fpga_result sysfs_get_afu_id(int, int, fpga_guid);
fpga_result sysfs_get_pr_id(int, int, fpga_guid);
fpga_result sysfs_get_slots(int, int, uint32_t *);
fpga_result sysfs_get_bitstream_id(int, int, uint64_t *);
fpga_result sysfs_sbdf_from_path(const char *, int *, int *, int *, int *);
fpga_result opae_glob_path(char *);
fpga_result make_sysfs_group(char *, const char *, fpga_object *, int,
                             fpga_handle);
ssize_t eintr_write(int, void *, size_t);
char* cstr_dup(const char *str);
int parse_pcie_info(sysfs_fpga_device *device, char *buffer);
fpga_result sysfs_get_interface_id(fpga_token token, fpga_guid guid);
sysfs_fpga_region* make_region(sysfs_fpga_device*, char*, int, fpga_objtype);
int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
fpga_result re_match_region(const char *fmt, char *inpstr, char type[], size_t,
                            int *num);
}

#include <fstream>
#include <opae/enum.h>
#include <opae/fpga.h>
#include <opae/properties.h>
#include <sys/types.h>
#include <uuid/uuid.h>
#include <string>
#include <vector>
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


class sysfsinit_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  sysfsinit_c_p(){}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);
    ASSERT_EQ(xfpga_plugin_initialize(), FPGA_OK);
    ASSERT_GT(sysfs_device_count(), 0);
    sysfs_fpga_region *fme = nullptr;
    sysfs_fpga_region *port = nullptr;
    for (int i = 0; i < sysfs_device_count(); ++i) {
      fme = (fme == nullptr) ? sysfs_get_device(i)->fme : fme;
      port = (port == nullptr) ? sysfs_get_device(i)->port : port;
      if (fme && port) break;
    }
    ASSERT_NE(fme, nullptr);
    ASSERT_NE(port, nullptr);

    sysfs_fme = std::string(fme->sysfs_path);
    dev_fme = std::string("/dev/") + std::string(fme->sysfs_name);
    sysfs_port = std::string(port->sysfs_path);
    dev_port = std::string("/dev/") + std::string(port->sysfs_name);
  }
  virtual void TearDown() override {
    xfpga_plugin_finalize();
    system_->finalize();
  }

  int GetNumFpgas() {
    if (platform_.mock_sysfs != nullptr) {
      return platform_.devices.size();
    }

    int value;
    std::string cmd =
        "(ls -l /sys/class/fpga*/device*/*fme*/dev || "
        "ls -l /sys/class/fpga*/*intel*) |  (wc -l)";

    ExecuteCmd(cmd, value);
    return value;
  }

  int GetNumMatchedFpga() {
    if (platform_.mock_sysfs != nullptr) {
      return platform_.devices.size();
    }

    std::stringstream ss;
    ss << std::setw(4) << std::hex << platform_.devices[0].device_id;
    std::string deviceid (ss.str());

    std::string cmd =  "lspci | grep " + deviceid + " | wc -l";

    int value;
    ExecuteCmd(cmd, value);
    return value;
  }

  void ExecuteCmd(std::string cmd, int &value) {
    std::string line;
    std::string command = cmd + " > output.txt";

    EXPECT_EQ(std::system(command.c_str()), 0);

    std::ifstream file("output.txt");

    ASSERT_TRUE(file.is_open());
    EXPECT_TRUE(std::getline(file, line));
    file.close();

    EXPECT_EQ(std::system("rm output.txt"), 0);

    value = std::stoi(line);
  }

  test_platform platform_;
  test_system *system_;
  std::string sysfs_fme;
  std::string dev_fme;
  std::string sysfs_port;
  std::string dev_port;
};

// convert segment, bus, device, function to a 32 bit number
uint32_t to_uint32(uint16_t segment, uint8_t bus, uint8_t device,
                   uint8_t function) {
  return (segment << 16) | (bus << 8) | (device << 5) | (function & 7);
}

TEST_P(sysfsinit_c_p, sysfs_initialize) {
  std::map<uint64_t, test_device> devices;

  // define a callback to be used with sysfs_foreach_device
  // this callback is given a map of devices using the sbdf as the key
  // (as a 32-bit number);
  auto cb = [](const sysfs_fpga_device *r, void* data) -> fpga_result {
    auto& devs = *reinterpret_cast<std::map<uint64_t, test_device>*>(data);
    auto id = to_uint32(r->segment, r->bus, r->device, r->function);
    auto it = devs.find(id);
    if (it != devs.end()) {
      if (it->second.device_id == r->device_id &&
          it->second.vendor_id == r->vendor_id) {
        devs.erase(id);
      }
    }
    return FPGA_OK;
  };

  // build a map of tests devices where the key is the sbdf as a 32-bit number
  for (const auto &d : platform_.devices) {
    devices[to_uint32(d.segment, d.bus, d.device, d.function)] = d;
  }
  if (platform_.devices[0].num_vfs) {
    auto d = platform_.devices[0];
    d.function = 1;
    d.device_id++;
    devices[to_uint32(d.segment, d.bus, d.device, 1)] = d;
  }
  auto num_vfs = platform_.devices[0].num_vfs;
  // the size of this map should be equal to the number of devices in our
  // platform
  ASSERT_EQ(devices.size(), platform_.devices.size() + num_vfs);
  EXPECT_EQ(GetNumFpgas(), sysfs_device_count() - num_vfs);
  // call sysfs_foreach_device with our callback, cb
  sysfs_foreach_device(cb, &devices);
  // our devices map should be empty after this call as this callback removes
  // entries if the device structure matches a device object in the map
  EXPECT_EQ(devices.size(), 0);
}

TEST_P(sysfsinit_c_p, sysfs_get_device) {
  std::map<uint64_t, test_device> devices;

  // build a map of tests devices where the key is the sbdf as a 32-bit number
  for (const auto &d : platform_.devices) {
    devices[to_uint32(d.segment, d.bus, d.device, d.function)] = d;
  }

  // the size of this map should be equal to the number of devices in our
  // platform
  ASSERT_EQ(devices.size(), platform_.devices.size());
  auto num_vfs = platform_.devices[0].num_vfs;
  EXPECT_EQ(GetNumFpgas(), sysfs_device_count() - num_vfs);

  // use sysfs_get_device API to count how many devices match our devices map
  for (int i = 0; i < sysfs_device_count(); ++i) {
    auto device = sysfs_get_device(i);
    ASSERT_NE(device, nullptr);
    auto id = to_uint32(device->segment, device->bus, device->device, device->function);
    auto it = devices.find(id);
    if (it != devices.end() && it->second.device_id == device->device_id &&
        it->second.vendor_id == device->vendor_id) {
      devices.erase(id);
    }
  }
  // our devices map should be empty after the loop above
  EXPECT_EQ(devices.size(), 0);
}

/**
* @test   get_interface_id
* @details Given a valid token
           When I call sysfs_get_interface_id with that token
*          I get the expected interface_id
*/
TEST_P(sysfsinit_c_p, get_interface_id) {
  fpga_guid guid;
  fpga_properties props;
  fpga_token fme;
  uint32_t matches = 0;
  fpga_guid parsed_guid;
  ASSERT_EQ(fpgaGetProperties(nullptr, &props), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetDeviceID(props,platform_.devices[0].device_id), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetVendorID(props,platform_.devices[0].vendor_id), FPGA_OK);
  ASSERT_EQ(fpgaPropertiesSetObjectType(props, FPGA_DEVICE), FPGA_OK);
  ASSERT_EQ(xfpga_fpgaEnumerate(&props, 1, &fme, 1, &matches), FPGA_OK);
  EXPECT_EQ(matches, GetNumMatchedFpga());
  ASSERT_EQ(sysfs_get_interface_id(fme, guid), 0);
  EXPECT_EQ(uuid_parse(platform_.devices[0].fme_guid, parsed_guid), 0);
  EXPECT_EQ(uuid_compare(parsed_guid, guid), 0);
  EXPECT_EQ(xfpga_fpgaDestroyToken(&fme), FPGA_OK);
  EXPECT_EQ(fpgaDestroyProperties(&props), FPGA_OK);
}

TEST(sysfsinit_c_p, sysfs_parse_pcie) {
  sysfs_fpga_device device;
  char buffer1[] = "../../devices/pci0000:00/0000:00:02.0/0f0f:05:04.3/fpga/intel-fpga-dev.0";
  char buffer2[] = "../../devices/pci0000:5e/a0a0:5e:02.1/fpga_device/device0";
  auto res = parse_pcie_info(&device, buffer1);
  EXPECT_EQ(res, 0);
  EXPECT_EQ(device.segment, 0x0f0f);
  EXPECT_EQ(device.bus, 0x05);
  EXPECT_EQ(device.device, 0x04);
  EXPECT_EQ(device.function, 0x03);
  res = parse_pcie_info(&device, buffer2);
  EXPECT_EQ(res, 0);
  EXPECT_EQ(device.segment, 0xa0a0);
  EXPECT_EQ(device.bus, 0x5e);
  EXPECT_EQ(device.device, 0x02);
  EXPECT_EQ(device.function, 0x01);
}

INSTANTIATE_TEST_CASE_P(sysfsinit_c, sysfsinit_c_p,
                        ::testing::ValuesIn(test_platform::platforms()));

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
    ASSERT_EQ(xfpga_plugin_initialize(), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, 
                                        platform_.devices[0].device_id), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                                  &num_matches_),
              FPGA_OK);
    ASSERT_EQ(xfpga_fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);
    sysfs_fpga_region *fme = nullptr;
    sysfs_fpga_region *port = nullptr;
    if (sysfs_device_count() > 0) {
      for (int i = 0; i < sysfs_device_count(); ++i) {
        fme = fme == nullptr ? sysfs_get_device(i)->fme : fme;
        port = port == nullptr ? sysfs_get_device(i)->port : port;
      }
    }
    ASSERT_NE(fme, nullptr);
    ASSERT_NE(port, nullptr);

    sysfs_fme = std::string(fme->sysfs_path);
    dev_fme = std::string("/dev/") + std::string(fme->sysfs_name);
    sysfs_port = std::string(port->sysfs_path);
    dev_port = std::string("/dev/") + std::string(port->sysfs_name);
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
    xfpga_plugin_finalize();
    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
  std::string sysfs_fme;
  std::string dev_fme;
  std::string sysfs_port;
  std::string dev_port;
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
* @test    hw_type_invalid_test
* @details
*/
TEST_P(sysfs_c_p, hw_type_invalid_tests) {
  auto h = (struct _fpga_handle *)handle_;
  enum fpga_hw_type hw_type = FPGA_HW_UNKNOWN;
  fpga_token tok;

  auto res = get_fpga_hw_type(handle_, NULL);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);

  tok = h->token;
  h->token = NULL;
  res = get_fpga_hw_type(handle_, &hw_type);
  EXPECT_EQ(FPGA_INVALID_PARAM, res);

  h->token = tok;
  res = get_fpga_hw_type(handle_, &hw_type);
  EXPECT_EQ(FPGA_OK, res);
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
* @test    hw_type
* @details get_fpga_hw_type given valid parameters
*          returns FPGA_OK
*/
TEST_P(sysfs_c_p, hw_type) {
  enum fpga_hw_type hw_type = FPGA_HW_UNKNOWN;
  uint64_t real_vendorid = platform_.devices[0].vendor_id;
  uint64_t real_deviceid = platform_.devices[0].device_id;

  auto res = get_fpga_hw_type(handle_, &hw_type);
  EXPECT_EQ(res, FPGA_OK);

  EXPECT_EQ(hw_type, opae_id_to_hw_type(real_vendorid, real_deviceid));
}

/**
* @test    cstr_dup
* @details Duplicate an input string
*/
TEST_P(sysfs_c_p, cstr_dup) {
  std::string inp("this is an input string");
  char *dup = cstr_dup(inp.c_str());
  EXPECT_STREQ(dup, inp.c_str());
  free(dup);
}

/**
* @test    cstr_dup
* @details Invalidate malloc call
*/
TEST_P(sysfs_c_p, cstr_dup_1) {
  std::string inp("this is an input string");
  test_system::instance()->invalidate_malloc();
  char *dup = cstr_dup(inp.c_str());
  EXPECT_EQ(dup, nullptr);
}

/**
* @test    get_fme_path
* @details Given a valid sysfs path to a port node
*          When I call sysfs_get_fme_path with the path
*          Then the return value is FPGA_OK
*          And I get a sysfs path to the fme node
*          And its realpath is equal to the realpath of the SUT
*/
TEST_P(sysfs_c_p, get_fme_path) {
  char found_fme[PATH_MAX];
  char rpath1[PATH_MAX];
  char rpath2[PATH_MAX];
  ASSERT_EQ(sysfs_get_fme_path(sysfs_port.c_str(), found_fme), FPGA_OK);
  ASSERT_NE(realpath(sysfs_fme.c_str(), rpath1), nullptr);
  ASSERT_NE(realpath(found_fme, rpath2), nullptr);
  ASSERT_STREQ(rpath1, rpath2);
}

/**
* @test    get_fme_path_neg
* @details Given an invalid sysfs path to a port node
*          When I call sysfs_get_fme_path with the path
*          Then the return value is not FPGA_OK
*/
TEST_P(sysfs_c_p, get_fme_path_neg) {
  char found_fme[PATH_MAX];
  ASSERT_NE(sysfs_get_fme_path("/a/b/c", found_fme), FPGA_OK);
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
                        ::testing::ValuesIn(test_platform::hw_platforms({ "skx-p","dcp-rc" })));

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
TEST_P(sysfs_c_mock_p, make_object_glob) {
  _fpga_token *tok = static_cast<_fpga_token *>(tokens_[0]);
  fpga_object object;
  // errors is a sysfs directory - this should call make_sysfs_group()
  ASSERT_EQ(make_sysfs_object(tok->sysfspath, "errors", &object, 
                              FPGA_OBJECT_GLOB, 0),
            FPGA_OK);
  EXPECT_EQ(xfpga_fpgaDestroyObject(&object), FPGA_OK);
}

/**
 * @test    fpga_sysfs_02
 *          sysfs_write_u64
 */
TEST_P(sysfs_c_mock_p, fpga_sysfs_02) {
  fpga_result result;
  std::string str = sysfs_fme.c_str() + std::string("/socket_id");
  // valid path
  result = sysfs_write_u64(str.c_str(), 0);
  EXPECT_EQ(result, FPGA_OK);
}

/**
 * @test    fpga_sysfs_02
 *          sysfs_write_u64_decimal
 */

TEST_P(sysfs_c_mock_p, fpga_sysfs_03) {
  fpga_result result;
  std::string str = sysfs_fme.c_str() + std::string("/socket_id");
  // valid path
  result = sysfs_write_u64_decimal(str.c_str(), 0x100);
  EXPECT_EQ(result, FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(sysfs_c, sysfs_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p","dcp-rc" })));

class sysfs_c_mock_no_drv_p : public ::testing::TestWithParam<std::string> {
 protected:
  sysfs_c_mock_no_drv_p() {}
};

/**
 * @test    sysfs_get_pr_id
 * @details sysfs_get_pr_id given invalid path parameters. 
 *          It returns FPGA_NOT_FOUND.
 */
TEST_P(sysfs_c_mock_no_drv_p, sysfs_get_pr_id) {
  int dev = 0;
  int subdev = 0;
  fpga_guid guid;
  auto res = sysfs_get_pr_id(dev, subdev, guid);
  EXPECT_EQ(res, FPGA_NOT_FOUND);
}

/**
 * @test    sysfs_get_afu_id
 * @details sysfs_get_afu_id given invalid path parameters. 
 *          It returns FPGA_NOT_FOUND.
 */

TEST_P(sysfs_c_mock_no_drv_p, sysfs_get_afu_id) {
  int dev = 0;
  int subdev = 0;
  fpga_guid guid;
  auto res = sysfs_get_afu_id(dev, subdev, guid);
  EXPECT_EQ(res, FPGA_NOT_FOUND);
}

/**
 * @test    sysfs_get_socket_id
 * @details sysfs_get_socket_id given invalid parameters. 
 *          It returns FPGA_NOT_FOUND.
 */
TEST_P(sysfs_c_mock_no_drv_p, sysfs_get_socket_id) {
  int dev = 0;
  int subdev = 0;
  uint8_t socket_id;
  auto res = sysfs_get_socket_id(dev, subdev, &socket_id);
  EXPECT_EQ(res, FPGA_NOT_FOUND);
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

class sysfs_sockid_c_mock_p : public sysfs_c_mock_p { };
/**
 * @test    fpga_sysfs_02
 *          sysfs_write_u64
 */
TEST_P(sysfs_sockid_c_mock_p, fpga_sysfs_02) {
  fpga_result result;
  std::string str = sysfs_fme.c_str() + std::string("/socket_id");
  // valid path
  result = sysfs_write_u64(str.c_str(), 0);
  EXPECT_EQ(result, FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(sysfs_c, sysfs_sockid_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({ "skx-p","dcp-rc" })));


class sysfs_sockid_c_p : public sysfs_c_p { };

/**
* @test    fpga_sysfs_02
* @brief   Tests: sysfs_read_int,sysfs_read_u32
*          sysfs_read_u32_pair,sysfs_read_u64
*          sysfs_read_u64,sysfs_write_u64
*..........get_port_sysfs,sysfs_read_guid
*/
TEST_P(sysfs_sockid_c_p, fpga_sysfs_02) {
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

  result = sysfs_read_int(sysfs_fme.c_str(), NULL);
  EXPECT_NE(result, FPGA_OK);

  // Valid input path
  std::string str = sysfs_fme.c_str() + std::string("/socket_id");
  result = sysfs_read_int(str.c_str(), &i);
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

  result = sysfs_read_u32(sysfs_fme.c_str(), NULL);
  EXPECT_NE(result, FPGA_OK);

  // Valid input path
  result = sysfs_read_u32(str.c_str(), &u32);
  EXPECT_EQ(result, FPGA_OK);

  // Invalid input parameters
  result = sysfs_read_u32_pair(NULL, NULL, NULL, '\0');
  EXPECT_NE(result, FPGA_OK);

  // Invalid input parameters
  result = sysfs_read_u32_pair(NULL, NULL, NULL, 'a');
  EXPECT_NE(result, FPGA_OK);

  // Invalid input 'sep' character
  result = sysfs_read_u32_pair(str.c_str(), &u1, &u2, '\0');
  EXPECT_NE(result, FPGA_OK);

  // Invalid input path value
  result = sysfs_read_u32_pair(str.c_str(), &u1, &u2, 'a');
  EXPECT_NE(result, FPGA_OK);

  // Invalid input path type
  result = sysfs_read_u32_pair(sysfs_fme.c_str(), &u1, &u2, 'a');
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
  result = sysfs_read_u64(str.c_str(), &u64);
  EXPECT_EQ(result, FPGA_OK);

  // Invalid input parameters
  result = sysfs_write_u64(NULL, 0);
  EXPECT_NE(result, FPGA_OK);

  result = sysfs_write_u64(sysfs_fme.c_str(), 0x100);
  EXPECT_NE(result, FPGA_OK);

  result = sysfs_write_u64_decimal(NULL, 0);
  EXPECT_NE(result, FPGA_OK);

  result = sysfs_write_u64_decimal(sysfs_fme.c_str(), 0x100);
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
  result = get_port_sysfs(NULL, (char *)str.c_str());
  EXPECT_NE(result, FPGA_OK);

  // NULL handle
  result = get_fpga_hw_type(NULL, NULL);
  EXPECT_NE(result, FPGA_OK);
}

/**
 * @test    make_region
 * @details Given valid parameters to make_regions but failed on malloc,
 *          it returns nullptr for sysfs_fpga_region. 
 */
TEST_P(sysfs_sockid_c_p, make_regions) {
  sysfs_fpga_region *fpga_region;
  sysfs_fpga_device device;
  std::string name = "fme";
  int num = 1;
  fpga_objtype type = FPGA_DEVICE;
  test_system::instance()->invalidate_malloc();
  fpga_region = make_region(&device, const_cast<char*>(name.c_str()), num, type);
  EXPECT_EQ(fpga_region, nullptr);
}

/**
 * @test    sysfs_get_guid
 * @details Given invalid parameters to sysfs_get_guid. 
 *          it returns FPGA_EXCEPTION. When an invalid path is 
 *          passed in, it returns FPGA_NOT_FOUND.
 */
TEST_P(sysfs_sockid_c_p, sysfs_get_guid_neg) {
  fpga_guid guid;
  _fpga_token *tok = static_cast<_fpga_token *>(tokens_[0]);
  std::string sysfspath = tok->sysfspath;
 
  EXPECT_EQ(sysfs_get_guid(nullptr, nullptr, guid),FPGA_EXCEPTION); 

  EXPECT_EQ(sysfs_get_guid(tokens_[0], nullptr, guid),FPGA_EXCEPTION); 

  EXPECT_EQ(sysfs_get_guid(nullptr, const_cast<char*>(sysfspath.c_str()), guid),FPGA_EXCEPTION); 

  sysfspath = "";
  EXPECT_EQ(sysfs_get_guid(tokens_[0], const_cast<char*>(sysfspath.c_str()), guid),FPGA_NOT_FOUND); 
}

/**
 * @test    sysfs_path_is_valid
 * @details Given invalid parameters to sysfs_path_is_valid. 
 *          it returns FPGA_NOT_FOUND. 
 */
TEST_P(sysfs_sockid_c_p, sysfs_path_is_valid) {
  EXPECT_EQ(sysfs_path_is_valid(nullptr, nullptr), FPGA_NOT_FOUND);
}

/**
 * @test    get_port_sysfspath
 * @details When token's sysfs is invalid for get_port_sysfspath. 
 *          it returns FPGA_INVALID_PARAM. 
 */
TEST_P(sysfs_sockid_c_p, get_port_sysfs) {
  _fpga_handle *h = static_cast<_fpga_handle *>(handle_);
  _fpga_token *tok = static_cast<_fpga_token *>(h->token);

  EXPECT_EQ(get_port_sysfs(handle_, tok->sysfspath), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(sysfs_c, sysfs_sockid_c_p,
                       ::testing::ValuesIn(test_platform::platforms({"skx-p","dcp-rc"})));

/**
 * @test    match_region
 * @details Given an input string that matches the format used<br>
 *          by the kernel driver when making region (platform) devices.
 *          When I call re_match_region with the input string
 *          Then the return code is FPGA_OK
 *          And the output parameters match the "fme" string portion<br>
 *          and the number portion.
 */
TEST(sysfs_regex, match_region)
{
  const char *fmt = "intel-fpga-(fme|port)\\.([0-9]+)";
  char buffer[8];
  int num = -1;
  char inpstr[] = "intel-fpga-fme.9";
  EXPECT_EQ(re_match_region(fmt, inpstr, buffer, sizeof(buffer), &num),
            FPGA_OK);
  EXPECT_STREQ(buffer, "fme");
  EXPECT_EQ(num, 9);
}

/**
 * @test    match_region_neg
 * @details Given an input string that does not match the format used<br>
 *          by the kernel driver when making region (platform) devices.
 *          When I call re_match_region with the input string or an invalid<br>
 *          parameter
 *          Then the return code is either FPGA_NOT_FOUND or FPGA_INVALID_PARAM
 */
TEST(sysfs_regex, match_region_neg)
{
  const char *fmt = "intel-fpga-(fme|port)\\.([0-9]+)";
  char buffer[8];
  int num = -1;
  char badstr[] = "intel-fpga-abc.0";
  EXPECT_EQ(re_match_region(fmt, badstr, buffer, sizeof(buffer), &num),
            FPGA_NOT_FOUND);
  EXPECT_EQ(re_match_region(nullptr, badstr, buffer, sizeof(buffer), &num),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(re_match_region(fmt, nullptr, buffer, sizeof(buffer), &num),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(re_match_region(fmt, badstr, nullptr, 0, &num), FPGA_INVALID_PARAM);
  EXPECT_EQ(re_match_region(fmt, badstr, buffer, sizeof(buffer), nullptr),
            FPGA_INVALID_PARAM);
}
