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
        ("rdline-s", po::value<bool>()->default_value(true), "Emit read requests with shared cache hint")
        ("wrline-m", po::value<bool>()->default_value(true), "Emit write requests with modified cache hint")
        ("mcl", po::value<int>()->default_value(1), "Multi-line requests (0 for random sizes)")
        ("min-stride", po::value<int>()->default_value(0), "Minimum stride value")
        ("max-stride", po::value<int>()->default_value(128), "Maximum stride value")
        ("tc", po::value<int>()->default_value(10000000), "Test length (cycles)")
        ("ts", po::value<int>()->default_value(0), "Test length (seconds)")
        ("enable-warmup", po::value<bool>()->default_value(true), "Warm up VTP's TLB")
        ("test-mode", po::value<bool>()->default_value(false), "Generate simple memory patterns for testing address logic")
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
    assert(initMem(vm["enable-warmup"].as<bool>(),
                   vm["wrline-m"].as<bool>()));

    t_test_config config;
    memset(&config, 0, sizeof(config));

    // What's the AFU frequency (MHz)?
    uint64_t afu_mhz = getAFUMHz();

    config.cycles = uint64_t(vm["ts"].as<int>()) * afu_mhz * 1000 * 1000;
    if (config.cycles == 0)
    {
        // Didn't specify --ts.  Use cycles instead.
        config.cycles = uint64_t(vm["tc"].as<int>());
    }

    const uint64_t counter_bits = 40;
    if (config.cycles & (int64_t(-1) << counter_bits))
    {
        cerr << "Run length overflows " << counter_bits << " bit counter" << endl;
        exit(1);
    }

    config.vc = uint8_t(vm["vc"].as<int>());
    assert(config.vc < 4);

    config.rdline_s = vm["rdline-s"].as<bool>();
    config.wrline_m = vm["wrline-m"].as<bool>();

    config.mcl = uint64_t(vm["mcl"].as<int>());
    if ((config.mcl > 4) || (config.mcl == 3))
    {
        cerr << "Illegal multi-line (mcl) parameter:  " << config.mcl << endl;
        exit(1);
    }
    // Encode mcl as 3 bits.  The low 2 are the Verilog t_ccip_clLen and the
    // high bit indicates random sizes.
    config.mcl = (config.mcl - 1) & 7;

    if (vm["test-mode"].as<bool>())
    {
        cout << "# Mem Bytes, Stride, " << statsHeader() << endl;
        t_test_stats stats;

        config.buf_lines = 256;
        config.stride = 12;
        config.enable_writes = true;
        config.enable_reads = true;
        assert(runTest(&config, &stats) == 0);

        cout << 0x100 * CL(1) << " "
             << config.stride << " "
             << stats
             << endl;

        return 0;
    }

    uint64_t stride_incr = 1 + config.mcl;

    uint64_t min_stride = uint64_t(vm["min-stride"].as<int>());
    min_stride = (min_stride + stride_incr - 1) & ~ (stride_incr - 1);
    uint64_t max_stride = uint64_t(vm["max-stride"].as<int>()) + 1;

    bool vcmap_all = vm["vcmap-all"].as<bool>();
    bool vcmap_enable = vm["vcmap-enable"].as<bool>();
    bool vcmap_dynamic = vm["vcmap-dynamic"].as<bool>();
    int32_t vcmap_fixed_vl0_ratio = int32_t(vm["vcmap-fixed"].as<int>());
    cout << "# MCL = " << (config.mcl + 1) << endl
         << "# Cycles per test = " << config.cycles << endl
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

    // Read
    cout << "#" << endl
         << "# Reads " << (config.rdline_s ? "" : "not ") << "cached" << endl
         << "# Mem Bytes, Stride, " << statsHeader()
         << endl;

    for (uint64_t mem_lines = stride_incr; mem_lines * CL(1) <= buffer_bytes; mem_lines <<= 1)
    {
        config.buf_lines = mem_lines;

        // Vary stride
        uint64_t stride_limit = (mem_lines < max_stride ? mem_lines+1 : max_stride);
        for (uint64_t stride = min_stride; stride < stride_limit; stride += stride_incr)
        {
            t_test_stats stats;

            config.stride = stride;
            config.enable_writes = false;
            config.enable_reads = true;
            assert(runTest(&config, &stats) == 0);

            cout << mem_lines * CL(1) << " "
                 << stride << " "
                 << stats
                 << endl;
        }
    }

    // Write
    cout << endl
         << endl
         << "# Writes " << (config.wrline_m ? "" : "not ") << "cached" << endl
         << "# Mem Bytes, Stride, " << statsHeader()
         << endl;

    for (uint64_t mem_lines = stride_incr; mem_lines * CL(1) <= buffer_bytes; mem_lines <<= 1)
    {
        config.buf_lines = mem_lines;

        // Vary stride
        uint64_t stride_limit = (mem_lines < max_stride ? mem_lines+1 : max_stride);
        for (uint64_t stride = min_stride; stride < stride_limit; stride += stride_incr)
        {
            t_test_stats stats;

            config.stride = stride;
            config.enable_writes = true;
            config.enable_reads = false;
            assert(runTest(&config, &stats) == 0);

            cout << mem_lines * CL(1) << " "
                 << stride << " "
                 << stats
                 << endl;
        }
    }

    // Throughput (independent read and write)
    cout << endl
         << endl
         << "# Reads " << (config.rdline_s ? "" : "not ") << "cached +"
         << " Writes " << (config.wrline_m ? "" : "not ") << "cached" << endl
         << "# Mem Bytes, Stride, " << statsHeader()
         << endl;

    for (uint64_t mem_lines = stride_incr; mem_lines * CL(1) <= buffer_bytes; mem_lines <<= 1)
    {
        config.buf_lines = mem_lines;

        // Vary stride
        uint64_t stride_limit = (mem_lines < max_stride ? mem_lines+1 : max_stride);
        for (uint64_t stride = min_stride; stride < stride_limit; stride += stride_incr)
        {
            t_test_stats stats;

            config.stride = stride;
            config.enable_writes = true;
            config.enable_reads = true;
            assert(runTest(&config, &stats) == 0);

            cout << mem_lines * CL(1) << " "
                 << stride << " "
                 << stats
                 << endl;
        }
    }

    return 0;
}
