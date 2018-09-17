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
    // No test-specific options
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

    // Ignore the command line and set VC Map to fixed
    config.vc = 0;
#ifndef USE_LEGACY_AAL
    assert(FPGA_OK == mpfVcMapSetMapAll(svc.mpf_handle, false));
    assert(FPGA_OK == mpfVcMapSetMode(svc.mpf_handle, true, false, 0));
#else
    svc.pVCMAPService->vcmapSetMapAll(false);
    svc.pVCMAPService->vcmapSetMode(true, false);
#endif
    
    config.clear_caches = false;
    config.buf_lines = 32768;
    config.rdline_s = false;
    config.wrline_m = false;

    cout << "MCL, VL0 ratio (n/64), Read GB/s, Write GB/s, Read+Write GB/s" << endl;

    // Mode: read (0), write (1), read/write (2)
    for (int mode = 0; mode <= 2; mode += 1)
    {
        if (mode) cout << endl << endl;

        config.enable_writes = (mode != 0);
        config.enable_reads = (mode != 1);

        uint32_t mcl = 1;
        while (mcl <= 4)
        {
            if (mcl != 1) cout << endl << endl;

            // Encode mcl as 3 bits.  The low 2 are the Verilog t_ccip_clLen
            // and the high bit indicates random sizes.
            config.mcl = (mcl - 1) & 7;
            config.stride = 1 + config.mcl;

            for (uint64_t map_ratio_vl0 = 0; map_ratio_vl0 <= 64; map_ratio_vl0 += 1)
            {
                t_test_stats stats;

#ifndef USE_LEGACY_AAL
                mpfVcMapSetFixedMapping(svc.mpf_handle, true, map_ratio_vl0);
#else
                svc.pVCMAPService->vcmapSetFixedMapping(true, map_ratio_vl0);
#endif

                // Run twice.  The first time is just warmup.
                config.cycles = 128 * 65536;
                assert(runTest(&config, &stats) == 0);
                config.cycles = 512 * 65536;
                assert(runTest(&config, &stats) == 0);

                double rd_bytes = double(stats.read_lines) * CL(1);
                double wr_bytes = double(stats.write_lines) * CL(1);

                cout << mcl << " "
                     << map_ratio_vl0 << " "
                     << boost::format("%.1f") % ((rd_bytes / 0x40000000) / stats.run_sec) << " "
                     << boost::format("%.1f") % ((wr_bytes / 0x40000000) / stats.run_sec) << " "
                     << boost::format("%.1f") % (((rd_bytes + wr_bytes) / 0x40000000) / stats.run_sec) << " "
                     << endl;
            }

            mcl *= 2;
        }
    }

    return 0;
}
