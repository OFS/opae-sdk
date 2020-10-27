// Copyright(c) 2020, Intel Corporation
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
#include "afu_test.h"
#include "host_exerciser.h"

using test_afu = opae::afu_test::afu;
using opae::fpga::types::shared_buffer;

namespace host_exerciser {

class host_exerciser_lpbk : public test_command
{
public:
  host_exerciser_lpbk(){}
  virtual ~host_exerciser_lpbk(){}
  virtual const char *name() const
  {
    return "lpbk";
  }

  virtual const char *description() const
  {
    return "run simple loopback test";
  }

  void host_exerciser_status()
  {
    csr_status0 afu_status0(0);
    csr_status1 afu_status1(0);

    afu_status0.value = he_afu_->read64(CSR_STATUS0);
    afu_status1.value = he_afu_->read64(CSR_STATUS1);

    std::cout << "Host Exerciser numReads:"<< afu_status0.numReads << std::endl;
     std::cout << "Host Exerciser numReads:" << afu_status0.numWrites << std::endl;

    std::cout << "Host Exerciser numPendReads:" << afu_status1.numPendReads << std::endl;
    std::cout << "Host Exerciser numPendWrites:" << afu_status1.numPendWrites << std::endl;
  }

  void host_exerciser_errors()
  {
    csr_error afu_error(0);
    csr_error1 afu_error1(0);

    afu_error.value = he_afu_->read64(CSR_STATUS0);
    afu_error1.value = he_afu_->read64(CSR_STATUS1);

     std::cout << "Host Exerciser Error:" << afu_error.error << std::endl;
     std::cout << "Host Exerciser stride value:" << afu_error1.error << std::endl;
  }

  void host_exerciser_swtestmsg()
  {
    uint64_t swtest_msg;
    swtest_msg = he_afu_->read64(CSR_SWTEST_MSG);

    std::cout << "Host Exerciser swtest msg:" << swtest_msg << std::endl;
  }

  inline uint64_t cacheline_aligned_addr(uint64_t num) {
    return num >> LOG2_CL;
  }

  int parse_input_options(test_afu *afu,uint32_t *input_cmds)
  {
    auto d_afu = dynamic_cast<host_exerciser*>(afu);
    csr_cfg helpbk_cfg_(0);

    // Host Exerciser Mode
    if ((d_afu->mode_str_ != "lpbk") &&
        (d_afu->mode_str_  != "read") &&
        (d_afu->mode_str_ != "write") &&
        (d_afu->mode_str_  != "trput")) {
          std::cerr << "Invalid --mode: " << d_afu->mode_str_ << std::endl;
          return -1;
    }

    if (d_afu->mode_str_  == "lpbk") {
         helpbk_cfg_.cr_mode= HOST_EXEMODE_LPBK1;
    } else if (d_afu->mode_str_ == "read") {
         helpbk_cfg_.cr_mode = HOST_EXEMODE_READ;
    } else if (d_afu->mode_str_ == "write") {
         helpbk_cfg_.cr_mode = HOST_EXEMODE_WRITE;
    } else {
         helpbk_cfg_.cr_mode = HOST_EXEMODE_TRUPT;
    }

    // Host Exerciser Read
    if ((d_afu->read_str_ != "rdline_s") &&
        (d_afu->read_str_ != "rdline_l") &&
        (d_afu->read_str_ != "mixed")) {
          std::cerr << "Invalid --read: " << d_afu->read_str_ << std::endl;
          return -1;
    }

    if (d_afu->read_str_ == "rdline_s") {
       helpbk_cfg_.cr_rdsel = HOSTEXE_RDLINE_S;
    } else if (d_afu->read_str_ == "rdline_l") {
       helpbk_cfg_.cr_rdsel = HOSTEXE_RDLINE_L;
    } else {
       helpbk_cfg_.cr_rdsel = HOSTEXE_RDLINE_MIX;
    }

    // Host Exerciser Write
    if ((d_afu->write_str_ != "wrline_m") &&
      (d_afu->write_str_ != "wrline_l") ) {
        std::cerr << "Invalid --write: " << d_afu->write_str_ << std::endl;
        return -1;
    }

    if (d_afu->write_str_ == "wrline_m") {
         helpbk_cfg_.cr_wrthru_en = HOSTEXE_WRLINE_M;
    } else {
         helpbk_cfg_.cr_wrthru_en = HOSTEXE_WRLINE_L;
    }

    // Host Exerciser  lpbk delay
    if (d_afu->delay_)
         helpbk_cfg_.cr_delay_en = 1;

    // write
    if ((d_afu->multi_cl_ != 0) &&
        (d_afu->multi_cl_ != 1) &&
        (d_afu->multi_cl_ != 3)) {
         std::cerr << "Invalid --multi_cl_: " << d_afu->multi_cl_ << std::endl;
          return -1;
    }
    helpbk_cfg_.cr_multiCL_len = d_afu->multi_cl_;

    //test rollover or test termination
    if (d_afu->c_cont_)
        helpbk_cfg_.c_cont = 1;

    *input_cmds = helpbk_cfg_.value;
    return 0;
  }

  virtual int run(test_afu *afu, CLI::App *app)
  {
    (void)app;

    auto d_afu = dynamic_cast<host_exerciser*>(afu);
   he_afu_ = dynamic_cast<host_exerciser*>(afu);
   uint32_t input_cmds;

	auto ret = parse_input_options(afu,&input_cmds);
	if (ret != 0) {
		std::cerr << "Failed to parese input options" << std::endl;
		return ret;
	}

	std::cout << "INPUT CMDS:"<< input_cmds<< std::endl;

	/* Allocate Source Buffer
	   Write to CSR_SRC_ADDR */
	std::cout << "Allocate SRC Buffer" << std::endl;
	source_ = d_afu->allocate(LPBK1_BUFFER_ALLOCATION_SIZE);
	d_afu->write64(CSR_SRC_ADDR, cacheline_aligned_addr(source_->io_address()));
	std::fill_n(source_->c_type(), LPBK1_BUFFER_SIZE, 0xAF);

	/* Allocate Destination Buffer
	   Write to CSR_DST_ADDR */
	std::cout << "Allocate DST Buffer" << std::endl;
	destination_ = d_afu->allocate(LPBK1_BUFFER_ALLOCATION_SIZE);
	d_afu->write64(CSR_DST_ADDR, cacheline_aligned_addr(destination_->io_address()));
	std::fill_n(destination_->c_type(), LPBK1_BUFFER_SIZE, 0xBE);

	/* Allocate DSM Buffer
	 Write to CSR_AFU_DSM_BASEL */
	std::cout << "Allocate DSM Buffer" << std::endl;
	dsm_ = d_afu->allocate(LPBK1_DSM_SIZE);
	d_afu->write32(CSR_AFU_DSM_BASEL, cacheline_aligned_addr(dsm_->io_address()));
	std::fill_n(dsm_->c_type(), LPBK1_DSM_SIZE, 0x0);

	// Number of cache lines
	d_afu->write64(CSR_NUM_LINES, LPBK1_BUFFER_SIZE / (1 * CL));

	// Write to CSR_CFG
	csr_cfg  af_config(input_cmds);
	d_afu->write32(CSR_CFG, af_config.value);

	// Write to CSR_CTL
	std::cout << "Start Test" << std::endl;
	csr_ctl  afu_ctl(0);
	afu_ctl.Start = 1;
	d_afu->write32(CSR_CFG, afu_ctl.value);


	/* Wait for test completion */
	uint32_t           timeout;
	timeout = HELPBK_TEST_TIMEOUT;
	volatile uint8_t* status_ptr = dsm_->c_type() + DSM_STATUS_TEST_COMPLETE;


	while (0 == ((*status_ptr) & 0x1))
	{
		usleep(100);
		if (--timeout == 0) {
			std::cout << "HE LPBK TIME OUT" << std::endl;
			host_exerciser_errors();
			return -1;
		}
	}

	// stop the device
	afu_ctl.value = 0;
	afu_ctl.ForcedTestCmpl = 1;
	d_afu->write32(CSR_CTL, afu_ctl.value);

	std::cout << "Test Completed" << std::endl;
	host_exerciser_swtestmsg();
	host_exerciser_status();

	/* Compare buffer contents */
	d_afu->compare(source_, destination_);

    return 0;
  }

protected:

  test_afu *he_afu_;
  shared_buffer::ptr_t source_;
  shared_buffer::ptr_t destination_;
  shared_buffer::ptr_t dsm_;
};

} // end of namespace host_exerciser
