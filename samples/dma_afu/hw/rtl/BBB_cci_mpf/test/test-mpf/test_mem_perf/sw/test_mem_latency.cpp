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

#include "test_mem_perf.h"


// ========================================================================
//
// Each test must provide these functions used by main to find the
// specific test instance.
//
// ========================================================================

void testConfigOptions(po::options_description &desc)
{
    // Add test-specific options
    desc.add_options()
        ("vc", po::value<int>()->default_value(0), "Channel (0=VA, 1=VL0, 2=VH0, 3=VH1)")
        ("mcl", po::value<int>()->default_value(1), "Multi-line request size")
        ;
}

CCI_TEST* allocTest(const po::variables_map& vm, SVC_WRAPPER& svc)
{
    return new TEST_MEM_PERF(vm, svc);
}


// ========================================================================
//
//  Memory performance test.
//
// ========================================================================

int TEST_MEM_PERF::test()
{
    assert(initMem());

    t_test_config config;
    memset(&config, 0, sizeof(config));

    // What's the AFU frequency (MHz)?
    uint64_t afu_mhz = getAFUMHz();

    config.vc = uint8_t(vm["vc"].as<int>());
    assert(config.vc < 4);

    config.mcl = uint64_t(vm["mcl"].as<int>());
    if ((config.mcl > 4) || (config.mcl == 3) || (config.mcl == 0))
    {
        cerr << "Illegal multi-line (mcl) parameter:  " << config.mcl << endl;
        exit(1);
    }
    // Encode mcl as 3 bits.  The low 2 are the Verilog t_ccip_clLen and the
    // high bit indicates random sizes.
    config.mcl = (config.mcl - 1) & 7;

    bool vcmap_all = vm["vcmap-all"].as<bool>();
    bool vcmap_enable = vm["vcmap-enable"].as<bool>();
    bool vcmap_dynamic = vm["vcmap-dynamic"].as<bool>();
    int32_t vcmap_fixed_vl0_ratio = int32_t(vm["vcmap-fixed"].as<int>());
    cout << "# MCL = " << (config.mcl + 1) << endl
         << "# AFU MHz = " << afu_mhz << endl
         << "# VC = " << vcNumToName(config.vc) << endl
         << "# VC Map enabled: " << (vcmap_enable ? "true" : "false") << endl;
    if (vcmap_enable)
    {
        cout << "# VC Map all: " << (vcmap_all ? "true" : "false") << endl
             << "# VC Map dynamic: " << (vcmap_dynamic ? "true" : "false") << endl;
        if (! vcmap_dynamic)
        {
            cout << "# VC Map fixed VL0 ratio: " << vcmap_fixed_vl0_ratio << " / 64" << endl;
        }
    }


    for (int mode = 0; mode <= 2; mode += 1)
    {
        if (mode) cout << endl << endl;

        // Use the full cache unless both read and write are active, in which
        // case use half for each.  The read and write buffers are already
        // offset to avoid overlap in direct mapped caches.
        config.clear_caches = false;
        config.buf_lines = (mode <= 1 ? 1024 : 512);
        config.stride = 1 + config.mcl;
        config.enable_writes = (mode != 0);
        config.enable_reads = (mode != 1);
        config.rdline_s = true;
        config.wrline_m = true;

        // Cached
        cout << "#" << endl;
        if (config.enable_reads) cout << "# Reads cached" << endl;
        if (config.enable_writes) cout << "# Writes cached" << endl;
        cout << statsHeader()
             << endl;

        // Vary number of cycles in the run.  For small numbers of cycles this works
        // as a proxy for the number requests emitted.
        for (uint64_t cycles = 1; cycles <= 65536; cycles <<= 1)
        {
            t_test_stats stats;
            config.cycles = cycles;

            // Run twice.  The first time is just warmup.
            assert(runTest(&config, &stats) == 0);
            assert(runTestN(&config, &stats, 4) == 0);

            cout << stats << endl;
        }


        // Cache miss
        config.buf_lines = 32768;
        config.rdline_s = false;
        config.wrline_m = false;
        cout << endl << endl
             << "#" << endl;
        if (config.enable_reads) cout << "# Reads not cached" << endl;
        if (config.enable_writes) cout << "# Writes not cached" << endl;
        cout << statsHeader()
             << endl;

        uint64_t max_cycles = 128 * 65536;
        if (afu_mhz < 400)
        {
            max_cycles = uint64_t(double(max_cycles) * double(afu_mhz) / 400.0);
        }

        // Warmup
        if (mode < 2)
        {
            t_test_stats stats;
            config.cycles = max_cycles;
            assert(runTest(&config, &stats) == 0);
        }

        // Vary number of cycles in the run.  For small numbers of cycles this works
        // as a proxy for the number requests emitted.
        for (uint64_t cycles = 1; cycles <= max_cycles; cycles <<= 1)
        {
            t_test_stats stats;
            config.cycles = cycles;
            config.clear_caches = true;
            assert(runTestN(&config, &stats, 4) == 0);

            cout << stats << endl;
        }


        // Cache miss, varying offered load
        cout << endl << endl
             << "#" << endl
             << "# Offered load varied by limiting concurrent requests" << endl;
        if (config.enable_reads) cout << "# Reads not cached" << endl;
        if (config.enable_writes) cout << "# Writes not cached" << endl;
        cout << "Maximum active lines, "
             << statsHeader()
             << endl;

        // Vary the maximum number of outstanding requests.
        uint64_t credits = 1;
        while (credits <= 1024)
        {
            t_test_stats stats;
            config.clear_caches = true;

            // Convert credits from requests to lines
            uint64_t line_credits = credits / (config.mcl + 1);
            if (line_credits == 0) line_credits = 1;
            config.rd_req_max_credits = line_credits;
            config.wr_req_max_credits = line_credits;

            assert(runTestN(&config, &stats, 4) == 0);

            cout << credits << " " << stats << endl;

            if (credits == 1) credits = 0;
            credits += 4;
        }
    }

    return 0;
}
