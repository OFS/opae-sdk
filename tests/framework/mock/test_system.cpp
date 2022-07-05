// Copyright(c) 2017-2022, Intel Corporation
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
#include <glob.h>
#include <stdarg.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include "test_utils.h"
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <uuid/uuid.h>
#include <ftw.h>

void *__builtin_return_address(unsigned level);

namespace opae {
namespace testing {

static const char *dev_pattern =
    R"regex(/dev/(intel-fpga|dfl)-(fme|port)\.([0-9]+))regex";
static const char *sysclass_pattern =
    R"regex(/sys/class/fpga((?:_region)?/(region|intel-fpga-dev\.)([0-9]+))regex";

static std::map<std::string, std::string> fpga_sysfs_path_map = {
    {"/dev/intel", "/sys/class/fpga/intel-fpga-dev."},
    {"/dev/dfl", "/sys/class/fpga_region/region"}};

mock_object::mock_object(const std::string &devpath,
                         const std::string &sysclass,
                         fpga_device_id device_id,
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
                     .num_vfs = 8,
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
                     .subsystem_vendor_id = 0x1234,
                     .subsystem_device_id = 0x1234,
                     .fme_num_errors = 0x1234,
                     .port_num_errors = 0x1234,
                     .gbs_guid = "C544CE5C-F630-44E1-8551-59BD87AF432E",
                     .has_afu = true,
                     .mdata = ""};
}

test_system::test_system() : initialized_(false), root_("") {}

test_system *test_system::instance_ = nullptr;
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
  return (void)result;
}


extern "C" {
int process_fpath(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftw) {
  (void)sb;
  (void)ftw;
  if (typeflag & FTW_DP) {
    if (rmdir(fpath)) {
      if (errno == ENOTDIR) {
        goto do_unlink;
      }
      std::cerr << "error removing directory: " << fpath << " - " << strerror(errno) << "\n";
      return -1;
    }
  }
do_unlink:
  if (unlink(fpath) && errno != ENOENT) {
      std::cerr << "error removing node: " << fpath << " - " << strerror(errno) << "\n";
      return -1;
  }
  return 0;
}
}

int test_system::remove_sysfs_dir(const char *path) {
  if (root_.find("tmpsysfs") != std::string::npos) {
    auto real_path = path == nullptr ? root_ : get_sysfs_path(path);
    return nftw(real_path.c_str(), process_fpath, 100, FTW_DEPTH | FTW_PHYS);
  }
  return 0;
}

int test_system::remove_sysfs() {
  return remove_sysfs_dir();
}


void test_system::set_root(const char *root) { root_ = root; }
std::string test_system::get_root() { return root_; }

std::string test_system::get_sysfs_path(const std::string &src) {
  auto it = registered_files_.find(src);
  if (it != registered_files_.end()) {
    return it->second;
  }
  if (src.find("/sys") == 0 || src.find("/dev/intel-fpga") == 0 ||
      src.find("/dev/dfl-") == 0) {
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

std::vector<uint8_t> test_system::assemble_gbs_header(const test_device &td,
                                                      const char *mdata) {
  if (mdata) {
    test_device copy = td;
    copy.mdata = mdata;
    return assemble_gbs_header(copy);
  }
  return std::vector<uint8_t>(0);
}

void test_system::initialize() {
  invalidate_malloc_ = false;
  invalidate_malloc_after_ = 0;
  invalidate_malloc_when_called_from_ = nullptr;

  invalidate_calloc_ = false;
  invalidate_calloc_after_ = 0;
  invalidate_calloc_when_called_from_ = nullptr;

  invalidate_read_ = false;
  invalidate_read_after_ = 0;
  invalidate_read_when_called_from_ = nullptr;

  hijack_sched_setaffinity_ = false;
  hijack_sched_setaffinity_return_val_ = 0;
  hijack_sched_setaffinity_after_ = 0;
  hijack_sched_setaffinity_caller_ = nullptr;

  invalidate_strdup_ = false;
  invalidate_strdup_after_ = 0;
  invalidate_strdup_when_called_from_ = nullptr;

  for (const auto &kv : default_ioctl_handlers_) {
    register_ioctl_handler(kv.first, kv.second);
  }

  initialized_ = true;
}

void test_system::finalize() {
  if (!initialized_) {
    return;
  }

  {
    std::lock_guard<std::mutex> guard(fds_mutex_);
    for (auto kv : fds_) {
      if (kv.second.extra_)
        delete kv.second.extra_;
    }
    fds_.clear();
  }

  {
    std::lock_guard<std::mutex> guard(fopens_mutex_);
    for (auto kv : fopens_) {
      if (kv.first)
        ::fclose(kv.first);
    }
    fopens_.clear();
  }

  {
    std::lock_guard<std::mutex> guard(popens_mutex_);
    for (auto kv : popen_requests_) {
      ::fclose(kv.first);
      unlink(kv.second.extra_->c_str());
      delete kv.second.extra_;
    }
    popen_requests_.clear();
  }

  {
    std::lock_guard<std::mutex> guard(opendirs_mutex_);
    for (auto kv : opendirs_) {
      ::closedir(kv.first);
    }
    opendirs_.clear();
  }

  {
    std::lock_guard<std::mutex> guard(globs_mutex_);
    for (auto kv : globs_) {
      ::globfree(kv.second.extra_);
      delete kv.second.extra_;
    }
    globs_.clear();
  }

  {
    std::lock_guard<std::mutex> guard(mem_allocs_mutex_);
    for (auto kv : mem_allocs_) {
      ::free(kv.first);
    }
    mem_allocs_.clear();
  }

  remove_sysfs();
  root_ = "";

  for (auto kv : registered_files_) {
    unlink(kv.second.c_str());
  }
  registered_files_.clear();

  ioctl_handlers_.clear();

  initialized_ = false;
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
  return this->fopen(path.c_str(), "w+");
}

void test_system::normalize_guid(std::string &guid_str, bool with_hyphens) {
  // normalizing a guid string can make it easier to compare guid strings
  // and can also put the string in a format that can be parsed into actual
  // guid bytes (uuid_parse expects the string to include hyphens).
  const size_t std_guid_str_size = 36;
  const size_t char_guid_str_size = 32;
  if (guid_str.back() == '\n') {
    guid_str.erase(guid_str.end() - 1);
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

  for (auto &c : guid_str) {
    c = std::tolower(c, lc);
  }
}

template <typename T>
T get_sysfs_unsigned(const std::string &sysclass, const std::string &target)
{
  T t = 0;
  std::ifstream fs;
  fs.open(sysclass + target);
  if (fs.is_open()) {
    std::string line;
    std::getline(fs, line);
    fs.close();
    return static_cast<T>(std::stoul(line, 0, 16));
  }
  return t;
}

fpga_device_id get_fpga_device_id(const std::string &sysclass) {
  uint16_t vendor = get_sysfs_unsigned<uint16_t>(sysclass, "/device/vendor");
  uint16_t device = get_sysfs_unsigned<uint16_t>(sysclass, "/device/device");
  uint16_t subsystem_vendor =
    get_sysfs_unsigned<uint16_t>(sysclass, "/device/subsystem_vendor");
  uint16_t subsystem_device =
    get_sysfs_unsigned<uint16_t>(sysclass, "/device/subsystem_device");

  return fpga_device_id(vendor, device, subsystem_vendor, subsystem_device);
}

std::string test_system::get_sysfs_claass_path(const std::string &path) {
  for (auto it : fpga_sysfs_path_map) {
    if (path.find(it.first) == 0) {
      return it.second;
    }
  }
  return "";
}

int test_system::open(const std::string &path, int flags) {
  if (!initialized_) {
    return ::open(path.c_str(), flags);
  }
  std::string syspath = get_sysfs_path(path);
  int fd;
  auto r1 = regex<>::create(sysclass_pattern);
  auto r2 = regex<>::create(dev_pattern);
  match_t::ptr_t m;

  mock_object *mo;

  // check if we are opening a driver attribute file
  // or a device file to save the fd in an internal map
  // this can be used later, (especially in ioctl)
  if (r1 && (m = r1->match(path))) {
    // path matches /sys/class/fpga/intel-fpga-dev\..*
    // we are opening a driver attribute file

    if (flags == O_WRONLY) {
      // truncate the file to zero to emulate the sysfs behavior.
      flags |= O_TRUNC;
    }

    fd = ::open(syspath.c_str(), flags);
    auto sysclass_path = m->group(0);
    auto device_id = get_fpga_device_id(get_sysfs_path(sysclass_path));

    if (fd >= 0) {
      std::lock_guard<std::mutex> guard(fds_mutex_);
      mo = new mock_object(path, sysclass_path, device_id);
      fds_[fd] = Resource<mock_object>("open()", caller(), mo);
    }

  } else if (r2 && (m = r2->match(path))) {
    // path matches /dev/intel-fpga-(fme|port)\..*
    // we are opening a device
    fd = ::open(syspath.c_str(), flags);
    auto sysclass_path = get_sysfs_claass_path(path) + m->group(3);
    auto device_id = get_fpga_device_id(get_sysfs_path(sysclass_path));
    if (m->group(2) == "fme") {
      if (fd >= 0) {
        std::lock_guard<std::mutex> guard(fds_mutex_);
        mo = new mock_fme(path, sysclass_path, device_id);
        fds_[fd] = Resource<mock_object>("open()", caller(), mo);
      }
    } else if (m->group(2) == "port") {
      if (fd >= 0) {
        std::lock_guard<std::mutex> guard(fds_mutex_);
        mo = new mock_port(path, sysclass_path, device_id);
        fds_[fd] = Resource<mock_object>("open()", caller(), mo);
      }
    }
  } else {
    fd = ::open(syspath.c_str(), flags);
    if (fd >= 0) {
      std::lock_guard<std::mutex> guard(fds_mutex_);
      fds_[fd] = Resource<mock_object>("open()", caller());
    }
  }
  return fd;
}

int test_system::open(const std::string &path, int flags, mode_t mode) {
  if (!initialized_) {
    return ::open(path.c_str(), flags, mode);
  }

  std::string syspath = get_sysfs_path(path);
  int fd = ::open(syspath.c_str(), flags, mode);

  if (syspath.find(root_) == 0) {
    if (fd >= 0) {
      std::lock_guard<std::mutex> guard(fds_mutex_);

      std::map<int, Resource<mock_object>>::iterator it = fds_.find(fd);
      if (it != fds_.end()) {
        if (it->second.extra_) {
          delete it->second.extra_;
          it->second.extra_ = nullptr;
        }
      }
      mock_object *mo = new mock_object(path, "", fpga_device_id(0, 0, 0, 0));

      fds_[fd] = Resource<mock_object>("open_create()", caller(), mo);
    }
  } else if (fd >= 0) {
    std::lock_guard<std::mutex> guard(fds_mutex_);
    fds_[fd] = Resource<mock_object>("open_create()", caller());
  }

  return fd;
}

int test_system::close(int fd) {
  if (initialized_ && fd >= 0) {
    std::lock_guard<std::mutex> guard(fds_mutex_);
    std::map<int, Resource<mock_object>>::iterator it = fds_.find(fd);
    if (it != fds_.end()) {
      if (it->second.extra_)
        delete it->second.extra_;
      fds_.erase(it);
    }
  }
  return ::close(fd);
}

void test_system::invalidate_read(uint32_t after,
                                  const char *when_called_from) {
  invalidate_read_ = true;
  invalidate_read_after_ = after;
  invalidate_read_when_called_from_ = when_called_from;
}

ssize_t test_system::read(int fd, void *buf, size_t count) {
  if (invalidate_read_) {
    if (!invalidate_read_when_called_from_) {
      if (!invalidate_read_after_) {
        invalidate_read_ = false;
        return -1;
      }

      --invalidate_read_after_;

    } else {
      int res;
      std::string call = caller();

      res = call.compare(invalidate_read_when_called_from_);

      if (!invalidate_read_after_ && !res) {
        invalidate_read_ = false;
        invalidate_read_when_called_from_ = nullptr;
        return -1;
      } else if (!res)
        --invalidate_read_after_;
    }
  }
  return ::read(fd, buf, count);
}

FILE *test_system::fopen(const std::string &path, const std::string &mode) {
  std::string syspath = get_sysfs_path(path);
  FILE *fp = ::fopen(syspath.c_str(), mode.c_str());
  if (fp) {
    std::lock_guard<std::mutex> guard(fopens_mutex_);
    fopens_[fp] = Resource<void>("fopen()", caller());
  }
  return fp;
}

int test_system::fclose(FILE *stream) {
  std::lock_guard<std::mutex> guard(fopens_mutex_);
  std::map<FILE *, Resource<void>>::iterator it = fopens_.find(stream);
  if (it != fopens_.end()) {
    fopens_.erase(it);
  }
  return ::fclose(stream);
}

FILE *test_system::popen(const std::string &cmd, const std::string &type) {
  // Is this something we're interested in?
  if (0 == cmd.compare(0, 5, "rdmsr")) {
    char tmpfile[20];
    strcpy(tmpfile, "popen-XXXXXX.tmp");
    ::close(mkstemps(tmpfile, 4));

    FILE *fp = ::fopen(tmpfile, "w+");

    size_t last_spc = cmd.find_last_of(' ');
    std::string msr(cmd.substr(last_spc + 1));

    if (0 == msr.compare("0x35")) {
      fprintf(fp, "0x0000000000180030");
    } else if (0 == msr.compare("0x610")) {
      fprintf(fp, "0x000388d000148758");
    } else if (0 == msr.compare("0x606")) {
      fprintf(fp, "0x00000000000a0e03");
    }

    fseek(fp, 0, SEEK_SET);

    {
      std::lock_guard<std::mutex> guard(popens_mutex_);
      popen_requests_[fp] = Resource<std::string>("popen()", caller(), new std::string(tmpfile));
    }

    return fp;
  } else {
    return ::popen(cmd.c_str(), type.c_str());
  }
}

int test_system::pclose(FILE *stream) {
  std::lock_guard<std::mutex> guard(popens_mutex_);
  // Is this something we intercepted?
  std::map<FILE *, Resource<std::string>>::iterator it = popen_requests_.find(stream);
  if (it != popen_requests_.end()) {
    unlink(it->second.extra_->c_str());
    delete it->second.extra_;
    ::fclose(stream);
    popen_requests_.erase(it);
    return 0;  // process exit status
  }
  return ::pclose(stream);
}

int test_system::ioctl(int fd, unsigned long request, va_list argp) {
  mock_object *mo = nullptr;
  {
    std::lock_guard<std::mutex> guard(fds_mutex_);
    auto mi = fds_.find(fd);
    if (mi != fds_.end()) {
      mo = mi->second.extra_;
    }
  }

  if (mo == nullptr) {
    char *arg = va_arg(argp, char *);
    return ::ioctl(fd, request, arg);
  }

  // replace handler_it->second with mo.
  auto handler_it = ioctl_handlers_.find(request);
  if (handler_it != ioctl_handlers_.end()) {
    return handler_it->second(mo, request, argp);
  }

  return mo->ioctl(request, argp);
}

DIR *test_system::opendir(const std::string &path) {
  std::string syspath = get_sysfs_path(path);
  DIR *dir = ::opendir(syspath.c_str());

  if (dir) {
    std::lock_guard<std::mutex> guard(opendirs_mutex_);
    opendirs_[dir] = Resource<void>("opendir()", caller());
  }

  return dir;
}

int test_system::closedir(DIR *dir) {
  std::lock_guard<std::mutex> guard(opendirs_mutex_);
  std::map<DIR *, Resource<void>>::iterator it = opendirs_.find(dir);
  if (it != opendirs_.end()) {
    opendirs_.erase(it);
  }
  return ::closedir(dir);
}

ssize_t test_system::readlink(const std::string &path, char *buf, size_t bufsize) {
  std::string syspath = get_sysfs_path(path);
  return ::readlink(syspath.c_str(), buf, bufsize);
}

int test_system::stat(const std::string &pathname, struct stat *statbuf) {
  std::string syspath = get_sysfs_path(pathname);
  int res = ::stat(syspath.c_str(), statbuf);

  if (!res && pathname.length() > 5) {
    // If path is rooted at /dev, assume it is a char device.
    std::string p(pathname, 0, 5);
    if (p == std::string("/dev/")) {
            statbuf->st_mode &= ~S_IFMT;
            statbuf->st_mode |= S_IFCHR;
    }
  }

  return res;
}

int test_system::lstat(const std::string &pathname, struct stat *statbuf) {
  std::string syspath = get_sysfs_path(pathname);
  int res = ::lstat(syspath.c_str(), statbuf);

  if (!res && pathname.length() > 5) {
    // If path is rooted at /dev, assume it is a char device.
    std::string p(pathname, 0, 5);
    if (p == std::string("/dev/")) {
            statbuf->st_mode &= ~S_IFMT;
            statbuf->st_mode |= S_IFCHR;
    }
  }

  return res;
}

int test_system::fstatat(int dirfd, const std::string &pathname,
                         struct stat *statbuf, int flags)
{
  std::string syspath = get_sysfs_path(pathname);
  int res = ::fstatat(dirfd, syspath.c_str(), statbuf, flags);

  if (!res && pathname.length() > 5) {
    // If path is rooted at /dev, assume it is a char device.
    std::string p(pathname, 0, 5);
    if (p == std::string("/dev/")) {
            statbuf->st_mode &= ~S_IFMT;
            statbuf->st_mode |= S_IFCHR;
    }
  }

  return res;
}

int test_system::access(const std::string &pathname, int mode) {
  std::string syspath = get_sysfs_path(pathname);
  return ::access(syspath.c_str(), mode);
}

int test_system::scandir(const char *dirp, struct dirent ***namelist,
                         filter_func filter, compare_func cmp) {
  std::string syspath = get_sysfs_path(dirp);
  return ::scandir(syspath.c_str(), namelist, filter, cmp);
}

int test_system::sched_setaffinity(pid_t pid, size_t cpusetsize,
                                   const cpu_set_t *mask) {
  UNUSED_PARAM(pid);
  UNUSED_PARAM(cpusetsize);
  UNUSED_PARAM(mask);
  if (hijack_sched_setaffinity_) {
    if (!hijack_sched_setaffinity_caller_) {
      if (!hijack_sched_setaffinity_after_) {
        hijack_sched_setaffinity_ = false;
        int res = hijack_sched_setaffinity_return_val_;
        hijack_sched_setaffinity_return_val_ = 0;
        return res;
      }

      --hijack_sched_setaffinity_after_;

    } else {
      int res;
      std::string call = caller();

      res = call.compare(hijack_sched_setaffinity_caller_);

      if (!hijack_sched_setaffinity_after_ && !res) {
        hijack_sched_setaffinity_ = false;
        hijack_sched_setaffinity_caller_ = nullptr;
        res = hijack_sched_setaffinity_return_val_;
        hijack_sched_setaffinity_return_val_ = 0;
        return res;
      } else if (!res)
        --hijack_sched_setaffinity_after_;
    }
  }
  return 0;  // return success - we don't actually
             // want to change the affinity.
}

void test_system::hijack_sched_setaffinity(int return_val, uint32_t after,
                                           const char *when_called_from) {
  hijack_sched_setaffinity_ = true;
  hijack_sched_setaffinity_return_val_ = return_val;
  hijack_sched_setaffinity_after_ = after;
  hijack_sched_setaffinity_caller_ = when_called_from;
}

int test_system::glob(const char *pattern, int flags,
                      int (*errfunc)(const char *epath, int eerrno),
                      glob_t *pglob) {
  if (pattern == nullptr) {
    return ::glob(pattern, flags, errfunc, pglob);
  }

  std::string path = get_sysfs_path(pattern);

  int res = ::glob(path.c_str(), flags, errfunc, pglob);
  if (!res) {
    for (unsigned int i = 0; i < pglob->gl_pathc; ++i) {
      std::string tmppath(pglob->gl_pathv[i]);
      if (tmppath.find(get_root()) == 0) {
        auto p = pglob->gl_pathv[i];
        auto root_len = get_root().size();
        auto new_len = tmppath.size() - root_len;
        std::copy(tmppath.begin() + root_len, tmppath.end(), p);
        p[new_len] = '\0';
      }
    }
  }

  std::lock_guard<std::mutex> guard(globs_mutex_);
  globs_[pglob] = Resource<glob_t>("glob()", caller(), new glob_t(*pglob));

  return res;
}

void test_system::globfree(glob_t *pglob) {
  if (pglob->gl_pathv)
    ::globfree(pglob);

  std::lock_guard<std::mutex> guard(globs_mutex_);
  std::map<glob_t *, Resource<glob_t>>::iterator it = globs_.find(pglob);
  if (it != globs_.end()) {
    delete it->second.extra_;
    globs_.erase(it);
  }
}

char *test_system::realpath(const std::string &inpath, char *dst)
{
  if (!initialized_ || root_.empty()) {
    return ::realpath(inpath.c_str(), dst);
  }
  char *retvalue = ::realpath(get_sysfs_path(inpath).c_str(), dst);
  if (retvalue) {
    std::string dst_str(dst);
    char prefix[PATH_MAX] = {0};
    char *prefix_ptr = ::realpath(root_.c_str(), prefix);
    std::string prefix_str(prefix_ptr ? prefix_ptr : "");
    if (prefix_str.size() && dst_str.find(prefix_str) == 0) {
      auto cleaned_str = dst_str.substr(prefix_str.size());
      std::copy(cleaned_str.begin(), cleaned_str.end(), &dst[0]);
      dst[cleaned_str.size()] = '\0';
      retvalue = &dst[0];
    }
  }
  return retvalue;
}

void *test_system::malloc(size_t size) {
  if (invalidate_malloc_) {
    if (!invalidate_malloc_when_called_from_) {
      if (!invalidate_malloc_after_) {
        invalidate_malloc_ = false;
        return nullptr;
      }

      --invalidate_malloc_after_;

    } else {
      int res;
      std::string call = caller();

      res = call.compare(invalidate_malloc_when_called_from_);

      if (!invalidate_malloc_after_ && !res) {
        invalidate_malloc_ = false;
        invalidate_malloc_when_called_from_ = nullptr;
        return nullptr;
      } else if (!res)
        --invalidate_malloc_after_;
    }
  }

  void *p = ::malloc(size);
  if (p) {
    std::lock_guard<std::mutex> guard(mem_allocs_mutex_);
    mem_allocs_[p] = Resource<void>("malloc()", caller());
  }
  return p;
}

void test_system::invalidate_malloc(uint32_t after,
                                    const char *when_called_from) {
  invalidate_malloc_ = true;
  invalidate_malloc_after_ = after;
  invalidate_malloc_when_called_from_ = when_called_from;
}

void *test_system::calloc(size_t nmemb, size_t size) {
  if (invalidate_calloc_) {
    if (!invalidate_calloc_when_called_from_) {
      if (!invalidate_calloc_after_) {
        invalidate_calloc_ = false;
        return nullptr;
      }

      --invalidate_calloc_after_;

    } else {
      int res;
      std::string call = caller();

      res = call.compare(invalidate_calloc_when_called_from_);

      if (!invalidate_calloc_after_ && !res) {
        invalidate_calloc_ = false;
        invalidate_calloc_when_called_from_ = nullptr;
        return nullptr;
      } else if (!res)
        --invalidate_calloc_after_;
    }
  }

  void *p = ::calloc(nmemb, size);
  if (p) {
    std::lock_guard<std::mutex> guard(mem_allocs_mutex_);
    mem_allocs_[p] = Resource<void>("calloc()", caller());
  }
  return p;
}

void test_system::invalidate_calloc(uint32_t after,
                                    const char *when_called_from) {
  invalidate_calloc_ = true;
  invalidate_calloc_after_ = after;
  invalidate_calloc_when_called_from_ = when_called_from;
}

void test_system::free(void *ptr) {
  if (ptr) {
    std::lock_guard<std::mutex> guard(mem_allocs_mutex_);
    std::map<void *, Resource<void>>::iterator it = mem_allocs_.find(ptr);
    if (it != mem_allocs_.end()) {
      mem_allocs_.erase(it);
    }
    ::free(ptr);
  }
}

std::string test_system::caller() const
{
  void *addr = __builtin_return_address(2);
  Dl_info info;
  dladdr(addr, &info);
  return std::string(info.dli_sname ? info.dli_sname : "<unknown>");
}

bool test_system::check_resources()
{
  bool res = true;

  // open() / close()
  {
    std::lock_guard<std::mutex> guard(fds_mutex_);
    for (auto kv : fds_) {
      std::cerr << "*** Leak Detected *** "
                << kv.second.allocator_
                << " created file descriptor "
                << kv.first
                << " at "
                << kv.second.origin_
                << "(), requiring a close()."
                << std::endl;
      res = false;
    }
  }

  // fopen() / fclose()
  {
    std::lock_guard<std::mutex> guard(fopens_mutex_);
    for (auto kv : fopens_) {
      std::cerr << "*** Leak Detected *** "
                << kv.second.allocator_
                << " created FILE * "
                << kv.first
                << " at "
                << kv.second.origin_
                << "(), requiring an fclose()."
                << std::endl;
        res = false;
    }
  }

  // popen() / pclose()
  {
    std::lock_guard<std::mutex> guard(popens_mutex_);
    for (auto kv : popen_requests_) {
      std::cerr << "*** Leak Detected *** "
                << kv.second.allocator_
                << " created FILE * "
                << kv.first
                << " at "
                << kv.second.origin_
                << "(), requiring a pclose()."
                << std::endl;
      res = false;
    }
  }

  // opendir() / closedir()
  {
    std::lock_guard<std::mutex> guard(opendirs_mutex_);
    for (auto kv : opendirs_) {
      std::cerr << "*** Leak Detected *** "
                << kv.second.allocator_
                << " created DIR * "
                << kv.first
                << " at "
                << kv.second.origin_
                << "(), requiring a closedir()."
                << std::endl;
      res = false;
    }
  }

  // glob() / globfree()
  {
    std::lock_guard<std::mutex> guard(globs_mutex_);
    for (auto kv : globs_) {
      std::cerr << "*** Leak Detected *** "
                << kv.second.allocator_
                << " allocated memory in "
                << kv.first
                << " at "
                << kv.second.origin_
                << "(), requiring a globfree()."
                << std::endl;
      res = false;
    }
  }

  // malloc() / calloc() / free()
  {
    std::lock_guard<std::mutex> guard(mem_allocs_mutex_);
    for (auto kv : mem_allocs_) {
      std::cerr << "*** Leak Detected *** "
                << kv.second.allocator_
                << " allocated memory "
                << kv.first
                << " at "
                << kv.second.origin_
                << "(), requiring a free()."
                << std::endl;
        res = false;
    }
  }

  return res;
}

char *test_system::canonicalize_file_name(const std::string &path)
{
  std::string syspath = get_sysfs_path(path);
  char *p = ::canonicalize_file_name(syspath.c_str());

  if (p) {
    std::lock_guard<std::mutex> guard(mem_allocs_mutex_);
    mem_allocs_[p] = Resource<void>("canonicalize_file_name()", caller());
  }

  return p;
}

char *test_system::strdup(const char *s)
{
  if (invalidate_strdup_) {
    if (!invalidate_strdup_when_called_from_) {
      if (!invalidate_strdup_after_) {
        invalidate_strdup_ = false;
        return nullptr;
      }

      --invalidate_strdup_after_;

    } else {
      int res;
      std::string call = caller();

      res = call.compare(invalidate_strdup_when_called_from_);

      if (!invalidate_strdup_after_ && !res) {
        invalidate_strdup_ = false;
        invalidate_strdup_when_called_from_ = nullptr;
        return nullptr;
      } else if (!res)
        --invalidate_strdup_after_;
    }
  }

  char *p = ::strdup(s);
  if (p) {
    std::lock_guard<std::mutex> guard(mem_allocs_mutex_);
    mem_allocs_[p] = Resource<void>("strdup()", caller());
  }
  return p;
}

void test_system::invalidate_strdup(uint32_t after,
                                    const char *when_called_from)
{
  invalidate_strdup_ = true;
  invalidate_strdup_after_ = after;
  invalidate_strdup_when_called_from_ = when_called_from;
}

}  // end of namespace testing
}  // end of namespace opae
