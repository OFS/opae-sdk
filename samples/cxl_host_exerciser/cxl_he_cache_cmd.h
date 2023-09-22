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
        he_stide_(0), he_test_(0), he_test_all_(false) {}

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

    app->add_option("--stride", he_stide_, "Enable stride mode")
        ->default_val("0");

    // Line repeat count
    app->add_option("--linerepcount", he_linerep_count_, "Line repeat count")
        ->transform(CLI::Range(1, 256))
        ->default_val("10");

    // Test all
    app->add_option("--testall", he_test_all_, "Run all tests")
        ->default_val("false");
  }

  int he_run_fpga_rd_cache_hit_test() {
    cout << "********** FPGA Read cache hit test start**********" << endl;
    /*
    STEPS
    1) Allocate DSM, Read buffer // flush
    2) set cache lines 32kb/64
    3) set line repeat count
    4) Set RdShared (CXL) config
    5) Run test ( AFU copies cache from host memory to FPGA cache)
    6) set line repeat count
    7) Set RdShared (CXL) config
    8) Run test ( AFU read cache from FPGA cache)
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_RD_NUM_LINES, FPGA_32KB_CACHE_LINES);

    cout << "Read number Lines:" << FPGA_32KB_CACHE_LINES << endl;
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
    rd_table_ctl_.enable_address_stride = 1;
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
    he_ctl_.Start = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);
    he_ctl_.Start = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    // wait for completion
    if (!he_wait_test_completion()) {
      cerr << "timeout error" << endl;
      he_perf_counters();
      host_exerciser_errors();
      host_exe_->free_cache_read();
      host_exe_->free_dsm();
      return -1;
    }

    he_perf_counters();

    cout << "********** AFU Copied host cache to FPGA Cache successfully "
            "********** " << endl;

    // set RD_CONFIG RdShared (CXL)
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = he_linerep_count_;
    he_rd_cfg_.read_traffic_enable = 1;
    he_rd_cfg_.opcode = RD_LINE_S;
    host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);

    // set RD_ADDR_TABLE_CTRL
    rd_table_ctl_.value = 0;
    rd_table_ctl_.enable_address_stride = 1;
    host_exe_->write64(HE_RD_ADDR_TABLE_CTRL, rd_table_ctl_.value);

    // Start test
    he_ctl_.Start = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);
    he_ctl_.Start = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    // wait for completion
    if (!he_wait_test_completion()) {
      cerr << "timeout error" << endl;
      he_perf_counters();
      host_exerciser_errors();
      host_exe_->free_cache_read();
      host_exe_->free_dsm();
      return -1;
    }

    he_perf_counters();
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
    1) Allocate DSM, Read buffer, Write buffer // flush
    2) set cache lines 32kb/64
    3) set line repeat count
    4) Set RdShared (CXL) config
    5) Run test ( AFU copies cache from host memory to FPGA cache)
    6) set line repeat count
    7) Set WrLine_M/WrPart_M (CXL) config
    8) Run test ( AFU writes to FPGA cache)
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_RD_NUM_LINES, FPGA_32KB_CACHE_LINES);

    cout << "Read/write number Lines:" << FPGA_32KB_CACHE_LINES << endl;
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
    rd_table_ctl_.enable_address_stride = 1;
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
    he_ctl_.Start = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);
    he_ctl_.Start = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    // wait for completion
    if (!he_wait_test_completion()) {
      cerr << "timeout error" << endl;
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
    host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);

    // Set WR_ADDR_TABLE_CTRL
    wr_table_ctl_.value = 0;
    wr_table_ctl_.enable_address_stride = 1;
    host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

    host_exe_->write64(HE_WR_NUM_LINES, FPGA_32KB_CACHE_LINES);
    // Start test
    he_ctl_.Start = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);
    he_ctl_.Start = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    // wait for completion
    if (!he_wait_test_completion()) {
      cerr << "timeout error" << endl;
      he_perf_counters();
      host_exerciser_errors();
      host_exe_->free_cache_read_write();
      host_exe_->free_dsm();
      return -1;
    }

    he_perf_counters();
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
    1) Allocate DSM, Read buffer, Write buffer
    2) Write number of lines more then 32kb 2mb/64
    3) Set RdShared (CXL) config
    4) Run test (Buffer is not present in FPGA - FPGA read Cache miss )
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_RD_NUM_LINES, FPGA_2MB_CACHE_LINES);

    cout << "Read number Lines:" << FPGA_2MB_CACHE_LINES << endl;
    cout << "Line Repeat Count:" << he_linerep_count_ << endl;
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;

    // set RD_CONFIG RdShared (CXL)
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = he_linerep_count_;
    he_rd_cfg_.read_traffic_enable = 1;
    he_rd_cfg_.opcode = RD_LINE_S;
    host_exe_->write64(HE_RD_CONFIG, he_rd_cfg_.value);

    // set RD_ADDR_TABLE_CTRL
    rd_table_ctl_.value = 0;
    rd_table_ctl_.enable_address_stride = 1;
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

    // start test
    he_ctl_.Start = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);
    he_ctl_.Start = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    // wait for completion
    if (!he_wait_test_completion()) {
      cerr << "timeout error" << endl;
      he_perf_counters();
      host_exerciser_errors();
      host_exe_->free_cache_read();
      host_exe_->free_dsm();
      return -1;
    }

    he_perf_counters();
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
    1) Allocate DSM, Read buffer, Write buffer
    2) Write number of lines more then 32 kb  2mb/64
    3) Set WR ItoMWr (CXL) config
    4) Run test ( Buffer is not present in FPGA - FPGA write Cache miss )
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_WR_NUM_LINES, FPGA_2MB_CACHE_LINES);

    cout << "Read/write number Lines:" << FPGA_2MB_CACHE_LINES << endl;
    cout << "Line Repeat Count:" << he_linerep_count_ << endl;
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
        << endl;

    // set W_CONFIG
    he_wr_cfg_.value = 0;
    he_wr_cfg_.line_repeat_count = he_linerep_count_;
    he_wr_cfg_.write_traffic_enable = 1;
    he_wr_cfg_.opcode = WR_LINE_M;
    host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);

    // Set WR_ADDR_TABLE_CTRL
    wr_table_ctl_.value = 0;
    wr_table_ctl_.enable_address_stride = 1;
    host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

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

    // start test
    he_ctl_.Start = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);
    he_ctl_.Start = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    // wait for completion
    if (!he_wait_test_completion()) {
      cerr << "timeout error" << endl;
      he_perf_counters();
      host_exerciser_errors();
      host_exe_->free_cache_read_write();
      host_exe_->free_dsm();
      return -1;
    }

    he_perf_counters();
    host_exe_->free_cache_read_write();
    host_exe_->free_dsm();

    cout << "********** AFU Write FPGA Cache Miss successfully ********** "
         << endl;
    cout << "********** FPGA Write cache miss test end**********" << endl;
    return 0;
  }

  int he_run_host_rd_cache_hit_test() {

      cout << "********** 1 Host LLC Read cache hit test start**********" << endl;
    /*
    STEPS
    1) Allocate DSM, Read buffer
    2) create thread read buffer
    3) Set RdLine_I (CXL) config
    4) Run test ( AFU reads from host cache to FPGA cache)
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_RD_NUM_LINES, FPGA_32KB_CACHE_LINES);

    cout << "Read number Lines:" << FPGA_32KB_CACHE_LINES << endl;
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
    std::thread t1(he_cache_thread, host_exe_->get_read(), BUFFER_SIZE_2MB);
    sleep(1);

    // start
    he_ctl_.Start = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);
    he_ctl_.Start = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    // wait for completion
    if (!he_wait_test_completion()) {
      cerr << "timeout error" << endl;
      he_perf_counters();
      host_exerciser_errors();

      g_stop_thread = true;
      t1.join();
      sleep(1);
      host_exe_->free_cache_read();
      host_exe_->free_dsm();
      return -1;
    }

    g_stop_thread = true;
    t1.join();

    he_perf_counters();
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
    2) create thread read buffer
    3) Set ItoMWr (CXL) config
    4) Run test ( AFU write to host cache)
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);

    host_exe_->write64(HE_WR_NUM_LINES, FPGA_32KB_CACHE_LINES);
    cout << "Write number Lines:" << FPGA_32KB_CACHE_LINES  << endl;
    cout << "Line Repeat Count:" << he_linerep_count_ << endl;
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
        << endl;

    // set RD_CONFIG
    he_wr_cfg_.value = 0;
    he_wr_cfg_.line_repeat_count = he_linerep_count_;
    he_wr_cfg_.write_traffic_enable = 1;
    he_wr_cfg_.opcode = WR_LINE_I;
    host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);

    // set RD_ADDR_TABLE_CTRL
    wr_table_ctl_.value = 0;
    wr_table_ctl_.enable_address_stride = 1;
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
    std::thread t1(he_cache_thread, host_exe_->get_write(), BUFFER_SIZE_2MB);
    sleep(1);

    // start
    he_ctl_.Start = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);
    he_ctl_.Start = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    // wait for completion
    if (!he_wait_test_completion()) {
      cerr << "timeout error" << endl;

      he_perf_counters();
      host_exerciser_errors();
      g_stop_thread = true;
      t1.join();
      sleep(1);
      host_exe_->free_cache_write();
      host_exe_->free_dsm();
      return -1;
    }

    g_stop_thread = true;
    t1.join();
    he_perf_counters();
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
    2) flush host read buffer cachde
    3) Set RdLine_I (CXL) config
    4) Run test ( AFU reads from host cache to FPGA cache)
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_RD_NUM_LINES, FPGA_32KB_CACHE_LINES - 1);
    cout << "Read/write number Lines:" << FPGA_32KB_CACHE_LINES - 1 << endl;
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

    // start
    he_ctl_.Start = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);
    he_ctl_.Start = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    // wait for completion
    if (!he_wait_test_completion()) {
      cerr << "timeout error" << endl;
      he_perf_counters();
      host_exerciser_errors();
      host_exe_->free_cache_read();
      host_exe_->free_dsm();
      return -1;
    }

    he_perf_counters();
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
    2) flush host write buffer cachde
    3) Set RdLine_I (CXL) config
    4) Run test ( AFU reads from host cache to FPGA cache)
    */

    // HE_INFO
    // Set write number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    host_exe_->write64(HE_WR_NUM_LINES, 1);
    cout << "Write number Lines:" << 1 << endl;
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

    // start
    he_ctl_.Start = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);
    he_ctl_.Start = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    // wait for completion
    if (!he_wait_test_completion()) {
      cerr << "timeout error" << endl;
      he_perf_counters();
      host_exerciser_errors();
      host_exe_->free_cache_write();
      host_exe_->free_dsm();
      return -1;
    }

    he_perf_counters();
    host_exe_->free_cache_write();
    host_exe_->free_dsm();

    cout << "********** Ran  Host LLC Write cache miss successfully ********** "
         << endl;

    cout << "********** Host LLC Write cache miss test end**********" << endl;
    return 0;
  }

  virtual int run(test_afu *afu, CLI::App *app) {
    (void)app;
    int ret = 0;

    host_exe_ = dynamic_cast<host_exerciser *>(afu);

    if (!verify_numa_node()) {
      numa_node_ = 0;
      cout << "numa nodes are available set numa node to 0" << endl;
    };

    // reset HE cache
    he_ctl_.value = 0;
    he_ctl_.ResetL = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    he_ctl_.value = 0;
    he_ctl_.ResetL = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    if (he_test_all_ == true) {
      int retvalue = 0;
      ret = he_run_fpga_rd_cache_hit_test();
      if (ret != 0) {
        retvalue = ret;
      }
      ret = he_run_fpga_wr_cache_hit_test();
      if (ret != 0) {
        retvalue = ret;
      }

      ret = he_run_fpga_rd_cache_miss_test();
      if (ret != 0) {
        retvalue = ret;
      }
      ret = he_run_fpga_wr_cache_miss_test();
      if (ret != 0) {
        retvalue = ret;
      }
      ret = he_run_host_rd_cache_hit_test();
      if (ret != 0) {
        retvalue = ret;
      }
      ret = he_run_host_wr_cache_hit_test();
      if (ret != 0) {
        retvalue = ret;
      }

      ret = he_run_host_rd_cache_miss_test();
      if (ret != 0) {
        retvalue = ret;
      }
      ret = he_run_host_wr_cache_miss_test();
      if (ret != 0) {
        retvalue = ret;
      }

      return retvalue;
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
  uint32_t he_stide_;
  uint32_t he_target_;
  uint32_t he_test_;
  bool he_test_all_;
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
