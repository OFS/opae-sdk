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
 * mock-system.cpp
 */

#include "test_system.h"
#include <dlfcn.h>
#include <stdarg.h>
#include <algorithm>
#include "c_test_system.h"

int mock_fme::ioctl(int request, va_list argp) {
  (void) request;
  (void) argp;
  return 0; }

int mock_port::ioctl(int request, va_list argp) {
  (void) request;
  (void) argp;
  return 0; }


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
                     .fme_object_id = 9,
                     .port_object_id = 9,
                     .vendor_id = 0x1234,
                     .device_id = 0x1234,
                     .fme_num_errors = 0x1234,
                     .port_num_errors = 0x1234};
}

typedef std::map<std::string, test_platform> platform_db;

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
                       .fme_object_id = 0xf500000,
                       .port_object_id = 0xf400000,
                       .vendor_id = 0x8086,
                       .device_id = 0xbcc0,
                       .fme_num_errors = 9,
                       .port_num_errors = 3}}}}};

test_platform test_platform::get(const std::string &key) {
  return PLATFORMS[key];
}

bool test_platform::exists(const std::string &key) {
  return PLATFORMS.find(key) != PLATFORMS.end();
}

std::vector<std::string> test_platform::keys(bool sorted) {
  std::vector<std::string> keys(PLATFORMS.size());
  std::transform(PLATFORMS.begin(), PLATFORMS.end(), keys.begin(),
                 [](const std::pair<std::string, test_platform> &it) {
                   return it.first;
                 });
  if (sorted) {
    std::sort(keys.begin(), keys.end());
  }

  return keys;
}

test_system *test_system::instance_ = 0;

test_system::test_system() : root_("") {
  open_ = (open_func)dlsym(RTLD_NEXT, "open");
  open_create_ = (open_create_func)open_;
  close_ = (close_func)dlsym(RTLD_NEXT, "close");
  ioctl_ = (ioctl_func)dlsym(RTLD_NEXT, "ioctl");
  opendir_ = (opendir_func)dlsym(RTLD_NEXT, "opendir");
  readlink_ = (readlink_func)dlsym(RTLD_NEXT, "readlink");
  xstat_ = (__xstat_func)dlsym(RTLD_NEXT, "__xstat");
  lstat_ = (__xstat_func)dlsym(RTLD_NEXT, "__lxstat");
}

test_system *test_system::instance() {
  if (test_system::instance_ == nullptr) {
    test_system::instance_ = new test_system();
  }
  return test_system::instance_;
}

std::string test_system::prepare_syfs(const test_platform &platform) {
  std::string tmpsysfs = "tmpsysfs-XXXXXX";
  if (platform.mock_sysfs != nullptr) {
    tmpsysfs = mkdtemp(const_cast<char *>(tmpsysfs.c_str()));
    std::string cmd = "tar xzf " + std::string(platform.mock_sysfs) + " -C " +
                      tmpsysfs + " --strip 1";
    std::system(cmd.c_str());
    root_ = tmpsysfs;
    return tmpsysfs;
  }
  return "/";
}

void test_system::set_root(const char *root) { root_ = root; }

std::string test_system::get_sysfs_path(const std::string &src) {
  if (src.find("/sys/class/fpga") == 0 || src.find("/dev/intel-fpga") == 0) {
    if (!root_.empty() && root_.size() > 1) {
      return root_ + src;
    }
  }
  return src;
}

void test_system::initialize() {
  ASSERT_FN(open_);
  ASSERT_FN(open_create_);
  ASSERT_FN(close_);
  ASSERT_FN(ioctl_);
  ASSERT_FN(readlink_);
  ASSERT_FN(xstat_);
  ASSERT_FN(lstat_);
}

void test_system::finalize() {
  for (auto kv : fds_) {
    if (kv.second) {
      delete kv.second;
      kv.second = nullptr;
    }
  }
  root_ = "";
  fds_.clear();
}

int test_system::open(const std::string &path, int flags) {
  std::string syspath = get_sysfs_path(path);
  int fd = open_(syspath.c_str(), flags);
  if (syspath.find(root_) == 0) {
    if (path.find("/dev/intel-fpga-fme") == 0) {
      fds_[fd] = new mock_fme(path);
    } else if (path.find("/dev/intel-fpga-port") == 0) {
      fds_[fd] = new mock_port(path);
    } else {
      fds_[fd] = new mock_object(path);
    }
  }
  return fd;
}

int test_system::open(const std::string &path, int flags, mode_t mode) {
  std::string syspath = get_sysfs_path(path);
  int fd = open_create_(syspath.c_str(), flags, mode);
  if (syspath.find(root_) == 0) {
    fds_[fd] = new mock_object(path);
  }
  return fd;
}

int test_system::close(int fd) { return close_(fd); }

int test_system::ioctl(int fd, unsigned long request, va_list argp) {
  auto it = fds_.find(fd);
  if (it == fds_.end()) {
    char *arg = va_arg(argp, char *);
    return ioctl_(fd, request, arg);
  }
  return it->second->ioctl(request, argp);
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

// C functions

int opae_test_open(const char *path, int flags) {
  return test_system::instance()->open(path, flags);
}

int opae_test_open_create(const char *path, int flags, mode_t mode) {
  return test_system::instance()->open(path, flags, mode);
}

int opae_test_close(int fd) { return test_system::instance()->close(fd); }

int opae_test_ioctl(int fd, unsigned long request, va_list argp) {
  return test_system::instance()->ioctl(fd, request, argp);
}

DIR *opae_test_opendir(const char *name) {
  return test_system::instance()->opendir(name);
}

ssize_t opae_test_readlink(const char *path, char *buf, size_t bufsize) {
  return test_system::instance()->readlink(path, buf, bufsize);
}

int opae_test_xstat(int ver, const char *path, struct stat *buf) {
  return test_system::instance()->xstat(ver, path, buf);
}

int opae_test_lstat(int ver, const char *path, struct stat *buf) {
  return test_system::instance()->lstat(ver, path, buf);
}
