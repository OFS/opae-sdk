// Copyright(c) 2007-2016, Intel Corporation
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

#include "test_cci_mpf_null.h"
#include <time.h>
#include <boost/format.hpp>
#include <stdlib.h>
#include <immintrin.h>

// ========================================================================
//
// Each test must provide these functions used by main to find the
// specific test instance.
//
// ========================================================================

const char* testAFUID()
{
    return "BFD75B03-9608-4E82-AE22-F61A62B8F992";
}

void testConfigOptions(po::options_description &desc)
{
    // Add test-specific options
    desc.add_options()
        ("rd-vc", po::value<int>()->default_value(5), "Channel (0=VA, 1=VL0, 2=VH0, 3=VH1, 4=VH0+VH1, 5=VL0+VH0+VH1)")
        ("wr-vc", po::value<int>()->default_value(4), "Channel (0=VA, 1=VL0, 2=VH0, 3=VH1, 4=VH0+VH1, 5=VL0+VH0+VH1)")
        ("dsm-vc", po::value<int>()->default_value(2), "Result signaling channel (0=VA, 1=VL0, 2=VH0, 3=VH1)")
        ("rdline-mode", po::value<string>()->default_value("I"), "Emit read requests with mode (I [invalid], S [shared])")
        ("wrline-mode", po::value<string>()->default_value("I"), "Emit write requests with mode (I [invalid], M [modified], or P [push])")
        ("mcl", po::value<int>()->default_value(0), "Multi-line requests (0 for multiple sizes)")
        ("repeat", po::value<int>()->default_value(1), "Number of repetitions")
        ("tc", po::value<int>()->default_value(0), "Test length (cycles)")
        ("ts", po::value<int>()->default_value(1), "Test length (seconds)")
        ;
}

CCI_TEST* allocTest(const po::variables_map& vm, SVC_WRAPPER& svc)
{
    return new TEST_CCI_MPF_NULL(vm, svc);
}


// ========================================================================
//
// cci_mpf_null test.
//
// ========================================================================

int TEST_CCI_MPF_NULL::test()
{
    // Allocate memory for control
    uint64_t dsm_pa;
    volatile uint64_t* dsm = (uint64_t*) this->malloc(4096, &dsm_pa);
    assert(NULL != dsm);
    memset((void*)dsm, 0, 4096);

    // Allocate memory for read/write tests.  The HW indicates the expected
    // number of 2MB memory buffers in CSR 0.
    uint32_t n_buffers = readTestCSR(0);
    
    volatile uint64_t** buf = new volatile uint64_t*[n_buffers];
    uint64_t* buf_pa = new uint64_t[n_buffers];

    for (uint32_t i = 0; i < n_buffers; i += 1)
    {
        const uint64_t n_bytes = 2 * 1024 * 1024;
        buf[i] = (uint64_t*) this->malloc(n_bytes, &buf_pa[i]);
        assert(NULL != buf[i]);
        memset((void*)buf[i], 0, n_bytes);

        // Initialize the buffer with the expected pattern.  For start up that's
        // just the line index in the high bits of the line.
        for (uint32_t j = 0; j < (n_bytes / CL(1)); j += 1)
        {
            buf[i][j*8 + 7] = uint64_t(j) << 48;
        }
    }

    //
    // Configure the HW test
    //
    writeTestCSR(1, uint64_t(dsm_pa) / CL(1));
    for (uint32_t i = 0; i < n_buffers; i += 1)
    {
        writeTestCSR(2, uint64_t(buf_pa[i]) / CL(1));
    }

    // What's the AFU frequency (MHz)?
    uint64_t afu_mhz = getAFUMHz();

    uint64_t cycles = uint64_t(vm["tc"].as<int>());
    if (cycles == 0)
    {
        // Didn't specify --tc.  Use seconds instead.
        cycles = uint64_t(vm["ts"].as<int>()) * afu_mhz * 1000 * 1000;
    }

    const uint64_t counter_bits = 40;
    if (cycles & (int64_t(-1) << counter_bits))
    {
        cerr << "Run length overflows " << counter_bits << " bit counter" << endl;
        exit(1);
    }

    uint64_t rdline_mode;
    switch (toupper(vm["rdline-mode"].as<string>()[0]))
    {
      case 'I':
        rdline_mode = 0;
        break;
      case 'S':
        rdline_mode = 1;
        break;
      default:
        cerr << "Invalid --rdline-mode.  Expected I or S." << endl;
        exit(1);
    }

    uint64_t wrline_mode;
    switch (toupper(vm["wrline-mode"].as<string>()[0]))
    {
      case 'I':
        wrline_mode = 0;
        break;
      case 'M':
        wrline_mode = 1;
        break;
      case 'P':
        wrline_mode = 2;
        break;
      default:
        cerr << "Invalid --wrline-mode.  Expected I, M or P." << endl;
        exit(1);
    }

    uint64_t mcl = uint64_t(vm["mcl"].as<int>());
    if ((mcl > 4) || (mcl == 3))
    {
        cerr << "Illegal multi-line (mcl) parameter:  " << mcl << endl;
        exit(1);
    }
    // Encode mcl as 3 bits.  The low 2 are the Verilog t_ccip_clLen and the
    // high bit indicates variable sizes.
    mcl = (mcl == 0) ? 4 : (mcl - 1);

    uint64_t rd_vc_map = uint64_t(vm["rd-vc"].as<int>());
    if (rd_vc_map > 5)
    {
        cerr << "Illegal read VC (rd-vc) parameter:  " << rd_vc_map << endl;
        exit(1);
    }
    uint64_t wr_vc_map = uint64_t(vm["wr-vc"].as<int>());
    if (wr_vc_map > 5)
    {
        cerr << "Illegal write VC (wr-vc) parameter:  " << wr_vc_map << endl;
        exit(1);
    }
    uint64_t dsm_vc = uint64_t(vm["dsm-vc"].as<int>());

    uint64_t trips = uint64_t(vm["repeat"].as<int>());
    uint64_t iter = 0;
    uint64_t n_errors = 0;
    uint64_t n_write_errors = 0;
    uint64_t n_read_errors = 0;

    uint64_t vl0_lines = readCommonCSR(CCI_TEST::CSR_COMMON_VL0_RD_LINES) +
                         readCommonCSR(CCI_TEST::CSR_COMMON_VL0_WR_LINES);
    uint64_t vh0_lines = readCommonCSR(CCI_TEST::CSR_COMMON_VH0_LINES);
    uint64_t vh1_lines = readCommonCSR(CCI_TEST::CSR_COMMON_VH1_LINES);

    cout << "Starting tests..." << endl;

    //
    // Trigger read/write errors to check error detection by writing test
    // CSR 7.  The AFU has an error injector that can be turned on here.
    //
    uint64_t trigger_error_cycle_interval = 100000000;
    // One for generating write errors, 0 for generating read errors
    uint64_t trigger_error_writes = 0;
    //writeTestCSR(7, (trigger_error_cycle_interval << 1) | trigger_error_writes);

    while (trips--)
    {
        memset((void*)dsm, 0, 4096);

        // Start the test
        writeTestCSR(0,
                     (cycles << 14) |
                     (dsm_vc << 12) |
                     (mcl << 9) |
                     (wrline_mode << 7) |
                     (rdline_mode << 6) |
                     (wr_vc_map << 3) |
                     rd_vc_map);

        uint64_t iter_state_end = 0;
        volatile uint64_t* dsm_next = dsm;
        uint64_t cycles;

        // Wait for test to signal it is complete
        while (true)
        {
            _mm_pause();
            if (uint8_t(*dsm_next) == 0) continue;

            uint64_t msg = dsm_next[0];
            cycles = dsm_next[1];
            *dsm_next = 0;

            // Move to the next line
            dsm_next += (CL(1) / 8);
            if (dsm_next == &dsm[4096 / 8]) dsm_next = dsm;

            if (msg & 2)
            {
                // Read/write error
                uint64_t error_buf_idx = (msg >> 8) & 0xff;
                uint64_t error_buf_offset = (msg >> 16) & 0xffff;
                uint64_t error_tag = (msg >> 32) & 0xffff;
                
                //
                // Is it a read or a write error?  Check the copy in memory
                // to see if it looks right.
                //
                volatile uint64_t* error_line = &buf[error_buf_idx][error_buf_offset * (CL(1) / 8)];

                uint64_t check = (error_line[0] ^ error_tag) |
                                 error_line[1] |
                                 error_line[2] |
                                 error_line[3] |
                                 error_line[4] |
                                 error_line[5] |
                                 error_line[6] |
                                 (error_line[7] ^ (error_buf_offset << 48));

                // Repair the line to avoid duplicate claims of write errors
                error_line[0] = error_tag;
                error_line[1] = 0;
                error_line[2] = 0;
                error_line[3] = 0;
                error_line[4] = 0;
                error_line[5] = 0;
                error_line[6] = 0;
                error_line[7] = (error_buf_offset << 48);

                bool write_error = (check != 0);
                if (write_error)
                {
                    n_write_errors += 1;
                }
                else
                {
                    n_read_errors += 1;
                }

                cout << "  ** "
                     << (write_error ? "WRITE" : "READ") << " ERROR: "
                     << "buffer " << error_buf_idx
                     << ", line offset " << error_buf_offset
                     << ", tag 0x" << hex << error_tag << dec
                     << " **"
                     << endl;
            }
            else
            {
                // Done!
                break;
            }
        }

        // Wait for all traffic to complete
        uint64_t wait_trips = 0;
        uint64_t st;
        while ((st = readTestCSR(7)) & 0xc)
        {
            if (wait_trips == 100000)
            {
                // Give up
                if (st & 0x4)
                {
                    cerr << "FPGA dropped a read response.  Giving up!" << endl;
                }
                if (st & 0x8)
                {
                    cerr << "FPGA dropped a write response.  Giving up!" << endl;
                }
                exit(1);
            }

            wait_trips += 1;
        }

        // Actual number of cycles executed was reported in dsm[1]
        totalCycles += cycles;
        // Run length in seconds
        double run_sec = double(cycles) / (double(afu_mhz) * 1000.0 * 1000.0);

        uint64_t error_cnt = readTestCSR(3);
        uint64_t read_cnt = readTestCSR(4);
        uint64_t write_cnt = readTestCSR(5);

        cout << "[" << ++iter << "] "
             << read_cnt << " reads ("
             << boost::format("%.1f") % ((double(read_cnt) * CL(1) / 0x40000000) / run_sec) << " GB/s), "
             << write_cnt << " writes ("
             << boost::format("%.1f") % ((double(write_cnt) * CL(1) / 0x40000000) / run_sec) << " GB/s), "
             << error_cnt << ((error_cnt != 1) ? " errors" : " error")
             << endl;

        uint64_t vl0_lines_n = readCommonCSR(CCI_TEST::CSR_COMMON_VL0_RD_LINES) +
                               readCommonCSR(CCI_TEST::CSR_COMMON_VL0_WR_LINES);
        uint64_t vh0_lines_n = readCommonCSR(CCI_TEST::CSR_COMMON_VH0_LINES);
        uint64_t vh1_lines_n = readCommonCSR(CCI_TEST::CSR_COMMON_VH1_LINES);

        cout << "    VL0 " << vl0_lines_n - vl0_lines
             << " : VH0 " << vh0_lines_n - vh0_lines
             << " : VH1 " << vh1_lines_n - vh1_lines
             << endl;
        vl0_lines = vl0_lines_n;
        vh0_lines = vh0_lines_n;
        vh1_lines = vh1_lines_n;

        n_errors += error_cnt;
    }

    uint64_t n_uncategorized_errors = n_errors - n_write_errors - n_read_errors;

    cout << "#" << endl
         << "# Write errors: " << n_write_errors << endl
         << "# Read errors:  " << n_read_errors << endl;

    if (n_uncategorized_errors)
    {
        cout << "# Other errors: " << n_uncategorized_errors << endl;
    }

    cout << "# Total errors: " << n_errors << endl
         << "#" << endl;

    return (n_errors ? 1 : 0);
}


uint64_t
TEST_CCI_MPF_NULL::testNumCyclesExecuted()
{
    return totalCycles;
}


void
TEST_CCI_MPF_NULL::dbgRegDump(uint64_t r)
{
    cout << "Test cci_mpf_null state:" << endl
         << "  State:           " << ((r >> 8) & 255) << endl
         << "  FIU C0 Alm Full: " << (r & 1) << endl
         << "  FIU C1 Alm Full: " << ((r >> 1) & 1) << endl
         << "  C0 Not Empty:    " << ((r >> 2) & 1) << endl
         << "  C1 Not Empty:    " << ((r >> 3) & 1) << endl;
}
