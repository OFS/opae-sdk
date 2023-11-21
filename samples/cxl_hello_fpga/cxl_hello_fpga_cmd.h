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

#include "cxl_he_cmd.h"
#include "cxl_hello_fpga.h"
#include "he_cache_test.h"

// HE exit global flag
volatile bool g_he_exit = false;

// host exerciser signal handler
void he_sig_handler(int) {
  g_he_exit = true;
  cout << "HE signal handler exit app" << endl;
}

namespace hello_fpga {

class he_cache_cmd : public he_cmd {
 public:
  he_cache_cmd() : he_test_(0) {}

  virtual ~he_cache_cmd() {}

  virtual const char *name() const override { return "hellofpga"; }

  virtual const char *description() const override {
    return "run simple cxl hello fpga test";
  }

  virtual const char *afu_id() const override { return HE_CACHE_AFU_ID; }

  virtual uint64_t featureid() const override { return MEM_TG_FEATURE_ID; }

  virtual uint64_t guidl() const override { return MEM_TG_FEATURE_GUIDL; }

  virtual uint64_t guidh() const override { return MEM_TG_FEATURE_GUIDH; }

  virtual void add_options(CLI::App *app) override {
    // test mode
    app->add_option("host exerciser cache test")
        ->transform(CLI::CheckedTransformer(he_test_modes))
        ->default_val("hellofpga");
  }

  bool hello_fpga_data_intg_check(uint8_t *buf_address) {
    uint8_t *data_buff_addr_ = NULL;
    uint16_t hw_CL = 0;
    uint32_t cl_idx = 0;
    uint8_t idx = 0;
    uint32_t header_data_ = 0;
    uint64_t hw_phy_addr_ = 0;
    volatile uint64_t *act_wr_buff_phy_addr = NULL;

    act_wr_buff_phy_addr = host_exe_->get_write_buff_phy_addr();

    for (cl_idx = 0; cl_idx < (HELLO_FPGA_NUMCACHE_LINES); cl_idx++) {
      data_buff_addr_ = (buf_address + (CL * cl_idx));

      hw_CL = (((uint16_t)data_buff_addr_[0]) |
               ((uint16_t)(data_buff_addr_[1]) << 8));

      if (hw_CL != cl_idx) {
        cout << "\n Cache Line counter mismatch " << endl;
        host_exe_->free_cache_read_write();
        host_exe_->free_dsm();
        return false;
      }

      header_data_ = (data_buff_addr_[2] | (data_buff_addr_[3] << 8) |
                      (data_buff_addr_[4] << 16) | ((data_buff_addr_[5] & 0x3F) << 24));

      if (HELLO_FPGA_CL_HEADER != header_data_) {
        cout << "\n Cache Line Header value mismatch - " << header_data_
             << endl;
        host_exe_->free_cache_read_write();
        host_exe_->free_dsm();
        return false;
      }

      if (0 != ((data_buff_addr_[5] >> 6) & 0x3)) {
        cout << "\n Stride value mismatch " << endl;
        host_exe_->free_cache_read_write();
        host_exe_->free_dsm();
        return false;
      }

      for (idx = 6; idx < 56; idx++) {
        if (0 != data_buff_addr_[idx]) {
          cout << "\n A non-ZERO value is observed at byte " << idx << endl;
          host_exe_->free_cache_read_write();
          host_exe_->free_dsm();
          return false;
        }
      }

      hw_phy_addr_ = ((uint64_t)(data_buff_addr_[56]) |
                      ((uint64_t)(data_buff_addr_[57]) << 8) |
                      ((uint64_t)(data_buff_addr_[58]) << 16) |
                      ((uint64_t)(data_buff_addr_[59]) << 24) |
                      ((uint64_t)(data_buff_addr_[60]) << 32) |
                      ((uint64_t)(data_buff_addr_[61]) << 40) |
                      ((uint64_t)(data_buff_addr_[62]) << 48) |
                      ((uint64_t)(data_buff_addr_[63]) << 56));

      if (*act_wr_buff_phy_addr != hw_phy_addr_) {
        cout << "\n Cache Line Physical Address mismatch " << endl;
        host_exe_->free_cache_read_write();
        host_exe_->free_dsm();
        return false;
      }

      *act_wr_buff_phy_addr = *act_wr_buff_phy_addr + CL;
    }
    return true;
  }
  
  int he_run_hello_fpga_test() {
    cout << "Hello FPGA Start" << endl;

    uint8_t *data_buff_addr = NULL;

    /*
    STEPS
    1) Allocate DSM, Read buffer, Write buffer
    2) Write number of lines more then 32 kb  2mb/64
    3) Set WR ItoMWr (CXL) config
    4) Run test ( Buffer is not present in FPGA - FPGA write Cache miss )
    */

    /*
     HE_INFO
     Set Read number Lines
    */
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_WR_NUM_LINES, HELLO_FPGA_NUMCACHE_LINES);

    cout << "Read/write number Lines:" << HELLO_FPGA_NUMCACHE_LINES << endl;
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
         << endl;

    /*
     * set W_CONFIG
     */
    he_wr_cfg_.value = 0;
    he_wr_cfg_.write_traffic_enable = 1;
    he_wr_cfg_.opcode = WR_LINE_M;
    host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);

    /*
     * Set WR_ADDR_TABLE_CTRL
     */
    wr_table_ctl_.value = 0;
    wr_table_ctl_.enable_address_stride = 1;
    host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

    /*
     * Allocate DSM buffer
     */
    if (!host_exe_->allocate_dsm()) {
      cerr << "allocate dsm failed" << endl;
      return -1;
    }

    /*
     * Allocate Read, Write buffer
     */
    if (!host_exe_->allocate_cache_read_write(BUFFER_SIZE_2MB, numa_node_)) {
      cerr << "allocate cache read failed" << endl;
      host_exe_->free_dsm();
      return -1;
    }

    /*
     * Obtain Write address
     */
    data_buff_addr = host_exe_->get_read_write();

    memset(data_buff_addr, 0xFF, BUFFER_SIZE_2MB);

    /*
     * Start test
     */
    he_start_test();

    cout << "AFU Configuration : Successful" << endl;
    /*
     * wait for completion
     */
    if (!he_wait_test_completion()) {
      he_perf_counters();
      host_exerciser_errors();
      host_exe_->free_cache_read_write();
      host_exe_->free_dsm();
      return -1;
    }

    if (!hello_fpga_data_intg_check(data_buff_addr)) {
      host_exe_->free_cache_read_write();
      host_exe_->free_dsm();
      cout << "DATA Integrity Check : Failed" << endl;
      return -1;
    }
    cout << "DATA Integrity Check : Successful" << endl;

    host_exe_->free_cache_read_write();
    host_exe_->free_dsm();

    cout << "Hello FPGA Executed Successfully" << endl;
    return 0;
  }

  virtual int run(test_afu *afu, CLI::App *app) {
    (void)app;
    int ret = 0;

    host_exe_ = dynamic_cast<hello_fpga *>(afu);

    if (!verify_numa_node()) {
      numa_node_ = 0;
      cout << "numa nodes are available set numa node to 0" << endl;
    };

    // reset HE cache
    he_ctl_.value = 0;
    he_ctl_.ResetL = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    he_ctl_.ResetL = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    print_csr();

    if (he_test_ == HE_HELLO_FPGA) {
      ret = he_run_hello_fpga_test();
      return ret;
    }

    return 0;
  }

 protected:
  uint32_t he_test_;
};

}  // end of namespace hello_fpga
