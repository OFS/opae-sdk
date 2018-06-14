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

#ifndef __TEST_MEM_PERF_H__
#define __TEST_MEM_PERF_H__ 1

#include <math.h>
#include <iostream>
#include <boost/format.hpp>

#include "cci_test.h"

class TEST_MEM_PERF : public CCI_TEST
{
  private:
    enum
    {
        TEST_CSR_BASE = 32
    };

    typedef struct
    {
        uint64_t cycles;
        uint64_t buf_lines;
        uint64_t stride;
        uint8_t vc;
        uint8_t mcl;
        bool rdline_s;
        bool wrline_m;
        bool enable_writes;
        bool enable_reads;
        bool clear_caches;

        // Offered load: Maximum outstanding reads or writes
        uint32_t rd_req_max_credits;
        uint32_t wr_req_max_credits;
    }
    t_test_config;

    typedef struct
    {
        uint64_t actual_cycles;
        double run_sec;

        uint64_t read_lines;
        uint64_t write_lines;
        uint64_t read_cache_line_hits;
        uint64_t write_cache_line_hits;
        uint64_t vl0_rd_lines;
        uint64_t vl0_wr_lines;
        uint64_t vh0_lines;
        uint64_t vh1_lines;
        uint64_t read_almost_full_cycles;
        uint64_t write_almost_full_cycles;

        uint64_t read_max_inflight_lines;
        uint64_t read_average_latency;
        uint64_t write_max_inflight_lines;
        uint64_t write_average_latency;
    }
    t_test_stats;

  public:
    TEST_MEM_PERF(const po::variables_map& vm, SVC_WRAPPER& svc) :
        CCI_TEST(vm, svc),
        dsm(NULL),
        buffer_bytes(0),
        rd_mem(NULL),
        wr_mem(NULL),
        totalCycles(0)
    {};

    ~TEST_MEM_PERF() {};

    // Returns 0 on success
    int test();

    uint64_t testNumCyclesExecuted();

  private:
    int runTest(const t_test_config* config, t_test_stats* stats);
    // Invoke runTest n times and return the average
    int runTestN(const t_test_config* config, t_test_stats* stats, int n);

    bool initMem(bool enableWarmup = false, bool cached = false);

    // Warm up both VTP and the first 2K lines in VL0 for a region
    void warmUp(void* buf, uint64_t n_bytes, bool cached);

    string statsHeader(void)
    {
        return "Read GB/s, Write GB/s, VL0 lines, VH0 lines, VH1 lines, VL0 Rd Hits per 1000, VL0 Wr Hits per 1000, Read Max Inflight Lines, Read Ave Cycle Lat, Write Max Inflight Lines, Write Ave Cycle Lat, Read AlmFull %, Write AlmFull %";
    }

    friend std::ostream& operator<< (std::ostream& os, const t_test_stats& stats)
    {
        uint32_t vl0_rd_hit_rate = 0;
        if (stats.vl0_rd_lines)
        {
            vl0_rd_hit_rate = round((1000.0 * stats.read_cache_line_hits) / stats.vl0_rd_lines);
        }

        uint32_t vl0_wr_hit_rate = 0;
        if (stats.vl0_wr_lines)
        {
            vl0_wr_hit_rate = round((1000.0 * stats.write_cache_line_hits) / stats.vl0_wr_lines);
        }

        os << boost::format("%.1f") % ((double(stats.read_lines) * CL(1) / 0x40000000) / stats.run_sec) << " "
           << boost::format("%.1f") % ((double(stats.write_lines) * CL(1) / 0x40000000) / stats.run_sec) << " "
           << stats.vl0_rd_lines + stats.vl0_wr_lines << " "
           << stats.vh0_lines << " "
           << stats.vh1_lines << " "
           << vl0_rd_hit_rate << " "
           << vl0_wr_hit_rate << " "
           << stats.read_max_inflight_lines << " "
           << stats.read_average_latency << " "
           << stats.write_max_inflight_lines << " "
           << stats.write_average_latency << " "
           << boost::format("%.4f") % (100.0 * double(stats.read_almost_full_cycles) / double(stats.actual_cycles)) << " "
           << boost::format("%.4f") % (100.0 * double(stats.write_almost_full_cycles) / double(stats.actual_cycles)) << " ";
        return os;
    }

    void dbgRegDump(uint64_t r);

    volatile uint64_t* dsm;
    uint64_t buffer_bytes;
    uint64_t* rd_mem;
    uint64_t* wr_mem;
    uint64_t totalCycles;
};

#endif // _TEST_MEM_PERF_H_
