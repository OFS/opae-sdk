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

//
// This test is used to discover the proper parameters for the latency QoS
// shim.  Parameters are platform dependent.  We usually pick a configuration
// that works on MCL=4 R+W, which permits maximum throughput at all sizes.
//
// Because the latency QoS shim is inserted near the FIU edge, the presence
// of a ROB typically doesn't affect the optimal credit limit.
//

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
        ("c0-qos-enable", po::value<bool>()->default_value(true), "Enable latency QoS for reads")
        ("c1-qos-enable", po::value<bool>()->default_value(true), "Enable latency QoS for writes")
        ("c0-qos-epoch-len", po::value<int>()->default_value(63), "Latency QoS read epoch length")
        ("c1-qos-epoch-len", po::value<int>()->default_value(63), "Latency QoS write epoch length")
        ("max-active-lines", po::value<int>()->default_value(1023), "Maximum active lines considered per channel")
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

    uint64_t c0_qos_enable = uint64_t(vm["c0-qos-enable"].as<bool>());
    uint64_t c1_qos_enable = uint64_t(vm["c1-qos-enable"].as<bool>());
    uint64_t c0_qos_epoch_len = uint64_t(vm["c0-qos-epoch-len"].as<int>());
    uint64_t c1_qos_epoch_len = uint64_t(vm["c1-qos-epoch-len"].as<int>());
    uint64_t max_active_lines = uint64_t(vm["max-active-lines"].as<int>());

    const static string mode_str[] = { "R ", "W ", "RW" };

    cout << "# MCL, VC, RW, " << statsHeader() << endl;

    t_test_stats stats;
    uint64_t c0_max_active_lines;
    uint64_t c1_max_active_lines;

    for (config.mcl = 0; config.mcl < 4; config.mcl = (config.mcl << 1) | 1)
    {
        config.vc = 3;
        while (config.vc != 0)
        {
            config.vc -= 1;

            c0_max_active_lines = 512;
            c1_max_active_lines = 512;

            for (int mode = 0; mode <= 2; mode += 1)
            {
                config.clear_caches = false;
                config.buf_lines = 32768;
                config.stride = 1 + config.mcl;
                config.rdline_s = false;
                config.wrline_m = false;

                config.enable_writes = (mode != 0);
                config.enable_reads = (mode != 1);

                config.cycles = 128 * 65536;

                int64_t peak_bw = 0.0;
                int peak_same_cnt = 0;

                if (mode < 2)
                {
                    // Read and write passes look for the point at which
                    // bandwidth peaks.
                    uint32_t active_incr = 4;

                    for (uint64_t active_lines = 32; active_lines < max_active_lines; active_lines += active_incr)
                    {
                        if (mode == 0)
                        {
                            c0_max_active_lines = active_lines;
                        }
                        else
                        {
                            c1_max_active_lines = active_lines;
                        }

                        uint64_t qos_config = (c0_qos_enable |
                                               (c1_qos_enable << 1) |
                                               (c0_max_active_lines << 2) |
                                               (c1_max_active_lines << 17) |
                                               (c0_qos_epoch_len << 32) |
                                               (c1_qos_epoch_len << 48));

#ifndef USE_LEGACY_AAL
                        if (mpfShimPresent(svc.mpf_handle, CCI_MPF_SHIM_LATENCY_QOS))
                        {
                            mpfLatencyQosSetConfig(svc.mpf_handle, qos_config);
                        }
#else
                        if (svc.pLATQOSService)
                        {
                            svc.pLATQOSService->latqosSetConfig(qos_config);
                        }
#endif

                        assert(runTestN(&config, &stats, 2) == 0);

                        // Did BW increase by at least 2%?
                        int64_t bw = stats.read_lines + stats.write_lines;
                        if ((bw - peak_bw) > (peak_bw / 50))
                        {
                            peak_bw = bw;
                            peak_same_cnt = 0;
                        }
                        else
                        {
                            peak_same_cnt += 1;
                        }

                        if (peak_same_cnt == 32)
                        {
                            if (mode == 0)
                            {
                                c0_max_active_lines = active_lines - 32 * active_incr;
                            }
                            else
                            {
                                c1_max_active_lines = active_lines - 32 * active_incr;
                            }
                            break;
                        }
                    }
                }
                else
                {
                    // R+W mode.  Just report a number
                    assert(runTestN(&config, &stats, 2) == 0);
                }

                cout << uint32_t(config.mcl + 1) << " "
                     << uint32_t(config.vc) << " "
                     << mode_str[mode] << " "
                     << c0_max_active_lines << " "
                     << c1_max_active_lines << " "
                     << stats
                     << endl;
            }
        }
    }

    cout << endl
         << "Recommended configuration:" << endl;

    if (c0_qos_enable)
    {
        cout << "    Read c0 active lines max:  " << c0_max_active_lines << endl;
    }

    if (c1_qos_enable)
    {
        cout << "    Write c1 active lines max: " << c1_max_active_lines << endl;
    }

    return 0;
}
