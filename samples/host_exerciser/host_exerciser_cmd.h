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
#include <opae/types_enum.h>
#include <opae/cxx/core.h>

using test_afu = opae::afu_test::afu;
using opae::fpga::types::shared_buffer;
using opae::fpga::types::token;
namespace fpga = opae::fpga::types;

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
          he_lpbk_max_reqlen_ = HOSTEXE_CLS_8;
          he_lpbk_api_ver_ = 0;
          he_lpbk_atomics_supported_ = false;
          is_ase_sim_ = false;
    }
    virtual ~host_exerciser_cmd() {}

    void host_exerciser_status()
    {
        he_status0 he_status0;
        he_status1 he_status1;

        he_status0.value = host_exe_->read64(HE_STATUS0);
        he_status1.value = host_exe_->read64(HE_STATUS1);

        uint64_t tmp;

        tmp = he_status0.numReads;
        host_exe_->logger_->info("Host Exerciser numReads: {0}", tmp);
        tmp = he_status0.numWrites;
        host_exe_->logger_->info("Host Exerciser numWrites: {0}", tmp);

        tmp = he_status1.numPendReads;
        host_exe_->logger_->info("Host Exerciser numPendReads: {0}", tmp);
        tmp = he_status1.numPendWrites;
        host_exe_->logger_->info("Host Exerciser numPendWrites: {0}", tmp);

        tmp = he_status1.numPendEmifReads;
        host_exe_->logger_->info("Host Exerciser numPendEmifReads: {0}", tmp);
        tmp = he_status1.numPendEmifWrites;
        host_exe_->logger_->info("Host Exerciser numPendEmifWrites: {0}", tmp);
    }

    void host_exerciser_errors()
    {
        he_error he_error;

        if (host_exe_ == NULL)
            return;

        he_error.value = host_exe_->read64(HE_ERROR);

        std::cout << "Host Exerciser Error:" << he_error.error << std::endl;

    }

    uint64_t host_exerciser_swtestmsg()
    {
        uint64_t swtest_msg;

        if (host_exe_ == NULL)
            return 0;

        swtest_msg = host_exe_->read64(HE_SWTEST_MSG);
        if (swtest_msg) {
            std::cout << "Host Exerciser swtest msg:" << swtest_msg << std::endl;
        }

        return swtest_msg;
    }

    inline uint64_t cacheline_aligned_addr(uint64_t num) {
        return num >> LOG2_CL;
    }

    // Convert number of transactions to bandwidth (GB/s)
    double he_num_xfers_to_bw(uint64_t num_lines, uint64_t num_ticks)
    {
        return (double)(num_lines * 64) / ((1000.0 / host_exe_->he_clock_mhz_ * num_ticks));
    }

    inline uint64_t dsm_num_ticks(const volatile he_dsm_status *dsm_status)
    {
        return dsm_status->num_ticks;
    }

    // Read count has two parts -- the size was increased after the first release
    inline uint64_t dsm_num_reads(const volatile he_dsm_status *dsm_status)
    {
        return (uint64_t(dsm_status->num_reads_h) << 32) | dsm_status->num_reads_l;
    }

    // Write count has two parts -- the size was increased after the first release
    inline uint64_t dsm_num_writes(const volatile he_dsm_status *dsm_status)
    {
        return (uint64_t(dsm_status->num_writes_h) << 32) | dsm_status->num_writes_l;
    }

    void he_perf_counters()
    {
        volatile he_dsm_status *dsm_status = NULL;
        volatile uint8_t* status_ptr = dsm_->c_type();
        uint64_t num_cache_lines = 0;
        if (!status_ptr)
           return;

        dsm_status = reinterpret_cast<he_dsm_status *>((uint8_t*)status_ptr);
        if (!dsm_status)
            return;

        host_exe_->logger_->info("Host Exerciser Performance Counter:");
        // calculate number of cache lines in continuous mode
        if (host_exe_->he_continuousmode_) {
            if (he_lpbk_cfg_.TestMode == HOST_EXEMODE_READ)
                num_cache_lines = dsm_num_reads(dsm_status);
            else if (he_lpbk_cfg_.TestMode == HOST_EXEMODE_WRITE)
                num_cache_lines = (dsm_num_writes(dsm_status));
            else
                num_cache_lines = dsm_num_reads(dsm_status) + dsm_num_writes(dsm_status);
        } else {
            num_cache_lines = (LPBK1_BUFFER_SIZE / (1 * CL));
        }

        host_exerciser_status();

        uint64_t tmp;

        tmp = dsm_num_ticks(dsm_status);
        host_exe_->logger_->info("Number of clocks: {0}", tmp);
        tmp = dsm_num_reads(dsm_status);
        host_exe_->logger_->info("Total number of Reads sent: {0}", tmp);
        tmp = dsm_num_writes(dsm_status);
        host_exe_->logger_->info("Total number of Writes sent: {0}", tmp);

        // print bandwidth
        if (dsm_num_ticks(dsm_status) > 0) {
            double perf_data = he_num_xfers_to_bw(num_cache_lines, dsm_num_ticks(dsm_status));
            host_exe_->logger_->info("Bandwidth: {0:0.3f} GB/s", perf_data);
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
        uint32_t timeout = HELPBK_TEST_TIMEOUT;
        if (is_ase_sim_) {
            // Much longer timeout when running HW simulation
            timeout *= 100;
        }

        volatile uint8_t* status_ptr = dsm_->c_type();

        while (0 == ((*status_ptr) & 0x1)) {
            usleep(HELPBK_TEST_SLEEP_INVL);
            if (--timeout == 0) {
                std::cout << "HE LPBK TIME OUT" << std::endl;
                host_exerciser_errors();
                host_exerciser_status();
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
        // the value at the start of each line to the second position so
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

    uint64_t he_compare_buffer()
    {
        uint64_t status = host_exerciser_swtestmsg();
        if (status) {
            std::cerr << "HE LB CSR error flag is set" << std::endl;
            return status;
        }

        // Compare buffer contents only loopback test mode
        if (he_lpbk_cfg_.TestMode != HOST_EXEMODE_LPBK1)
            return 0;

        // Normal (non-atomic) is a simple comparison
        if (he_lpbk_cfg_.AtomicFunc == HOSTEXE_ATOMIC_OFF) {
            return (source_->compare(destination_, source_->size()));
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

            if (upd_a != src_a) {
                std::cerr << "Atomic update error" << std::endl;
                return 1;
            }

            // The destination is comparatively easy. For all functions it is
            // simply the original source value.
            uint64_t dst_a = destination_->read<uint64_t>(i*CL);
            if (size_is_4) {
                src_b &= 0xffffffff;
                dst_a &= 0xffffffff;
            }

            if (dst_a != src_b) {
                std::cerr << "Atomic read error or write error" << std::endl;
                return 1;
            }
        }

        return 0;
    }

    bool he_continuousmode()
    {
        uint32_t count = 0;
        if (host_exe_->he_continuousmode_ && host_exe_->he_contmodetime_ > 0)
        { 
            host_exe_->logger_->debug("continuous mode time: {0} seconds", host_exe_->he_contmodetime_);
            host_exe_->logger_->debug("Ctrl+C  to stop continuous mode");

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

    inline void set_cfg_reqlen(hostexe_req_len req_len)
    {
        // Legal request lengths have grown over time, leading to the field being
        // split in the configuration register.
        he_lpbk_cfg_.ReqLen_High = req_len >> 2;
        he_lpbk_cfg_.ReqLen = req_len & 3;
    }

    inline void set_cfg_reqlen(uint32_t req_len)
    {
        set_cfg_reqlen(static_cast<hostexe_req_len>(req_len));
    }

    inline hostexe_req_len get_cfg_reqlen()
    {
        return static_cast<hostexe_req_len>((he_lpbk_cfg_.ReqLen_High << 2) |
                                            he_lpbk_cfg_.ReqLen);
    }

    int parse_input_options()
    {

        if (!host_exe_)
            return -1;

        // ASE simulation?
        auto afu_props = fpga::properties::get(token_);
        uint32_t vendor_id = afu_props->vendor_id;
        uint32_t device_id = afu_props->device_id;
        is_ase_sim_ = (vendor_id == 0x8086) && (device_id == 0xa5e);
        if (is_ase_sim_) {
            std::cout << "Simulation: ASE mode" << std::endl;
        }

        // Host Exerciser Mode
        he_lpbk_cfg_.TestMode = host_exe_->he_modes_;

        // Host Exerciser Read
        if (host_exe_->he_req_cls_len_ > he_lpbk_max_reqlen_) {
            std::cerr << "Request length " << host_exe_->he_req_cls_len_
                      << " is not supported by this platform." << std::endl;
            return -1;
        }
        set_cfg_reqlen(host_exe_->he_req_cls_len_);

        // Host Exerciser  lpbk delay
        if (host_exe_->he_delay_)
             he_lpbk_cfg_.DelayEn = 1;

        //test rollover or test termination
        if (host_exe_->he_continuousmode_)
             he_lpbk_cfg_.Continuous = 1;

        // Set interleave in Throughput
        if (he_lpbk_cfg_.TestMode == HOST_EXEMODE_TRPUT) {
              he_lpbk_cfg_.TputInterleave = host_exe_->he_interleave_;
        }

        // Set Interrupt test mode
        if (host_exe_->option_passed("--interrupt")) {
            if (host_exe_->he_interrupt_ < afu_props->num_interrupts) {
                he_lpbk_cfg_.IntrTestMode = 1;
            } else {
                std::cerr << "Invalid Input interrupts vector number:" << host_exe_->he_interrupt_ << std::endl;
                std::cout << "Please enter Interrupts vector range 0 to " << afu_props->num_interrupts -1 << std::endl;
                return -1;
            }
        }

        // Atomic functions
        he_lpbk_cfg_.AtomicFunc = host_exe_->he_req_atomic_func_;
        if (he_lpbk_cfg_.AtomicFunc != HOSTEXE_ATOMIC_OFF) {
            if (!he_lpbk_atomics_supported_) {
                std::cerr << "The platform does not support atomic functions" << std::endl;
                return -1;
            }
            if ((get_cfg_reqlen() != HOSTEXE_CLS_1) && (he_lpbk_cfg_.TestMode == HOST_EXEMODE_LPBK1)) {
                std::cerr << "Atomic function in lpbk mode requires cl_1" << std::endl;
                return -1;
            }
        }

        // Encoding
        he_lpbk_cfg_.Encoding = host_exe_->he_req_encoding_;
        if ((he_lpbk_cfg_.Encoding != HOSTEXE_ENCODING_DEFAULT) && (he_lpbk_api_ver_ == 0)) {
            std::cerr << "Host exerciser hardware API version 0 does not support encoding changes" << std::endl;
            return -1;
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

    // The test state has been configured. Run one test instance.
    int run_single_test()
    {
        int status = 0;

        // Clear the status buffer
        std::fill_n(dsm_->c_type(), LPBK1_DSM_SIZE, 0x0);

        host_exe_->logger_->debug("Start Test");

        // Write to CSR_CFG
        host_exe_->write64(HE_CFG, he_lpbk_cfg_.value);
        host_exe_->logger_->debug("Input Config: {0}", he_lpbk_cfg_.value);

        event::ptr_t ev = nullptr;
        if (he_lpbk_cfg_.IntrTestMode == 1) {
            he_interrupt_.VectorNum = host_exe_->he_interrupt_;
            host_exe_->write32(HE_INTERRUPT0, he_interrupt_.value);
            ev = host_exe_->register_interrupt(host_exe_->he_interrupt_);
            std::cout << "Using Interrupts\n";
        }

        if (host_exe_->logger_->should_log(spdlog::level::debug)) {
            std::cout << std::endl;
            he_dump_buffer(source_, "Pre-execution source");
            he_dump_buffer(destination_, "Pre-execution destination");
            std::cout << std::endl;
        }

        // Write to CSR_CTL
        he_lpbk_ctl_.value = 0;
        he_lpbk_ctl_.Start = 1;
        he_lpbk_ctl_.ResetL = 1;
        host_exe_->write32(HE_CTL, he_lpbk_ctl_.value);

        // Interrupt test mode
        if (he_lpbk_cfg_.IntrTestMode == 1) {
            if (!he_interrupt(ev)) {
                status = -1;
                if (host_exe_->logger_->should_log(spdlog::level::debug)) {
                    std::cout << std::endl;
                    he_dump_buffer(source_, "Post-execution source");
                    he_dump_buffer(destination_, "Post-execution destination");
                    std::cout << std::endl;
                }
            }
            else {
                status = he_compare_buffer();
                if (host_exe_->logger_->should_log(spdlog::level::debug)) {
                    std::cout << std::endl;
                    he_dump_buffer(source_, "Post-execution source");
                    he_dump_buffer(destination_, "Post-execution destination");
                    std::cout << std::endl;
                }
                he_perf_counters();
            }
        } else if (host_exe_->he_continuousmode_) {
            // Continuous mode
            he_continuousmode();

            if (host_exe_->logger_->should_log(spdlog::level::debug)) {
                std::cout << std::endl;
                he_dump_buffer(source_, "Post-execution source");
                he_dump_buffer(destination_, "Post-execution destination");
                std::cout << std::endl;
            }
        } else {
            // Regular mode
            if (!he_wait_test_completion()) {
                status = -1;
            }
            else {
                if (host_exe_->logger_->should_log(spdlog::level::debug)) {
                    std::cout << std::endl;
                    he_dump_buffer(source_, "Post-execution source");
                    he_dump_buffer(destination_, "Post-execution destination");
                    std::cout << std::endl;
                }

                status = he_compare_buffer();
                he_perf_counters();
            }
        }

        // assert reset he-lpbk
        he_lpbk_ctl_.value = 0;
        host_exe_->write32(HE_CTL, he_lpbk_ctl_.value);
        usleep(1000);

        // deassert reset he-lpbk
        he_lpbk_ctl_.value = 0;
        he_lpbk_ctl_.ResetL = 1;
        host_exe_->write32(HE_CTL, he_lpbk_ctl_.value);

        return status;
    }

    // Sequence through all the test modes
    int run_all_tests()
    {
        int status = 0;

        // Start with loopback tests
        he_lpbk_cfg_.TestMode = HOST_EXEMODE_LPBK1;
        he_lpbk_cfg_.AtomicFunc = HOSTEXE_ATOMIC_OFF;
        host_exe_->he_continuousmode_ = false;
        he_lpbk_cfg_.Continuous = 0;

        // Test multiple request sizes
        std::cout << std::endl << "Testing loopback, varying payload size:" << std::endl;
        for (std::map<std::string, uint32_t>::const_iterator cls=he_req_cls_len.begin(); cls!=he_req_cls_len.end(); ++cls) {
            // Set request length
            if (cls->second > he_lpbk_max_reqlen_) break;
            set_cfg_reqlen(cls->second);

            // Initialize buffer values
            he_init_src_buffer(source_);
            std::fill_n(destination_->c_type(), LPBK1_BUFFER_SIZE, 0xBE);

            int test_status = run_single_test();
            status |= test_status;

            std::cout << "  " << cls->first << ": "
                      << (test_status ? "FAIL" : "PASS") << std::endl;
        }
        set_cfg_reqlen(HOSTEXE_CLS_1);

        // Test atomic functions if the API supports it
        if (he_lpbk_atomics_supported_) {
            std::cout << std::endl << "Testing atomic functions:" << std::endl;
            for (std::map<std::string, uint32_t>::const_iterator af=he_req_atomic_func.begin(); af!=he_req_atomic_func.end(); ++af) {
                // Don't test "off"
                if (af->second == HOSTEXE_ATOMIC_OFF) continue;

                he_lpbk_cfg_.AtomicFunc = af->second;

                // Initialize buffer values
                he_init_src_buffer(source_);
                std::fill_n(destination_->c_type(), LPBK1_BUFFER_SIZE, 0xBE);

                int test_status = run_single_test();
                status |= test_status;

                std::cout << "  " << af->first << ": "
                          << (test_status ? "FAIL" : "PASS") << std::endl;
            }

            std::cout << std::endl << "Testing atomic functions interspersed in continuous mode:" << std::endl;
            uint32_t trip = 0;
            for (std::map<std::string, uint32_t>::const_reverse_iterator cls=he_req_cls_len.rbegin(); cls!=he_req_cls_len.rend(); ++cls) {
                he_lpbk_cfg_.TestMode = HOST_EXEMODE_TRPUT;

                if (cls->second > he_lpbk_max_reqlen_) break;
                set_cfg_reqlen(cls->second);

                he_lpbk_cfg_.AtomicFunc = (trip & 1 ? HOSTEXE_ATOMIC_FADD_4 : HOSTEXE_ATOMIC_CAS_8);
                host_exe_->he_continuousmode_ = true;
                he_lpbk_cfg_.Continuous = 1;

                int test_status = run_single_test();
                status |= test_status;

                std::cout << "  " << cls->first
                          <<" and " << (trip & 1 ? "fadd_4" : "cas_8") << ": "
                          << (test_status ? "FAIL" : "PASS") << std::endl;

                trip += 1;
            }

            he_lpbk_cfg_.AtomicFunc = HOSTEXE_ATOMIC_OFF;
        }

        // Test throughput, varying read, write and size
        std::cout << std::endl << "Testing throughput (GB/s), varying payload size ("
                  << host_exe_->he_contmodetime_ << " seconds each):" << std::endl;
        std::cout.setf(std::ios::fixed);
        std::cout.precision(2);

        // Get a pointer to the device status memory, needed to compute throughput
        volatile he_dsm_status *dsm_status =
            reinterpret_cast<he_dsm_status *>((uint8_t*)dsm_->c_type());

        host_exe_->he_continuousmode_ = true;
        he_lpbk_cfg_.Continuous = 1;
        for (std::map<std::string, uint32_t>::const_iterator cls=he_req_cls_len.begin(); cls!=he_req_cls_len.end(); ++cls) {
            if (cls->second > he_lpbk_max_reqlen_) break;
            set_cfg_reqlen(cls->second);

            he_lpbk_cfg_.TestMode = HOST_EXEMODE_READ;
            int test_status = run_single_test();
            status |= test_status;
            std::cout << "  " << cls->first << " READ:  ";
            if (test_status)
                std::cout << "FAIL" << std::endl;
            else {
                std::cout << he_num_xfers_to_bw(dsm_num_reads(dsm_status), dsm_num_ticks(dsm_status))
                          << std::endl;
            }

            he_lpbk_cfg_.TestMode = HOST_EXEMODE_WRITE;
            test_status = run_single_test();
            status |= test_status;
            std::cout << "  " << cls->first << " WRITE: ";
            if (test_status)
                std::cout << "FAIL" << std::endl;
            else {
                std::cout << he_num_xfers_to_bw(dsm_num_writes(dsm_status), dsm_num_ticks(dsm_status))
                          << std::endl;
            }

            he_lpbk_cfg_.TestMode = HOST_EXEMODE_TRPUT;
            test_status = run_single_test();
            status |= test_status;
            std::cout << "  " << cls->first << " TRPUT: ";
            if (test_status)
                std::cout << "FAIL" << std::endl;
            else {
                std::cout << he_num_xfers_to_bw(dsm_num_reads(dsm_status) + dsm_num_writes(dsm_status),
                                                dsm_num_ticks(dsm_status))
                          << std::endl;
            }

            std::cout << std::endl;
        }

        return status;
    }

    virtual int run(test_afu *afu, CLI::App *app)
    {
        (void)app;

        auto d_afu = dynamic_cast<host_exerciser*>(afu);
        host_exe_ = dynamic_cast<host_exerciser*>(afu);

        token_ = d_afu->get_token();
        token_device_ = d_afu->get_token_device();

        // Check if memory calibration has failed and error out before proceeding
        // with the test. The dfl-emif driver creates sysfs entries to report the
        // calibration status for each memory channel. sysobjects are the OPAE-API's
        // abstraction for sysfs entries. However, at this time, these are only
        // accessible through tokens that use the xfpga plugin and not the vfio
        // plugin. Hence our use of the DEVICE token (token_device_). One
        // non-ideality of the following implementation is the use of
        // MAX_NUM_MEM_CHANNELS. We are essentially doing a brute-force query of
        // sysfs entries since we don't know how many mem channels exist on the
        // given platform. What about glob wildcards? Why not simply glob for
        // "*dfl*/**/inf*_cal_fail" and use the OPAE-API's support for arrays of
        // sysobjects? The reason is that, at the time of this writing, the
        // xfpga-plugin's sysobject implementation does not support arrays
        // specifically when the glob contains a recursive wildcard "/**/". It's a
        // strange and perhaps unnecessary limitation. Therefore, future work is to
        // fix that and clean up the code below.
        for (size_t i = 0; i < MAX_NUM_MEM_CHANNELS; i++) {
          std::stringstream mem_cal_glob;
          // Construct the glob string to search for the cal_fail sysfs entry
          // for the i'th mem channel
          mem_cal_glob << "*dfl*/**/inf" << i << "_cal_fail";
          // Ask for a sysobject with this glob string
          fpga::sysobject::ptr_t testobj = fpga::sysobject::get(
              token_device_, mem_cal_glob.str().c_str(), FPGA_OBJECT_GLOB);

          // if test obj !=null, the sysfs entry was found.
          // Read the calibration status from the sysfs entry.
          // A non-zero value (typically '1') means
          // calibration has failed --> we error out.
          if (testobj && testobj->read64(0)) { 
            std::cout
                << "This sysfs entry reports that memory calibration has failed:"
                << mem_cal_glob.str().c_str() << std::endl;
            return -1;
          }
        }


        // Read HW details
        uint64_t he_info = host_exe_->read64(HE_INFO0);
        he_lpbk_api_ver_ = (he_info >> 16);
        std::cout << "API version: " << uint32_t(he_lpbk_api_ver_) << std::endl;

        // For atomics support, the version must not be zero and bit 24 must be 0.
        he_lpbk_atomics_supported_ = (he_lpbk_api_ver_ != 0) &&
                                     (0 == ((he_info >> 24) & 1));

        // The maximum request length before API version 2 was 8.
        he_lpbk_max_reqlen_ = (he_lpbk_api_ver_ < 2) ? HOSTEXE_CLS_8 : HOSTEXE_CLS_16;

        if (0 == host_exe_->he_clock_mhz_) {
            uint16_t freq = he_info;
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
        else {
            std::cout << "AFU clock from command line: "
                      << host_exe_->he_clock_mhz_ << " MHz" << std::endl;
        }

        auto ret = parse_input_options();
        if (ret != 0) {
            std::cerr << "Failed to parse input options" << std::endl;
            return ret;
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
        try { 
            std::cout << "Allocate SRC Buffer" << std::endl;
            source_ = d_afu->allocate(LPBK1_BUFFER_ALLOCATION_SIZE);
        }
        catch (opae::fpga::types::except &ex) {
            std::cerr << "SRC Buffer allocation failed. Please check that hugepages are reserved." << std::endl;
            throw;
        }
        host_exe_->logger_->debug("    VA 0x{0}  IOVA 0x{1:x}",
                                  (void*)source_->c_type(), source_->io_address());
        d_afu->write64(HE_SRC_ADDR, cacheline_aligned_addr(source_->io_address()));
        he_init_src_buffer(source_);

        /* Allocate Destination Buffer
            Write to CSR_DST_ADDR */
        try {
            std::cout << "Allocate DST Buffer" << std::endl;
            destination_ = d_afu->allocate(LPBK1_BUFFER_ALLOCATION_SIZE);
        }
        catch (fpga::except &ex) {
            std::cerr << "DST Buffer allocation failed. Please check that hugepages are reserved." << std::endl;
            throw;
        }
        host_exe_->logger_->debug("    VA 0x{0}  IOVA 0x{1:x}",
                                  (void*)destination_->c_type(), destination_->io_address());
        d_afu->write64(HE_DST_ADDR, cacheline_aligned_addr(destination_->io_address()));
        std::fill_n(destination_->c_type(), LPBK1_BUFFER_SIZE, 0xBE);

        /* Allocate DSM Buffer
            Write to CSR_AFU_DSM_BASEL */
        try {
            std::cout << "Allocate DSM Buffer" << std::endl;
            dsm_ = d_afu->allocate(LPBK1_DSM_SIZE);
        }
        catch (fpga::except &ex) {
            std::cerr << "DSM Buffer allocation failed. Please check that hugepages are reserved." << std::endl;
            throw;
        }
        host_exe_->logger_->debug("    VA 0x{0}  IOVA 0x{1:x}",
                                  (void*)dsm_->c_type(), dsm_->io_address());
        d_afu->write32(HE_DSM_BASEL, cacheline_aligned_addr(dsm_->io_address()));
        d_afu->write32(HE_DSM_BASEH, cacheline_aligned_addr(dsm_->io_address()) >> 32);
        std::fill_n(dsm_->c_type(), LPBK1_DSM_SIZE, 0x0);

        // Number of cache lines
        d_afu->write64(HE_NUM_LINES, (LPBK1_BUFFER_SIZE / (1 * CL)) -1);

        int status = 0;
        if (host_exe_->he_test_all_)
            status = run_all_tests();
        else
            status = run_single_test();

        return status;
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
    token::ptr_t token_device_;
    hostexe_req_len he_lpbk_max_reqlen_;
    uint8_t he_lpbk_api_ver_;
    bool he_lpbk_atomics_supported_;
    bool is_ase_sim_;
};

} // end of namespace host_exerciser

