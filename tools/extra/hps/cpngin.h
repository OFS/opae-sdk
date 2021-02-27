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
#pragma once
#include <chrono>
#include <fstream>
#include <thread>
#include "hps.h"
#include "ofs_cpeng.h"
#include "ofs_primitives.h"

namespace hps {

//const char *cpngin_guid = "44bfc10d-b42a-44e5-bd42-57dc93ea7f91";
const char *cpngin_guid = nullptr;


class cpngin : public opae::afu_test::command
{
public:
  cpngin()
    : filename_("hps.img")
    , destination_offset_(0)
    , timeout_usec_(60)
  {

  }
  virtual ~cpngin(){}
  virtual const char *name() const
  {
    return "cpngin";
  }

  virtual const char *description() const
  {
    return "Run copy engine commands";
  }

  virtual const char *afu_id() const
  {
    return cpngin_guid;
  }

  virtual void add_options(CLI::App *app)
  {
    std::map<std::string, uint32_t> units = {{"s", 1E6}, {"ms", 1E3}, {"us", 1}};
    app->add_option("-f,--filename", filename_, "Image file to copy")
      ->default_val("hps.img")
      ->check(CLI::ExistingFile);
    app->add_option("-d,--destination", destination_offset_, "HPS DDR Offset")
      ->default_val(destination_offset_);
    app->add_option("-t,--timeout", timeout_usec_, "Timeout (sec)")
      ->default_val(timeout_usec_)
      ->transform(CLI::AsNumberWithUnit(units));
  }

  virtual int run(opae::afu_test::afu *afu, CLI::App *app)
  {
    (void)app;
    auto log = spdlog::get(this->name());
    using opae::fpga::types::shared_buffer;
    using std::chrono::milliseconds;
    using std::chrono::microseconds;
    ofs_cpeng cpeng;
    ofs_cpeng_init(&cpeng, afu->handle()->mmio_ptr(0), 0);
    int timeout_status = 0;
    OFS_WAIT_FOR(cpeng.r_CSR_HPS2HOST_RDY_STATUS->f_HPS_RDY, 1, 100, timeout_status);

    if (timeout_status) {
      log->warn("HPS is not ready");
      // return 1;
    }
    std::ifstream inp(filename_, std::ios::binary | std::ios::ate);
    auto sz = inp.tellg();
    inp.seekg(0, std::ios::beg);
    auto buffer = shared_buffer::allocate(afu->handle(), sz);
    auto ptr = reinterpret_cast<char*>(const_cast<uint8_t*>(buffer->c_type()));
    if (!inp.read(ptr, sz)){
      log->error("error reading file: {}", filename_);
      return 2;
    }
    log->info("opened file {} with size {}", filename_, sz);

    cpeng.r_CSR_SRC_ADDR->f_CSR_SRC_ADDR = buffer->io_address();
    log->debug("src_address: 0x{:x}",
               static_cast<uint64_t>(cpeng.r_CSR_SRC_ADDR->f_CSR_SRC_ADDR));

    cpeng.r_CSR_DST_ADDR->f_CSR_DST_ADDR = destination_offset_;
    log->debug("dst_offset: 0x{:x}",
               static_cast<uint64_t>(cpeng.r_CSR_DATA_SIZE->f_CSR_DATA_SIZE));

    cpeng.r_CSR_DATA_SIZE->f_CSR_DATA_SIZE = sz;
    log->debug("data_size: {}",
               static_cast<uint64_t>(cpeng.r_CSR_DATA_SIZE->f_CSR_DATA_SIZE));

    cpeng.r_CSR_HOST2CE_MRD_START->f_MRD_START = 1;
    log->debug("mrd_start: {}",
               static_cast<uint64_t>(cpeng.r_CSR_HOST2CE_MRD_START->f_MRD_START));

    log->info("waiting for {:n} usec", timeout_usec_);
    std::future<void> f = std::async(std::launch::async,
        [cpeng](){
          while(cpeng.r_CSR_CE2HOST_STATUS->f_CE_DMA_STS == 0x1) {
            std::this_thread::sleep_for(microseconds(100));
          }
        });
    auto status = f.wait_for(microseconds(timeout_usec_));
    if (status == std::future_status::timeout) {
      log->error("timed out waiting for Host to CE transfer");
      throw std::runtime_error("timed out waiting for Host to CE tranfer");
    }

    log->debug("ce_dma_status: 0x{:x}",
               static_cast<uint64_t>(cpeng.r_CSR_CE2HOST_STATUS->f_CE_DMA_STS));
    if (cpeng.r_CSR_CE2HOST_STATUS->f_CE_DMA_STS != 0b10) {
        auto value = static_cast<uint64_t>(cpeng.r_CSR_CE2HOST_STATUS->f_CE_DMA_STS);
        log->error("error encountered with Host to CE transfer, value is {}", value);
        throw std::runtime_error("error encountered with Host to CE transfer");
    }

    auto gpio = cpeng.r_CSR_HOST2HPS_GPIO;
    gpio->f_HOST_HPS_CPL = 1;

    auto hps2host_vfy_status = cpeng.r_CSR_HPS2HOST_VFY_STATUS;
    (void)hps2host_vfy_status;
    // verify?



    return 0;
  }


private:
  std::string filename_;
  uint64_t destination_offset_;
  uint32_t timeout_usec_;
};

} // end of namespace hps
