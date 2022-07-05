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
#ifndef _TEST_SYSTEM_H
#define _TEST_SYSTEM_H

#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <opae/fpga.h>
#include <stddef.h>
#include <sched.h>
#include <map>
#include <string>
#include <vector>
#include <json-c/json.h>
#include <thread>
#include <mutex>
#include <atomic>
#include "platform/fpga_hw.h"
#include <glob.h>

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

struct fpga_device_id
{
  fpga_device_id(uint16_t v, uint16_t d, uint16_t sv, uint16_t sd) :
    vendor(v),
    device(d),
    subsystem_vendor(sv),
    subsystem_device(sd)
  {}

  bool is_n6000_sku1() const
  {
    return vendor == 0x8086 && device == 0xbcce &&
           subsystem_vendor == 0x8086 &&
           subsystem_device == 0x1771;
  }

  uint16_t vendor;
  uint16_t device;
  uint16_t subsystem_vendor;
  uint16_t subsystem_device;
};

class mock_object {
 public:
  enum type_t { sysfs_attr = 0, fme, afu };
  mock_object(const std::string &devpath, const std::string &sysclass,
              fpga_device_id device_id, type_t type = sysfs_attr);
  virtual ~mock_object() {}

  virtual int ioctl(int request, va_list arg) {
    UNUSED_PARAM(request);
    UNUSED_PARAM(arg);
    throw std::logic_error("not implemented");
    return 0;
  }

  std::string sysclass() const { return sysclass_; }
  fpga_device_id device_id() const { return device_id_; }
  type_t type() const { return type_; }

 private:
  std::string devpath_;
  std::string sysclass_;
  fpga_device_id device_id_;
  type_t type_;
};

class mock_fme : public mock_object {
 public:
  mock_fme(const std::string &devpath, const std::string &sysclass,
           fpga_device_id device_id)
      : mock_object(devpath, sysclass, device_id, fme) {}
  virtual int ioctl(int request, va_list argp) override;
};

class mock_port : public mock_object {
 public:
  mock_port(const std::string &devpath, const std::string &sysclass,
            fpga_device_id device_id)
      : mock_object(devpath, sysclass, device_id, fme) {}
  virtual int ioctl(int request, va_list argp) override;
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
  int remove_sysfs();
  int remove_sysfs_dir(const char *path = nullptr);
  std::string get_sysfs_claass_path(const std::string &path);

  int open(const std::string &path, int flags);
  int open(const std::string &path, int flags, mode_t m);
  int close(int fd);

  void invalidate_read(uint32_t after=0, const char *when_called_from=nullptr);
  ssize_t read(int fd, void *buf, size_t count);

  FILE * fopen(const std::string &path, const std::string &mode);
  int fclose(FILE *stream);

  FILE * popen(const std::string &cmd, const std::string &type);
  int pclose(FILE *stream);

  int ioctl(int fd, unsigned long request, va_list argp);

  DIR *opendir(const std::string &path);
  int closedir(DIR *dir);

  ssize_t readlink(const std::string &path, char *buf, size_t bufsize);

  int stat(const std::string &pathname, struct stat *statbuf);
  int lstat(const std::string &pathname, struct stat *statbuf);
  int fstatat(int dirfd, const std::string &pathname, struct stat *statbuf, int flags);
  int access(const std::string &pathname, int mode);
  int scandir(const char *dirp, struct dirent ***namelist, filter_func filter,
              compare_func cmp);

  int sched_setaffinity(pid_t pid, size_t cpusetsize,
                        const cpu_set_t *mask);
  void hijack_sched_setaffinity(int return_val, uint32_t after=0,
                                const char *when_called_from=nullptr);
                        
  int glob(const char *pattern, int flags,
           int (*errfunc) (const char *epath, int eerrno),
           glob_t *pglob);
  void globfree(glob_t *pglob);

  char *realpath(const std::string &inpath, char *dst);
                
  void *malloc(size_t size);
  void invalidate_malloc(uint32_t after=0, const char *when_called_from=nullptr);

  void *calloc(size_t nmemb, size_t size);
  void invalidate_calloc(uint32_t after=0, const char *when_called_from=nullptr);

  void free(void *ptr);

  char *canonicalize_file_name(const std::string &path);
  char *strdup(const char *s);
  void invalidate_strdup(uint32_t after=0, const char *when_called_from=nullptr);

  bool default_ioctl_handler(int request, ioctl_handler_t);
  bool register_ioctl_handler(int request, ioctl_handler_t);

  FILE *register_file(const std::string &path);

  void normalize_guid(std::string &guid_str, bool with_hyphens = true);

  std::string caller() const;
  bool check_resources();

  template <typename E>
  class Resource
  {
   public:
    Resource() :
      allocator_(""),
      origin_(""),
      extra_(nullptr)
    {}

    Resource(const char *allocator, const std::string &origin, E *extra=nullptr):
      allocator_(allocator),
      origin_(origin),
      extra_(extra)
    {}

    Resource(const Resource &other) :
      allocator_(other.allocator_),
      origin_(other.origin_),
      extra_(other.extra_)
    {}

    Resource & operator=(const Resource &other)
    {
      if (&other != this) {
        allocator_ = other.allocator_;
        origin_ = other.origin_;
        extra_ = other.extra_;
      }
      return *this;
    }

    std::string allocator_; // "open", "malloc", etc.
    std::string origin_;    // calling function
    E *extra_;              // extra data associated with the allocation
  };

 private:
  test_system();
  std::mutex fds_mutex_;
  std::atomic_bool initialized_;
  std::string root_;
  std::map<int, Resource<mock_object>> fds_;
  std::mutex fopens_mutex_;
  std::map<FILE *, Resource<void>> fopens_;
  std::mutex mem_allocs_mutex_;
  std::map<void *, Resource<void>> mem_allocs_;
  std::mutex popens_mutex_;
  std::map<FILE *, Resource<std::string>> popen_requests_;
  std::mutex opendirs_mutex_;
  std::map<DIR *, Resource<void>> opendirs_;
  std::mutex globs_mutex_;
  std::map<glob_t *, Resource<glob_t>> globs_;

  std::map<int, ioctl_handler_t> default_ioctl_handlers_;
  std::map<int, ioctl_handler_t> ioctl_handlers_;
  std::map<std::string, std::string> registered_files_;
  static test_system *instance_;

  bool invalidate_malloc_;
  uint32_t invalidate_malloc_after_;
  const char *invalidate_malloc_when_called_from_;

  bool invalidate_calloc_;
  uint32_t invalidate_calloc_after_;
  const char *invalidate_calloc_when_called_from_;

  bool invalidate_read_;
  uint32_t invalidate_read_after_;
  const char *invalidate_read_when_called_from_;

  bool hijack_sched_setaffinity_;
  int hijack_sched_setaffinity_return_val_;
  uint32_t hijack_sched_setaffinity_after_;
  const char *hijack_sched_setaffinity_caller_;

  bool invalidate_strdup_;
  uint32_t invalidate_strdup_after_;
  const char *invalidate_strdup_when_called_from_;
};

}  // end of namespace testing
}  // end of namespace opae

#endif /* !_TEST_SYSTEM_H */
