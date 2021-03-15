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
#include <chrono>
#include <fstream>
#include <thread>
#include "afu_test.h"
#include "ofs_cpeng.h"


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
    , chunk_(0)
  {
    log_ = spdlog::get(this->name());
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
      ->default_val("hps.img")
      ->check(CLI::ExistingFile);
    app->add_option("-d,--destination", destination_offset_, "HPS DDR Offset")
      ->default_val(destination_offset_);
    app->add_option("-t,--timeout", timeout_usec_, "Timeout")
      ->default_val(timeout_usec_)
      ->transform(CLI::AsNumberWithUnit(units));
    app->add_option("-c,--chunk", chunk_, "Chunk size. 0 indicates no chunks")
      ->default_val(chunk_);
  }

  virtual int run(opae::afu_test::afu *afu, __attribute__((unused)) CLI::App *app)
  {
    ofs_cpeng cpeng;

    // Initialize cpeng driver
    ofs_cpeng_init(&cpeng, afu->handle()->c_type());
    if (ofs_cpeng_wait_for_hps_ready(&cpeng, timeout_usec_)) {
      log_->warn("HPS is not ready");
      return 1;
    }

    // Read file into shared buffer
    std::ifstream inp(filename_, std::ios::binary | std::ios::ate);
    auto sz = inp.tellg();
    inp.seekg(0, std::ios::beg);
    auto buffer = shared_buffer::allocate(afu->handle(), sz);
    auto ptr = reinterpret_cast<char*>(const_cast<uint8_t*>(buffer->c_type()));
    if (!inp.read(ptr, sz)){
      log_->error("error reading file: {}", filename_);
      return 2;
    }
    log_->info("opened file {} with size {}", filename_, sz);

    // Call cpeng driver copy_buffer
    auto copy_status =
      ofs_cpeng_copy_image(&cpeng,
          buffer->io_address(), destination_offset_, sz, chunk_, timeout_usec_);
    if (copy_status) {
      log_->error("Erro calling ofs_cpeng_copy_image");
      if (ofs_cpeng_dma_status_error(&cpeng)) {
        uint64_t axist_cpl = ofs_cpeng_ce_axist_cpl_sts(&cpeng);
        uint64_t acelite_bresp = ofs_cpeng_ce_acelite_bresp_sts(&cpeng);
        if (axist_cpl) {
          log_->error("CE_AXIST_CPL_STS: {:x}", axist_cpl);
        }
        if (acelite_bresp) {
          log_->error("CE_ACELITE_BRESP_STS: {:x}", acelite_bresp);
        }
      }
    }
    return copy_status;
  }


private:
  std::string filename_;
  uint64_t destination_offset_;
  uint32_t timeout_usec_;
  uint32_t chunk_;
  std::shared_ptr<spdlog::logger> log_;
};

int main(int argc, char *argv[])
{
  opae::afu_test::afu hps(cpeng_guid);
  hps.register_command<cpeng>();
  hps.main(argc, argv);
  return 0;
}

