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
#include <map>
#include <numa.h>
#include <unistd.h>

#include "cxl_he_cmd.h"
#include "cxl_host_exerciser.h"
#include "he_cache_test.h"

namespace host_exerciser {

class he_cmd : public test_command {
public:
  he_cmd() : host_exe_(NULL), he_clock_mhz_(400), numa_node_(0), he_target_(0) {

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

    // print bandwidth
    if (dsm_status->num_ticks > 0) {
      double perf_data =
          he_num_xfers_to_bw(dsm_status->num_reads + dsm_status->num_writes,
                             dsm_status->num_ticks);
      host_exe_->logger_->info("Bandwidth: {0:0.3f} GB/s", perf_data);
    }

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

    if (he_target_ == HE_TARGET_HOST) {
      numa_node_ = node;
      printf("HE_TARGET_HOST numa_node_:%d\n", numa_node_);

    } else {
      // find fpga numa node numebr
      numa_node_ = 2;
      printf("HE_TARGET_FPGA numa_node_:%d\n", numa_node_);
    }

    int num_config_cpu = numa_num_configured_cpus();
    printf("num_config_cpu:%d\n", num_config_cpu);

    int num_task_nodes = numa_num_task_nodes();
    printf("num_task_nodes:%d\n", num_task_nodes);

    return true;
  }

protected:
  host_exerciser *host_exe_;
  uint32_t he_clock_mhz_;
  uint32_t numa_node_;
  uint32_t he_target_;

  he_ctl he_ctl_;
  he_info he_info_;
  he_rd_config he_rd_cfg_;
  he_wr_config he_wr_cfg_;
  he_rd_addr_table_ctrl rd_table_ctl_;
  he_wr_addr_table_ctrl wr_table_ctl_;
};
} // end of namespace host_exerciser
