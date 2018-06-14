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

#ifndef __TEST_CCI_MPF_NULL_H__
#define __TEST_CCI_MPF_NULL_H__ 1

#include "cci_test.h"

class TEST_CCI_MPF_NULL : public CCI_TEST
{
  private:
    enum
    {
        TEST_CSR_BASE = 32
    };

  public:
    TEST_CCI_MPF_NULL(const po::variables_map& vm, SVC_WRAPPER& svc) :
        CCI_TEST(vm, svc),
        totalCycles(0),
        doBufferTests(false)
    {
        memset(testBuffers, 0, sizeof(testBuffers));
    }

    ~TEST_CCI_MPF_NULL() {};

    // Returns 0 on success
    int test();

    uint64_t testNumCyclesExecuted();

  private:
    void reallocTestBuffers();
    // Return true about 20% of the time
    bool rand20();

    void dbgRegDump(uint64_t r);

    uint64_t totalCycles;

    // Used to test VTP malloc/free when --buffer-alloc-test=1
    void* testBuffers[10];
    bool doBufferTests;
};

#endif // _TEST_CCI_MPF_NULL_H_
