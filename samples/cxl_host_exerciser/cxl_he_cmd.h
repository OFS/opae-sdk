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

#define HE_TEST_STARTED      "Test started ......"
#define HE_PRTEST_SCENARIO   "Pretest scenario started ......"
#define HE_PING_PONG         "Ping pong test started ......"
#define HE_RUNNING_POINTER   "Running pointer test started ......"

#define PFN_MASK_SIZE	8

namespace host_exerciser {

class he_cmd : public test_command {
public:
  he_cmd() : host_exe_(NULL), he_clock_mhz_(400), numa_node_(0), he_target_(0),
    he_bias_(0) {

    he_ctl_.value = 0;
    he_info_.value = 0;
    he_rd_cfg_.value = 0;
    he_wr_cfg_.value = 0;
    rd_table_ctl_.value = 0;
    wr_table_ctl_.value = 0;
    he_rd_num_lines_.value = 0;
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
    if (!dsm_status)
      return;

    cout << "\n********* DSM Status CSR Start *********" << endl;
    cout << "test completed :" << dsm_status->test_completed << endl;
    cout << "dsm number:" << dsm_status->dsm_number << endl;
    cout << "error vector:" << dsm_status->err_vector << endl;
    cout << "num ticks:" << std::dec << dsm_status->num_ticks << endl;
    cout << "num reads:" << dsm_status->num_reads << endl;
    cout << "num writes:" << dsm_status->num_writes << endl;
    cout << "penalty start:" << dsm_status->penalty_start << endl;
    cout << "penalty end:" << dsm_status->penalty_end << endl;
    cout << "actual data:" << dsm_status->actual_data << endl;
    cout << "expected data:" << dsm_status->expected_data << endl;

    double latency = 0;
    double perf_data = 0;
    // print bandwidth
    if (dsm_status->num_ticks > 0) {
        perf_data = he_num_xfers_to_bw(dsm_status->num_reads +
            dsm_status->num_writes, dsm_status->num_ticks);

        if (cxl_latency == HE_CXL_RD_LATENCY) {
            //To convert clock ticks to nanoseconds,multiply the clock ticks by 2.5
            latency = (double)(dsm_status->num_ticks * 2.5);
            host_exe_->logger_->info("Bandwidth: {0:0.3f} GB/s Total transaction time: {1:0.2f}  nanoseconds",
                perf_data, latency);
        } else {
            host_exe_->logger_->info("Bandwidth: {0:0.3f} GB/s", perf_data);
        }
    } else {
        host_exe_->logger_->info("Read Latency: N/A");
    }

    cout << "********* DSM Status CSR end *********" << endl;
  }

  void print_csr() {

    host_exe_->logger_->debug("HE_DFH:0x{:x}", host_exe_->read64(HE_DFH));
    host_exe_->logger_->debug("HE_ID_L:0x{:x}", host_exe_->read64(HE_ID_L));
    host_exe_->logger_->debug("HE_ID_H:0x{:x}", host_exe_->read64(HE_ID_H));

    host_exe_->logger_->debug("HE_SCRATCHPAD0:0x{:x}",
    host_exe_->read64(HE_SCRATCHPAD0));

    host_exe_->logger_->debug("HE_DSM_BASE:0x{:x}", host_exe_->read64(HE_DSM_BASE));

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
    uint32_t timeout = HE_CACHE_TEST_TIMEOUT;

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
            host_exe_->set_mmap_access(HE_CACHE_DMA_MMAP_R);
        }
    }

    return true;
  }


  void he_start_test(const char* str = HE_TEST_STARTED, uint8_t test_type = RD_WR_TEST) {
      // start test
    he_ctl_.test_type = test_type;
    he_ctl_.Start = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);
    he_ctl_.Start = 1;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    he_ctl_.Start = 0;
    host_exe_->write64(HE_CTL, he_ctl_.value);

    cout << str << endl;

    host_exe_->logger_->debug("Test type :0x{:x} ", test_type);
    host_exe_->logger_->debug("HE_CTL:0x{:x}", host_exe_->read64(HE_CTL));
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


  bool he_get_perf(double* perf_data, double* latency,
      he_cxl_latency cxl_latency = HE_CXL_LATENCY_NONE) {
      volatile he_cache_dsm_status* dsm_status = NULL;

      dsm_status = reinterpret_cast<he_cache_dsm_status*>(
          (uint8_t*)(host_exe_->get_dsm()));
      if (!dsm_status)
          return false;

      if (dsm_status->num_ticks > 0) {
          *perf_data =
              he_num_xfers_to_bw(dsm_status->num_reads + dsm_status->num_writes,
                  dsm_status->num_ticks);

          if (cxl_latency == HE_CXL_RD_LATENCY && dsm_status->num_reads > 0) {
              *latency = (double)((dsm_status->num_ticks / (double)dsm_status->num_reads)
                  * (2.5));

          }
          return true;
      }

      return false;
  }

  uint64_t get_ticks() {
      volatile he_cache_dsm_status* dsm_status = NULL;

      dsm_status = reinterpret_cast<he_cache_dsm_status*>(
          (uint8_t*)(host_exe_->get_dsm()));
      if (!dsm_status)
          return 0;
      if (dsm_status->num_ticks > 0)
          return dsm_status->num_ticks;
      else
          return 0;
  }

  uint64_t get_penalty_start_ticks() {
      volatile he_cache_dsm_status* dsm_status = NULL;

      dsm_status = reinterpret_cast<he_cache_dsm_status*>(
          (uint8_t*)(host_exe_->get_dsm()));
      if (!dsm_status)
          return 0;
      if (dsm_status->penalty_start > 0)
          return dsm_status->penalty_start;
      else
          return 0;
  }
  
  int  get_mtime(const char* file_name, struct timespec* mtime)
  {
      struct stat s;

      if (!lstat(file_name, &s)) {
          if (S_ISLNK(s.st_mode)) {
              mtime->tv_sec = mtime->tv_nsec = 0;
              return -1;
          }
          mtime->tv_sec = s.st_mtime;
          mtime->tv_nsec = s.st_mtim.tv_nsec;
          return 0;
      }
      else {
          mtime->tv_sec = mtime->tv_nsec = 0;
          return errno;
      }
  }

  int file_open(const char* file_name, int mode)
  {
      struct stat before, after;
      int fd;
      int ret;

      ret = lstat(file_name, &before);
      if (ret == 0) {
          if (S_ISLNK(before.st_mode))
              return -1;
      }

      fd = open(file_name, mode, 0x666);
      if (fd < 0)
          return -1;

      if (ret == 0) {
          if (fstat(fd, &after) == 0) {
              if (before.st_ino != after.st_ino) {
                  close(fd);
                  return -1;
              }
          }
          else {
              close(fd);
              return -1;
          }
      }
      return fd;
  }

   uint64_t __mem_virt2phys(const void* virtaddr)
  {
      int page_size = getpagesize();
      struct timespec mtime1, mtime2;
      const char* fname = "/proc/self/pagemap";
      unsigned long pfn;
      uint64_t page;
      off_t offset;
      int fd, retval;

      retval = get_mtime(fname, &mtime1);
      if (retval) {
          cerr << "stat failed\n";
          return -1;
      }

      fd = file_open("/proc/self/pagemap", O_RDONLY | O_EXCL);
      if (fd < 0)
          return -1;

      pfn = (unsigned long)virtaddr / page_size;
      offset = pfn * sizeof(uint64_t);
      if (lseek(fd, offset, SEEK_SET) == (off_t)-1) {
          cerr << "seek error\n";
          close(fd);
          return -1;
      }

      retval = get_mtime(fname, &mtime2);
      if (retval) {
          cerr << "stat failed\n";
          close(fd);
          return -1;
      }

      if (mtime1.tv_sec != mtime2.tv_sec
          || mtime1.tv_nsec != mtime2.tv_nsec) {
          cerr << "file got modified after open \n";
          close(fd);
          return -1;
      }

      retval = read(fd, &page, PFN_MASK_SIZE);
      close(fd);
      if (retval != PFN_MASK_SIZE)
          return -1;
      if ((page & 0x7fffffffffffffULL) == 0)
          return -1;

      return ((page & 0x7fffffffffffffULL) * page_size)
          + ((unsigned long)virtaddr & (page_size - 1));
  }

   bool create_linked_list(uint64_t *virt_ptr_a, uint64_t phy_ptr_a,
       uint64_t data, uint64_t max_size, he_bias_support bias_a = HOSTMEM_BIAS,
       uint64_t *virt_ptr_b = NULL, uint64_t phy_ptr_b = 0,
       he_bias_support bias_b = FPGAMEM_HOST_BIAS) {

       uint64_t *temp_virt_ptr_a       = virt_ptr_a;
       uint64_t temp_phy_ptr_a         = phy_ptr_a;
       uint64_t *temp_virt_ptr_b       = virt_ptr_b;
       uint64_t temp_phy_ptr_b         = phy_ptr_b;
       uint64_t i                      = 0;
       struct he_cache_running_ptr *temp_node = NULL;

       host_exe_->logger_->debug("virt_ptr_a:{:p}", fmt::ptr(virt_ptr_a));
       host_exe_->logger_->debug("phy_ptr_a:0x{:x}", phy_ptr_a);
       host_exe_->logger_->debug("virt_ptr_b:{:p}", fmt::ptr(virt_ptr_b));
       host_exe_->logger_->debug("phy_ptr_b:0x{:x}", phy_ptr_b);
       host_exe_->logger_->debug("max_size:{0}", max_size);
       host_exe_->logger_->debug("data:{:x}", data);

       if (virt_ptr_a == NULL || phy_ptr_a == 0) {
           cerr << "Invalid input arguments" << endl;
           return false;
       }

       // Linked list on host or fpga memory
       if (virt_ptr_a != NULL && phy_ptr_a != 0 &&
           virt_ptr_b == NULL && phy_ptr_b == 0) {

           temp_node = (struct he_cache_running_ptr*)(temp_virt_ptr_a);

           for (i = 0; i < max_size; ++i) {
               temp_node->phy_next_ptr = temp_phy_ptr_a + 64;
               temp_node->data = data;
               temp_node->virt_next_ptr = (struct he_cache_running_ptr*)(temp_virt_ptr_a + 8);
               temp_node->biasmode = bias_a;

               temp_node++;
               temp_phy_ptr_a = temp_phy_ptr_a + 64;
               temp_virt_ptr_a = temp_virt_ptr_a + 8;
           }

           temp_node->phy_next_ptr = 0;
           temp_node->virt_next_ptr = NULL;
           temp_node->data = 0;
       }

       // Linked list on host and fpga memory
       if (virt_ptr_a != NULL && phy_ptr_a != 0 &&
           virt_ptr_b != NULL && phy_ptr_b != 0) {

           struct he_cache_running_ptr* temp_node_a = 
               (struct he_cache_running_ptr*)(temp_virt_ptr_a);

           struct he_cache_running_ptr* temp_node_b =
               (struct he_cache_running_ptr*)(temp_virt_ptr_b);

           int which = 0;
           for (i = 0; i <  max_size; ++i) {

               if (which == 0) {
                   temp_node_a->phy_next_ptr = temp_phy_ptr_b;
                   temp_node_a->data =  data;
                   temp_node_a->biasmode = bias_b;
                   temp_node_a->virt_next_ptr = temp_node_b;
                   ++temp_node_a;
                   temp_phy_ptr_a += 64;

               } else {
                   temp_node_b->phy_next_ptr = temp_phy_ptr_a;
                   temp_node_b->data = data;
                   temp_node_b->biasmode = bias_a;
                   temp_node_b->virt_next_ptr = temp_node_a;
                   ++temp_node_b;
                   temp_phy_ptr_b += 64;
               }

               which = 1 - which;
           }

           temp_node_a->phy_next_ptr = 0;
           temp_node_a->virt_next_ptr = nullptr;

           temp_node_b->phy_next_ptr = 0;
           temp_node_b->virt_next_ptr = nullptr;
       }

        return true;
   }

   bool verify_linked_list(uint64_t *virt_ptr, uint64_t phy_ptr,
       uint64_t data, uint64_t max_size) {

       bool  retval                      = true;
       struct he_cache_running_ptr *temp = NULL;
       uint64_t i                        = 0;

       host_exe_->logger_->debug("virt_ptr:{:p}", fmt::ptr(virt_ptr));
       host_exe_->logger_->debug("phy_ptr:0x{:x}", phy_ptr);
       host_exe_->logger_->debug("max_size:{0}", max_size);
       host_exe_->logger_->debug("data:{:x}", data);

       temp = (struct he_cache_running_ptr*)(virt_ptr);
       for (i = 0; i < max_size; i++) {

               if (temp == NULL) {
                   retval = false;
                   break;
           }
           // 1's complement of data
           if (temp->data != ~data) {
               cerr << "Failed to convert data to 1's complement at index:"
                   << i << endl;
               retval = false;
               break;
           }
           temp = temp->virt_next_ptr;
       }
       return retval;
   }
   
   bool print_linked_list(uint64_t *virt_ptr, uint64_t phy_ptr,
       uint64_t data, uint64_t max_size) {

       bool  retval                               = true;
       volatile struct he_cache_running_ptr *temp = NULL;
       uint64_t i                                 = 0;

       host_exe_->logger_->debug("virt_ptr:{:p}", fmt::ptr(virt_ptr));
       host_exe_->logger_->debug("phy_ptr:0x{:x}", phy_ptr);
       host_exe_->logger_->debug("max_size:{0}", max_size);
       host_exe_->logger_->debug("data:{:x}", data);

       temp = (struct he_cache_running_ptr*)(virt_ptr);
       for (i = 0; i < max_size; i++) {

           if (temp == NULL) {
               retval = false;
               break;
           }
           cout << "data:" << std::hex << temp->data << endl;
           temp = temp->virt_next_ptr;
           cout << "temp->virt_next_ptr:" << temp->virt_next_ptr << endl;
       }
       return retval;
   }

protected:
  host_exerciser *host_exe_;
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
  he_rd_num_lines he_rd_num_lines_;
};
} // end of namespace host_exerciser
