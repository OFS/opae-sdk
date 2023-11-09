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
#include "cxl_host_exerciser.h"
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

namespace host_exerciser {

void he_cache_thread(uint8_t *buf_ptr, uint64_t len);

class he_cache_cmd : public he_cmd {
public:
  he_cache_cmd()
      : he_continuousmode_(false), he_contmodetime_(0), he_linerep_count_(0),
        he_stride_(0), he_test_(0), he_test_all_(false), he_dev_instance_(0),
        he_stride_cmd_(false), he_cls_count_(FPGA_512CACHE_LINES),
        he_latency_iterations_(0) {}

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
        ->default_val("fpgardcachehit");

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

    // Cache lines count
    app->add_option("--clscount", he_cls_count_, "Cache lines count")
        ->transform(CLI::Range(1, 512))
        ->default_val("512");

    // Number of latency test iterations
    app->add_option("--latency_iterations", he_latency_iterations_,
        "Number of latency test iterations")
        ->transform(CLI::Range(0, 5000))
        ->default_val("0");

  }

  int he_run_fpga_rd_cache_hit_test() {
    cout << "********** FPGA Read cache hit test start**********" << endl;
    /*
    STEPS
    1) Allocate DSM, Read buffer
    2) Scenario setup:
        1) Set cache lines and line repeat count
        2) Set RdShared (CXL) config
        3) Run test (AFU copies cache from host/fpga memory to FPGA cache)
    3) Run test (AFU read cache from FPGA cache)
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_RD_NUM_LINES, he_cls_count_);

    cout << "Read number Lines:" << he_cls_count_ << endl;
    cout << "Line Repeat Count:" << he_linerep_count_ << endl;
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Numa node:" << numa_node_ << endl;

    // set RD_CONFIG RdShared (CXL)
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = 1;
    he_rd_cfg_.read_traffic_enable = 1;
    he_rd_cfg_.opcode = RD_LINE_S;
    host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);

    // set RD_ADDR_TABLE_CTRL
    rd_table_ctl_.value = 0;
    if (he_stride_cmd_) {
        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = he_stride_;
    } else if (he_target_ == HE_TARGET_FPGA) {
        // Set Stride to 3 for Target FPGA Memory
        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = 3;
    } else {
        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = he_stride_;
    }
    host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

    // Allocate DSM buffer
    if (!host_exe_->allocate_dsm()) {
      cerr << "allocate dsm failed" << endl;
      return -1;
    }

    // Allocate Read buffer
    if (!host_exe_->allocate_cache_read(BUFFER_SIZE_2MB, numa_node_)) {
      cerr << "allocate cache read failed" << endl;
      host_exe_->free_dsm();
      return -1;
    }

    // Start test
    he_start_test(HE_PRTEST_SCENARIO);

    // wait for completion
    if (!he_wait_test_completion()) {
      he_perf_counters();
      host_exerciser_errors();
      host_exe_->free_cache_read();
      host_exe_->free_dsm();
      return -1;
    }

    he_perf_counters(HE_CXL_RD_LATENCY);

    cout << "********** AFU Copied host cache to FPGA Cache successfully "
            "********** " << endl;

    // set RD_CONFIG RdShared (CXL)
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = he_linerep_count_;
    he_rd_cfg_.read_traffic_enable = 1;
    he_rd_cfg_.opcode = RD_LINE_S;

    // set RD_ADDR_TABLE_CTRL
    rd_table_ctl_.value = 0;
    if (he_stride_cmd_) {
        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = he_stride_;
    } else if (he_target_ == HE_TARGET_FPGA) {
        // Set Stride to 3 for Target FPGA Memory
        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = 3;
    } else {
        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = he_stride_;
    }

    // Continuous mode
    if (he_continuousmode_) {
        he_rd_cfg_.continuous_mode_enable = 0x1;
        host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);
        host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

        // Start test
        he_start_test();

        // Continuous mode
        he_continuousmode();

        // performance
        he_perf_counters(HE_CXL_RD_LATENCY);

    } else if(he_latency_iterations_ > 0) {
        // Latency iterations test
        double perf_data         = 0;
        double latency           = 0;
        double total_perf_data   = 0;
        double total_latency     = 0;

        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = 1;

        host_exe_->write64(HE_RD_NUM_LINES, 1);
        host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);
        host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

        for (uint64_t i = 0; i < he_latency_iterations_; i++) {
            // Start test
            he_start_test();

            // wait for completion
            if (!he_wait_test_completion()) {
                he_perf_counters();
                host_exerciser_errors();
                host_exe_->free_cache_read();
                host_exe_->free_dsm();
                return -1;
            }

            if (he_get_perf(&perf_data, &latency, HE_CXL_RD_LATENCY)) {
                total_perf_data = total_perf_data + perf_data;
                total_latency = total_latency + latency;
            }
            host_exe_->logger_->info("Iteration: {0}  latency: {1:0.3f} nanoseconds \
                     BandWidth: {2:0.3f} GB/s", i, latency, perf_data);
        } //end for loop
        host_exe_->logger_->info("Average Latency: {0:0.3f} nanoseconds",
            total_latency / he_latency_iterations_);
        host_exe_->logger_->info("Average BandWidth: {0:0.3f} GB/s",
            total_perf_data / he_latency_iterations_);

    } else {
        // fpga read cache hit test
        host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);
        host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);

        // Start test
        he_start_test();

        // wait for completion
        if (!he_wait_test_completion()) {
            he_perf_counters();
            host_exerciser_errors();
            host_exe_->free_cache_read();
            host_exe_->free_dsm();
            return -1;
        }
        he_perf_counters(HE_CXL_RD_LATENCY);
    }

    host_exe_->free_dsm();
    host_exe_->free_cache_read();

    cout << "********** AFU reads cache from FPGA Cache successfully"
        " **********" << endl;
    cout << "********** FPGA Read cache hit test end**********" << endl;
    return 0;
  }

  int he_run_fpga_wr_cache_hit_test() {

    cout << "********** FPGA Write cache hit test start**********" << endl;
    /*
    STEPS
    1) Allocate DSM, Read/Write buffer
    2) Scenario setup:
        1) Set cache lines and line repeat count
        2) Set RdShared CXL config
        3) Run test (AFU copies cache from host/fpga memory to FPGA cache)
    3) Set Write CXL config
    4) Run test (AFU writes to FPGA cache)
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_RD_NUM_LINES, he_cls_count_);

    cout << "Read/write number Lines:" << he_cls_count_ << endl;
    cout << "Line Repeat Count:" << he_linerep_count_ << endl;
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
        << endl;

    // set RD_CONFIG RdShared (CXL)
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = 1;
    he_rd_cfg_.read_traffic_enable = 1;
    he_rd_cfg_.opcode = RD_LINE_S;
    host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);

    // set RD_ADDR_TABLE_CTRL
    rd_table_ctl_.value = 0;
    if (he_stride_cmd_) {
        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = he_stride_;
    } else if (he_target_ == HE_TARGET_FPGA) {
        // Set Stride to 3 for Target FPGA Memory
        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = 3;
    } else {
        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = he_stride_;
    }
    host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

    // Allocate DSM buffer
    if (!host_exe_->allocate_dsm()) {
      cerr << "allocate dsm failed" << endl;
      return -1;
    }

    // Allocate Read, Write buffer
    if (!host_exe_->allocate_cache_read_write(BUFFER_SIZE_2MB, numa_node_)) {
      cerr << "allocate cache read failed" << endl;
      host_exe_->free_dsm();
      return -1;
    }

    // Start test
    he_start_test(HE_PRTEST_SCENARIO);

    // wait for completion
    if (!he_wait_test_completion()) {
      he_perf_counters();
      host_exerciser_errors();
      host_exe_->free_cache_read_write();
      host_exe_->free_dsm();
      return -1;
    }

    he_perf_counters();

    cout << "********** AFU Copied host cache to FPGA Cache successfully "
            "********** " << endl;

    // set W_CONFIG
    he_wr_cfg_.value = 0;
    he_wr_cfg_.line_repeat_count = he_linerep_count_;
    he_wr_cfg_.write_traffic_enable = 1;
    he_wr_cfg_.opcode = WR_LINE_M;

    // set RD_ADDR_TABLE_CTRL
    he_rd_cfg_.value = 0;
    host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);
    rd_table_ctl_.value = 0;
    host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

    // Set WR_ADDR_TABLE_CTRL
    wr_table_ctl_.value = 0;
    if (he_stride_cmd_) {
        wr_table_ctl_.enable_address_stride = 1;
        wr_table_ctl_.stride = he_stride_;
    } else if (he_target_ == HE_TARGET_FPGA) {
        // Set Stride to 3 for Target FPGA Memory
        wr_table_ctl_.enable_address_stride = 1;
        wr_table_ctl_.stride = 3;
    } else {
        wr_table_ctl_.enable_address_stride = 1;
        wr_table_ctl_.stride = he_stride_;
    }

    // continuous mode
    if (he_continuousmode_) {
        he_rd_cfg_.continuous_mode_enable = 0x1;
        host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);
        host_exe_->write64(HE_WR_NUM_LINES, he_cls_count_);
        host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

        // Start test
        he_start_test();

        // Continuous mode
        he_continuousmode();

        // performance
        he_perf_counters();

    } else if (he_latency_iterations_ > 0) {
        // Latency iterations test
        double perf_data = 0;
        double total_perf_data = 0;

        wr_table_ctl_.enable_address_stride = 1;
        wr_table_ctl_.stride = 1;
        host_exe_->write64(HE_WR_NUM_LINES, 1);
        host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);
        host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

        for (uint64_t i = 0; i < he_latency_iterations_; i++) {
            // Start test
            he_start_test();

            // wait for completion
            if (!he_wait_test_completion()) {
                he_perf_counters();
                host_exerciser_errors();
                host_exe_->free_cache_read_write();
                host_exe_->free_dsm();
                return -1;
            }

            if (he_get_perf(&perf_data, NULL)) {
                total_perf_data = total_perf_data + perf_data;
            }
            host_exe_->logger_->info("Iteration: {0} BandWidth: {2:0.3f} GB/s",
                i, perf_data);
        } //end for loop
        host_exe_->logger_->info("Average BandWidth: {0:0.3f} GB/s",
            total_perf_data / he_latency_iterations_);

    } else {
        // fpga Write cache hit test
        host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);
        host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);
        host_exe_->write64(HE_WR_NUM_LINES, he_cls_count_);

        // Start test
        he_start_test();

        // wait for completion
        if (!he_wait_test_completion()) {
            he_perf_counters();
            host_exerciser_errors();
            host_exe_->free_cache_read_write();
            host_exe_->free_dsm();
            return -1;
        }
        he_perf_counters();
    }

    cout << "********** AFU Write to  FPGA Cache  successfully ********** "
         << endl;

    host_exe_->free_cache_read_write();
    host_exe_->free_dsm();

    cout << "********** FPGA Write cache hit test end**********" << endl;

    return 0;
  }

  int he_run_fpga_rd_cache_miss_test() {

    cout << "********** FPGA Read cache miss test start**********" << endl;
    /*
    STEPS
    1) Allocate DSM, Read buffer
    2) Set cache lines and line repeat count
    3) Set Read CXL config
    4) Run test (Buffer is not present in FPGA - FPGA read cache miss)
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_RD_NUM_LINES, he_cls_count_);

    cout << "Read number Lines:" << he_cls_count_ << endl;
    cout << "Line Repeat Count:" << he_linerep_count_ << endl;
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;

    // set RD_CONFIG RdShared (CXL)
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = he_linerep_count_;
    he_rd_cfg_.read_traffic_enable = 1;
    he_rd_cfg_.opcode = RD_LINE_S;

    // set RD_ADDR_TABLE_CTRL
    rd_table_ctl_.value = 0;
    if (he_stride_cmd_) {
        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = he_stride_;
    } else if (he_target_ == HE_TARGET_FPGA) {
        // Set Stride to 3 for Target FPGA Memory
        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = 3;
    } else {
        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = he_stride_;
    }

    // Allocate DSM buffer
    if (!host_exe_->allocate_dsm()) {
      cerr << "allocate dsm failed" << endl;
      return -1;
    }

    // Allocate Read buffer
    if (!host_exe_->allocate_cache_read(BUFFER_SIZE_2MB, numa_node_)) {
      cerr << "allocate cache read failed" << endl;
      host_exe_->free_dsm();
      return -1;
    }

    // continuous mode
    if (he_continuousmode_) {
        he_rd_cfg_.continuous_mode_enable = 0x1;
        host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);
        host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

        // Start test
        he_start_test();

        // Continuous mode
        he_continuousmode();

        // performance
        he_perf_counters(HE_CXL_RD_LATENCY);

    } else if (he_latency_iterations_ > 0) {

        // Latency loop test
        double perf_data = 0;
        double latency = 0;
        double total_perf_data = 0;
        double total_latency = 0;

        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = 1;
        host_exe_->write64(HE_RD_NUM_LINES, 1);
        host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);
        host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

        for (uint64_t i = 0; i < he_latency_iterations_; i++) {
            // Start test
            he_start_test();

            // wait for completion
            if (!he_wait_test_completion()) {
                he_perf_counters();
                host_exerciser_errors();
                host_exe_->free_cache_read();
                host_exe_->free_dsm();
                return -1;
            }

            if (he_get_perf(&perf_data, &latency, HE_CXL_RD_LATENCY)) {
                total_perf_data = total_perf_data + perf_data;
                total_latency = total_latency + latency;
            }
            host_exe_->logger_->info("Iteration: {0}  latency: {1:0.3f} nanoseconds \
                     BandWidth: {2:0.3f} GB/s", i, latency, perf_data);
        } // end for loop
        host_exe_->logger_->info("Average Latency: {0:0.3f} nanoseconds",
            total_latency / he_latency_iterations_);
        host_exe_->logger_->info("Average BandWidth: {0:0.3f} GB/s",
            total_perf_data / he_latency_iterations_);

    } else {
        // fpga read cache hit test
        host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);
        host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);

        // Start test
        he_start_test();

        // wait for completion
        if (!he_wait_test_completion()) {
            he_perf_counters();
            host_exerciser_errors();
            host_exe_->free_cache_read();
            host_exe_->free_dsm();
            return -1;
        }
        he_perf_counters(HE_CXL_RD_LATENCY);
    }

    host_exe_->free_cache_read();
    host_exe_->free_dsm();

    cout << "********** AFU Read FPGA Cache Miss successfully ********** "
         << endl;
    cout << "********** FPGA Read cache miss test end**********" << endl;
    return 0;
  }

  int he_run_fpga_wr_cache_miss_test() {

    cout << "********** FPGA write cache miss test start**********" << endl;
    /*
    STEPS
    1) Allocate DSM, Write buffer
    2) Set cache lines and line repeat count
    3) Set Write CXL config
    4) Run test ( Buffer is not present in FPGA - FPGA write cache miss)
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_WR_NUM_LINES, he_cls_count_);

    cout << "Read/write number Lines:" << he_cls_count_ << endl;
    cout << "Line Repeat Count:" << he_linerep_count_ << endl;
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
        << endl;

    // set Write config
    he_wr_cfg_.value = 0;
    he_wr_cfg_.line_repeat_count = he_linerep_count_;
    he_wr_cfg_.write_traffic_enable = 1;
    he_wr_cfg_.opcode = WR_LINE_I;

    // Set WR_ADDR_TABLE_CTRL
    wr_table_ctl_.value = 0;
    if (he_stride_cmd_) {
        wr_table_ctl_.enable_address_stride = 1;
        wr_table_ctl_.stride = he_stride_;
    } else if (he_target_ == HE_TARGET_FPGA) {
        // Set Stride to 3 for Target FPGA Memory
        wr_table_ctl_.enable_address_stride = 1;
        wr_table_ctl_.stride = 3;
    } else {
        wr_table_ctl_.enable_address_stride = 1;
        wr_table_ctl_.stride = he_stride_;
    }

    // Allocate DSM buffer
    if (!host_exe_->allocate_dsm()) {
      cerr << "allocate dsm failed" << endl;
      return -1;
    }

    // Allocate Read, Write buffer
    if (!host_exe_->allocate_cache_write(BUFFER_SIZE_2MB, numa_node_)) {
      cerr << "allocate cache read failed" << endl;
      host_exe_->free_dsm();
      return -1;
    }

    // continuous mode
    if (he_continuousmode_) {
        he_rd_cfg_.continuous_mode_enable = 0x1;
        host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);
        host_exe_->write64(HE_WR_NUM_LINES, he_cls_count_);
        host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

        // Start test
        he_start_test();

        // Continuous mode
        he_continuousmode();

        // performance
        he_perf_counters();

    } else if (he_latency_iterations_ > 0) {
        // Latency loop test
        double perf_data = 0;
        double total_perf_data = 0;

        wr_table_ctl_.enable_address_stride = 1;
        wr_table_ctl_.stride = 1;
        host_exe_->write64(HE_WR_NUM_LINES, 1);
        host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);
        host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

        for (uint64_t i = 0; i < he_latency_iterations_; i++) {
            // Start test
            he_start_test();

            // wait for completion
            if (!he_wait_test_completion()) {
                he_perf_counters();
                host_exerciser_errors();
                host_exe_->free_cache_write();
                host_exe_->free_dsm();
                return -1;
            }

            if (he_get_perf(&perf_data, NULL)) {
                total_perf_data = total_perf_data + perf_data;
            }
            host_exe_->logger_->info("Iteration: {0} BandWidth: {2:0.3f} GB/s",
                i, perf_data);
        } //end for loop
        host_exe_->logger_->info("Average BandWidth: {0:0.3f} GB/s",
            total_perf_data / he_latency_iterations_);

    } else {
        // fpga Write cache hit test
        host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);
        host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);
        host_exe_->write64(HE_WR_NUM_LINES, he_cls_count_);

        // Start test
        he_start_test();

        // wait for completion
        if (!he_wait_test_completion()) {
            he_perf_counters();
            host_exerciser_errors();
            host_exe_->free_cache_write();
            host_exe_->free_dsm();
            return -1;
        }
        he_perf_counters();
    }

    host_exe_->free_cache_write();
    host_exe_->free_dsm();

    cout << "********** AFU Write FPGA Cache Miss successfully ********** "
         << endl;
    cout << "********** FPGA Write cache miss test end**********" << endl;
    return 0;
  }

  int he_run_host_rd_cache_hit_test() {

    cout << "**********  Host LLC Read cache hit test start**********" << endl;
    /*
    STEPS
    1) Allocate DSM, Read buffer
    2) Create thread read buffer (move cache lines to HOST LLC)
    3) Set Read CXL config
    4) Run test (AFU reads from host cache to FPGA cache)
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_RD_NUM_LINES, he_cls_count_);

    cout << "Read number Lines:" << he_cls_count_ << endl;
    cout << "Line Repeat Count:" << he_linerep_count_ << endl;
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
        << endl;

    // set RD_CONFIG RdShared (CXL)
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = he_linerep_count_;
    he_rd_cfg_.read_traffic_enable = 1;
    he_rd_cfg_.opcode = RD_LINE_I;
    host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);

    // set RD_ADDR_TABLE_CTRL
    rd_table_ctl_.value = 0;
    rd_table_ctl_.enable_address_stride = 1;
    rd_table_ctl_.stride = he_stride_;
    host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

    // Allocate DSM buffer
    if (!host_exe_->allocate_dsm()) {
      cerr << "alloc dsm failed" << endl;
      return -1;
    }

    // Allocate Read buffer
    if (!host_exe_->allocate_cache_read(BUFFER_SIZE_2MB, numa_node_)) {
      cerr << "allocate cache read failed" << endl;
      host_exe_->free_dsm();
      return -1;
    }

    cout << " create thread - moves read buffer to host cache " << endl;
    std::thread t1(he_cache_thread, host_exe_->get_read(), BUFFER_SIZE_32KB);
    sleep(1);


    // continuous mode
    if (he_continuousmode_) {
        he_rd_cfg_.continuous_mode_enable = 0x1;
        host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);
        host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

        // Start test
        he_start_test();

        // Continuous mode
        he_continuousmode();

        // performance
        he_perf_counters(HE_CXL_RD_LATENCY);

    } else if (he_latency_iterations_ > 0) {
        // Latency loop test
        double perf_data = 0;
        double latency = 0;
        double total_perf_data = 0;
        double total_latency = 0;

        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = 1;

        host_exe_->write64(HE_RD_NUM_LINES, 1);
        host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);
        host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

        for (uint64_t i = 0; i < he_latency_iterations_; i++) {
            // Start test
            he_start_test();

            // wait for completion
            if (!he_wait_test_completion()) {
                he_perf_counters();
                host_exerciser_errors();
                g_stop_thread = true;
                t1.join();
                sleep(1);
                host_exe_->free_cache_read();
                host_exe_->free_dsm();
                return -1;
            }

            if (he_get_perf(&perf_data, &latency, HE_CXL_RD_LATENCY)) {
                total_perf_data = total_perf_data + perf_data;
                total_latency = total_latency + latency;
            }
            host_exe_->logger_->info("Iteration: {0}  latency: {1:0.3f} nanoseconds \
                     BandWidth: {2:0.3f} GB/s", i, latency, perf_data);
        } //end for loop
        host_exe_->logger_->info("Average Latency: {0:0.3f} nanoseconds",
            total_latency / he_latency_iterations_);
        host_exe_->logger_->info("Average BandWidth: {0:0.3f} GB/s",
            total_perf_data / he_latency_iterations_);

    } else {
        // fpga read cache hit test
        host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);
        host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);

        // Start test
        he_start_test();

        // wait for completion
        if (!he_wait_test_completion()) {
            he_perf_counters();
            host_exerciser_errors();
            g_stop_thread = true;
            t1.join();
            sleep(1);
            host_exe_->free_cache_read();
            host_exe_->free_dsm();
            return -1;
        }
        he_perf_counters(HE_CXL_RD_LATENCY);
    }

    g_stop_thread = true;
    t1.join();

    sleep(1);
    host_exe_->free_cache_read();
    host_exe_->free_dsm();

    cout << "********** AFU Copied host cache to FPGA Cache successfully "
            "********** " << endl;
    cout << "********** Host LLC cache hit test end**********" << endl;
    return 0;
  }

  int he_run_host_wr_cache_hit_test() {

    cout << "********** Host LLC Write cache hit test start**********" << endl;
    /*
    STEPS
    1) Allocate DSM, Write buffer
    2) Create thread read buffer (move cache lines to HOST LLC)
    3) Set Write CXL config
    4) Run test (AFU write to host cache)
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);

    host_exe_->write64(HE_WR_NUM_LINES, he_cls_count_);
    cout << "Write number Lines:" << he_cls_count_ << endl;
    cout << "Line Repeat Count:" << he_linerep_count_ << endl;
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
        << endl;

    // set RD_CONFIG
    he_wr_cfg_.value = 0;
    he_wr_cfg_.line_repeat_count = he_linerep_count_;
    he_wr_cfg_.write_traffic_enable = 1;
    he_wr_cfg_.opcode = WR_PUSH_I;
    host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);

    // set RD_ADDR_TABLE_CTRL
    wr_table_ctl_.value = 0;
    wr_table_ctl_.enable_address_stride = 1;
    wr_table_ctl_.stride = he_stride_;
    host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

    // Allocate DSM buffer
    if (!host_exe_->allocate_dsm()) {
      cerr << "alloc dsm failed" << endl;
      return -1;
    }

    // Allocate Read buffer
    if (!host_exe_->allocate_cache_write(BUFFER_SIZE_2MB, numa_node_)) {
      cerr << "allocate cache read failed" << endl;
      host_exe_->free_dsm();
      return -1;
    }

    cout << " create thread - moves read buffer to host cache " << endl;
    std::thread t1(he_cache_thread, host_exe_->get_write(), BUFFER_SIZE_32KB);
    sleep(1);

    // continuous mode
    if (he_continuousmode_) {
        he_rd_cfg_.continuous_mode_enable = 0x1;
        host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);
        host_exe_->write64(HE_WR_NUM_LINES, he_cls_count_);
        host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

        // Start test
        he_start_test();

        // Continuous mode
        he_continuousmode();

        // performance
        he_perf_counters();

    } else if (he_latency_iterations_ > 0) {
        // Latency loop test
        double perf_data = 0;
        double total_perf_data = 0;

        wr_table_ctl_.enable_address_stride = 1;
        wr_table_ctl_.stride = 1;
        host_exe_->write64(HE_WR_NUM_LINES, 1);
        host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);
        host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

        for (uint64_t i = 0; i < he_latency_iterations_; i++) {
            // Start test
            he_start_test();

            // wait for completion
            if (!he_wait_test_completion()) {
                he_perf_counters();
                host_exerciser_errors();
                g_stop_thread = true;
                t1.join();
                sleep(1);
                host_exe_->free_cache_write();
                host_exe_->free_dsm();
                return -1;
            }

            if (he_get_perf(&perf_data, NULL)) {
                total_perf_data = total_perf_data + perf_data;
            }
            host_exe_->logger_->info("Iteration: {0} BandWidth: {2:0.3f} GB/s",
                i, perf_data);
        } // end for loop
        host_exe_->logger_->info("Average BandWidth: {0:0.3f} GB/s",
            total_perf_data / he_latency_iterations_);

    } else {
        // fpga Write cache hit test
        host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);
        host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);
        host_exe_->write64(HE_WR_NUM_LINES, he_cls_count_);

        // Start test
        he_start_test();

        // wait for completion
        if (!he_wait_test_completion()) {
            he_perf_counters();
            host_exerciser_errors();
            g_stop_thread = true;
            t1.join();
            sleep(1);
            host_exe_->free_cache_write();
            host_exe_->free_dsm();
            return -1;
        }
        he_perf_counters();
    }

    g_stop_thread = true;
    t1.join();
    cout << "********** AFU write  host cache successfully ********** " << endl;

    sleep(1);
    host_exe_->free_cache_write();
    host_exe_->free_dsm();

    cout << "********** Host LLC cache hit Write test end**********" << endl;
    return 0;
  }

  int he_run_host_rd_cache_miss_test() {

    cout << "********** Host LLC Read cache miss test start**********" << endl;
    /*
    STEPS
    1) Allocate DSM, Read buffer
    2) Flush host read buffer cache
    3) Set read CXL config
    4) Run test (AFU reads from host cache(cache lines are not in host LLC))
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_RD_NUM_LINES, he_cls_count_);
    cout << "Read/write number Lines:" << he_cls_count_ << endl;
    cout << "Line Repeat Count:" << he_linerep_count_ << endl;
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
        << endl;

    // set RD_CONFIG
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = he_linerep_count_;
    he_rd_cfg_.read_traffic_enable = 1;
    he_rd_cfg_.opcode = RD_LINE_I;
    host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);

    // set RD_ADDR_TABLE_CTR
    rd_table_ctl_.value = 0;
    rd_table_ctl_.enable_address_stride = 1;
    rd_table_ctl_.stride = he_stride_;
    host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

    // Allocate DSM buffer
    if (!host_exe_->allocate_dsm()) {
      cerr << "alloc dsm failed" << endl;
      return -1;
    }

    // Allocate Read buffer
    if (!host_exe_->allocate_cache_read(BUFFER_SIZE_2MB, numa_node_)) {
      cerr << "allocate cache read failed" << endl;
      host_exe_->free_dsm();
      return -1;
    }

    // continuous mode
    if (he_continuousmode_) {
        he_rd_cfg_.continuous_mode_enable = 0x1;
        host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);
        host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

        // Start test
        he_start_test();

        // Continuous mode
        he_continuousmode();

        // performance
        he_perf_counters(HE_CXL_RD_LATENCY);

    } else if (he_latency_iterations_ > 0) {
        // Latency loop test
        double perf_data = 0;
        double latency = 0;
        double total_perf_data = 0;
        double total_latency = 0;

        rd_table_ctl_.enable_address_stride = 1;
        rd_table_ctl_.stride = 1;

        host_exe_->write64(HE_RD_NUM_LINES, 1);
        host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);
        host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

        for (uint64_t i = 0; i < he_latency_iterations_; i++) {
            // Start test
            he_start_test();

            // wait for completion
            if (!he_wait_test_completion()) {
                he_perf_counters();
                host_exerciser_errors();
                host_exe_->free_cache_read();
                host_exe_->free_dsm();
                return -1;
            }

            if (he_get_perf(&perf_data, &latency, HE_CXL_RD_LATENCY)) {
                total_perf_data = total_perf_data + perf_data;
                total_latency = total_latency + latency;
            }
            host_exe_->logger_->info("Iteration: {0}  latency: {1:0.3f} nanoseconds \
                     BandWidth: {2:0.3f} GB/s", i, latency, perf_data);
        } //end for loop
        host_exe_->logger_->info("Average Latency: {0:0.3f} nanoseconds",
            total_latency / he_latency_iterations_);
        host_exe_->logger_->info("Average BandWidth: {0:0.3f} GB/s",
            total_perf_data / he_latency_iterations_);

    } else {
        // fpga read cache hit test
        host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);
        host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);

        // Start test
        he_start_test();

        // wait for completion
        if (!he_wait_test_completion()) {
            he_perf_counters();
            host_exerciser_errors();
            host_exe_->free_cache_read();
            host_exe_->free_dsm();
            return -1;
        }
        he_perf_counters(HE_CXL_RD_LATENCY);
    }

    host_exe_->free_cache_read();
    host_exe_->free_dsm();

    cout << "********** Ran  Host LLC Read cache miss successfully ********** "
         << endl;

    cout << "********** Host LLC Read cache miss test end**********" << endl;
    return 0;
  }

  int he_run_host_wr_cache_miss_test() {

    cout << "********** Host LLC Write cache miss test start**********" << endl;
    /*
    STEPS
    1) Allocate DSM, write buffer
    2) Flush host write buffer cache
    3) Set write CXL config
    4) Run test (AFU writes host memory (cache lines are not in host LLC))
    */

    // HE_INFO
    // Set write number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_WR_NUM_LINES, he_cls_count_);
    cout << "Write number Lines:" << he_cls_count_ << endl;
    cout << "Line Repeat Count:" << he_linerep_count_ << endl;
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
        << endl;

    // set RD_CONFIG
    he_wr_cfg_.value = 0;
    he_wr_cfg_.line_repeat_count = he_linerep_count_;
    he_wr_cfg_.write_traffic_enable = 1;
    he_wr_cfg_.opcode = WR_PUSH_I;
    host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);

    // set RD_ADDR_TABLE_CTR
    wr_table_ctl_.value = 0;
    wr_table_ctl_.enable_address_stride = 1;
    wr_table_ctl_.stride = he_stride_;
    host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, rd_table_ctl_.value);

    // Allocate DSM buffer
    if (!host_exe_->allocate_dsm()) {
      cerr << "alloc dsm failed" << endl;
      return -1;
    }

    // Allocate write buffer
    if (!host_exe_->allocate_cache_write(BUFFER_SIZE_2MB, numa_node_)) {
      cerr << "allocate cache read failed" << endl;
      host_exe_->free_dsm();
      return -1;
    }

    // continuous mode
    if (he_continuousmode_) {
        he_rd_cfg_.continuous_mode_enable = 0x1;
        host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);
        host_exe_->write64(HE_WR_NUM_LINES, he_cls_count_);
        host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

        // Start test
        he_start_test();

        // Continuous mode
        he_continuousmode();

        // performance
        he_perf_counters();

    } else if (he_latency_iterations_ > 0) {
        // Latency loop test
        double perf_data = 0;
        double total_perf_data = 0;

        wr_table_ctl_.enable_address_stride = 1;
        wr_table_ctl_.stride = 1;
        host_exe_->write64(HE_WR_NUM_LINES, 1);
        host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);
        host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

        for (uint64_t i = 0; i < he_latency_iterations_; i++) {
            // Start test
            he_start_test();

            // wait for completion
            if (!he_wait_test_completion()) {
                he_perf_counters();
                host_exerciser_errors();
                host_exe_->free_cache_write();
                host_exe_->free_dsm();
                return -1;
            }

            if (he_get_perf(&perf_data, NULL)) {
                total_perf_data = total_perf_data + perf_data;
            }
            host_exe_->logger_->info("Iteration: {0} BandWidth: {2:0.3f} GB/s",
                i, perf_data);
        } // end for loop
        host_exe_->logger_->info("Average BandWidth: {0:0.3f} GB/s",
            total_perf_data / he_latency_iterations_);

    } else {
        // fpga Write cache hit test
        host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);
        host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);
        host_exe_->write64(HE_WR_NUM_LINES, he_cls_count_);

        // Start test
        he_start_test();

        // wait for completion
        if (!he_wait_test_completion()) {
            he_perf_counters();
            host_exerciser_errors();
            host_exe_->free_cache_write();
            host_exe_->free_dsm();
            return -1;
        }
        he_perf_counters();
    }

    host_exe_->free_cache_write();
    host_exe_->free_dsm();

    cout << "********** Ran  Host LLC Write cache miss successfully ********** "
         << endl;

    cout << "********** Host LLC Write cache miss test end**********" << endl;
    return 0;
  }


  void he_forcetestcmpl()
  {
      // Force stop test
      he_ctl_.value = 0;
      he_ctl_.ForcedTestCmpl = 1;
      host_exe_->write64(HE_CTL, he_ctl_.value);

      if (!he_wait_test_completion())
          sleep(1);

      he_ctl_.value = 0;
      host_exe_->write64(HE_CTL, he_ctl_.value);
      usleep(1000);
  }


  bool he_continuousmode()
  {
      uint32_t count = 0;
      if (he_continuousmode_ && he_contmodetime_ > 0)
      {
          host_exe_->logger_->info("continuous mode time: {0} seconds", he_contmodetime_);
          host_exe_->logger_->info("Ctrl+C  to stop continuous mode");

          while (!g_he_exit) {
              sleep(1);
              count++;
              if (count > he_contmodetime_)
                  break;
          }
          he_forcetestcmpl();
      }
      return true;
  }

  virtual int run(test_afu *afu, CLI::App *app) {
    (void)app;
    int ret = 0;

    host_exe_ = dynamic_cast<host_exerciser *>(afu);

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


    if (he_test_ == HE_FPGA_RD_CACHE_HIT) {
      ret = he_run_fpga_rd_cache_hit_test();
      return ret;
    }

    if (he_test_ == HE_FPGA_WR_CACHE_HIT) {
      ret = he_run_fpga_wr_cache_hit_test();
      return ret;
    }

    if (he_test_ == HE_FPGA_RD_CACHE_MISS) {
      ret = he_run_fpga_rd_cache_miss_test();
      return ret;
    }

    if (he_test_ == HE_FPGA_WR_CACHE_MISS) {
      ret = he_run_fpga_wr_cache_miss_test();
      return ret;
    }

    if (he_test_ == HE_HOST_RD_CACHE_HIT) {
      ret = he_run_host_rd_cache_hit_test();
      return ret;
    }

    if (he_test_ == HE_HOST_WR_CACHE_HIT) {
      ret = he_run_host_wr_cache_hit_test();
      return ret;
    }

    if (he_test_ == HE_HOST_RD_CACHE_MISS) {
      ret = he_run_host_rd_cache_miss_test();
      return ret;
    }

    if (he_test_ == HE_HOST_WR_CACHE_MISS) {
      ret = he_run_host_wr_cache_miss_test();
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
  uint32_t he_cls_count_;
  uint64_t he_latency_iterations_;
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

} // end of namespace host_exerciser
