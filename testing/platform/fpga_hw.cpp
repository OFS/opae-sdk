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
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "test_utils.h"
#include <glob.h>
#include <iomanip>
#include <sstream>

#define FPGA_BBS_VER_MAJOR(i) (((i) >> 56) & 0xf)
#define FPGA_BBS_VER_MINOR(i) (((i) >> 52) & 0xf)
#define FPGA_BBS_VER_PATCH(i) (((i) >> 48) & 0xf)

namespace opae {
namespace testing {

test_device test_device::unknown() {
  return test_device{.fme_guid = "C544CE5C-F630-44E1-8551-59BD87AF432E",
                     .afu_guid = "C544CE5C-F630-44E1-8551-59BD87AF432E",
                     .segment = 0x1919,
                     .bus = 0x0A,
                     .device = 9,
                     .function = 5,
                     .num_vfs = 0,
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
    R"mdata({"version": 1,
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
     "platform-name": "MCP"}"
)mdata";

const char *rc_mdata =
    R"mdata({"version": 1,
   "afu-image":
    {"clock-frequency-high": 312,
     "clock-frequency-low": 156,
     "interface-uuid": "f64e598b-ea11-517f-a28e-7bc65d885104",
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
     "platform-name": "PAC"}"
)mdata";

const char *vc_mdata =
    R"mdata({"version": 1,
   "afu-image":
    {"clock-frequency-high": 312,
     "clock-frequency-low": 156,
     "interface-uuid": "cf9b1c50-37c9-45e9-8030-f921b17d2b3a",
     "magic-no": 488605312,
     "accelerator-clusters":
      [
        {
          "total-contexts": 1,
          "name": "nlb3",
          "accelerator-type-uuid": "9aeffe5f-8457-0612-c000-c9660d824272"
        }
      ]
     },
     "platform-name": "PAC"}";
)mdata";
static platform_db MOCK_PLATFORMS = {
    {"skx-p",
     test_platform{.mock_sysfs = "mock_sys_tmp-1socket-nlb0.tar.gz",
                   .driver = fpga_driver::linux_intel,
                   .devices = {test_device{
                       .fme_guid = "1A422218-6DBA-448E-B302-425CBCDE1406",
                       .afu_guid = "D8424DC4-A4A3-C413-F89E-433683F9040B",
                       .segment = 0x0,
                       .bus = 0x5e,
                       .device = 0,
                       .function = 0,
                       .num_vfs = 0,
                       .socket_id = 0,
                       .num_slots = 1,
                       .bbs_id = 0x06400002fc614bb9,
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
    {"skx-p-1vf",
     test_platform{.mock_sysfs = "mock_sys_tmp-1socket-nlb0-vf.tar.gz",
                   .driver = fpga_driver::linux_intel,
                   .devices = {test_device{
                       .fme_guid = "1A422218-6DBA-448E-B302-425CBCDE1406",
                       .afu_guid = "D8424DC4-A4A3-C413-F89E-433683F9040B",
                       .segment = 0x0,
                       .bus = 0x5e,
                       .device = 0,
                       .function = 0,
                       .num_vfs = 1,
                       .socket_id = 0,
                       .num_slots = 1,
                       .bbs_id = 0x06400002fc614bb9,
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
                   .driver = fpga_driver::linux_intel,
                   .devices = {test_device{
                       .fme_guid = "F64E598B-EA11-517F-A28E-7BC65D885104",
                       .afu_guid = "D8424DC4-A4A3-C413-F89E-433683F9040B",
                       .segment = 0x0,
                       .bus = 0x05,
                       .device = 0,
                       .function = 0,
                       .num_vfs = 0,
                       .socket_id = 0,
                       .num_slots = 1,
                       .bbs_id = 0x0113000200000177,
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
   {"skx-p-dfl0",
     test_platform{.mock_sysfs = "mock_sys_tmp-dfl0-nlb0.tar.gz",
                   .driver = fpga_driver::linux_dfl0,
                   .devices = {test_device{
                       .fme_guid = "1A422218-6DBA-448E-B302-425CBCDE1406",
                       .afu_guid = "D8424DC4-A4A3-C413-F89E-433683F9040B",
                       .segment = 0x0,
                       .bus = 0x5e,
                       .device = 0,
                       .function = 0,
                       .num_vfs = 0,
                       .socket_id = 0,
                       .num_slots = 1,
                       .bbs_id = 0x06400002fc614bb9,
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
   { "skx-p-dfl0_patchset2",
     test_platform{.mock_sysfs = "mock_sys_tmp-dfl0_patchset2-nlb0.tar.gz",
                .driver = fpga_driver::linux_dfl0,
                .devices = {test_device{
                       .fme_guid = "1A422218-6DBA-448E-B302-425CBCDE1406",
                       .afu_guid = "D8424DC4-A4A3-C413-F89E-433683F9040B",
                       .segment = 0x0,
                       .bus = 0x5e,
                       .device = 0,
                       .function = 0,
                       .num_vfs = 0,
                       .socket_id = 0,
                       .num_slots = 1,
                       .bbs_id = 0x06400002fc614bb9,
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
                       .mdata = skx_mdata}}} },
	{ "dcp-rc-dfl0_patchset2",
		test_platform{.mock_sysfs = "mock_sys_tmp-dfl0_patchset2-nlb0.tar.gz",
				.driver = fpga_driver::linux_dfl0,
				.devices = {test_device{
					   .fme_guid = "F64E598B-EA11-517F-A28E-7BC65D885104",
					   .afu_guid = "D8424DC4-A4A3-C413-F89E-433683F9040B",
					   .segment = 0x0,
					   .bus = 0x05,
					   .device = 0,
					   .function = 0,
					   .num_vfs = 0,
					   .socket_id = 0,
					   .num_slots = 1,
					   .bbs_id = 0x0113000200000177,
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
					   .mdata = rc_mdata}}} },
   {"dcp-vc",
     test_platform{.mock_sysfs = "mock_sys_tmp-dcp-vc.tar.gz",
            	   .driver = fpga_driver::linux_intel,
                   .devices = {test_device{
                       .fme_guid = "CF9B1C50-37C9-45E9-8030-F921B17D2B3A",
                       .afu_guid = "9AEFFE5F-8457-0612-C000-C9660D824272",
                       .segment = 0x0,
                       .bus = 0x05,
                       .device = 0,
                       .function = 0,
                       .num_vfs = 0,
                       .socket_id = 0,
                       .num_slots = 1,
                       .bbs_id = 0x222000200567bd1,
                       .bbs_version = {2, 2, 2},
                       .state = FPGA_ACCELERATOR_UNASSIGNED,
                       .num_mmio = 0x2,
                       .num_interrupts = 0,
                       .fme_object_id = 0xf500000,
                       .port_object_id = 0xf400000,
                       .vendor_id = 0x8086,
                       .device_id = 0x0b30,
                       .fme_num_errors = 9,
                       .port_num_errors = 3,
                       .gbs_guid = "58656f6e-4650-4741-b747-425376303031",
                       .mdata = vc_mdata}}}}
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
    std::initializer_list<std::string> names, fpga_driver drv) {
  std::vector<std::string> keys(names);
  if (keys.empty()) {
    keys = fpga_db::instance()->keys();
  }
  // from the list of platform names requested, remove the ones not found in
  // the platform db
  keys.erase(
    std::remove_if(keys.begin(), keys.end(), [drv](const std::string &n) {
      auto db = fpga_db::instance();
      return !db->exists(n) || (drv != fpga_driver::linux_any
                                && drv != db->get(n).driver)
                            || db->get(n).devices.empty();
  }), keys.end());
  return keys;
}

std::vector<std::string> test_platform::mock_platforms(
    std::initializer_list<std::string> names, fpga_driver drv) {
  std::vector<std::string> keys(names);
  if (keys.empty()) {
    keys = fpga_db::instance()->keys();
  }
  std::vector<std::string> want;
  std::copy_if(keys.begin(), keys.end(), std::back_inserter(want),
               [drv](const std::string &k) {
                 auto db = fpga_db::instance();
                 return db->exists(k) && (drv == fpga_driver::linux_any ||
                                          db->get(k).driver == drv),
                        db->get(k).mock_sysfs != nullptr;
               });
  return want;
}

std::vector<std::string> test_platform::hw_platforms(
    std::initializer_list<std::string> names, fpga_driver drv) {
  std::vector<std::string> keys(names);
  if (keys.empty()) {
    keys = fpga_db::instance()->keys();
  }
  std::vector<std::string> want;
  std::copy_if(keys.begin(), keys.end(), std::back_inserter(want),
               [drv](const std::string &k) {
                 auto db = fpga_db::instance();
                 return db->exists(k) && (drv == fpga_driver::linux_any ||
                                          db->get(k).driver == drv) &&
                        !db->get(k).devices.empty() &&
                        db->get(k).mock_sysfs == nullptr;
               });
  return want;
}

const std::string PCI_DEVICES = "/sys/bus/pci/devices";

typedef std::pair<uint16_t, uint64_t> ven_dev_id;
typedef std::tuple<uint16_t, uint64_t, fpga_driver> platform_cfg;
std::map<ven_dev_id, std::vector<std::string>> known_devices = {
  { { 0x8086, 0xbcc0}, std::vector<std::string>() },
  { { 0x8086, 0xbcc1}, std::vector<std::string>() },
  { { 0x8086, 0x09c4}, std::vector<std::string>() },
  { { 0x8086, 0x09c5}, std::vector<std::string>() },
  { { 0x8086, 0x0b30}, std::vector<std::string>() },
  { { 0x8086, 0x0b31}, std::vector<std::string>() },
};

static std::vector<ven_dev_id> supported_devices() {
  std::vector<ven_dev_id> devs;
  for (auto kv : known_devices) {
    devs.push_back(kv.first);
  }
  return devs;
}

static std::string read_file(const std::string &path) {
  std::ifstream df;
  struct stat st;
  std::string value_string;

  if (stat(path.c_str(), &st)) {
    std::cerr << std::string("WARNING: stat:") + path <<  ":" << strerror(errno) << "\n";
    return "";
  }

  df.open(path);
  if (!df.is_open()) {
    std::cerr << std::string("WARNING: could not open file ") + path << "\n";
    return "";
  }
  df >> value_string;
  return value_string;
}
template<typename T>
static T parse_file_int(const std::string &path) {
  std::string value_string = read_file(path);

  if (value_string.empty())
    return 0;

  T value;

  try {
    value = std::stol(value_string, nullptr, 0);
  }
  catch (std::invalid_argument& e) {
    std::cerr << "WARNING: unable to convert integer from file: " << path << std::endl;
    return 0;
  }
  catch (std::out_of_range& e) {
    std::cerr << "WARNING: value too large from file: " << path << std::endl;
    return 0;
  }

  return value;
}

static std::string make_path(int seg, int bus, int dev, int func){
    std::stringstream num;
    num << std::setw(2) << std::hex << bus; 
    std::string b (num.str());
    num.clear();
    num.str(std::string());

    num << std::setw(4) << std::setfill('0') << seg;
    std::string s (num.str());

    num.clear();
    num.str(std::string());
    num << std::setw(2) << std::setfill('0') << dev;
    std::string d (num.str());

    std::string device_string = s + ":" + b + ":" + d + "." + std::to_string(func);
    return device_string;
}

static std::string glob_first_path(const std::string path) {
  glob_t glob_buf;
  glob_buf.gl_pathc = 0;
  glob_buf.gl_pathv = NULL;
  int globres = glob(path.c_str(), 0, NULL, &glob_buf);
  std::string found_path;

  if (!globres){
    if (glob_buf.gl_pathc > 1) {
        std::cerr << "Ambiguous object key - using first one" << std::endl;
    }
    found_path = std::string(glob_buf.gl_pathv[0]);
  }
  else {
    switch (globres) {
    case GLOB_NOSPACE:
        std::cerr << "FPGA No Memory found." << std::endl;
        break;
    case GLOB_NOMATCH:
        std::cerr << "FPGA Not found." << std::endl;
        break;
    }
  }

  if (glob_buf.gl_pathc && glob_buf.gl_pathv) {
    globfree(&glob_buf);
  }

  return found_path;
}

static std::string format_uuid(const std::string &uuid) {
  std::string formatted_uuid = uuid;
  formatted_uuid.insert(8, "-");
  formatted_uuid.insert(13, "-");
  formatted_uuid.insert(18, "-");
  formatted_uuid.insert(23, "-");
  std::transform(formatted_uuid.begin(), formatted_uuid.end(), formatted_uuid.begin(), ::toupper);
  return formatted_uuid;
}

template<typename T>
static  T read_attribute(const std::string &pci_dir, const std::string &attr) {
  std::string attr_path = pci_dir + "/" + attr;
  struct stat st;


  if (stat(attr_path.c_str(), &st)) {
    std::cerr << std::string("WARNING: stat:") + attr_path <<  ":" << strerror(errno) << "\n";
    return 0;
  }
   return parse_file_int<T>(attr_path);
}

static uint16_t read_socket_id(const std::string devices) {
  std::string glob_path = PCI_DEVICES + "/" + devices + "/fpga*/*/*fme.*/socket_id";
  std::string socket_path = glob_first_path(glob_path);
  return parse_file_int<uint16_t>(socket_path);
}

static uint16_t read_device_id(const std::string &pci_dir) {
  std::string device_path = pci_dir + "/device";
  return parse_file_int<uint16_t>(device_path);
}

static uint16_t read_vendor_id(const std::string &pci_dir) {
  std::string vendor_path = pci_dir + "/vendor";
  return parse_file_int<uint16_t>(vendor_path);
}

static uint64_t read_bitstream_id(const std::string &pci_dir) {
  std::string bitstream_path = pci_dir + "/fpga*/*/*-fme.*/bitstream_id";
  bitstream_path = glob_first_path(bitstream_path);
  return parse_file_int<uint64_t>(bitstream_path);
}

static std::string read_afu_id(const std::string &pci_dir) {
  std::string afu_path = pci_dir + "/fpga*/*/*-port.*/afu_id";

  afu_path = glob_first_path(afu_path);
  return format_uuid(read_file(afu_path));
}

static std::string read_pr_interface_id(const std::string &pci_dir) {
  std::string pr_interface_path = pci_dir + "/fpga/intel-fpga-dev.*/intel-fpga-fme.*/pr/interface_id";
  pr_interface_path = glob_first_path(pr_interface_path);

  if (pr_interface_path.empty()) {
    pr_interface_path = pci_dir + "/fpga_region/region*/dfl-fme.*/dfl-fme-region.*/fpga_region/region*/compat_id";
    pr_interface_path = glob_first_path(pr_interface_path);
  }

  return format_uuid(read_file(pr_interface_path));
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
  std::vector<ven_dev_id>::const_iterator it = std::find(devices.begin(), devices.end(), ven_dev_id(vid, did));
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
    std::string msg = "error scanning pci devices: " + std::string(strerror(errno));
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

fpga_db::fpga_db()
{

}

fpga_db *fpga_db::instance() {
  if (fpga_db::instance_ == nullptr) {
    fpga_db::instance_ = new fpga_db();
    fpga_db::instance_->discover_hw();
  }
  return fpga_db::instance_;
}

static std::map<platform_cfg, std::string> platform_names = {
  {  platform_cfg(0x8086, 0xbcc0, fpga_driver::linux_intel), "skx-p" },
  {  platform_cfg(0x8086, 0xbcc1, fpga_driver::linux_intel), "skx-p-v" },
  {  platform_cfg(0x8086, 0x09c4, fpga_driver::linux_intel), "dcp-rc" },
  {  platform_cfg(0x8086, 0x09c5, fpga_driver::linux_intel), "dcp-rc-v" },
  {  platform_cfg(0x8086, 0xbcc0, fpga_driver::linux_dfl0),  "skx-p-dfl0" },
  {  platform_cfg(0x8086, 0xbcc0, fpga_driver::linux_dfl0),  "skx-p-dfl0_patchset2" },
  {  platform_cfg(0x8086, 0x09c4, fpga_driver::linux_dfl0), "dcp-rc-dfl0_patchset2" },
  {  platform_cfg(0x8086, 0x0b30, fpga_driver::linux_intel), "dcp-vc" },
  {  platform_cfg(0x8086, 0x0b31, fpga_driver::linux_intel), "dcp-vc-v" }
  
};

const char *PCI_DEV_PATTERN = "([0-9a-fA-F]{4}):([0-9a-fA-F]{2}):([0-9]{2})\\.([0-9])";

test_device make_device(uint16_t ven_id, uint16_t dev_id, const std::string &platform, const std::string &pci_path) {
  test_device dev = MOCK_PLATFORMS[platform].devices[0];

  auto r = regex<>::create(PCI_DEV_PATTERN);
  auto m = r->match(pci_path);
  if (m) {
    dev.segment = std::stoi(m->group(1), nullptr, 16);
    dev.bus = std::stoi(m->group(2), nullptr, 16);
    dev.device = std::stoi(m->group(3), nullptr, 10);
    dev.function = std::stoi(m->group(4), nullptr, 10);
    dev.num_vfs = read_attribute<uint8_t>(pci_path, "sriov_numvfs");
    dev.vendor_id = ven_id;
    dev.device_id = dev_id;

    std::string device_string = make_path(dev.segment, dev.bus, dev.device, dev.function);
    dev.socket_id = read_socket_id(device_string);
    uint64_t bitstream_id = read_bitstream_id(pci_path);
    dev.bbs_id = bitstream_id;
    dev.bbs_version = {(uint8_t)FPGA_BBS_VER_MAJOR(bitstream_id),
                         (uint8_t)FPGA_BBS_VER_MINOR(bitstream_id),
                         (uint16_t)FPGA_BBS_VER_PATCH(bitstream_id)};
    strcpy(dev.fme_guid, read_pr_interface_id(pci_path).c_str());
    strcpy(dev.afu_guid, read_afu_id(pci_path).c_str());
  } else {
    std::cerr << "error matching pci dev pattern (" << pci_path << ")\n";
  }
  return dev;
}

/**
 * @brief read the 'driver' symlink to determine if the driver is dfl or intel
 *
 * @param path a sysfs path representing a device (under
 * /sys/bus/pci/<s:b:d:f>)
 *
 * @return fpga_driver enumerating indicating what kind of driver is bound to
 * the device
 */
fpga_driver get_driver(const std::string &path)
{
  char buffer[PATH_MAX] = { 0 };
  std::string sysfs_drvpath = path + "/driver";
  ssize_t lnk_len = readlink(sysfs_drvpath.c_str(), buffer, PATH_MAX);
  if (!lnk_len) {
    auto msg = std::string("error readling link: ") + sysfs_drvpath;
    throw std::runtime_error(msg);
  }
  std::string bname = basename(buffer);
  if (bname == "intel-fpga-pci") {
    return fpga_driver::linux_intel;
  }
  if (bname == "dfl-pci") {
    return fpga_driver::linux_dfl0;
  }
  return fpga_driver::none;
}

std::pair<std::string, test_platform> make_platform(uint16_t ven_id, uint16_t dev_id, const std::vector<std::string> &pci_paths) {
  test_platform platform;
  // test_platform data structure only supports one driver (for now)
  // TODO: assert that all devices represented by pci_patsh are all bound to
  // the same driver - for now, just use the first path
  platform.driver = get_driver(pci_paths[0]);
  // this is discovered hw platform, set mock_sysfs to null
  platform.mock_sysfs = nullptr;
  std::string name = platform_names[platform_cfg(ven_id, dev_id, platform.driver)];
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

test_platform fpga_db::get(const std::string &key) {
  return platforms_[key];
}

bool fpga_db::exists(const std::string &key) {
  return platforms_.find(key) != platforms_.end();
}

}  // end of namespace testing
}  // end of namespace opae
