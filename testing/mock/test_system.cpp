// Copyright(c) 2017, Intel Corporation
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
/*
 * test-system.cpp
 */

#include "test_system.h"
#include <stdarg.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include "c_test_system.h"
#include "test_utils.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <dlfcn.h>
#include <sys/stat.h>
#include <uuid/uuid.h>

void *__builtin_return_address(unsigned level);

// hijack malloc
static bool _invalidate_malloc = false;
static uint32_t _invalidate_malloc_after = 0;
static const char *_invalidate_malloc_when_called_from = nullptr;
void *malloc(size_t size) {
  if (_invalidate_malloc) {
    if (!_invalidate_malloc_when_called_from) {
      if (!_invalidate_malloc_after) {
        _invalidate_malloc = false;
        return nullptr;
      }

      --_invalidate_malloc_after;

    } else {
      void *caller = __builtin_return_address(0);
      int res;
      Dl_info info;

      dladdr(caller, &info);
      if (!info.dli_sname)
        res = 1;
      else
        res = strcmp(info.dli_sname, _invalidate_malloc_when_called_from);

      if (!_invalidate_malloc_after && !res) {
        _invalidate_malloc = false;
        _invalidate_malloc_when_called_from = nullptr;
        return nullptr;
      } else if (!res)
        --_invalidate_malloc_after;
    }
  }
  return __libc_malloc(size);
}

// hijack calloc
static bool _invalidate_calloc = false;
static uint32_t _invalidate_calloc_after = 0;
static const char *_invalidate_calloc_when_called_from = nullptr;
void *calloc(size_t nmemb, size_t size) {
  if (_invalidate_calloc) {
    if (!_invalidate_calloc_when_called_from) {
      if (!_invalidate_calloc_after) {
        _invalidate_calloc = false;
        return nullptr;
      }

      --_invalidate_calloc_after;

    } else {
      void *caller = __builtin_return_address(0);
      int res;
      Dl_info info;

      dladdr(caller, &info);
      if (!info.dli_sname)
        res = 1;
      else
        res = strcmp(info.dli_sname, _invalidate_calloc_when_called_from);

      if (!_invalidate_calloc_after && !res) {
        _invalidate_calloc = false;
        _invalidate_calloc_when_called_from = nullptr;
        return nullptr;
      } else if (!res)
        --_invalidate_calloc_after;
    }
  }
  return __libc_calloc(nmemb, size);
}

namespace opae {
namespace testing {

static const char *dev_pattern =
    R"regex(/dev/intel-fpga-\(fme\|port\)\.\([0-9]\+\))regex";
static const char *sysclass_pattern =
    R"regex(/sys/class/fpga/intel-fpga-dev\.\([0-9]\+\))regex";

mock_object::mock_object(const std::string &devpath,
                         const std::string &sysclass, uint32_t device_id,
                         type_t type)
    : devpath_(devpath),
      sysclass_(sysclass),
      device_id_(device_id),
      type_(type) {}

int mock_fme::ioctl(int request, va_list argp) {
  (void)request;
  (void)argp;
  return 0;
}

int mock_port::ioctl(int request, va_list argp) {
  (void)request;
  (void)argp;
  return 0;
}

#define ASSERT_FN(fn)                              \
  do {                                             \
    if (fn == nullptr) {                           \
      throw std::runtime_error(#fn " not loaded"); \
    }                                              \
  } while (false);

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

typedef std::map<std::string, test_platform> platform_db;

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

const char *rc_mdata =
    R"mdata({"version": 112,
   "afu-image":
    {"clock-frequency-high": 312,
     "clock-frequency-low": 156,
     "interface-uuid": "bb0023cf-0bd2-579a-90ef-97fe743a6c63",
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

static platform_db PLATFORMS = {
    {"skx-p-1s",
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
                       .bbs_id = 0x63000023b637277,
                       .bbs_version = {6, 3, 0},
                       .state = FPGA_ACCELERATOR_UNASSIGNED,
                       .num_mmio = 0x2,
                       .num_interrupts = 0,
                       .fme_object_id = 0xf500000,
                       .port_object_id = 0xf400000,
                       .vendor_id = 0x8086,
                       .device_id = 0xbcc0,
                       .fme_num_errors = 9,
                       .port_num_errors = 3,
                       .gbs_guid = "58656f6e-4650-4741-b747-425376303031",
                       .mdata = skx_mdata}}}},
    {"dcp-rc",
     test_platform{.mock_sysfs = "mock_sys_tmp-dcp-rc-nlb3.tar.gz",
                   .devices = {test_device{
                       .fme_guid = "BB0023CF-0BD2-579A-90EF-97FE743A6C63",
                       .afu_guid = "D8424DC4-A4A3-C413-F89E-433683F9040B",
                       .segment = 0x0,
                       .bus = 0x05,
                       .device = 0,
                       .function = 0,
                       .socket_id = 0,
                       .num_slots = 1,
                       .bbs_id = 0x112000200000159,
                       .bbs_version = {1, 1, 2},
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
};

test_platform test_platform::get(const std::string &key) {
  return PLATFORMS[key];
}

bool test_platform::exists(const std::string &key) {
  return PLATFORMS.find(key) != PLATFORMS.end();
}

std::vector<std::string> test_platform::keys(bool sorted) {
  std::vector<std::string> keys(PLATFORMS.size());
  std::transform(
      PLATFORMS.begin(), PLATFORMS.end(), keys.begin(),
      [](const std::pair<std::string, test_platform> &it) { return it.first; });
  if (sorted) {
    std::sort(keys.begin(), keys.end());
  }

  return keys;
}

test_system *test_system::instance_ = nullptr;

test_system::test_system() : root_("") {
  open_ = (open_func)dlsym(RTLD_NEXT, "open");
  open_create_ = (open_create_func)open_;
  read_ = (read_func)dlsym(RTLD_NEXT, "read");
  fopen_ = (fopen_func)dlsym(RTLD_NEXT, "fopen");
  close_ = (close_func)dlsym(RTLD_NEXT, "close");
  ioctl_ = (ioctl_func)dlsym(RTLD_NEXT, "ioctl");
  opendir_ = (opendir_func)dlsym(RTLD_NEXT, "opendir");
  readlink_ = (readlink_func)dlsym(RTLD_NEXT, "readlink");
  xstat_ = (__xstat_func)dlsym(RTLD_NEXT, "__xstat");
  lstat_ = (__xstat_func)dlsym(RTLD_NEXT, "__lxstat");
  scandir_ = (scandir_func)dlsym(RTLD_NEXT, "scandir");
}

test_system *test_system::instance() {
  if (test_system::instance_ == nullptr) {
    test_system::instance_ = new test_system();
  }
  return test_system::instance_;
}

void test_system::prepare_syfs(const test_platform &platform) {
  int result = 0;
  char tmpsysfs[]{"tmpsysfs-XXXXXX"};

  if (platform.mock_sysfs != nullptr) {
    char *tmp = mkdtemp(tmpsysfs);
    if (tmp == nullptr) {
      throw std::runtime_error("error making tmpsysfs");
    }
    root_ = std::string(tmp);
    std::string cmd = "tar xzf " + std::string(platform.mock_sysfs) + " -C " +
                      root_ + " --strip 1";
    result = std::system(cmd.c_str());
  }
  return (void) result;
}

void test_system::remove_sysfs() {
  int result = 0;
  if (root_.find("tmpsysfs") != std::string::npos) {
    struct stat st;
    if (stat(root_.c_str(), &st)) {
      std::cerr << "Error stat'ing root dir (" << root_
                << "):" << strerror(errno) << "\n";
      return;
    }
    if (S_ISDIR(st.st_mode)) {
      auto cmd = "rm -rf " + root_;
      result = std::system(cmd.c_str());
    }
  }
  return (void) result;
}

void test_system::set_root(const char *root) { root_ = root; }
std::string test_system::get_root() { return root_; }

std::string test_system::get_sysfs_path(const std::string &src) {
  auto it = registered_files_.find(src);
  if (it != registered_files_.end()) {
    return it->second;
  }
  if (src.find("/sys") == 0 || src.find("/dev/intel-fpga") == 0) {
    if (!root_.empty() && root_.size() > 1) {
      return root_ + src;
    }
  }
  return src;
}

std::vector<uint8_t> test_system::assemble_gbs_header(const test_device &td) {
  std::vector<uint8_t> gbs_header(20, 0);
  if (uuid_parse(td.gbs_guid, gbs_header.data())) {
    std::string msg = "unable to parse UUID: ";
    msg.append(td.gbs_guid);
    throw std::runtime_error(msg);
  }
  uint32_t len = strlen(td.mdata);
  *reinterpret_cast<uint32_t *>(gbs_header.data() + 16) = len;
  std::copy(&td.mdata[0], &td.mdata[len], std::back_inserter(gbs_header));
  return gbs_header;
}

std::vector<uint8_t> test_system::assemble_gbs_header(const test_device &td, const char *mdata) {
  if (mdata) {
    test_device copy = td;
    copy.mdata = mdata;
    return assemble_gbs_header(copy);
  }
  return std::vector<uint8_t>(0);
}

void test_system::initialize() {
  ASSERT_FN(open_);
  ASSERT_FN(open_create_);
  ASSERT_FN(read_);
  ASSERT_FN(fopen_);
  ASSERT_FN(close_);
  ASSERT_FN(ioctl_);
  ASSERT_FN(readlink_);
  ASSERT_FN(xstat_);
  ASSERT_FN(lstat_);
  ASSERT_FN(scandir_);
  for (const auto &kv : default_ioctl_handlers_) {
    register_ioctl_handler(kv.first, kv.second);
  }
}

void test_system::finalize() {
  std::lock_guard<std::mutex> guard(fds_mutex_);
  for (auto kv : fds_) {
    if (kv.second) {
      delete kv.second;
      kv.second = nullptr;
    }
  }
  remove_sysfs();
  root_ = "";
  fds_.clear();
  for (auto kv : registered_files_) {
    unlink(kv.second.c_str());
  }
  ioctl_handlers_.clear();
}

bool test_system::default_ioctl_handler(int request, ioctl_handler_t h) {
  bool already_registered =
      default_ioctl_handlers_.find(request) != default_ioctl_handlers_.end();
  default_ioctl_handlers_[request] = h;
  return already_registered;
}

bool test_system::register_ioctl_handler(int request, ioctl_handler_t h) {
  bool already_registered =
      ioctl_handlers_.find(request) != ioctl_handlers_.end();
  ioctl_handlers_[request] = h;
  return already_registered;
}

FILE *test_system::register_file(const std::string &path) {
  auto it = registered_files_.find(path);
  if (it == registered_files_.end()) {
    registered_files_[path] =
        "/tmp/testfile" + std::to_string(registered_files_.size());
  }

  auto fptr = fopen(path.c_str(), "w+");
  return fptr;
}

void test_system::normalize_guid(std::string &guid_str, bool with_hyphens) {
  // normalizing a guid string can make it easier to compare guid strings
  // and can also put the string in a format that can be parsed into actual
  // guid bytes (uuid_parse expects the string to include hyphens).
  const size_t std_guid_str_size = 36;
  const size_t char_guid_str_size = 32;
  if (guid_str.back() == '\n') {
    guid_str.erase(guid_str.end()-1);
  }
  std::locale lc;
  auto c_idx = guid_str.find('-');
  if (with_hyphens && c_idx == std::string::npos) {
    // if we want the standard UUID format with hyphens (8-4-4-4-12)
    if (guid_str.size() == char_guid_str_size) {
      int idx = 20;
      while (c_idx != 8) {
        guid_str.insert(idx, 1, '-');
        idx -= 4;
        c_idx = guid_str.find('-');
      }
    } else {
      throw std::invalid_argument("invalid guid string");
    }
  } else if (!with_hyphens && c_idx == 8) {
    // we want the hex characters only, no other extra chars
    if (guid_str.size() == std_guid_str_size) {
      while (c_idx != std::string::npos) {
        guid_str.erase(c_idx, 1);
        c_idx = guid_str.find('-');
      }
    } else {
      throw std::invalid_argument("invalid guid string");
    }
  }

  for (auto & c : guid_str) {
    c = std::tolower(c, lc);
  }
}

uint32_t get_device_id(const std::string &sysclass) {
  uint32_t res(0);
  std::ifstream fs;
  fs.open(sysclass + "/device/device");
  if (fs.is_open()) {
    std::string line;
    std::getline(fs, line);
    fs.close();
    return std::stoul(line, 0, 16);
  }
  return res;
}

int test_system::open(const std::string &path, int flags) {
  std::string syspath = get_sysfs_path(path);
  int fd = open_(syspath.c_str(), flags);
  auto r1 = regex<>::create(sysclass_pattern);
  auto r2 = regex<>::create(dev_pattern);
  match_t::ptr_t m;

  // check if we are opening a driver attribute file
  // or a device file to save the fd in an internal map
  // this can be used later, (especially in ioctl)
  if (r1 && (m = r1->match(path))) {
    // path matches /sys/class/fpga/intel-fpga-dev\..*
    // we are opening a driver attribute file
    auto sysclass_path = m->group(0);
    auto device_id = get_device_id(get_sysfs_path(sysclass_path));
    std::lock_guard<std::mutex> guard(fds_mutex_);
    fds_[fd] = new mock_object(path, sysclass_path, device_id);
  } else if (r2 && (m = r2->match(path))) {
    // path matches /dev/intel-fpga-(fme|port)\..*
    // we are opening a device
    auto sysclass_path = "/sys/class/fpga/intel-fpga-dev." + m->group(2);
    auto device_id = get_device_id(get_sysfs_path(sysclass_path));
    if (m->group(1) == "fme") {
      std::lock_guard<std::mutex> guard(fds_mutex_);
      fds_[fd] = new mock_fme(path, sysclass_path, device_id);
    } else if (m->group(1) == "port") {
      std::lock_guard<std::mutex> guard(fds_mutex_);
      fds_[fd] = new mock_port(path, sysclass_path, device_id);
    }
  }
  return fd;
}

int test_system::open(const std::string &path, int flags, mode_t mode) {
  std::string syspath = get_sysfs_path(path);
  int fd = open_create_(syspath.c_str(), flags, mode);
  if (syspath.find(root_) == 0) {
    std::lock_guard<std::mutex> guard(fds_mutex_);
    std::map<int, mock_object *>::iterator it = fds_.find(fd);
    if (it != fds_.end()) { delete it->second; }
    fds_[fd] = new mock_object(path, "", 0);
  }
  return fd;
}

static bool _invalidate_read = false;
static uint32_t _invalidate_read_after = 0;
static const char *_invalidate_read_when_called_from = nullptr;
void test_system::invalidate_read(uint32_t after,
                                  const char *when_called_from) {
  _invalidate_read = true;
  _invalidate_read_after = after;
  _invalidate_read_when_called_from = when_called_from;
}

ssize_t test_system::read(int fd, void *buf, size_t count) {
  if (_invalidate_read) {
    if (!_invalidate_read_when_called_from) {
      if (!_invalidate_read_after) {
        _invalidate_read = false;
        return -1;
      }

      --_invalidate_read_after;

    } else {
      // 2 here, because we were called through..
      // 0 test_system.cpp:opae_test_read()
      // 1 mock.c:read()
      // 2 <caller>
      void *caller = __builtin_return_address(2);
      int res;
      Dl_info info;

      dladdr(caller, &info);
      if (!info.dli_sname)
        res = 1;
      else
        res = strcmp(info.dli_sname, _invalidate_read_when_called_from);

      if (!_invalidate_read_after && !res) {
        _invalidate_read = false;
        _invalidate_read_when_called_from = nullptr;
        return -1;
      } else if (!res)
        --_invalidate_read_after;
    }
  }
  return read_(fd, buf, count);
}

FILE *test_system::fopen(const std::string &path, const std::string &mode) {
  std::string syspath = get_sysfs_path(path);
  return fopen_(syspath.c_str(), mode.c_str());
}

int test_system::close(int fd) {
  std::lock_guard<std::mutex> guard(fds_mutex_);
  std::map<int, mock_object *>::iterator it = fds_.find(fd);
  if (it != fds_.end()) {
    delete it->second;
    fds_.erase(it);
  }
  return close_(fd);
}

int test_system::ioctl(int fd, unsigned long request, va_list argp) {
  mock_object *mo  = nullptr;
  {
      std::lock_guard<std::mutex> guard(fds_mutex_);
      auto mi = fds_.find(fd);
      if (mi != fds_.end()) {
          mo = mi->second;
      }
  }
  
  if (mo == nullptr) {
      char *arg = va_arg(argp, char *);
      return ioctl_(fd, request, arg);
  }
  
  // replace mock_it->second with mo
  auto handler_it = ioctl_handlers_.find(request);
  if (handler_it != ioctl_handlers_.end()) {
      return handler_it->second(mo, request, argp);
  }
  return mo->ioctl(request, argp);

}

DIR *test_system::opendir(const char *path) {
  std::string syspath = get_sysfs_path(path);
  return opendir_(syspath.c_str());
}

ssize_t test_system::readlink(const char *path, char *buf, size_t bufsize) {
  std::string syspath = get_sysfs_path(path);
  return readlink_(syspath.c_str(), buf, bufsize);
}

int test_system::xstat(int ver, const char *path, struct stat *buf) {
  std::string syspath = get_sysfs_path(path);
  return xstat_(ver, syspath.c_str(), buf);
}

int test_system::lstat(int ver, const char *path, struct stat *buf) {
  std::string syspath = get_sysfs_path(path);
  return lstat_(ver, syspath.c_str(), buf);
}

int test_system::scandir(const char *dirp, struct dirent ***namelist,
                         filter_func filter, compare_func cmp) {
  std::string syspath = get_sysfs_path(dirp);
  return scandir_(syspath.c_str(), namelist, filter, cmp);
}

void test_system::invalidate_malloc(uint32_t after,
                                    const char *when_called_from) {
  _invalidate_malloc = true;
  _invalidate_malloc_after = after;
  _invalidate_malloc_when_called_from = when_called_from;
}

void test_system::invalidate_calloc(uint32_t after,
                                    const char *when_called_from) {
  _invalidate_calloc = true;
  _invalidate_calloc_after = after;
  _invalidate_calloc_when_called_from = when_called_from;
}

}  // end of namespace testing
}  // end of namespace opae

// C functions

int opae_test_open(const char *path, int flags) {
  return opae::testing::test_system::instance()->open(path, flags);
}

int opae_test_open_create(const char *path, int flags, mode_t mode) {
  return opae::testing::test_system::instance()->open(path, flags, mode);
}

ssize_t opae_test_read(int fd, void *buf, size_t count) {
  return opae::testing::test_system::instance()->read(fd, buf, count);
}

FILE *opae_test_fopen(const char *path, const char *mode) {
  return opae::testing::test_system::instance()->fopen(path, mode);
}

int opae_test_close(int fd) {
  return opae::testing::test_system::instance()->close(fd);
}

int opae_test_ioctl(int fd, unsigned long request, va_list argp) {
  return opae::testing::test_system::instance()->ioctl(fd, request, argp);
}

DIR *opae_test_opendir(const char *name) {
  return opae::testing::test_system::instance()->opendir(name);
}

ssize_t opae_test_readlink(const char *path, char *buf, size_t bufsize) {
  return opae::testing::test_system::instance()->readlink(path, buf, bufsize);
}

int opae_test_xstat(int ver, const char *path, struct stat *buf) {
  return opae::testing::test_system::instance()->xstat(ver, path, buf);
}

int opae_test_lstat(int ver, const char *path, struct stat *buf) {
  return opae::testing::test_system::instance()->lstat(ver, path, buf);
}

int opae_test_scandir(const char *dirp, struct dirent ***namelist,
                      filter_func filter, compare_func cmp) {
  return opae::testing::test_system::instance()->scandir(dirp, namelist, filter,
                                                         cmp);
}
