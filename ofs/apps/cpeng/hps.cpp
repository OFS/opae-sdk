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
#include <unistd.h>
#include "afu_test.h"
#include "ofs_cpeng.h"

#define CACHELINE_SZ 64u

const char *cpeng_guid = "44bfc10d-b42a-44e5-bd42-57dc93ea7f91";

using opae::fpga::types::shared_buffer;
using opae_exception = opae::fpga::types::exception;
using usec = std::chrono::microseconds;
using msec = std::chrono::milliseconds;

const size_t pg_size = sysconf(_SC_PAGESIZE);
constexpr size_t MB(uint32_t count) {
  return count * 1024 * 1024;
}
const usec default_timeout_usec(msec(10));

class cpeng : public opae::afu_test::command
{
public:
  cpeng()
    : filename_("u-boot.itb")
    , destination_offset_(0x2000000)
    , timeout_usec_(default_timeout_usec.count())
    , chunk_(pg_size)
    , soft_reset_(false)
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
    app->add_option("-f,--filename", filename_, "Image file to copy")
      ->default_val(filename_)
      ->check(CLI::ExistingFile);
    app->add_option("-d,--destination", destination_offset_, "HPS DDR Offset")
      ->default_str(std::to_string(destination_offset_));
    app->add_option("-t,--timeout", timeout_usec_, "Timeout")
      ->default_str(std::to_string(timeout_usec_))
      ->transform(CLI::AsNumberWithUnit(units));
    app->add_option("-c,--chunk", chunk_, "Chunk size. 0 indicates no chunks")
      ->default_str(std::to_string(chunk_));
    app->add_flag("--soft-reset", soft_reset_, "Issue soft reset only");
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

    // Open file, get the size
    std::ifstream inp(filename_, std::ios::binary | std::ios::ate);
    size_t sz = inp.tellg();
    inp.seekg(0, std::ios::beg);

    // if chunk_ CLI arg is 0, use the file size
    // otherwise, use the smaller of chunk_ and file size
    size_t chunk = chunk_ ? std::min(static_cast<size_t>(chunk_), sz) : sz;
    shared_buffer::ptr_t buffer(0);
    try {
      buffer = shared_buffer::allocate(afu->handle(), chunk);
    } catch (opae_exception &ex) {
      log_->error("could not allocate {} bytes of memory", chunk);
      if (chunk > pg_size) {
        auto hugepage_sz = chunk <= MB(2) ? "2MB" : "1GB";
        log_->error("might need {} hugepages reserved", hugepage_sz);
      }
      return 1;
    }
    // get a char* of the buffer so we can read into it every chunk iteration
    auto ptr = reinterpret_cast<char*>(const_cast<uint8_t*>(buffer->c_type()));

    size_t written = 0;
    log_->info("starting copy of file:{}, size: {}, chunk size: {}",
               filename_, sz, chunk);
    uint32_t n_chunks = 0;
    // set the data req. limit to 512 (default is 1k)
    ofs_cpeng_set_data_req_limit(&cpeng, 0b10);
    while (written < sz) {
        inp.read(ptr, chunk);
        if (ofs_cpeng_copy_chunk(
              &cpeng, buffer->io_address(),
              destination_offset_ + written, chunk, timeout_usec_)) {
          log_->warn("copy chunk, dma_status: {:x}",
                      ofs_cpeng_dma_status(&cpeng));
          if (dmastatus_err(&cpeng)) {
            return 2;
          }
        }
        ++n_chunks;
        written += chunk;
        chunk = std::min(chunk, sz-written);
    }
    log_->info("transerred file in {} chunk(s)", n_chunks);
    // by this point we've copied the file itself
    // check if bytes written are not cacheline aligned
    // if not, add padding so that total written is cacheline aligned
    auto padding = ((written + CACHELINE_SZ) & ~(CACHELINE_SZ-1)) - written;
    if (padding) {
      log_->debug("padded file with {} bytes", padding);
      // let's reuse our buffer
      memset(ptr, 0, padding);
      if (ofs_cpeng_copy_chunk(
            &cpeng, buffer->io_address(),
            destination_offset_ + written, padding, timeout_usec_)) {
        log_->error("copying padding, dma_status: {:x}",
                    ofs_cpeng_dma_status(&cpeng));
        if (dmastatus_err(&cpeng)) {
          return 3;
        }
      }
    }
    ofs_cpeng_image_complete(&cpeng);
    wait_for_verify(&cpeng);

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

  void wait_for_verify(ofs_cpeng *cpeng)
  {
      // wait for both kernel and ssbl verify
      auto sleep_time = usec(200);
      log_->info("waiting for ssbl verify...");
      uint32_t verify = 0;
      while(!verify) {
        verify = ofs_cpeng_hps_ssbl_verify(cpeng);
        std::this_thread::sleep_for(sleep_time);
      }
      if (verify != 0b1){
        log_->error("error with ssbl verify: {:x}", verify);
        return;
      }
      log_->info("waiting for kernel verify...");
      verify = 0;
      while(!verify) {
        verify = ofs_cpeng_hps_kernel_verify(cpeng);
        std::this_thread::sleep_for(sleep_time);
      }
      if (verify != 0b1){
        log_->error("error with kernel verify: {:x}", verify);
      }
  }

  std::string filename_;
  uint64_t destination_offset_;
  uint32_t timeout_usec_;
  uint32_t chunk_;
  bool soft_reset_;
  std::shared_ptr<spdlog::logger> log_;
};

int main(int argc, char *argv[])
{
  opae::afu_test::afu hps(cpeng_guid);
  hps.register_command<cpeng>();
  hps.main(argc, argv);
  return 0;
}

