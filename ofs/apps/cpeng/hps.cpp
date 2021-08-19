// Copyright(c) 2021, Intel Corporation
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
#include <algorithm>
#include <chrono>
#include <fstream>
#include <thread>
#include <vector>
#include <unistd.h>
#include "afu_test.h"
#include "ofs_cpeng.h"

const char *cpeng_guid = "44bfc10d-b42a-44e5-bd42-57dc93ea7f91";

using opae::fpga::types::shared_buffer;
using opae_exception = opae::fpga::types::exception;
using usec = std::chrono::microseconds;
using msec = std::chrono::milliseconds;

const size_t pg_size = sysconf(_SC_PAGESIZE);
constexpr size_t MB(uint32_t count) {
  return count * 1024 * 1024;
}

constexpr size_t aligned(size_t n, size_t sz) {
  return (n + sz-1) & ~(sz-1);
}
const usec default_timeout_usec(msec(1000));

// TODO: make enums in ofs cpeng yaml spec
std::map<uint32_t, uint8_t> limit_map {
  { 64, 0b00},
  { 128, 0b01},
  { 512, 0b10},
  { 1024, 0b11}
};

class cpeng : public opae::afu_test::command
{
public:
  cpeng()
    : filename_("u-boot.itb")
    , destination_offset_(0x2000000)
    , timeout_usec_(default_timeout_usec.count())
    , chunk_(pg_size)
    , data_request_limit_(512)
    , soft_reset_(false)
    , skip_ssbl_verify_(false)
    , skip_kernel_verify_(false)
  {
  }
  virtual ~cpeng(){}
  virtual const char *name() const
  {
    return "cpeng";
  }

  virtual const char *description() const
  {
    return "Run copy engine commands";
  }

  virtual const char *afu_id() const
  {
    return cpeng_guid;
  }

  virtual void add_options(CLI::App *app)
  {
    std::map<std::string, uint32_t> units = {{"s", 1E6}, {"ms", 1E3}, {"us", 1}};
    std::vector<uint32_t> limits = { 64, 128, 512, 1024 };
    app->add_option("-f,--filename", filename_, "Image file to copy")
      ->default_val(filename_)
      ->check(CLI::ExistingFile);
    app->add_option("-d,--destination", destination_offset_, "HPS DDR Offset")
      ->default_str(std::to_string(destination_offset_));
    app->add_option("-t,--timeout", timeout_usec_, "Timeout")
      ->default_str(std::to_string(timeout_usec_))
      ->transform(CLI::AsNumberWithUnit(units));
    app->add_option("-r,--data-request-limit",
                    data_request_limit_,
                    "data request limit is pcie transfer width")
      ->default_str(std::to_string(data_request_limit_))
      ->check(CLI::IsMember(limits));
    app->add_option("-c,--chunk", chunk_, "Chunk size. 0 indicates no chunks")
      ->default_str(std::to_string(chunk_));
    app->add_flag("--soft-reset", soft_reset_, "Issue soft reset only");
    app->add_flag("--skip-ssbl-verify", skip_ssbl_verify_, "Do not wait for ssbl verify");
    app->add_flag("--skip-kernel-verify", skip_kernel_verify_, "Do not wait for kernel verify");
  }

  virtual int run(opae::afu_test::afu *afu, __attribute__((unused)) CLI::App *app)
  {
    log_ = spdlog::get(this->name());
    ofs_cpeng cpeng;

    // Initialize cpeng driver
    ofs_cpeng_init(&cpeng, afu->handle()->c_type());
    if (ofs_cpeng_wait_for_hps_ready(&cpeng, timeout_usec_)) {
      log_->warn("HPS is not ready");
      return 1;
    }

    if (soft_reset_) {
      log_->debug("issuing soft reset only");
      ofs_cpeng_ce_soft_reset(&cpeng);
      return 0;
    }

    // check the chunks size is a multiple of data request limit
    if (chunk_ % data_request_limit_) {
      log_->error("chunk size ({}) must be aligned to request limit size ({})",
                   chunk_, data_request_limit_);
      return 2;
    }

    // Open file, get the size
    std::ifstream inp(filename_, std::ios::binary | std::ios::ate);
    size_t sz = inp.tellg();
    inp.seekg(0, std::ios::beg);

    // if chunk_ CLI arg is 0, use the file size
    // otherwise, use the smaller of chunk_ and file size
    size_t chunk = chunk_ ? std::min(static_cast<size_t>(chunk_), sz) : sz;
    shared_buffer::ptr_t buffer(0);
    // make sure we align our buffer size to data request limit
    try {
      buffer = shared_buffer::allocate(afu->handle(),
                                       aligned(chunk, data_request_limit_));
    } catch (opae_exception &ex) {
      log_->error("could not allocate {} bytes of memory", chunk);
      if (chunk > pg_size) {
        auto hugepage_sz = chunk <= MB(2) ? "2MB" : "1GB";
        log_->error("might need {} hugepages reserved", hugepage_sz);
      }
      return 3;
    }
    // get a char* of the buffer so we can read into it every chunk iteration
    auto ptr = reinterpret_cast<char*>(const_cast<uint8_t*>(buffer->c_type()));

    memset(ptr, 0, buffer->size());
    size_t written = 0;
    log_->info("starting copy of file:{}, size: {}, chunk size: {}",
               filename_, sz, chunk);
    uint32_t n_chunks = 0;
    // set our xfer size to chunk size
    // but align it with req. limit size in case
    // our chunk is the entire file
    auto xfer_sz = aligned(chunk, data_request_limit_);
    // set the data req. limit to CLI arg (default arg is 512, default in HW is 1k)
    ofs_cpeng_set_data_req_limit(&cpeng, limit_map[data_request_limit_]);
    while (written < sz) {
        auto unread = sz-written;
        inp.read(ptr, chunk);
        if (ofs_cpeng_copy_chunk(
              &cpeng, buffer->io_address(),
              destination_offset_ + written,
              xfer_sz,
              timeout_usec_)) {
          auto status = ofs_cpeng_dma_status(&cpeng);
          log_->warn("copy chunk: {}, size: {}, unread: {}, dma_status: {:x}",
                      n_chunks, xfer_sz, unread, status);
          if (dmastatus_err(&cpeng)) {
            return 4;
          }
        }
        ++n_chunks;
        written += chunk;
        // if we're at the last chunk,
        // 1. zero out our buffer
        // 2. set 'chunk' to number of unread bytes
        // 3. set 'xfer_sz' to the req. limit aligned value of the last chunk
        if (unread < chunk) {
          memset(ptr, 0, chunk);
          chunk = unread;
          xfer_sz = aligned(chunk, data_request_limit_);
          log_->info("last chunk {}, aligned {}", chunk, xfer_sz);
        }
    }
    log_->info("transferred file in {} chunk(s)", n_chunks);
    ofs_cpeng_image_complete(&cpeng);

    // wait for both ssbl and kernel verify (if not skipped)
    if (!skip_ssbl_verify_) {
      wait_for_verify("ssbl", &cpeng, ofs_cpeng_hps_ssbl_verify);
    }
    if (!skip_kernel_verify_) {
      wait_for_verify("kernel", &cpeng, ofs_cpeng_hps_kernel_verify);
    }

    return 0;
  }


private:
  bool dmastatus_err(ofs_cpeng *cpeng)
  {
    if (ofs_cpeng_dma_status_error(cpeng)) {
        uint64_t axist_cpl = ofs_cpeng_ce_axist_cpl_sts(cpeng);
        uint64_t acelite_bresp = ofs_cpeng_ce_acelite_bresp_sts(cpeng);
        uint64_t fifo1_status = ofs_cpeng_ce_fifo1_status(cpeng);
        uint64_t fifo2_status = ofs_cpeng_ce_fifo2_status(cpeng);
        if (axist_cpl) {
          log_->error("CE_AXIST_CPL_STS: {:x}", axist_cpl);
        }
        if (acelite_bresp) {
          log_->error("CE_ACELITE_BRESP_STS: {:x}", acelite_bresp);
        }
        if (fifo1_status) {
          log_->error("CE_FIFO1_STS: {:x}", fifo1_status);
        }
        if (fifo2_status) {
          log_->error("CE_FIFO2_STS: {:x}", fifo2_status);
        }
        ofs_cpeng_ce_soft_reset(cpeng);
        return true;
      }
      return false;
  }

  void wait_for_verify(const char *stage, ofs_cpeng *cpeng, int (*verify_fn)(ofs_cpeng *))
  {
      log_->info("waiting for {} verify...", stage);
      auto sleep_time = usec(200);
      uint32_t verify = 0;
      while(!verify) {
        verify = verify_fn(cpeng);
        std::this_thread::sleep_for(sleep_time);
      }
      if (verify != 0b1){
        log_->error("error with {} verify: {:x}", stage, verify);
      }
      log_->info("{} verified", stage);
  }

  std::string filename_;
  uint64_t destination_offset_;
  uint32_t timeout_usec_;
  uint32_t chunk_;
  uint32_t data_request_limit_;
  bool soft_reset_;
  bool skip_ssbl_verify_;
  bool skip_kernel_verify_;
  std::shared_ptr<spdlog::logger> log_;
};


class heartbeat : public opae::afu_test::command
{
public:
  heartbeat()
  {
  }
  virtual ~heartbeat(){}
  virtual const char *name() const
  {
    return "heartbeat";
  }

  virtual const char *description() const
  {
    return "Check for HPS heartbeat";
  }

  virtual const char *afu_id() const
  {
    return cpeng_guid;
  }

  virtual int run(opae::afu_test::afu *afu, __attribute__((unused)) CLI::App *app)
  {
    log_ = spdlog::get(this->name());
    ofs_cpeng cpeng;

    // Initialize cpeng driver
    ofs_cpeng_init(&cpeng, afu->handle()->c_type());
    return check_heartbeat(&cpeng);
  }


private:
  int check_heartbeat(ofs_cpeng *cpeng)
  {
    uint64_t value = 0;
    while (true) {
      std::this_thread::sleep_for(msec(1000));
      auto next = ofs_cpeng_hps2host_rsp(cpeng);
      if (next <= value) {
        log_->warn("could not detect heartbeat, value: 0x{:x}", next);
      } else {
        log_->info("heartbeat value: 0x{:x}", next);
      }
      value = next;
    }
    return 0;
  }

  std::shared_ptr<spdlog::logger> log_;
};

int main(int argc, char *argv[])
{
  opae::afu_test::afu hps(cpeng_guid);
  hps.register_command<cpeng>();
  hps.register_command<heartbeat>();
  hps.main(argc, argv);
  return 0;
}

