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
#ifndef _TEST_SYSTEM_H
#define _TEST_SYSTEM_H

#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <opae/fpga.h>
#include <stddef.h>
#include <map>
#include <string>
#include <vector>
#include <json-c/json.h>
#include <thread>
#include <mutex>

extern "C" {
extern void *__libc_malloc(size_t size);
extern void *__libc_calloc(size_t nmemb, size_t size);
}
typedef struct stat stat_t;
typedef int (*filter_func)(const struct dirent *);
typedef int (*compare_func)(const struct dirent **, const struct dirent **);

namespace opae {
namespace testing {

constexpr size_t KiB(size_t n) { return n * 1024; }
constexpr size_t MiB(size_t n) { return n * 1024 * KiB(1); }

#ifndef UNUSED_PARAM
#define UNUSED_PARAM(x) ((void)x)
#endif  // UNUSED_PARAM

class mock_object {
 public:
  enum type_t { sysfs_attr = 0, fme, afu };
  mock_object(const std::string &devpath, const std::string &sysclass,
              uint32_t device_id, type_t type = sysfs_attr);
  virtual ~mock_object() {}

  virtual int ioctl(int request, va_list arg) {
    UNUSED_PARAM(request);
    UNUSED_PARAM(arg);
    throw std::logic_error("not implemented");
    return 0;
  }

  std::string sysclass() const { return sysclass_; }
  uint32_t device_id() const { return device_id_; }
  type_t type() const { return type_; }

 private:
  std::string devpath_;
  std::string sysclass_;
  uint32_t device_id_;
  type_t type_;
};

class mock_fme : public mock_object {
 public:
  mock_fme(const std::string &devpath, const std::string &sysclass,
           uint32_t device_id)
      : mock_object(devpath, sysclass, device_id, fme) {}
  virtual int ioctl(int request, va_list argp) override;
};

class mock_port : public mock_object {
 public:
  mock_port(const std::string &devpath, const std::string &sysclass,
            uint32_t device_id)
      : mock_object(devpath, sysclass, device_id, fme) {}
  virtual int ioctl(int request, va_list argp) override;
};

struct test_device {
  const char *fme_guid;
  const char *afu_guid;
  uint16_t segment;
  uint8_t bus;
  uint8_t device;
  uint8_t function;
  uint8_t socket_id;
  uint32_t num_slots;
  uint64_t bbs_id;
  fpga_version bbs_version;
  fpga_accelerator_state state;
  uint32_t num_mmio;
  uint32_t num_interrupts;
  uint64_t fme_object_id;
  uint64_t port_object_id;
  uint16_t vendor_id;
  uint32_t device_id;
  uint32_t fme_num_errors;
  uint32_t port_num_errors;
  const char *gbs_guid;
  const char *mdata;
  static test_device unknown();
};

struct test_platform {
  const char *mock_sysfs;
  std::vector<test_device> devices;
  static test_platform get(const std::string &key);
  static bool exists(const std::string &key);
  static std::vector<std::string> keys(bool sorted = false);
};

template <int _R, long _E>
static int dummy_ioctl(mock_object *, int, va_list) {
  errno = _E;
  return _R;
}

class test_system {
 public:
  typedef int (*ioctl_handler_t)(mock_object *, int, va_list);
  static test_system *instance();

  void set_root(const char *root);
  std::string get_root();
  std::string get_sysfs_path(const std::string &src);
  std::vector<uint8_t> assemble_gbs_header(const test_device &td);
  std::vector<uint8_t> assemble_gbs_header(const test_device &td, const char* mdata);

  void initialize();
  void finalize();
  void prepare_syfs(const test_platform &platform);
  void remove_sysfs();

  int open(const std::string &path, int flags);
  int open(const std::string &path, int flags, mode_t m);
  void invalidate_read(uint32_t after=0, const char *when_called_from=nullptr);
  ssize_t read(int fd, void *buf, size_t count);

  FILE * fopen(const std::string &path, const std::string &mode);

  int close(int fd);
  int ioctl(int fd, unsigned long request, va_list argp);

  DIR *opendir(const char *name);
  ssize_t readlink(const char *path, char *buf, size_t bufsize);
  int xstat(int ver, const char *path, stat_t *buf);
  int lstat(int ver, const char *path, stat_t *buf);
  int scandir(const char *dirp, struct dirent ***namelist, filter_func filter,
              compare_func cmp);
  void invalidate_malloc(uint32_t after=0, const char *when_called_from=nullptr);
  void invalidate_calloc(uint32_t after=0, const char *when_called_from=nullptr);

  bool default_ioctl_handler(int request, ioctl_handler_t);
  bool register_ioctl_handler(int request, ioctl_handler_t);

  FILE *register_file(const std::string &path);

 private:
  test_system();
  std::mutex fds_mutex_;
  std::string root_;
  std::map<int, mock_object *> fds_;
  std::map<int, ioctl_handler_t> default_ioctl_handlers_;
  std::map<int, ioctl_handler_t> ioctl_handlers_;
  std::map<std::string, std::string> registered_files_;
  static test_system *instance_;

  typedef int (*open_func)(const char *pathname, int flags);
  typedef int (*open_create_func)(const char *pathname, int flags, mode_t mode);
  typedef ssize_t (*read_func)(int fd, void *buf, size_t count);
  typedef FILE * (*fopen_func)(const char *path, const char *mode);
  typedef int (*close_func)(int fd);
  typedef int (*ioctl_func)(int fd, unsigned long request, char *argp);
  typedef DIR *(*opendir_func)(const char *name);
  typedef ssize_t (*readlink_func)(const char *pathname, char *buf,
                                   size_t bufsiz);
  typedef int (*__xstat_func)(int ver, const char *pathname, struct stat *buf);
  typedef int (*scandir_func)(const char *, struct dirent ***, filter_func,
                              compare_func);

  open_func open_;
  open_create_func open_create_;
  read_func read_;
  fopen_func fopen_;
  close_func close_;
  ioctl_func ioctl_;
  opendir_func opendir_;
  readlink_func readlink_;
  __xstat_func xstat_;
  __xstat_func lstat_;
  scandir_func scandir_;
};

}  // end of namespace testing
}  // end of namespace opae

#endif /* !_TEST_SYSTEM_H */
