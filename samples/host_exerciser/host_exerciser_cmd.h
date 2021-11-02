// Copyright(c) 2020-2021, Intel Corporation
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

#include <unistd.h>

#include "afu_test.h"
#include "host_exerciser.h"

using test_afu = opae::afu_test::afu;
using opae::fpga::types::shared_buffer;
using opae::fpga::types::token;

// HE exit global flag
volatile bool g_he_exit = false;

// host exerciser signal handler
void he_sig_handler(int)
{
    g_he_exit = true;
    printf("HE signal handler exit app \n");
}

namespace host_exerciser {


class host_exerciser_cmd : public test_command
{
public:
    host_exerciser_cmd()
        :host_exe_(NULL) {
          he_lpbk_cfg_.value = 0;
          he_lpbk_ctl_.value = 0;
    }
    virtual ~host_exerciser_cmd() {}

    void host_exerciser_status()
    {
        he_status0 he_status0;
        he_status1 he_status1;

        he_status0.value = host_exe_->read64(HE_STATUS0);
        he_status1.value = host_exe_->read64(HE_STATUS1);

        std::cout << "Host Exerciser numReads:" << he_status0.numReads << std::endl;
        std::cout << "Host Exerciser numWrites:" << he_status0.numWrites << std::endl;

        std::cout << "Host Exerciser numPendReads:" << he_status1.numPendReads << std::endl;
        std::cout << "Host Exerciser numPendWrites:" << he_status1.numPendWrites << std::endl;
    }

    void host_exerciser_errors()
    {
        he_error he_error;

        if (host_exe_ == NULL)
            return;

        he_error.value = host_exe_->read64(HE_ERROR);

        std::cout << "Host Exerciser Error:" << he_error.error << std::endl;

    }

    void host_exerciser_swtestmsg()
    {
        uint64_t swtest_msg;

        if (host_exe_ == NULL)
            return;

        swtest_msg = host_exe_->read64(HE_SWTEST_MSG);

        std::cout << "Host Exerciser swtest msg:" << swtest_msg << std::endl;
    }

    inline uint64_t cacheline_aligned_addr(uint64_t num) {
        return num >> LOG2_CL;
    }

    void he_perf_counters()
    {
        struct he_dsm_status *dsm_status = NULL;
        volatile uint8_t* status_ptr = dsm_->c_type();
        uint64_t num_cache_lines = 0;
        if (!status_ptr)
           return;

        dsm_status = reinterpret_cast<he_dsm_status *>((uint8_t*)status_ptr);
        if (!dsm_status)
            return;

        std::cout << "\nHost Exerciser Performance Counter:" << std::endl;
        // calculate number of cache lines in continuous mode
        if (host_exe_->he_continuousmode_) {
            if (he_lpbk_cfg_.TestMode == HOST_EXEMODE_LPBK1)
                num_cache_lines = dsm_status->num_writes * 2;
            if (he_lpbk_cfg_.TestMode == HOST_EXEMODE_READ)
                num_cache_lines = dsm_status->num_reads;
            if (he_lpbk_cfg_.TestMode == HOST_EXEMODE_WRITE)
                num_cache_lines = (dsm_status->num_writes);
            if (he_lpbk_cfg_.TestMode == HOST_EXEMODE_TRUPT)
                num_cache_lines = dsm_status->num_writes * 2;
        } else {
            num_cache_lines = (LPBK1_BUFFER_SIZE / (1 * CL));
            host_exerciser_status();
        }

        std::cout << "Number of clocks:" <<
            dsm_status->num_ticks << std::endl;
        std::cout << "Total number of Reads sent:" <<
            dsm_status->num_reads << std::endl;
        std::cout << "Total number of Writes sent :" <<
            dsm_status->num_writes << std::endl;

        // print bandwidth
        if (dsm_status->num_ticks > 0) {
            double perf_data = (double)(num_cache_lines * 64) /
                ((1000.0 / host_exe_->he_clock_mhz_ * (dsm_status->num_ticks)));
            std::cout << "Bandwidth: " << std::setprecision(3) <<
                perf_data << " GB/s" << std::endl;
        }
    }

    bool he_interrupt(event::ptr_t ev)
    {
        try {
            host_exe_->interrupt_wait(ev, 10000);
            if (he_lpbk_cfg_.TestMode == HOST_EXEMODE_LPBK1)
	            host_exe_->compare(source_, destination_);
        }
        catch (std::exception &ex) {
            std::cout << "Exception: " << ex.what() << std::endl;
            host_exerciser_errors();
            return false;
        }
        return true;
    }

    bool he_wait_test_completion()
    {
        /* Wait for test completion */
        uint32_t           timeout = HELPBK_TEST_TIMEOUT;
        volatile uint8_t* status_ptr = dsm_->c_type();

        while (0 == ((*status_ptr) & 0x1)) {
            usleep(HELPBK_TEST_SLEEP_INVL);
            if (--timeout == 0) {
                std::cout << "HE LPBK TIME OUT" << std::endl;
                host_exerciser_errors();
                return false;
            }
        }
        return true;
    }

    void he_forcetestcmpl()
    {
        // Force stop test
        he_lpbk_ctl_.value = 0;
        he_lpbk_ctl_.ResetL = 1;
        he_lpbk_ctl_.ForcedTestCmpl = 1;
        host_exe_->write32(HE_CTL, he_lpbk_ctl_.value);

        if (! he_wait_test_completion())
            sleep(1);

        he_lpbk_ctl_.value = 0;
        host_exe_->write32(HE_CTL, he_lpbk_ctl_.value);
        usleep(1000);
    }

    void he_init_src_buffer(shared_buffer::ptr_t buffer)
    {
        // Fill the source buffer with random values
        host_exe_->fill(buffer);

        // Compare and swap? If so, seed the source buffers with values will
        // match. The hardware tests uses the line index as the test.
        if (he_lpbk_cfg_.AtomicFunc == HOSTEXE_ATOMIC_CAS_4) {
            for (uint32_t i = 0; i < buffer->size()/CL; i += 3) {
                buffer->write<uint32_t>(i, i*CL);
            }
        }
        if (he_lpbk_cfg_.AtomicFunc == HOSTEXE_ATOMIC_CAS_8) {
            for (uint32_t i = 0; i < buffer->size()/CL; i += 3) {
                buffer->write<uint64_t>(i, i*CL);
            }
        }

        // In atomic mode, at most the first 8 bytes of each line will be
        // updated and copied. In the source buffer, write a function of
        // the value at the start of each line to the second postion so
        // it can be used as a check later.
        if (he_lpbk_cfg_.AtomicFunc != HOSTEXE_ATOMIC_OFF) {
            for (uint32_t i = 0; i < buffer->size()/CL; i += 1) {
                uint64_t v = buffer->read<uint64_t>(i*CL);
                buffer->write<uint64_t>(v ^ 0xabababababababab, i*CL + 8);
            }
        }
    }

    void he_dump_buffer(shared_buffer::ptr_t buffer, const char* msg)
    {
        std::cout << msg << ":" << std::endl;

        // Dump the first 8 lines of a buffer
        for (uint64_t i = 0; i < 8; i++)
        {
            std::cout << std::hex
                      << " " << std::setfill('0') << std::setw(16) << buffer->read<uint64_t>(i*CL + 8*7)
                      << " " << std::setfill('0') << std::setw(16) << buffer->read<uint64_t>(i*CL + 8*6)
                      << " " << std::setfill('0') << std::setw(16) << buffer->read<uint64_t>(i*CL + 8*5)
                      << " " << std::setfill('0') << std::setw(16) << buffer->read<uint64_t>(i*CL + 8*4)
                      << " " << std::setfill('0') << std::setw(16) << buffer->read<uint64_t>(i*CL + 8*3)
                      << " " << std::setfill('0') << std::setw(16) << buffer->read<uint64_t>(i*CL + 8*2)
                      << " " << std::setfill('0') << std::setw(16) << buffer->read<uint64_t>(i*CL + 8*1)
                      << " " << std::setfill('0') << std::setw(16) << buffer->read<uint64_t>(i*CL)
                      << std::dec << std::endl;
        }

        std::cout << " ..." << std::endl;
    }

    void he_compare_buffer()
    {
        host_exerciser_swtestmsg();

        // Compare buffer contents only loopback test mode
        if (he_lpbk_cfg_.TestMode != HOST_EXEMODE_LPBK1)
            return;

        // Normal (non-atomic) is a simple comparison
        if (he_lpbk_cfg_.AtomicFunc == HOSTEXE_ATOMIC_OFF) {
            host_exe_->compare(source_, destination_);
            return;
        }

        // Atomic mode is a far more complicated comparison. The source buffer
        // is modified and some of the original state is copied to the destination.
        bool size_is_4 = ((he_lpbk_cfg_.AtomicFunc & 2) == 0);
        bool is_fetch_add = (he_lpbk_cfg_.AtomicFunc & 0xc) == 0;
        bool is_swap = (he_lpbk_cfg_.AtomicFunc & 0xc) == 4;
        bool is_cas = (he_lpbk_cfg_.AtomicFunc & 0xc) == 8;

        // Ignore the last entry to work around an off by one copy error
        for (uint64_t i = 0; i < source_->size()/CL; i += 1) {
            // In source_, the first entry in every line is the atomically modified
            // value. The second entry holds the original value, hashed with a constant
            // so it isn't a simple repetition.
            uint64_t src_a = source_->read<uint64_t>(i*CL);
            // Read the original value and reverse the hash.
            uint64_t src_b = source_->read<uint64_t>(i*CL + 8) ^ 0xabababababababab;
            uint64_t upd_a = 0;

            // Compute expected value
            if (is_fetch_add) {
                // Hardware added the line index (i) to each line, either preserving
                // the high 4 bytes or modifying all 8 bytes.
                if (size_is_4)
                    upd_a = (src_b & 0xffffffff00000000) | ((src_b + i) & 0xffffffff);
                else
                    upd_a = src_b + i;
            }
            if (is_swap) {
                // Hardware swapped the line index (i) into each line
                if (size_is_4)
                    upd_a = (src_b & 0xffffffff00000000) | (i & 0xffffffff);
                else
                    upd_a = i;
            }
            if (is_cas) {
                // Hardware swapped the bit inverse of line index (i) in each line when
                // the original value is the line index.
                if (size_is_4)
                    upd_a = ((src_b & 0xffffffff) == i) ? (src_b & 0xffffffff00000000) | (~i & 0xffffffff) : src_b;
                else
                    upd_a = (src_b == i) ? ~i : src_b;
            }

            if (upd_a != src_a)
                throw std::runtime_error("Atomic update error");

            // The destination is comparatively easy. For all functions it is
            // simply the original source value.
            uint64_t dst_a = destination_->read<uint64_t>(i*CL);
            if (size_is_4) {
                src_b &= 0xffffffff;
                dst_a &= 0xffffffff;
            }

            if (dst_a != src_b)
                throw std::runtime_error("Atomic read error or write error");
        }
    }

    bool he_continuousmode()
    {
        uint32_t count = 0;
        if (host_exe_->he_continuousmode_ && host_exe_->he_contmodetime_ > 0)
        { 
            std::cout << "host exerciser continuous mode time:"<<
	            host_exe_->he_contmodetime_ << " seconds" << std::endl;
            std::cout << "Ctrl+C  to stop continuous mode" << std::endl;
            while (!g_he_exit) {
                sleep(1);
                count++;
                if (count > host_exe_->he_contmodetime_)
                    break;
            }
            he_forcetestcmpl();
            he_perf_counters();
        }
        return true;
    }

    int parse_input_options()
    {

        if (!host_exe_)
            return -1;

        // Host Exerciser Mode
        he_lpbk_cfg_.TestMode = host_exe_->he_modes_;

        // Host Exerciser Read
        he_lpbk_cfg_.ReqLen = host_exe_->he_req_cls_len_;

        // Host Exerciser  lpbk delay
        if (host_exe_->he_delay_)
             he_lpbk_cfg_.DelayEn = 1;

        //test rollover or test termination
        if (host_exe_->he_continuousmode_)
             he_lpbk_cfg_.Continuous = 1;

        // Set interleave in Throughput
        if (he_lpbk_cfg_.TestMode == HOST_EXEMODE_TRUPT) {
              he_lpbk_cfg_.TputInterleave = host_exe_->he_interleave_;
        }

        // Set Interrupt test mode
        if (host_exe_->he_interrupt_ <= 3) {
            he_lpbk_cfg_.IntrTestMode = 1;
        }

        // Atomic functions
        he_lpbk_cfg_.AtomicFunc = host_exe_->he_req_atomic_func_;
        if (he_lpbk_cfg_.AtomicFunc != HOSTEXE_ATOMIC_OFF) {
            if (he_lpbk_cfg_.ReqLen != HOSTEXE_CLS_1) {
                std::cerr << "Atomic function mode requires cl_1" << std::endl;
                return -1;
            }
        }

        if (host_exe_->he_continuousmode_  && 
            (he_lpbk_cfg_.IntrTestMode == 1)) {
            std::cerr << "Interrupts not supported in continuous mode"
                << std::endl;
            return -1;
        }

        if (host_exe_->he_continuousmode_ &&
            (host_exe_->he_contmodetime_ == 0)) {
            std::cerr << "Please add cmd line input continuous mode time"
                << std::endl;
            return -1;
        }

        return 0;
    }


    virtual int run(test_afu *afu, CLI::App *app)
    {
        (void)app;

        auto d_afu = dynamic_cast<host_exerciser*>(afu);
        host_exe_ = dynamic_cast<host_exerciser*>(afu);

        token_ = d_afu->get_token();

        auto ret = parse_input_options();
        if (ret != 0) {
            std::cerr << "Failed to parse input options" << std::endl;
            return ret;
        }
        std::cout << "Input Config:" << he_lpbk_cfg_.value << std::endl;

        if (0 == host_exe_->he_clock_mhz_) {
            // Does the AFU record its clock info?
            uint16_t freq = host_exe_->read64(HE_INFO0);
            if (freq) {
                host_exe_->he_clock_mhz_ = freq;
                std::cout << "AFU clock: "
                          << host_exe_->he_clock_mhz_ << " MHz" << std::endl;
            }
            else {
                host_exe_->he_clock_mhz_ = 350;
                std::cout << "Frequency of AFU clock unknown. Assuming "
                          << host_exe_->he_clock_mhz_ << " MHz." << std::endl;
            }
        }

        // assert reset he-lpbk
        he_lpbk_ctl_.value = 0;
        d_afu->write32(HE_CTL, he_lpbk_ctl_.value);
        usleep(1000);


        // deassert reset he-lpbk
        he_lpbk_ctl_.value = 0;
        he_lpbk_ctl_.ResetL = 1;
        d_afu->write32(HE_CTL, he_lpbk_ctl_.value);


        /* Allocate Source Buffer
        Write to CSR_SRC_ADDR */
        std::cout << "Allocate SRC Buffer" << std::endl;
        source_ = d_afu->allocate(LPBK1_BUFFER_ALLOCATION_SIZE);
        d_afu->write64(HE_SRC_ADDR, cacheline_aligned_addr(source_->io_address()));
        he_init_src_buffer(source_);

        /* Allocate Destination Buffer
            Write to CSR_DST_ADDR */
        std::cout << "Allocate DST Buffer" << std::endl;
        destination_ = d_afu->allocate(LPBK1_BUFFER_ALLOCATION_SIZE);
        d_afu->write64(HE_DST_ADDR, cacheline_aligned_addr(destination_->io_address()));
        std::fill_n(destination_->c_type(), LPBK1_BUFFER_SIZE, 0xBE);

        /* Allocate DSM Buffer
            Write to CSR_AFU_DSM_BASEL */
        std::cout << "Allocate DSM Buffer" << std::endl;
        dsm_ = d_afu->allocate(LPBK1_DSM_SIZE);
        d_afu->write32(HE_DSM_BASEL, cacheline_aligned_addr(dsm_->io_address()));
        d_afu->write32(HE_DSM_BASEH, cacheline_aligned_addr(dsm_->io_address()) >> 32);
        std::fill_n(dsm_->c_type(), LPBK1_DSM_SIZE, 0x0);


        // Number of cache lines
        d_afu->write64(HE_NUM_LINES, (LPBK1_BUFFER_SIZE / (1 * CL)) -1);

        // Write to CSR_CFG
        d_afu->write64(HE_CFG, he_lpbk_cfg_.value);

        event::ptr_t ev = nullptr;
        if (he_lpbk_cfg_.IntrTestMode == 1) {
            he_interrupt_.VectorNum = host_exe_->he_interrupt_;
            d_afu->write32(HE_INTERRUPT0, he_interrupt_.value);
            ev = d_afu->register_interrupt(host_exe_->he_interrupt_);
            std::cout << "Using Interrupts\n";
        }

        if (host_exe_->should_log(spdlog::level::debug)) {
            std::cout << std::endl;
            he_dump_buffer(source_, "Pre-execution source");
            he_dump_buffer(destination_, "Pre-execution destination");
            std::cout << std::endl;
        }

        // Write to CSR_CTL
        std::cout << "Start Test" << std::endl;
        he_lpbk_ctl_.value = 0;
        he_lpbk_ctl_.Start = 1;
        he_lpbk_ctl_.ResetL = 1;
        d_afu->write32(HE_CTL, he_lpbk_ctl_.value);

        // Interrupt test mode
        if (he_lpbk_cfg_.IntrTestMode == 1) {
            if (!he_interrupt(ev)) {
	            return -1;
            }
            he_compare_buffer();
            he_perf_counters();
        } else if (host_exe_->he_continuousmode_) {
            // Continuous mode
            he_continuousmode();
            return 0;
        } else {
            // Regular mode
            if (!he_wait_test_completion()) {
                return -1;
            }

            if (host_exe_->should_log(spdlog::level::debug)) {
                std::cout << std::endl;
                he_dump_buffer(source_, "Post-execution source");
                he_dump_buffer(destination_, "Post-execution destination");
                std::cout << std::endl;
            }

            he_compare_buffer();
            he_perf_counters();
        }

        return 0;
    }

protected:
    he_cfg he_lpbk_cfg_;
    he_ctl he_lpbk_ctl_;
    host_exerciser *host_exe_;
    shared_buffer::ptr_t source_;
    shared_buffer::ptr_t destination_;
    shared_buffer::ptr_t dsm_;
    he_interrupt0 he_interrupt_;
    token::ptr_t token_;
};

} // end of namespace host_exerciser

