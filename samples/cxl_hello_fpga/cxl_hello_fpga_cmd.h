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

#define UNUSED_PARAM(x) ((void)x)

// HE exit global flag
volatile bool g_he_exit = false;
volatile static bool g_stop_thread = false;

// host exerciser signal handler
void he_sig_handler(int) {
  g_he_exit = true;
  g_stop_thread = true;
  cout << "HE signal handler exit app" << endl;
}

namespace hello_fpga {

void he_cache_thread(uint8_t *buf_ptr, uint64_t len);

class he_cache_cmd : public he_cmd {
public:
  he_cache_cmd()
      : he_continuousmode_(false), he_contmodetime_(0), he_linerep_count_(0),
        he_stride_(0), he_test_(0), he_test_all_(false), he_dev_instance_(0),
        he_stride_cmd_(false) {}

  virtual ~he_cache_cmd() {}

  virtual const char *name() const override { return "cache"; }

  virtual const char *description() const override {
    return "run simple cxl he cache test";
  }

  virtual const char *afu_id() const override { return HE_CACHE_AFU_ID; }

  virtual uint64_t featureid() const override { return MEM_TG_FEATURE_ID; }

  virtual uint64_t guidl() const override { return MEM_TG_FEATURE_GUIDL; }

  virtual uint64_t guidh() const override { return MEM_TG_FEATURE_GUIDH; }

  virtual void add_options(CLI::App *app) override {

    // test mode
    app->add_option(
           "--test", he_test_,
           "host exerciser cache test")
        ->transform(CLI::CheckedTransformer(he_test_modes))
        ->default_val("hellofpga");

    // Continuous mode
    app->add_option("--continuousmode", he_continuousmode_,
                    "test rollover or test termination")
        ->default_val("false");

    // Continuous mode time
    app->add_option("--contmodetime", he_contmodetime_,
                    "Continuous mode time in seconds")
        ->default_val("1");

    // target host or fpga
    app->add_option("--target", he_target_,
                    "host exerciser run on host or fpga")
        ->transform(CLI::CheckedTransformer(he_targets))
        ->default_val("host");

    app->add_option("--bias", he_bias_,
        "CXL IP memory access Bias mode: host or device")
        ->transform(CLI::CheckedTransformer(he_bias))
        ->default_val("host");

    // device cache0 or cache1
    app->add_option("--device", he_dev_instance_,
        "run host exerciser device /dev/dfl-cxl-cache.0 (instance 0) \
        or /dev/dfl-cxl-cache.1 (instance 1)")
        ->transform(CLI::CheckedTransformer(he_cxl_device))
        ->default_val("/dev/dfl-cxl-cache.0");

    // Set sride
    app->add_option("--stride", he_stride_, "Set stride value")
        ->transform(CLI::Range(0, 3))->default_val("0");

    // Line repeat count
    app->add_option("--linerepcount", he_linerep_count_, "Line repeat count")
        ->transform(CLI::Range(1, 256))
        ->default_val("10");

    // Test all
    app->add_option("--testall", he_test_all_, "Run all tests")
        ->default_val("false");
  }


  int he_run_hello_fpga_test() {

    cout << "********** Hello FPGA Test Start **********" << endl;

     uint8_t *data_buff_addr = NULL;
     uint8_t *data_buff_addr_ = NULL;
     uint16_t hw_CL = 0;
     uint32_t cl_idx = 0;
     uint32_t header_data_ = 0;
     uint64_t idx = 0;
     uint64_t hw_phy_addr_ = 0;
     volatile uint64_t *act_wr_buff_phy_addr = NULL;

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
    cout << "Line Repeat Count:" << he_linerep_count_ << endl;
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
        << endl;

    /*
     * set W_CONFIG
     */
    he_wr_cfg_.value = 0;
    he_wr_cfg_.line_repeat_count = he_linerep_count_;
    he_wr_cfg_.write_traffic_enable = 1;
    he_wr_cfg_.opcode = WR_LINE_M;
    host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);

    /* 
     * Set WR_ADDR_TABLE_CTRL
     */
    wr_table_ctl_.value = 0;
    if (he_stride_cmd_) {
        wr_table_ctl_.enable_address_stride = 1;
        wr_table_ctl_.stride = he_stride_;
    } else if (he_target_ == HE_TARGET_FPGA) {
        /* Set Stride to 3 for Target FPGA Memory */
        wr_table_ctl_.enable_address_stride = 1;
        wr_table_ctl_.stride = 3;
    } else {
        wr_table_ctl_.enable_address_stride = 1;
        wr_table_ctl_.stride = he_stride_;
    }
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
    printf("\nRead Write_Buff_Addr : %p \n", data_buff_addr);

    memset(data_buff_addr, 0xFF, BUFFER_SIZE_2MB);


    cout << " ********** DISPLAY DATA Start ********** " << endl;

    cout << "\nInPut Buffer Data : \n" << endl;
    for(idx = 0; idx < (CL * HELLO_FPGA_NUMCACHE_LINES); idx++) {
        if((idx % CL) != 0) {
            printf("%02x", data_buff_addr[idx]);
        }
        else{
   	    printf("\n");
            printf("%02x", data_buff_addr[idx]);
	}
    }

    cout << "\n ********** DISPLAY DATA End ********** " << endl;

    /* 
     * Start test
     */
    he_start_test();

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

    cout << "\n ********** DISPLAY DATA Start ********** " << endl;

    cout << "\nOutPut Buffer Data : \n" << endl;
    for(idx = 0; idx < (CL * HELLO_FPGA_NUMCACHE_LINES); idx++) {
       if((idx % CL) != 0) {
           printf("%02x", data_buff_addr[idx]);
       }
       else{
           printf("\n");
           printf("%02x", data_buff_addr[idx]);
       }
    }

    cout << "\n ********** DISPLAY DATA End ********** " << endl;

    act_wr_buff_phy_addr = host_exe_->get_write_buff_phy_addr();

    cout << "\n ********** DATA Integrity Check Starts ********** " << endl;

    for(cl_idx = 0; cl_idx < (HELLO_FPGA_NUMCACHE_LINES); cl_idx++) {

	data_buff_addr_ = (data_buff_addr + (CL * cl_idx));
        header_data_ = (data_buff_addr_[0] |
	               (data_buff_addr_[1] << 8) |
	               (data_buff_addr_[2] << 16) |
	               (data_buff_addr_[3] << 24)); 

        cout << "\nHeader Data : " << hex << header_data_  << endl;

	if(HELLO_FPGA_CL_HEADER != header_data_) {
                cout << "\n Cache Line Header value mismatch - " << header_data_ << endl;
                host_exe_->free_cache_read_write();
                host_exe_->free_dsm();
		return -1;
	}

        for(idx = 4; idx < 53; idx++) {
		if(0 != data_buff_addr_[idx]) {
                    cout << "\n A non-ZERO value is observed at byte " << idx << endl;
                    host_exe_->free_cache_read_write();
                    host_exe_->free_dsm();
		    return -1;
		}

        }

	if(0 != data_buff_addr_[53]) {
                cout << "\n Stride value mismatch " << endl;
                host_exe_->free_cache_read_write();
                host_exe_->free_dsm();
		return -1;
	}

	hw_CL = (((uint16_t)data_buff_addr_[54]) | ((uint16_t)(data_buff_addr_[55]) << 8));

        cout << "\nHardware Cache Line Counter : " << hex << hw_CL << endl;

	if(hw_CL != cl_idx) {
                cout << "\n Cache Line counter mismatch " << endl;
                host_exe_->free_cache_read_write();
                host_exe_->free_dsm();
		return -1;
	}

	hw_phy_addr_ = ((uint64_t)(data_buff_addr_[56]) |
		       ((uint64_t)(data_buff_addr_[57]) << 8) |
		       ((uint64_t)(data_buff_addr_[58]) << 16) |
		       ((uint64_t)(data_buff_addr_[59]) << 24) |
		       ((uint64_t)(data_buff_addr_[60]) << 32) |
		       ((uint64_t)(data_buff_addr_[61]) << 40) |
		       ((uint64_t)(data_buff_addr_[62]) << 48) |
		       ((uint64_t)(data_buff_addr_[63]) << 56));

        cout << "HW updated Physical address : " << hex << hw_phy_addr_ << endl;
        cout << "Actual Physical address : " << hex << *act_wr_buff_phy_addr << endl;

	if(*act_wr_buff_phy_addr != hw_phy_addr_) {
                cout << "\n Cache Line Physical Address mismatch " << endl;
                host_exe_->free_cache_read_write();
                host_exe_->free_dsm();
		return -1;
	}
        cout << "\n Cache Line Physical Address match is successful !!!@@@\n\n" << endl;
       
	*act_wr_buff_phy_addr = *act_wr_buff_phy_addr + CL;

    }

    cout << "\n ********** DATA Integrity Check Ends ********** " << endl;

    host_exe_->free_cache_read_write();
    host_exe_->free_dsm();

    cout << "********** Hello FPGA Verified Successfully ********** "
         << endl;
    cout << "********** Hello FPGA Test Ends **********" << endl;
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

    CLI::Option* opt = app->get_option_no_throw("--stride");
    if (opt && opt->count() == 1) {
        he_stride_cmd_ = true;
    }

    // reset HE cache
    he_ctl_.value = 0;
    he_ctl_.ResetL = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    he_ctl_.ResetL = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    print_csr();

    if (!he_set_bias_mode()) {
        return -1;
    }

    if (he_test_ == HE_HELLO_FPGA) {
      ret = he_run_hello_fpga_test();
      return ret;
    }

    return 0;
  }

protected:
  bool he_continuousmode_;
  uint32_t he_contmodetime_;
  uint32_t he_linerep_count_;
  uint32_t he_stride_;
  uint32_t he_test_;
  bool he_test_all_;
  uint32_t he_dev_instance_;
  bool he_stride_cmd_;
};

void he_cache_thread(uint8_t *buf_ptr, uint64_t len) {

    uint64_t value;
    UNUSED_PARAM(value);
    uint64_t cache_lines = len / CL;
    uint64_t i = 0;

  if (buf_ptr == NULL || len == 0) {
    return;
  }

  while (true) {
    if (g_stop_thread == true) {
      return;
    }
    if (i < cache_lines) {
      value = *((volatile uint64_t *)(buf_ptr + i * 8));
    }
    i++;
    if (i >= cache_lines) {
      i = 0;
    }
  }

  return;
}

} // end of namespace hello_fpga
