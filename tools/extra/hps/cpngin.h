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
#include "cpngin_reg.h"

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
    using opae::fpga::types::shared_buffer;
    using std::chrono::milliseconds;
    using std::chrono::microseconds;
    (void)app;
    auto log = spdlog::get(this->name());
    hps *h = dynamic_cast<hps*>(afu);

    auto rdy_status = h->object64<CSR_HPS2HOST_RDY_STATUS>();
    uint32_t tries = 5000;
    while (!rdy_status->f_HPS_RDY && tries--) {
      std::this_thread::sleep_for(milliseconds(1));
    }
    if (!tries) {
      log->warn("HPS is not ready");
      // return 1;
    }
    std::ifstream inp(filename_, std::ios::binary | std::ios::ate);
    auto sz = inp.tellg();
    inp.seekg(0, std::ios::beg);
    auto buffer = shared_buffer::allocate(h->handle(), sz);
    auto ptr = reinterpret_cast<char*>(const_cast<uint8_t*>(buffer->c_type()));
    if (!inp.read(ptr, sz)){
      log->error("error reading file: {}", filename_);
      return 2;
    }

    log->info("opened file {} with size {}", filename_, sz);
    auto src_addr = h->object64<CSR_SRC_ADDR>();
    src_addr->f_CSR_SRC_ADDR = buffer->io_address();
    log->info("src address is 0x{:x}", static_cast<uint64_t>(src_addr->f_CSR_SRC_ADDR));

    auto dst_offset = h->object64<CSR_DST_ADDR>();
    dst_offset->f_CSR_DST_ADDR = destination_offset_;

    auto data_size = h->object64<CSR_DATA_SIZE>();
    data_size->f_CSR_DATA_SIZE = sz;

    auto host2ce_start = h->object64<CSR_HOST2CE_MRD_START>();
    host2ce_start->f_MRD_START = 1;

    auto ce2host_status = h->object64<CSR_CE2HOST_STATUS>();
    log->info("waiting for {:n} usec", timeout_usec_);
    std::future<void> f = std::async(std::launch::async,
        [ce2host_status](){
          while(ce2host_status->f_CE_DMA_STS == 0x1) {
            std::this_thread::sleep_for(microseconds(100));
          }
        });
    auto status = f.wait_for(microseconds(timeout_usec_));
    if (status == std::future_status::timeout) {
      log->error("timed out waiting for Host to CE transfer");
      throw std::runtime_error("timed out waiting for Host to CE tranfer");
    }

    log->info("CE_DMA_STS 0x{:x}", static_cast<uint64_t>(ce2host_status->f_CE_DMA_STS));
    if (ce2host_status->f_CE_DMA_STS != 0b10) {
        auto value = static_cast<uint64_t>(ce2host_status->f_CE_DMA_STS);
        log->error("error encountered with Host to CE transfer, value is {}", value);
        throw std::runtime_error("error encountered with Host to CE transfer");
    }

    auto gpio = h->object64<CSR_HOST2HPS_GPIO>();
    gpio->f_HOST_HPS_CPL = 1;

    auto hps2host_vfy_status = h->object64<CSR_HPS2HOST_VFY_STATUS>();
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
