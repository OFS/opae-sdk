// Copyright(c) 2023, Intel Corporation
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

#pragma once

#include <future>
#include <glob.h>
#include <inttypes.h>
#include <numa.h>
#include <opae/cxx/core.h>
#include <regex.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <opae/cxx/core.h>

#include "dfl-he-cache.h"

using namespace std;

const char *sbdf_pattern =
    "(([0-9a-fA-F]{4}):)?([0-9a-fA-F]{2}):([0-9a-fA-F]{2})\\.([0-9])";

enum { MATCHES_SIZE = 6 };
#define FEATURE_DEV                                                            \
  "/sys/bus/pci/devices/%s/"                                                   \
  "fpga_region/region*/dfl-fme*/dfl_dev*/feature_id"

#define MAX_SIZE 256

#define PROTECTION (PROT_READ | PROT_WRITE)

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000
#endif
#ifndef MAP_HUGE_SHIFT
#define MAP_HUGE_SHIFT 26
#endif

#define MAP_2M_HUGEPAGE (0x15 << MAP_HUGE_SHIFT) /* 2 ^ 0x15 = 2M */
#define MAP_1G_HUGEPAGE (0x1e << MAP_HUGE_SHIFT) /* 2 ^ 0x1e = 1G */

#ifdef __ia64__
#define ADDR ((void *)(0x8000000000000000UL))
#define FLAGS_4K (MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED)
#define FLAGS_2M (FLAGS_4K | MAP_2M_HUGEPAGE | MAP_HUGETLB)
#define FLAGS_1G (FLAGS_4K | MAP_1G_HUGEPAGE | MAP_HUGETLB)
#else
#define ADDR ((void *)(0x0UL))
#define FLAGS_4K (MAP_PRIVATE | MAP_ANONYMOUS)
#define FLAGS_2M (FLAGS_4K | MAP_2M_HUGEPAGE | MAP_HUGETLB)
#define FLAGS_1G (FLAGS_4K | MAP_1G_HUGEPAGE | MAP_HUGETLB)
#endif

#define KiB(x) ((x)*1024)
#define MiB(x) ((x)*1024 * 1024)
#define GiB(x) ((x)*1024 * 1024 * 1024)

#define DFL_HE_CACHE_DSM_BASE 0x030
#define DFL_HE_CACHE_WR_ADDR_TABLE_DATA 0x068
#define DFL_HE_CACHE_RD_ADDR_TABLE_DATA 0x088

void *alloc_2mb_hugepage(void) {
  void *addr;

  addr = mmap(ADDR, MiB(2), PROTECTION, FLAGS_2M, 0, 0);
  if (addr == MAP_FAILED) {
    printf("alloc_2mb_hugepage() failed: %s\n", strerror(errno));
    addr = NULL;
  }

  return addr;
}
void free_memory(void *addr, uint64_t len) { munmap(addr, len); }

void *alloc_32kb_hugepage(void) {
  void *addr;

  addr = mmap(ADDR, KiB(32), PROTECTION, FLAGS_4K, 0, 0);
  if (addr == MAP_FAILED) {
    printf("alloc_1kb_hugepage() failed: %s\n", strerror(errno));
    addr = NULL;
  }

  return addr;
}

void *alloc_4kb_hugepage(void) {
  void *addr;

  addr = mmap(ADDR, KiB(4), PROTECTION, FLAGS_4K, 0, 0);
  if (addr == MAP_FAILED) {
    printf("alloc_1kb_hugepage() failed: %s\n", strerror(errno));
    addr = NULL;
  }

  return addr;
}

bool sysfs_read_u64(const char *path, uint64_t *value) {
  ifstream fs;
  fs.open(path, ios::in);

  std::string s;
  if (fs.is_open()) {
    std::string line;
    std::getline(fs, line);
    *value = std::stoul(line, 0, 16);
    fs.close();
    return true;
  }
  return false;
}

namespace opae {
namespace afu_test {


template <typename T>
inline bool parse_match_int(const char *s, regmatch_t m, T &v, int radix = 10) {
  if (m.rm_so == -1 || m.rm_eo == -1)
    return false;
  errno = 0;
  v = std::strtoul(s + m.rm_so, NULL, radix);
  return errno == 0;
}

union pcie_address {
  struct {
    uint32_t function : 3;
    uint32_t device : 5;
    uint32_t bus : 8;
    uint32_t domain : 16;
  } fields;
  uint32_t value;

  static pcie_address parse(const char *s) {
    auto deleter = [&](regex_t *r) {
      regfree(r);
      delete r;
    };
    std::unique_ptr<regex_t, decltype(deleter)> re(new regex_t, deleter);
    regmatch_t matches[MATCHES_SIZE];

    int reg_res = regcomp(re.get(), sbdf_pattern, REG_EXTENDED | REG_ICASE);
    if (reg_res)
      throw std::runtime_error("could not compile regex");

    reg_res = regexec(re.get(), s, MATCHES_SIZE, matches, 0);
    if (reg_res)
      throw std::runtime_error("pcie address not valid format");

    uint16_t domain, bus, device, function;
    if (!parse_match_int(s, matches[2], domain, 16))
      domain = 0;
    if (!parse_match_int(s, matches[3], bus, 16))
      throw std::runtime_error("error parsing pcie address");
    if (!parse_match_int(s, matches[4], device, 16))
      throw std::runtime_error("error parsing pcie address");
    if (!parse_match_int(s, matches[5], function))
      throw std::runtime_error("error parsing; pcie address");
    pcie_address a;
    a.fields.domain = domain;
    a.fields.bus = bus;
    a.fields.device = device;
    a.fields.function = function;
    return a;
  }
};

class afu; // forward declaration

class command {
public:
  typedef std::shared_ptr<command> ptr_t;
  command() : running_(true) {}
  virtual ~command() {}
  virtual const char *name() const = 0;
  virtual const char *description() const = 0;
  virtual int run(afu *afu, CLI::App *app) = 0;
  virtual void add_options(CLI::App *app) { (void)app; }
  virtual const char *afu_id() const { return nullptr; }

  virtual uint64_t featureid() const = 0;
  virtual uint64_t guidl() const = 0;
  virtual uint64_t guidh() const = 0;

  bool running() const { return running_; }
  void stop() { running_ = false; }

private:
  std::atomic<bool> running_;
};

#if SPDLOG_VERSION >= 10900
// spdlog version 1.9.0 defines SPDLOG_LEVEL_NAMES as an array of string_view_t.
// Convert to vector of std::string to be used in CLI::IsMember().
inline std::vector<std::string> spdlog_levels() {
  std::vector<spdlog::string_view_t> levels_view = SPDLOG_LEVEL_NAMES;
  std::vector<std::string> levels_str(levels_view.size());
  std::transform(levels_view.begin(), levels_view.end(), levels_str.begin(),
                 [](spdlog::string_view_t sv) {
                   return std::string(sv.data(), sv.size());
                 });
  return levels_str;
}
#else
inline std::vector<std::string> spdlog_levels() { return SPDLOG_LEVEL_NAMES; }
#endif // SPDLOG_VERSION

class afu {
public:
  typedef int (*command_fn)(afu *afu, CLI::App *app);
  enum exit_codes {
    success = 0,
    not_run,
    not_found,
    no_access,
    exception,
    error
  };

  afu(const char *name, const char *afu_id = nullptr,
      const char *log_level = nullptr)
      : name_(name), afu_id_(afu_id ? afu_id : ""), app_(name_), pci_addr_(""),
        log_level_(log_level ? log_level : "info"), timeout_msec_(60000),
        current_command_(nullptr) {
    if (!afu_id_.empty())
      app_.add_option("-g,--guid", afu_id_, "GUID")->default_str(afu_id_);
    app_.add_option("-p,--pci-address", pci_addr_,
                    "[<domain>:]<bus>:<device>.<function>");
    app_.add_option("-l,--log-level", log_level_, "stdout logging level")
        ->default_str(log_level_)
        ->check(CLI::IsMember(spdlog_levels()));
    app_.add_option("-t,--timeout", timeout_msec_, "test timeout (msec)")
        ->default_str(std::to_string(timeout_msec_));
  }
  virtual ~afu() {
    if (logger_)
      spdlog::drop(logger_->name());
  }

  CLI::App &cli() { return app_; }

  int find_dev_feature() {
    glob_t pglob;
    char feature_path[MAX_SIZE] = {0};
    int gres = 0;
    uint64_t value = 0;
    size_t i = 0;

    if (!pci_addr_.empty()) {
      if (snprintf(feature_path, sizeof(feature_path), FEATURE_DEV,
                   pci_addr_.c_str()) < 0) {
        cerr << "snprintf buffer overflow" << endl;
        return 1;
      }
    } else {
      if (snprintf(feature_path, sizeof(feature_path), FEATURE_DEV, "*:*:*.*") <
          0) {
        cerr << "snprintf buffer overflow" << endl;
        return 2;
      }
    }

    gres = glob(feature_path, GLOB_NOSORT, NULL, &pglob);
    if (gres) {
      cerr << "Failed pattern match" << feature_path << ":" << strerror(errno)
           << endl;
      globfree(&pglob);
      return 3;
    }

    for (i = 0; i < pglob.gl_pathc; i++) {
      bool retval = sysfs_read_u64(pglob.gl_pathv[i], &value);
      if (!retval) {
        cerr << "Failed to read sysfs value" << endl;
        continue;
      }

      if (current_command()->featureid() == value) {
        string str(pglob.gl_pathv[i]);
        string substr_dev(str.substr(0, str.rfind("/")));
        globfree(&pglob);

        substr_dev.append("/he-cache/he-cache*");
        gres = glob(substr_dev.c_str(), GLOB_NOSORT, NULL, &pglob);
        if (gres) {
          cerr << "Failed pattern match" << substr_dev.c_str() << ":"
               << strerror(errno) << endl;
          globfree(&pglob);
          return 4;
        }
        string str1(pglob.gl_pathv[0]);
        globfree(&pglob);
        dev_path_.append("/dev");
        dev_path_.append(str1.substr(str1.rfind("/"), 13));

        return 0;
      }
    }

    return 5;
  }

  void unmap_mmio() {
    if (mmio_base_) {
      if (munmap(mmio_base_, rinfo_.size) == -1)
        cerr << "Failed to unmap MMIO:" << strerror(errno) << endl;
    }
  }

  bool map_mmio() {
    void *user_v;
    user_v = mmap(NULL, rinfo_.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_,
                  rinfo_.offset);
    if (user_v == MAP_FAILED) {
      cerr << "Failed to map MMIO:" << strerror(errno) << endl;
      return false;
    }
    mmio_base_ = (uint8_t *)user_v;

    return true;
  }

  int open_handle() {

    int res = 0;
    cout << "dev_path_:" << dev_path_ << endl;

    fd_ = open(dev_path_.c_str(), O_RDWR);
    if (fd_ < 0) {
      cerr << "open() failed:" << strerror(errno) << endl;
      return 1;
    }

    memset(&rinfo_, 0, sizeof(rinfo_));
    rinfo_.argsz = sizeof(rinfo_);
    res = ioctl(fd_, DFL_HE_CACHE_GET_REGION_INFO, &rinfo_);
    if (res) {
      cerr << "ioctl() DFL_HE_CACHE_GET_REGION_INFO failed:" << strerror(errno)
           << endl;
      close(fd_);
      return 2;
    }

    printf("MMIO region flags: 0x%x size: %llu offset: %llu\n", rinfo_.flags,
           rinfo_.size, rinfo_.offset);

    if (!map_mmio()) {
      cerr << "mmap failed:" << strerror(errno) << endl;
      close(fd_);
      return 3;
    }

    volatile uint64_t *u64 = (volatile uint64_t *)mmio_base_;
    printf("DFH     : 0x%016" PRIx64 "\n", *u64);
    printf("DFH + 8 : 0x%016" PRIx64 "\n", *(u64 + 1));
    printf("DFH + 16: 0x%016" PRIx64 "\n", *(u64 + 2));
    printf("DFH + 24: 0x%016" PRIx64 "\n", *(u64 + 3));

    return exit_codes::not_run;
  }

  int main(int argc, char *argv[]) {
    if (!commands_.empty())
      app_.require_subcommand();
    CLI11_PARSE(app_, argc, argv);

    command::ptr_t test(nullptr);
    CLI::App *app = nullptr;
    for (auto kv : commands_) {
      if (*kv.first) {
        app = kv.first;
        test = kv.second;
        break;
      }
    }
    if (!test) {
      std::cerr << "no command specified\n";
      return exit_codes::not_run;
    }

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    logger_ = std::make_shared<spdlog::logger>(test->name(), console_sink);
    spdlog::register_logger(logger_);
    logger_->set_level(spdlog::level::from_str(log_level_));
    current_command_ = test;
    if (find_dev_feature() != 0) {
      cerr << "fails to find feature" << endl;
      return exit_codes::exception;
    };

    int res = open_handle();
    if (res != exit_codes::not_run) {
      return res;
    }

    return run(app, test);
  }

  virtual int run(CLI::App *app, command::ptr_t test) {
    int res = exit_codes::not_run;
    current_command_ = test;

    try {
      std::future<int> f = std::async(std::launch::async, [this, test, app]() {
        return test->run(this, app);
      });
      auto status = f.wait_for(std::chrono::milliseconds(timeout_msec_));
      if (status == std::future_status::timeout) {
        std::cerr << "Error: test timed out" << std::endl;
        current_command_->stop();
        throw std::runtime_error("timeout");
      }
      res = f.get();
    } catch (std::exception &ex) {
      res = exit_codes::exception;
    }

    current_command_.reset();
    return res;
  }

  template <class T> CLI::App *register_command() {
    command::ptr_t cmd(new T());
    auto sub = app_.add_subcommand(cmd->name(), cmd->description());
    cmd->add_options(sub);
    commands_[sub] = cmd;
    return sub;
  }

  uint64_t read64(uint32_t offset) {
    uint64_t value = *((uint64_t *)(mmio_base_ + offset));
    return value;
  }

  void write64(uint32_t offset, uint64_t value) {
    *((uint64_t *)(mmio_base_ + offset)) = value;
    return;
  }

  uint32_t read32(uint32_t offset) {
    uint32_t value = *((uint64_t *)(mmio_base_ + offset));
    return value;
  }

  void write32(uint32_t offset, uint32_t value) {
    *((uint32_t *)(mmio_base_ + offset)) = value;
    return;
  }

  command::ptr_t current_command() const { return current_command_; }

  bool open_device() {

    // std::cerr << "open\n" << dev_str;
    fd_ = open(dev_path_.c_str(), O_RDWR);
    if (fd_ < 0) {
      printf("open() failed: %s\n", strerror(errno));
      return false;
    }

    return true;
  }

  bool close_device() {
    if (fd_ > 0)
      close(fd_);
    return true;
  }

  bool allocate_dsm(size_t len = KiB(4), uint32_t node = 0) {
    int res = 0;
    void *ptr = NULL;
    struct dfl_he_cache_dma_map dma_map;
    // cout << "allocate_dsm\n";

    memset(&dma_map, 0, sizeof(dma_map));

    ptr = alloc_4kb_hugepage();
    if (!ptr) {
      cerr << "failed to allocate 4k huge page:" << strerror(errno) << endl;
      return false;
    }

    dma_map.argsz = sizeof(dma_map);
    dma_map.user_addr = (__u64)ptr;
    dma_map.length = len;
    dma_map.numa_node = node;
    dma_map.csr_array[0] = DFL_HE_CACHE_DSM_BASE; // 0x030

    volatile uint64_t *u64 =
        (volatile uint64_t *)(mmio_base_ + DFL_HE_CACHE_DSM_BASE);

    res = ioctl(fd_, DFL_HE_CACHE_NUMA_DMA_MAP, &dma_map);
    if (res) {
      cerr << "ioctl DFL_HE_CACHE_NUMA_NODE_DSM_INFO failed" << strerror(errno)
           << endl;
      goto out_free;
    }
    printf("DSM_BASE: 0x%016" PRIx64 "\n", *u64);

    dsm_buffer_ = (uint8_t *)ptr;
    dsm_buf_len_ = len;
    return true;

  out_free:
    free_memory(ptr, len);
    return false;
  }

  bool free_dsm() {
    struct dfl_he_cache_dma_unmap dma_unmap;
    int res = 0;

    // cout << "free_dsm\n" << endl;
    memset(&dma_unmap, 0, sizeof(dma_unmap));

    dma_unmap.argsz = sizeof(dma_unmap);
    dma_unmap.user_addr = (__u64)dsm_buffer_;
    dma_unmap.length = dsm_buf_len_;
    dma_unmap.csr_array[0] = DFL_HE_CACHE_DSM_BASE; // 0x030

    volatile uint64_t *u64 =
        (volatile uint64_t *)(mmio_base_ + DFL_HE_CACHE_DSM_BASE);

    res = ioctl(fd_, DFL_HE_CACHE_NUMA_DMA_UNMAP, &dma_unmap);
    if (res) {
      cerr << "ioctl DFL_HE_CACHE_NUMA_DMA_UNMAP failed" << strerror(errno)
           << endl;
    }
    printf("DSM_BASE: 0x%016" PRIx64 "\n", *u64);
    free_memory(dsm_buffer_, dsm_buf_len_);

    return true;
  }

  bool allocate_cache_read(size_t len = MiB(2), uint32_t numa_node = 0) {

    int res = 0;
    void *ptr = NULL;
    struct dfl_he_cache_dma_map dma_map;

    // cout << "allocate_cache_read\n";

    memset(&dma_map, 0, sizeof(dma_map));

    ptr = alloc_2mb_hugepage();
    if (!ptr) {
      cerr << "failed to allocate huge pages\n" << endl;
      return false;
    }

    cout << "numa_node: " << numa_node << endl;

    dma_map.argsz = sizeof(dma_map);
    dma_map.user_addr = (__u64)ptr;
    dma_map.length = len;
    dma_map.numa_node = numa_node;
    dma_map.csr_array[0] = DFL_HE_CACHE_RD_ADDR_TABLE_DATA; // 0x88

    volatile uint64_t *u64 =
        (volatile uint64_t *)(mmio_base_ + DFL_HE_CACHE_RD_ADDR_TABLE_DATA);

    res = ioctl(fd_, DFL_HE_CACHE_NUMA_DMA_MAP, &dma_map);
    if (res) {
      cerr << "ioctl DFL_HE_CACHE_NUMA_DMA_MAP failed" << strerror(errno)
           << endl;
      goto out_free;
    }
    printf("DFL_HE_CACHE_RD_ADDR_TABLE_DATA: 0x%016" PRIx64 "\n", *u64);

    rd_buffer_ = (uint8_t *)ptr;
    rd_buf_len_ = len;
    return true;

  out_free:
    free_memory(ptr, len);
    return false;
  }

  bool free_cache_read() {
    struct dfl_he_cache_dma_unmap dma_unmap;
    int res = 0;

    memset(&dma_unmap, 0, sizeof(dma_unmap));

    dma_unmap.argsz = sizeof(dma_unmap);
    dma_unmap.user_addr = (__u64)rd_buffer_;
    dma_unmap.length = rd_buf_len_;
    dma_unmap.csr_array[0] = DFL_HE_CACHE_RD_ADDR_TABLE_DATA; // 0x88

    volatile uint64_t *u64 =
        (volatile uint64_t *)(mmio_base_ + DFL_HE_CACHE_RD_ADDR_TABLE_DATA);
    res = ioctl(fd_, DFL_HE_CACHE_NUMA_DMA_UNMAP, &dma_unmap);
    if (res) {
      cerr << "ioctl DFL_HE_CACHE_NUMA_DMA_UNMAP failed" << strerror(errno)
           << endl;
    }

    printf("DFL_HE_CACHE_RD_ADDR_TABLE_DATA: 0x%016" PRIx64 "\n", *u64);
    free_memory(rd_buffer_, rd_buf_len_);

    return true;
  }

  bool allocate_cache_write(size_t len = MiB(2), uint32_t numa_node = 0) {
    int res;
    void *ptr;
    struct dfl_he_cache_dma_map dma_map;

    // std::cout << "allocate_cache_write" << endl;

    memset(&dma_map, 0, sizeof(dma_map));

    ptr = alloc_2mb_hugepage();
    if (!ptr) {
      cerr << "failed to allocate huge pages\n" << endl;
      return false;
    }

    dma_map.argsz = sizeof(dma_map);
    dma_map.user_addr = (__u64)ptr;
    dma_map.length = len;
    dma_map.numa_node = numa_node;
    dma_map.csr_array[0] = DFL_HE_CACHE_WR_ADDR_TABLE_DATA; // 0x68;

    volatile uint64_t *u64 =
        (volatile uint64_t *)(mmio_base_ + DFL_HE_CACHE_WR_ADDR_TABLE_DATA);

    res = ioctl(fd_, DFL_HE_CACHE_NUMA_DMA_MAP, &dma_map);
    if (res) {
      cerr << "ioctl DFL_HE_CACHE_NUMA_DMA_MAP failed" << strerror(errno)
           << endl;
      goto out_free;
    }
    printf("\nDFL_HE_CACHE_WR_ADDR_TABLE_DATA: 0x%016" PRIx64 "\n", *u64);

    wr_buffer_ = (uint8_t *)ptr;

    return true;

  out_free:
    free_memory(ptr, len);
    return false;
  }

  bool free_cache_write() {
    struct dfl_he_cache_dma_unmap dma_unmap;
    int res;

    // cout << "free_cache_write" << endl;
    memset(&dma_unmap, 0, sizeof(dma_unmap));

    dma_unmap.argsz = sizeof(dma_unmap);
    dma_unmap.user_addr = (__u64)wr_buffer_;
    dma_unmap.length = wr_buf_len_;
    dma_unmap.csr_array[0] = DFL_HE_CACHE_WR_ADDR_TABLE_DATA; // 0x68;

    volatile uint64_t *u64 =
        (volatile uint64_t *)(mmio_base_ + DFL_HE_CACHE_WR_ADDR_TABLE_DATA);
    res = ioctl(fd_, DFL_HE_CACHE_NUMA_DMA_UNMAP, &dma_unmap);
    if (res) {
      cerr << "ioctl DFL_HE_CACHE_NUMA_DMA_UNMAP failed" << strerror(errno)
           << endl;
    }

    printf("\nDFL_HE_CACHE_WR_ADDR_TABLE_DATA: 0x%016" PRIx64 "\n", *u64);
    free_memory(wr_buffer_, wr_buf_len_);

    return true;
  }

  bool allocate_cache_read_write(size_t len = MiB(2), uint32_t numa_node = 0) {

    int res = 0;
    void *ptr = NULL;
    struct dfl_he_cache_dma_map dma_map;

    // cout<< "allocate_cache_read_write";

    memset(&dma_map, 0, sizeof(dma_map));
    ptr = alloc_2mb_hugepage();
    if (!ptr) {
      cerr << "failed to allocate huge pages\n" << endl;
      return false;
    }

    dma_map.argsz = sizeof(dma_map);
    dma_map.user_addr = (__u64)ptr;
    dma_map.length = len;
    dma_map.numa_node = numa_node;
    dma_map.csr_array[0] = DFL_HE_CACHE_RD_ADDR_TABLE_DATA; // 0x88;
    dma_map.csr_array[1] = DFL_HE_CACHE_WR_ADDR_TABLE_DATA; // 0x68;

    volatile uint64_t *u64_wr =
        (volatile uint64_t *)(mmio_base_ + DFL_HE_CACHE_WR_ADDR_TABLE_DATA);
    volatile uint64_t *u64_rd =
        (volatile uint64_t *)(mmio_base_ + DFL_HE_CACHE_RD_ADDR_TABLE_DATA);

    res = ioctl(fd_, DFL_HE_CACHE_NUMA_DMA_MAP, &dma_map);
    if (res) {
      cerr << "ioctl DFL_HE_CACHE_NUMA_DMA_MAP failed" << strerror(errno)
           << endl;
      goto out_free;
    }

    printf("\nDFL_HE_CACHE_WR_ADDR_TABLE_DATA: 0x%016" PRIx64 "\n", *u64_wr);
    printf("\nDFL_HE_CACHE_RD_ADDR_TABLE_DATAs: 0x%016" PRIx64 "\n", *u64_rd);

    rd_wr_buffer_ = (uint8_t *)ptr;
    rd_wr_buf_len_ = len;

    return true;

  out_free:
    free_memory(ptr, len);
    return false;
  }

  bool free_cache_read_write() {
    struct dfl_he_cache_dma_unmap dma_unmap;
    int res;

    // cout << "free_cache_read_write\n" << endl;

    memset(&dma_unmap, 0, sizeof(dma_unmap));

    dma_unmap.argsz = sizeof(dma_unmap);
    dma_unmap.user_addr = (__u64)rd_wr_buffer_;
    dma_unmap.length = rd_wr_buf_len_;
    dma_unmap.csr_array[0] = DFL_HE_CACHE_RD_ADDR_TABLE_DATA; // 0x88;
    dma_unmap.csr_array[1] = DFL_HE_CACHE_WR_ADDR_TABLE_DATA; // 0x68;

    volatile uint64_t *u64_wr =
        (volatile uint64_t *)(mmio_base_ + DFL_HE_CACHE_WR_ADDR_TABLE_DATA);
    volatile uint64_t *u64_rd =
        (volatile uint64_t *)(mmio_base_ + DFL_HE_CACHE_RD_ADDR_TABLE_DATA);

    res = ioctl(fd_, DFL_HE_CACHE_NUMA_DMA_UNMAP, &dma_unmap);
    if (res) {
      cerr << "ioctl DFL_HE_CACHE_NUMA_DMA_UNMAP failed" << strerror(errno)
           << endl;
    }

    printf("\nDFL_HE_CACHE_WR_ADDR_TABLE_DATA: 0x%016" PRIx64 "\n", *u64_wr);
    printf("\nDFL_HE_CACHE_RD_ADDR_TABLE_DATAs: 0x%016" PRIx64 "\n", *u64_rd);

    free_memory(rd_wr_buffer_, rd_wr_buf_len_);
    rd_wr_buffer_ = NULL;
    return true;
  }

  uint8_t *get_dsm() const { return dsm_buffer_; }

  uint8_t *get_read() const { return rd_buffer_; }

  uint8_t *get_write() const { return wr_buffer_; }

  uint8_t *get_read_write() const { return rd_wr_buffer_; }

protected:
  std::string name_;
  std::string afu_id_;
  CLI::App app_;
  std::string pci_addr_;
  std::string log_level_;
  uint32_t timeout_msec_;

  int fd_;
  uint8_t *mmio_base_;
  uint64_t mmio_len_;

  uint8_t *dsm_buffer_;
  uint64_t dsm_buf_len_;

  uint8_t *rd_buffer_;
  uint64_t rd_buf_len_;

  uint8_t *wr_buffer_;
  uint64_t wr_buf_len_;

  uint8_t *rd_wr_buffer_;
  uint64_t rd_wr_buf_len_;

  struct dfl_he_cache_region_info rinfo_;

  std::string dev_path_;

  command::ptr_t current_command_;
  std::map<CLI::App *, command::ptr_t> commands_;

public:
  std::shared_ptr<spdlog::logger> logger_;
};

} // end of namespace afu_test
} // end of namespace opae
