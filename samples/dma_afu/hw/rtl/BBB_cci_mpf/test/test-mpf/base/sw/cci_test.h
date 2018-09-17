//
// Copyright (c) 2016, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef __CCI_TEST_H__
#define __CCI_TEST_H__ 1

using namespace std;

#include <iostream>
#include <string>
#include <boost/program_options.hpp>
namespace po = boost::program_options;

#ifndef USE_LEGACY_AAL
#include "opae_svc_wrapper.h"
#else
#include "aal_svc_wrapper.h"
#endif

//
// Interface to a standard test.
//

class CCI_TEST
{
  protected:
    enum
    {
        TEST_CSR_BASE = 32
    };

  public:
    CCI_TEST(const po::variables_map& vm, SVC_WRAPPER& svc) :
        vm(vm),
        svc(svc)
    {};

    ~CCI_TEST() {};

    // Returns 0 on success
    virtual int test() = 0;

    // Number of cycles executed in test.  Optional virtual method.  The
    // base class returns 0.
    virtual uint64_t testNumCyclesExecuted()
    {
        return 0;
    }

    //
    // Wrappers for commonly used requests
    //

    bool hwIsSimulated(void) const { return svc.hwIsSimulated(); }

    void* malloc(size_t nBytes, uint64_t* ioAddress = NULL)
    {
        return svc.allocBuffer(nBytes, ioAddress);
    }

    void free(void* va)
    {
        svc.freeBuffer(va);
    }

    void writeTestCSR(uint32_t idx, uint64_t v)
    {
        svc.mmioWrite64(8 * (TEST_CSR_BASE + idx), v);
    }

    uint64_t readTestCSR(uint32_t idx)
    {
        return svc.mmioRead64(8 * (TEST_CSR_BASE + idx));
    }

    //
    // CSRs available on all tests
    //
    typedef enum 
    {
        CSR_COMMON_DFH = 0,
        CSR_COMMON_ID_L = 1,
        CSR_COMMON_ID_H = 2,
        CSR_COMMON_FREQ = 8,
        CSR_COMMON_CACHE_RD_HITS = 9,
        CSR_COMMON_CACHE_WR_HITS = 10,
        CSR_COMMON_VL0_RD_LINES = 11,
        CSR_COMMON_VL0_WR_LINES = 12,
        CSR_COMMON_VH0_LINES = 13,
        CSR_COMMON_VH1_LINES = 14,
        CSR_COMMON_FIU_STATE = 15,
        CSR_COMMON_RD_ALMOST_FULL_CYCLES = 16,
        CSR_COMMON_WR_ALMOST_FULL_CYCLES = 17
    }
    t_csr_common;

    uint64_t readCommonCSR(t_csr_common idx)
    {
        return svc.mmioRead64(8 * uint32_t(idx));
    }

    string vcNumToName(uint32_t vcNum)
    {
        switch (vcNum)
        {
          case 1:
            return "VL0";
          case 2:
            return "VH0";
          case 3:
            return "VH1";
          default:
            return "VA";
        }
    }

    uint64_t getAFUMHz()
    {
        // What's the AFU frequency (MHz)?
        uint64_t afu_mhz = readCommonCSR(CSR_COMMON_FREQ);

        // Some low frequencies are a clue that the run-time configurable
        // frequency is being used.
        if (afu_mhz == 2)
        {
            // What's the frequency of uClk_usr?  For now this is a
            // run-time user-provided parameter.
            afu_mhz = uint64_t(vm["uclk-freq"].as<int>());
        }
        else if (afu_mhz == 1)
        {
            afu_mhz = uint64_t(vm["uclk-freq"].as<int>()) >> 1;
        }

        if (afu_mhz == 0)
        {
            cerr << "--uclk-freq must be specified when connecting to uClk_usr" << endl;
            exit(1);
        }

        return afu_mhz;
    }

  protected:
    const po::variables_map& vm;
    SVC_WRAPPER& svc;
};


//
// A test module must provide the following functions:
//

// AFU ID of the test hardware.
const char* testAFUID();

// Set command line option definitions for test.  This is outside the
// CCI_TEST class because it is needed to configure the base service
// before the test constructor is called.
void testConfigOptions(po::options_description &desc);

// Instantiate an instance of the specific test class
CCI_TEST* allocTest(const po::variables_map& vm, SVC_WRAPPER& svc);

#endif // __CCI_TEST_H__
