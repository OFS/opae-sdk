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
#include <numa.h>
#include <unistd.h>

#include <map>

#include "cxl_he_cmd.h"
#include "cxl_hello_fpga.h"
#include "he_cache_test.h"

#define HE_TEST_STARTED "Test started ......"

namespace hello_fpga {

class he_cmd : public test_command {
 public:
  he_cmd()
      : host_exe_(NULL),
        he_clock_mhz_(400),
        numa_node_(0),
        he_target_(0),
        he_bias_(0) {
    he_ctl_.value = 0;
    he_info_.value = 0;
    he_rd_cfg_.value = 0;
    he_wr_cfg_.value = 0;
    rd_table_ctl_.value = 0;
    wr_table_ctl_.value = 0;
  }

  virtual ~he_cmd() {}

  // Convert number of transactions to bandwidth (GB/s)
  double he_num_xfers_to_bw(uint64_t num_lines, uint64_t num_ticks) {
    return (double)(num_lines * 64) / ((1000.0 / he_clock_mhz_ * num_ticks));
  }

  void he_perf_counters(he_cxl_latency cxl_latency = HE_CXL_LATENCY_NONE) {
    volatile he_cache_dsm_status *dsm_status = NULL;

    dsm_status = reinterpret_cast<he_cache_dsm_status *>(
        (uint8_t *)(host_exe_->get_dsm()));
    if (!dsm_status) return;

    cout << "\n********* DSM Status CSR Start *********" << endl;
    cout << "test completed :" << dsm_status->test_completed << endl;
    cout << "dsm number:" << dsm_status->dsm_number << endl;
    cout << "error vector:" << dsm_status->err_vector << endl;
    cout << "num ticks:" << dsm_status->num_ticks << endl;
    cout << "num reads:" << dsm_status->num_reads << endl;
    cout << "num writes:" << dsm_status->num_writes << endl;
    cout << "penalty start:" << dsm_status->penalty_start << endl;
    cout << "penalty end:" << dsm_status->penalty_end << endl;
    cout << "actual data:" << dsm_status->actual_data << endl;
    cout << "expected data:" << dsm_status->expected_data << endl;

    // print bandwidth
    if (dsm_status->num_ticks > 0) {
      double perf_data =
          he_num_xfers_to_bw(dsm_status->num_reads + dsm_status->num_writes,
                             dsm_status->num_ticks);
      host_exe_->logger_->info("Bandwidth: {0:0.3f} GB/s", perf_data);
    }

    if (cxl_latency == HE_CXL_RD_LATENCY) {
      if (dsm_status->num_ticks > 0 && dsm_status->num_reads > 0) {
        double latency =
            (double)((dsm_status->num_ticks / (double)dsm_status->num_reads) *
                     (2.5));

        host_exe_->logger_->info("Read Latency : {0:0.2f}  nanoseconds",
                                 latency);
      } else {
        host_exe_->logger_->info("Read Latency: N/A");
      }
    }

    cout << "********* DSM Status CSR end *********" << endl;
  }

  void print_csr() {
    host_exe_->logger_->debug("HE_DFH:0x{:x}", host_exe_->read64(HE_DFH));
    host_exe_->logger_->debug("HE_ID_L:0x{:x}", host_exe_->read64(HE_ID_L));
    host_exe_->logger_->debug("HE_ID_H:0x{:x}", host_exe_->read64(HE_ID_H));

    host_exe_->logger_->debug("HE_SCRATCHPAD0:0x{:x}",
                              host_exe_->read64(HE_SCRATCHPAD0));

    host_exe_->logger_->debug("HE_DSM_BASE:0x{:x}",
                              host_exe_->read64(HE_DSM_BASE));

    host_exe_->logger_->debug("HE_CTL:0x{:x}", host_exe_->read64(HE_CTL));

    host_exe_->logger_->debug("HE_INFO:0x{:x}", host_exe_->read64(HE_INFO));

    host_exe_->logger_->debug("HE_WR_NUM_LINES:0x{:x}",
                              host_exe_->read64(HE_WR_NUM_LINES));

    host_exe_->logger_->debug("HE_WR_BYTE_ENABLE:0x{:x}",
                              host_exe_->read64(HE_WR_BYTE_ENABLE));

    host_exe_->logger_->debug("HE_WR_CONFIG:0x{:x}",
                              host_exe_->read64(HE_WR_CONFIG));

    host_exe_->logger_->debug("HE_WR_ADDR_TABLE_CTRL:0x{:x}",
                              host_exe_->read64(HE_WR_ADDR_TABLE_CTRL));

    host_exe_->logger_->debug("HE_WR_ADDR_TABLE_DATA:0x{:x}",
                              host_exe_->read64(HE_WR_ADDR_TABLE_DATA));

    host_exe_->logger_->debug("HE_RD_NUM_LINES:0x{:x}",
                              host_exe_->read64(HE_RD_NUM_LINES));

    host_exe_->logger_->debug("HE_RD_CONFIG:0x{:x}",
                              host_exe_->read64(HE_RD_CONFIG));

    host_exe_->logger_->debug("HE_RD_ADDR_TABLE_CTRL:0x{:x}",
                              host_exe_->read64(HE_RD_ADDR_TABLE_CTRL));

    host_exe_->logger_->debug("HE_RD_ADDR_TABLE_DATA:0x{:x}",
                              host_exe_->read64(HE_RD_ADDR_TABLE_DATA));

    host_exe_->logger_->debug("HE_ERROR_STATUS:0x{:x}",
                              host_exe_->read64(HE_ERROR_STATUS));

    host_exe_->logger_->debug("HE_ERROR_EXP_DATA:0x{:x}",
                              host_exe_->read64(HE_ERROR_EXP_DATA));

    host_exe_->logger_->debug("HE_ERROR_ACT_DATA0:0x{:x}",
                              host_exe_->read64(HE_ERROR_ACT_DATA0));

    host_exe_->logger_->debug("HE_ERROR_ACT_DATA1:0x{:x}",
                              host_exe_->read64(HE_ERROR_ACT_DATA1));

    host_exe_->logger_->debug("HE_ERROR_ACT_DATA2:0x{:x}",
                              host_exe_->read64(HE_ERROR_ACT_DATA2));

    host_exe_->logger_->debug("HE_ERROR_ACT_DATA3:0x{:x}",
                              host_exe_->read64(HE_ERROR_ACT_DATA3));

    host_exe_->logger_->debug("HE_ERROR_ACT_DATA4:0x{:x}",
                              host_exe_->read64(HE_ERROR_ACT_DATA4));

    host_exe_->logger_->debug("HE_ERROR_ACT_DATA5:0x{:x}",
                              host_exe_->read64(HE_ERROR_ACT_DATA5));

    host_exe_->logger_->debug("HE_ERROR_ACT_DATA6:0x{:x}",
                              host_exe_->read64(HE_ERROR_ACT_DATA6));

    host_exe_->logger_->debug("HE_ERROR_ACT_DATA7:0x{:x}",
                              host_exe_->read64(HE_ERROR_ACT_DATA7));
  }

  void host_exerciser_errors() {
    he_err_status err_status;
    uint64_t err = 0;
    if (host_exe_ == NULL) return;

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
    if (!host_exe_) return -1;

    return 0;
  }

  bool he_wait_test_completion(const char *str = HE_TEST_STARTED) {
    /* Wait for test completion */
    uint32_t timeout = HE_CACHE_TEST_TIMEOUT;

    cout << str << endl;
    volatile uint8_t *status_ptr = host_exe_->get_dsm();
    while (0 == ((*status_ptr) & 0x1)) {
      usleep(HE_CACHE_TEST_SLEEP_INVL);
      if (--timeout == 0) {
        cout << "HE Cache time out error" << endl;
        return false;
      }
    }
    return true;
  }

  bool he_set_bias_mode() {
    // Target memory HOST set BIAS host
    if (he_target_ == HE_TARGET_HOST) {
      he_ctl_.bias_support = HOSTMEM_BIAS;
      // Target memory FPGA set BIAS host/device
      if (he_bias_ == HOSTMEM_BIAS) {
        he_ctl_.bias_support = HOSTMEM_BIAS;
      } else {
        cerr << "Wrong BIAS mode for specified target memory type" << endl;
        return false;
      }
    } else {
      // Target memory FPGA set BIAS host/device
      if (he_bias_ == HOSTMEM_BIAS) {
        he_ctl_.bias_support = FPGAMEM_HOST_BIAS;
      } else {
        he_ctl_.bias_support = FPGAMEM_DEVICE_BIAS;
      }
    }

    return true;
  }

  void he_start_test() {
    // start test
    he_ctl_.Start = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);
    he_ctl_.Start = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);
  }

  bool verify_numa_node() {
    if (numa_available() < 0) {
      cerr << "System does not support NUMA API" << endl;
      return false;
    }

    int n = numa_max_node();
    cout << "Number nodes on system:" << n + 1 << endl;

    int numa_node = numa_node_of_cpu(sched_getcpu());
    cout << "HE Cache app numa node:" << numa_node << endl;

    if (he_target_ == HE_TARGET_HOST) {
      numa_node_ = numa_node;
      cout << "HE_TARGET_HOST numa node:" << numa_node_ << endl;
    } else {
      // find fpga numa node number
      numa_node_ = 2;
      cout << "HE_TARGET_FPGA numa node:" << numa_node_ << endl;
    }

    return true;
  }

 protected:
  hello_fpga *host_exe_;
  uint32_t he_clock_mhz_;
  uint32_t numa_node_;
  uint32_t he_target_;
  uint32_t he_bias_;

  he_ctl he_ctl_;
  he_info he_info_;
  he_rd_config he_rd_cfg_;
  he_wr_config he_wr_cfg_;
  he_rd_addr_table_ctrl rd_table_ctl_;
  he_wr_addr_table_ctrl wr_table_ctl_;
};
}  // end of namespace hello_fpga
