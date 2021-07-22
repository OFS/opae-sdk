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
#include "afu_test.h"
#include "ofs_cpeng.h"

#define CACHELINE_SZ 64u

const char *cpeng_guid = "44bfc10d-b42a-44e5-bd42-57dc93ea7f91";

using opae::fpga::types::shared_buffer;
using usec = std::chrono::microseconds;

class cpeng : public opae::afu_test::command
{
public:
  cpeng()
    : filename_("hps.img")
    , destination_offset_(0)
    , timeout_usec_(60000000)
    , chunk_(4096)
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

    // Read file into shared buffer
    std::ifstream inp(filename_, std::ios::binary | std::ios::ate);
    size_t sz = inp.tellg();
    inp.seekg(0, std::ios::beg);

    // chunk in 4k pages
    auto buffer = shared_buffer::allocate(afu->handle(), chunk_);
    auto ptr = reinterpret_cast<char*>(const_cast<uint8_t*>(buffer->c_type()));
    size_t offset = 0;
    size_t chunk = std::min(static_cast<size_t>(chunk_), sz);
    while (offset < sz) {
        inp.read(ptr, chunk);
        if (chunk < chunk_) {
          auto padded_sz = (chunk+CACHELINE_SZ) & ~(CACHELINE_SZ-1);
          if (padded_sz > chunk) {
            memset(ptr+chunk, 0, padded_sz-chunk);
            chunk = padded_sz;
          }
        }
        if (ofs_cpeng_copy_chunk(
              &cpeng, buffer->io_address(),
              destination_offset_ + offset, chunk, timeout_usec_)) {
          log_->error("could not copy chunk");
          return 1;
        }
        offset += chunk;
        chunk = std::min(chunk, sz-offset);
    }
    ofs_cpeng_image_complete(&cpeng);
    wait_for_verify(&cpeng);

    return 0;
  }


private:
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

