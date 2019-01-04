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
#include "fpga_hw.h"
#include <dirent.h>
#include <glob.h>
#include <string.h>
#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "test_utils.h"

namespace opae {
namespace testing {

test_device test_device::unknown() {
  return test_device{.fme_guid = "C544CE5C-F630-44E1-8551-59BD87AF432E",
                     .afu_guid = "C544CE5C-F630-44E1-8551-59BD87AF432E",
                     .segment = 0x1919,
                     .bus = 0x0A,
                     .device = 9,
                     .function = 5,
                     .socket_id = 9,
                     .num_slots = 9,
                     .bbs_id = 9,
                     .bbs_version = {0xFF, 0xFF, 0xFF},
                     .state = FPGA_ACCELERATOR_ASSIGNED,
                     .num_mmio = 0,
                     .num_interrupts = 0xf,
                     .fme_object_id = 9,
                     .port_object_id = 9,
                     .vendor_id = 0x1234,
                     .device_id = 0x1234,
                     .fme_num_errors = 0x1234,
                     .port_num_errors = 0x1234,
                     .gbs_guid = "C544CE5C-F630-44E1-8551-59BD87AF432E",
                     .mdata = ""};
}

const char *skx_mdata =
    R"mdata({"version": 640,
   "afu-image":
    {"clock-frequency-high": 312,
     "clock-frequency-low": 156,
     "power": 50,
     "interface-uuid": "1a422218-6dba-448e-b302-425cbcde1406",
     "magic-no": 488605312,
     "accelerator-clusters":
      [
        {
          "total-contexts": 1,
          "name": "nlb_400",
          "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
        }
      ]
     },
     "platform-name": "MCP"}";
)mdata";

const char *skx_mdata_ups =
    R"mdata({"version": 640,
   "afu-image":
    {"clock-frequency-high": 312,
     "clock-frequency-low": 156,
     "power": 50,
     "interface-uuid": "1a422218-6dba-448e-b302-425cbcde1406",
     "magic-no": 488605312,
     "accelerator-clusters":
      [
        {
          "total-contexts": 1,
          "name": "nlb_400",
          "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
        }
      ]
     },
     "platform-name": "MCP"}";
)mdata";

const char *rc_mdata =
    R"mdata({"version": 112,
   "afu-image":
    {"clock-frequency-high": 312,
     "clock-frequency-low": 156,
     "interface-uuid": "9926ab6d-6c92-5a68-aabc-a7d84c545738",
     "magic-no": 488605312,
     "accelerator-clusters":
      [
        {
          "total-contexts": 1,
          "name": "nlb3",
          "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"
        }
      ]
     },
     "platform-name": "PAC"}";
)mdata";

static platform_db MOCK_PLATFORMS = {
    {"skx-p",
     test_platform{.mock_sysfs = "mock_sys_tmp-1socket-nlb0.tar.gz",
                   .devices = {test_device{
                       .fme_guid = "1A422218-6DBA-448E-B302-425CBCDE1406",
                       .afu_guid = "D8424DC4-A4A3-C413-F89E-433683F9040B",
                       .segment = 0x0,
                       .bus = 0x5e,
                       .device = 0,
                       .function = 0,
                       .socket_id = 0,
                       .num_slots = 1,
                       .bbs_id = 0x6400002fc614bb9,
                       .bbs_version = {6, 4, 0},
                       .state = FPGA_ACCELERATOR_UNASSIGNED,
                       .num_mmio = 0x2,
                       .num_interrupts = 0,
                       .fme_object_id = 0xf500000,
                       .port_object_id = 0xf400000,
                       .vendor_id = 0x8086,
                       .device_id = 0xbcc0,
                       .fme_num_errors = 8,
                       .port_num_errors = 3,
                       .gbs_guid = "58656f6e-4650-4741-b747-425376303031",
                       .mdata = skx_mdata}}}},
    {"dcp-rc",
     test_platform{.mock_sysfs = "mock_sys_tmp-dcp-rc-nlb3.tar.gz",
                   .devices = {test_device{
                       .fme_guid = "9926AB6D-6C92-5A68-AABC-A7D84C545738",
                       .afu_guid = "D8424DC4-A4A3-C413-F89E-433683F9040B",
                       .segment = 0x0,
                       .bus = 0x05,
                       .device = 0,
                       .function = 0,
                       .socket_id = 0,
                       .num_slots = 1,
                       .bbs_id = 0x113000200000177,
                       .bbs_version = {1, 1, 3},
                       .state = FPGA_ACCELERATOR_UNASSIGNED,
                       .num_mmio = 0x2,
                       .num_interrupts = 0,
                       .fme_object_id = 0xf500000,
                       .port_object_id = 0xf400000,
                       .vendor_id = 0x8086,
                       .device_id = 0x09c4,
                       .fme_num_errors = 8,
                       .port_num_errors = 3,
                       .gbs_guid = "58656f6e-4650-4741-b747-425376303031",
                       .mdata = rc_mdata}}}},

    {"skx-p-dfl-v1",
     test_platform{.mock_sysfs = "mock_sys_tmp-dfl-v1-nlb0.tar.gz",
                   .devices = {test_device{
                       .fme_guid = "1A422218-6DBA-448E-B302-425CBCDE1406",
                       .afu_guid = "D8424DC4-A4A3-C413-F89E-433683F9040B",
                       .segment = 0x0,
                       .bus = 0x5e,
                       .device = 0,
                       .function = 0,
                       .socket_id = 0,
                       .num_slots = 1,
                       .bbs_id = 0x6400002fc614bb9,
                       .bbs_version = {6, 4, 0},
                       .state = FPGA_ACCELERATOR_UNASSIGNED,
                       .num_mmio = 0x2,
                       .num_interrupts = 0,
                       .fme_object_id = 0xf500000,
                       .port_object_id = 0xf400000,
                       .vendor_id = 0x8086,
                       .device_id = 0xbcc0,
                       .fme_num_errors = 8,
                       .port_num_errors = 3,
                       .gbs_guid = "58656f6e-4650-4741-b747-425376303031",
                       .mdata = skx_mdata_ups}}}},
};

test_platform test_platform::get(const std::string &key) {
  return fpga_db::instance()->get(key);
}

bool test_platform::exists(const std::string &key) {
  return fpga_db::instance()->exists(key);
}

std::vector<std::string> test_platform::keys(bool sorted) {
  return fpga_db::instance()->keys(sorted);
}

std::vector<std::string> test_platform::platforms(
    std::initializer_list<std::string> names) {
  std::vector<std::string> keys(names);
  if (keys.empty()) {
    keys = fpga_db::instance()->keys();
  }
  // from the list of platform names requested, remove the ones not found in
  // the platform db
  std::remove_if(keys.begin(), keys.end(), [](const std::string &n) {
    auto db = fpga_db::instance();
    return !db->exists(n);
  });
  return keys;
}

std::vector<std::string> test_platform::mock_platforms(
    std::initializer_list<std::string> names) {
  std::vector<std::string> keys(names);
  if (keys.empty()) {
    keys = fpga_db::instance()->keys();
  }
  std::vector<std::string> want;
  std::copy_if(keys.begin(), keys.end(), std::back_inserter(want),
               [](const std::string &k) {
                 auto db = fpga_db::instance();
                 return db->exists(k) && db->get(k).mock_sysfs != nullptr;
               });
  return want;
}

std::vector<std::string> test_platform::hw_platforms(
    std::initializer_list<std::string> names) {
  std::vector<std::string> keys(names);
  if (keys.empty()) {
    keys = fpga_db::instance()->keys();
  }
  std::vector<std::string> want;
  std::copy_if(keys.begin(), keys.end(), std::back_inserter(want),
               [](const std::string &k) {
                 auto db = fpga_db::instance();
                 return db->exists(k) && db->get(k).mock_sysfs == nullptr;
               });
  return want;
}

const std::string PCI_DEVICES = "/sys/bus/pci/devices";

typedef std::pair<uint16_t, uint64_t> ven_dev_id;
std::map<ven_dev_id, std::vector<std::string>> known_devices = {
    {{0x8086, 0xbcc0}, std::vector<std::string>()},
    {{0x8086, 0xbcc1}, std::vector<std::string>()},
    {{0x8086, 0x09c4}, std::vector<std::string>()},
    {{0x8086, 0x09c5}, std::vector<std::string>()}};

static std::vector<ven_dev_id> supported_devices() {
  std::vector<ven_dev_id> devs;
  for (auto kv : known_devices) {
    devs.push_back(kv.first);
  }
  return devs;
}

template <typename T>
static T parse_file(const std::string &path) {
  std::ifstream df;
  df.open(path);
  if (!df.is_open()) {
    std::cerr << std::string("WARNING: could not open file ") + path << "\n";
    return 0;
  }
  std::string value_string;
  T value;
  df >> value_string;
  value = std::stoi(value_string, nullptr, 0);
  return value;
}

static std::string make_path(int seg, int bus, int dev, int func) {
  std::stringstream num;
  num << std::setw(2) << std::hex << bus;
  std::string b(num.str());
  num.clear();
  num.str(std::string());

  num << std::setw(4) << std::setfill('0') << seg;
  std::string s(num.str());

  num.clear();
  num.str(std::string());
  num << std::setw(2) << std::setfill('0') << dev;
  std::string d(num.str());

  std::string device_string =
      s + ":" + b + ":" + d + "." + std::to_string(func);
  return device_string;
}

static uint16_t read_socket_id(const std::string devices) {
  std::string glob_path =
      PCI_DEVICES + "/" + devices + "/fpga*/*/*-fme.*/socket_id";
  std::string socket_path;

  glob_t glob_buf;
  glob_buf.gl_pathc = 0;
  glob_buf.gl_pathv = NULL;
  int globres = glob(glob_path.c_str(), 0, NULL, &glob_buf);

  if (!globres) {
    if (glob_buf.gl_pathc > 1) {
      std::cerr << std::string("Ambiguous object key - using first one")
                << "\n";
    }
    socket_path = std::string(glob_buf.gl_pathv[0]);
    globfree(&glob_buf);
  } else {
    switch (globres) {
      case GLOB_NOSPACE:
        std::cerr << std::string("FPGA No Memory found.") << "\n";
        break;
      case GLOB_NOMATCH:
        std::cerr << std::string("FPGA Not found.") << "\n";
        break;
    }
    goto err;
  }

  struct stat st;
  if (stat(socket_path.c_str(), &st)) {
    std::cerr << "Failed to get file stat."
              << "\n";
    goto err;
  }
  return parse_file<uint16_t>(socket_path);

err:
  if (glob_buf.gl_pathc && glob_buf.gl_pathv) {
    globfree(&glob_buf);
  }
  return -1;
}

static uint16_t read_device_id(const std::string &pci_dir) {
  std::string device_path = pci_dir + "/device";
  struct stat st;

  if (stat(device_path.c_str(), &st)) {
    std::cerr << std::string("WARNING: stat:") + device_path << ":"
              << strerror(errno) << "\n";
    return 0;
  }
  return parse_file<uint16_t>(device_path);
}

static uint16_t read_vendor_id(const std::string &pci_dir) {
  std::string device_path = pci_dir + "/vendor";
  struct stat st;

  if (stat(device_path.c_str(), &st)) {
    std::cerr << std::string("WARNING: stat:") + device_path << ":"
              << strerror(errno) << "\n";
    return 0;
  }
  return parse_file<uint16_t>(device_path);
}

int filter_fpga(const struct dirent *ent) {
  std::string ename(ent->d_name);
  if (ename[0] == '.') {
    return 0;
  }
  std::string pci_path = PCI_DEVICES + "/" + ename;
  auto did = read_device_id(pci_path);
  auto vid = read_vendor_id(pci_path);

  auto devices = supported_devices();
  std::vector<ven_dev_id>::const_iterator it =
      std::find(devices.begin(), devices.end(), ven_dev_id(vid, did));
  if (it == devices.end()) {
    return 0;
  }
  known_devices[ven_dev_id(vid, did)].push_back(pci_path);
  return 1;
}

std::vector<std::string> find_supported_devices() {
  struct dirent **dirs;
  int n = scandir(PCI_DEVICES.c_str(), &dirs, filter_fpga, alphasort);
  if (n == -1) {
    std::string msg =
        "error scanning pci devices: " + std::string(strerror(errno));
    throw std::runtime_error(msg);
  }
  std::vector<std::string> entries;
  while (n--) {
    entries.push_back(std::string(dirs[n]->d_name));
    free(dirs[n]);
  }
  free(dirs);
  return entries;
}

fpga_db *fpga_db::instance_ = nullptr;

fpga_db::fpga_db() {}

fpga_db *fpga_db::instance() {
  if (fpga_db::instance_ == nullptr) {
    fpga_db::instance_ = new fpga_db();
    fpga_db::instance_->discover_hw();
  }
  return fpga_db::instance_;
}

static std::map<ven_dev_id, std::string> devid_name = {
    {{0x8086, 0xbcc0}, "skx-p"},
    {{0x8086, 0xbcc1}, "skx-p-v"},
    {{0x8086, 0x09c4}, "dcp-rc"},
    {{0x8086, 0x09c5}, "dcp-rc-v"},
    {{0x8086, 0xbcc0}, "skx-p-dfl-v1"}};

const char *PCI_DEV_PATTERN =
    "([0-9a-fA-F]{4}):([0-9a-fA-F]{2}):([0-9]{2})\\.([0-9])";

test_device make_device(uint16_t ven_id, uint16_t dev_id,
                        const std::string &platform,
                        const std::string &pci_path) {
  test_device dev = MOCK_PLATFORMS[platform].devices[0];
  auto r = regex<>::create(PCI_DEV_PATTERN);
  auto m = r->match(pci_path);
  if (m) {
    dev.segment = std::stoi(m->group(1), nullptr, 16);
    dev.bus = std::stoi(m->group(2), nullptr, 16);
    dev.device = std::stoi(m->group(3), nullptr, 10);
    dev.function = std::stoi(m->group(4), nullptr, 10);
    dev.vendor_id = ven_id;
    dev.device_id = dev_id;

    std::string device_string =
        make_path(dev.segment, dev.bus, dev.device, dev.function);
    dev.socket_id = read_socket_id(device_string);
  } else {
    std::cerr << "error matching pci dev pattern (" << pci_path << ")\n";
  }
  return dev;
}

std::pair<std::string, test_platform> make_platform(
    uint16_t ven_id, uint16_t dev_id,
    const std::vector<std::string> &pci_paths) {
  std::string name = devid_name[{ven_id, dev_id}];
  test_platform platform;
  platform.mock_sysfs = nullptr;
  for (auto p : pci_paths) {
    platform.devices.push_back(make_device(ven_id, dev_id, name, p));
  }
  return std::make_pair(name, platform);
}

void fpga_db::discover_hw() {
  platform_db db;
#ifdef ENABLE_MOCK
  std::cout << "Mock is enabled." << std::endl;
  platforms_ = MOCK_PLATFORMS;
#else
  auto sys_pci_devs = find_supported_devices();

  for (auto kv : known_devices) {
    if (!kv.second.empty()) {
      ven_dev_id id = kv.first;
      platforms_.insert(make_platform(id.first, id.second, kv.second));
    }
  }
#endif
}

std::vector<std::string> fpga_db::keys(bool sorted) {
  std::vector<std::string> keys(platforms_.size());
  std::transform(
      platforms_.begin(), platforms_.end(), keys.begin(),
      [](const std::pair<std::string, test_platform> &it) { return it.first; });
  if (sorted) {
    std::sort(keys.begin(), keys.end());
  }

  return keys;
}

test_platform fpga_db::get(const std::string &key) { return platforms_[key]; }

bool fpga_db::exists(const std::string &key) {
  return platforms_.find(key) != platforms_.end();
}

}  // end of namespace testing
}  // end of namespace opae
