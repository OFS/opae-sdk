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

#include "cxl_host_exerciser.h"
#include "he_cache_test.h"
#include <map>
#include <numa.h>
#include <unistd.h>

using test_afu = opae::afu_test::afu;
using opae::fpga::types::shared_buffer;
using opae::fpga::types::token;
namespace fpga = opae::fpga::types;

#define UNUSED_PARAM(x) ((void)x)

// HE exit global flag
volatile bool g_he_exit = false;
volatile static bool g_stop_thread = false;

// host exerciser signal handler
void he_sig_handler(int) {
  g_he_exit = true;
  g_stop_thread = true;
  printf("HE signal handler exit app \n");
}

namespace host_exerciser {

std::mutex he_cache_read_mutex;
std::mutex he_cache_write_mutex;

class host_exerciser_cmd;

void he_cache_thread(uint8_t *buf_ptr, uint64_t len);

class host_exerciser_cmd : public test_command {
public:
  host_exerciser_cmd() : host_exe_(NULL), numa_node_(0) {}
  virtual ~host_exerciser_cmd() {}

  int he_run_fpga_rd_cache_hit_test() {
    cout << "********** FPGA Read cache hit test start**********" << endl;
    /*
    STEPS
    1) Allocate DSM, Read buffer // flush
    2) set cache lines 32kb/64
    3) set loop count
    4) Set RdShared (CXL) config
    5) Run test ( AFU copies cache from host memory to FPGA cache)
    6) Set RdShared (CXL) config
    5) Run test ( AFU read cache from FPGA cache)
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;

    cout << "Numa node:" << numa_node_ << endl;
    host_exe_->write64(HE_RD_NUM_LINES, FPGA_32KB_CACHE_LINES - 1);
    cout << "Read number Lines:" << FPGA_32KB_CACHE_LINES - 1 << endl;
    cout << "Line Repeat Count:" << host_exe_->he_linerep_count_ << endl;

    // set RD_CONFIG RdShared (CXL)
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = host_exe_->he_linerep_count_;
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
            "********** "
         << endl;

    // set RD_CONFIG RdShared (CXL)
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = host_exe_->he_linerep_count_;
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

    cout
        << "********** AFU reads cache from FPGA Cache successfully ********** "
        << endl;

    cout << "********** FPGA Read cache hit test end**********" << endl;
    return 0;
  }

  int he_run_fpga_wr_cache_hit_test() {
    cout << "********** FPGA Write cache hit test start**********" << endl;

    /*
    STEPS
    1) Allocate DSM, Read buffer, Write buffer // flush
    2) set cache lines 32kb/64
    3) set loop count
    4) Set RdShared (CXL) config
    5) Run test ( AFU copies cache from host memory to FPGA cache)
    6) Set WrLine_M/WrPart_M (CXL) config
    5) Run test ( AFU writes to FPGA cache)
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
         << endl;

    host_exe_->write64(HE_RD_NUM_LINES, FPGA_32KB_CACHE_LINES - 1);
    host_exe_->write64(HE_WR_NUM_LINES, FPGA_32KB_CACHE_LINES - 1);
    cout << "Read/write number Lines:" << FPGA_32KB_CACHE_LINES - 1 << endl;
    cout << "Line Repeat Count:" << host_exe_->he_linerep_count_ << endl;

    // set RD_CONFIG RdShared (CXL)
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = host_exe_->he_linerep_count_;
    he_rd_cfg_.read_traffic_enable = 1;
    he_rd_cfg_.opcode = RD_LINE_S;
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
            "********** "
         << endl;

    // set W_CONFIG
    he_wr_cfg_.value = 0;
    he_wr_cfg_.line_repeat_count = host_exe_->he_linerep_count_;
    he_wr_cfg_.write_traffic_enable = 1;
    he_wr_cfg_.opcode = WR_LINE_M;
    host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);

    // Set WR_ADDR_TABLE_CTRL
    wr_table_ctl_.value = 0;
    wr_table_ctl_.enable_address_stride = 1;
    host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

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
    2) Write number of lines more then 32 kb  2mb/64
    3) Set RdShared (CXL) config
    4) Run test (Buffer is not present in FPGA - FPGA read Cache miss )

   // 2) Set RdShared (CXL) config
    //3) Run test ( AFU copies cache from host memory to FPGA cache)
    //4) Set write Evict (CXL) config
    //5) Run test ( AFU Invalidate to FPGA cache)
    3) Set RdShared (CXL) config
    4) Run test (Buffer is not present in FPGA - FPGA read Cache miss )
    */

    // 2MB / 64

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;

    host_exe_->write64(HE_RD_NUM_LINES, FPGA_2MB_CACHE_LINES - 1);
    cout << "Read number Lines:" << FPGA_2MB_CACHE_LINES - 1 << endl;
    cout << "Line Repeat Count:" << host_exe_->he_linerep_count_ << endl;

    // set RD_CONFIG RdShared (CXL)
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = host_exe_->he_linerep_count_;
    he_rd_cfg_.read_traffic_enable = 1;
    he_rd_cfg_.opcode = RD_LINE_S;
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

    // Allocate Read, Write buffer
    if (!host_exe_->allocate_cache_read_write(BUFFER_SIZE_2MB, numa_node_)) {
      cerr << "allocate cache read write failed" << endl;
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

    //2) Set RdShared (CXL) config
    //3) Run test ( AFU copies cache from host to HDM
    //4) Set write Evict  (CXL) config
    //5) Run test ( AFU Invalidate to FPGA cache)
    6) Set WR ItoMWr (CXL) config
    7) Run test ( Buffer is not present in FPGA - FPGA write Cache miss )
    */

    // HE_INFO
    // Set Read number Lines
    he_info_.value = host_exe_->read64(HE_INFO);
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
         << endl;

    host_exe_->write64(HE_WR_NUM_LINES, FPGA_2MB_CACHE_LINES - 1);
    cout << "Read/write number Lines:" << FPGA_2MB_CACHE_LINES - 1 << endl;
    cout << "Line Repeat Count:" << host_exe_->he_linerep_count_ << endl;

    // set W_CONFIG
    he_wr_cfg_.value = 0;
    he_wr_cfg_.line_repeat_count = host_exe_->he_linerep_count_;
    he_wr_cfg_.write_traffic_enable = 1;
    he_wr_cfg_.opcode = WR_LINE_M;
    host_exe_->write64(HE_WR_CONFIG, he_wr_cfg_.value);

    // Set WR_ADDR_TABLE_CTRL
    wr_table_ctl_.value = 0;
    wr_table_ctl_.enable_address_stride = 1;
    host_exe_->write64(HE_WR_ADDR_TABLE_CTRL, wr_table_ctl_.value);

    // Allocate DSM buffer
    if (!host_exe_->allocate_dsm()) {
      cerr << "alloc dsm failed" << endl;
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
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
         << endl;

    host_exe_->write64(HE_RD_NUM_LINES, FPGA_32KB_CACHE_LINES - 1);
    cout << "Read number Lines:" << FPGA_32KB_CACHE_LINES - 1 << endl;
    cout << "Line Repeat Count:" << host_exe_->he_linerep_count_ << endl;

    // set RD_CONFIG RdShared (CXL)
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = host_exe_->he_linerep_count_;
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

    he_perf_counters();

    g_stop_thread = true;
    t1.join();

    he_perf_counters();
    sleep(1);
    host_exe_->free_cache_read();
    host_exe_->free_dsm();

    cout << "********** AFU Copied host cache to FPGA Cache successfully "
            "********** "
         << endl;

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
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
         << endl;

    host_exe_->write64(HE_WR_NUM_LINES, FPGA_32KB_CACHE_LINES - 1);
    cout << "Write number Lines:" << FPGA_32KB_CACHE_LINES - 1 << endl;
    cout << "Line Repeat Count:" << host_exe_->he_linerep_count_ << endl;

    // set RD_CONFIG
    he_wr_cfg_.value = 0;
    he_wr_cfg_.line_repeat_count = host_exe_->he_linerep_count_;
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
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
         << endl;

    host_exe_->write64(HE_RD_NUM_LINES, FPGA_32KB_CACHE_LINES - 1);
    cout << "Read/write number Lines:" << FPGA_32KB_CACHE_LINES - 1 << endl;
    cout << "Line Repeat Count:" << host_exe_->he_linerep_count_ << endl;

    // set RD_CONFIG
    he_rd_cfg_.value = 0;
    he_rd_cfg_.line_repeat_count = host_exe_->he_linerep_count_;
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

    // flush host cache
    // int status = cacheflush((host_exe_->get_read(), BUFFER_SIZE_2MB, BCACHE);

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
    cout << "Read address table size:" << he_info_.read_addr_table_size << endl;
    cout << "Write address table size:" << he_info_.write_addr_table_size
         << endl;

    host_exe_->write64(HE_WR_NUM_LINES, FPGA_32KB_CACHE_LINES - 1);
    cout << "Write number Lines:" << FPGA_32KB_CACHE_LINES - 1 << endl;
    cout << "Line Repeat Count:" << host_exe_->he_linerep_count_ << endl;

    // set RD_CONFIG
    he_wr_cfg_.value = 0;
    he_wr_cfg_.line_repeat_count = host_exe_->he_linerep_count_;
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

    // Allocate Read buffer
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

  void he_perf_counters() {
    volatile he_cache_dsm_status *dsm_status = NULL;

    dsm_status = reinterpret_cast<he_cache_dsm_status *>(
        (uint8_t *)(host_exe_->get_dsm()));
    if (!dsm_status)
      return;

    std::cout << "\n********* DSM Status CSR Start *********" << std::endl;

    std::cout << "test completed :" << dsm_status->test_completed << std::endl;
    std::cout << "dsm number:" << dsm_status->dsm_number << std::endl;
    std::cout << "error vector:" << dsm_status->err_vector << std::endl;
    std::cout << "num ticks:" << dsm_status->num_ticks << std::endl;
    std::cout << "num reads:" << dsm_status->num_reads << std::endl;
    std::cout << "num writes:" << dsm_status->num_writes << std::endl;
    std::cout << "penalty start:" << dsm_status->penalty_start << std::endl;
    std::cout << "penalty end:" << dsm_status->penalty_end << std::endl;
    std::cout << "actual data:" << dsm_status->actual_data << std::endl;
    std::cout << "expected data:" << dsm_status->expected_data << std::endl;

    std::cout << "********* DSM Status CSR end *********" << std::endl;
  }

  void host_exerciser_errors() {
    he_err_status err_status;
    uint64_t err = 0;
    if (host_exe_ == NULL)
      return;

    err_status.value = host_exe_->read64(HE_ERROR_STATUS);
    if (err_status.data_error == 1) {
      cout << "Data Integrity Check error occured" << endl;
    }

    if (err_status.err_index > 0) {
      cout << "Error occurred at cache line address:" << err_status.err_index
           << endl;
    }

    err = host_exe_->read64(HE_ERROR_EXP_DATA);
    cout << "Error Expected Data:" << err << endl;

    err = host_exe_->read64(HE_ERROR_ACT_DATA0);
    cout << "Error Expected Data0:" << err << endl;

    err = host_exe_->read64(HE_ERROR_ACT_DATA1);
    cout << "Error Expected Data1:" << err << endl;

    err = host_exe_->read64(HE_ERROR_ACT_DATA2);
    cout << "Error Expected Data2:" << err << endl;

    err = host_exe_->read64(HE_ERROR_ACT_DATA3);
    cout << "Error Expected Data3:" << err << endl;

    err = host_exe_->read64(HE_ERROR_ACT_DATA4);
    cout << "Error Expected Data4:" << err << endl;

    err = host_exe_->read64(HE_ERROR_ACT_DATA5);
    cout << "Error Expected Data5:" << err << endl;

    err = host_exe_->read64(HE_ERROR_ACT_DATA6);
    cout << "Error Expected Data6:" << err << endl;

    err = host_exe_->read64(HE_ERROR_ACT_DATA7);
    cout << "Error Expected Data7:" << err << endl;
  }

  int parse_input_options() {

    if (!host_exe_)
      return -1;

    return 0;
  }

  bool he_wait_test_completion() {
    /* Wait for test completion */
    uint32_t timeout = HELPBK_TEST_TIMEOUT;

    volatile uint8_t *status_ptr = host_exe_->get_dsm();
    while (0 == ((*status_ptr) & 0x1)) {
      usleep(HELPBK_TEST_SLEEP_INVL);
      if (--timeout == 0) {
        cout << "HE LPBK TIME OUT" << std::endl;

        return false;
      }
    }
    return true;
  }

  bool verify_numa_node() {

    if (numa_available() < 0) {
      printf("System does not support NUMA API!\n");
      return false;
    }

    printf("SUpported NUMA API!\n");

    int n = numa_max_node();
    printf("There are %d nodes on your system\n", n + 1);

    int cup_num = sched_getcpu();
    printf("cup_num:%d\n", cup_num);

    int node = numa_node_of_cpu(cup_num);
    printf("node:%d\n", node);

    if (host_exe_->he_target_ == HE_TARGET_HOST) {
      numa_node_ = node;
      printf("HE_TARGET_HOST numa_node_:%d\n", numa_node_);

    } else {
      // find fpga numa node numebr
      numa_node_ = 2;
      printf("HE_TARGET_FPGA numa_node_:%d\n", numa_node_);
    }

    int num_task = numa_num_task_nodes();
    printf("num_task:%d\n", num_task);

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

    // reset HE cache
    he_ctl_.value = 0;
    he_ctl_.ResetL = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    he_ctl_.value = 0;
    he_ctl_.ResetL = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    if (host_exe_->he_test_ == HE_FPGA_RD_CACHE_HIT) {
      ret = he_run_fpga_rd_cache_hit_test();
      return ret;
    }

    if (host_exe_->he_test_ == HE_FPGA_WR_CACHE_HIT) {
      ret = he_run_fpga_wr_cache_hit_test();
      return ret;
    }

    if (host_exe_->he_test_ == HE_FPGA_RD_CACHE_MISS) {
      ret = he_run_fpga_rd_cache_miss_test();
      return ret;
    }

    if (host_exe_->he_test_ == HE_FPGA_WR_CACHE_MISS) {
      ret = he_run_fpga_wr_cache_miss_test();
      return ret;
    }

    if (host_exe_->he_test_ == HE_HOST_RD_CACHE_HIT) {
      ret = he_run_host_rd_cache_hit_test();
      return ret;
    }

    if (host_exe_->he_test_ == HE_HOST_WR_CACHE_HIT) {
      ret = he_run_host_wr_cache_hit_test();
      return ret;
    }

    if (host_exe_->he_test_ == HE_HOST_RD_CACHE_MISS) {
      ret = he_run_host_rd_cache_miss_test();
      return ret;
    }

    if (host_exe_->he_test_ == HE_HOST_WR_CACHE_MISS) {
      ret = he_run_host_wr_cache_miss_test();
      return ret;
    }

    return 0;
  }

protected:
  host_exerciser *host_exe_;
  token::ptr_t token_;

  he_ctl he_ctl_;
  he_info he_info_;
  he_rd_config he_rd_cfg_;
  he_wr_config he_wr_cfg_;

  he_rd_addr_table_ctrl rd_table_ctl_;
  he_wr_addr_table_ctrl wr_table_ctl_;
  uint8_t *dsm_buf_;
  uint8_t *rd_buf_;

  uint32_t numa_node_;
};

void he_cache_thread(uint8_t *buf_ptr, uint64_t len) {
  cout << "he_cache_thread  enter" << endl;
  if (buf_ptr == NULL || len == 0) {
    return;
  }
  uint64_t value;
  UNUSED_PARAM(value);
  uint64_t cache_lines = len / 64;
  uint64_t i = 0;
  cout << "he_cache_thread  cache_lines:" << cache_lines << endl;

  while (true) {

    if (g_stop_thread == true) {
      cout << "he_cache_thread g_stop_thread " << endl;
      return;
    }
    // cout << "he_cache_thread:i "<<i << endl;
    if (i < cache_lines) {
      value = *((volatile uint64_t *)(buf_ptr + i * 8));
    }
    i++;
    if (i >= cache_lines) {
      i = 0;
    }
  }

  cout << "he_cache_thread  end" << endl;
  return;
}

} // end of namespace host_exerciser
