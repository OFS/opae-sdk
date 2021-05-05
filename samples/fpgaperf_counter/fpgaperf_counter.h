// Copyright(c) 2021, Intel Corporation
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

#ifndef __FPGA_PERF_COUNTER_H__
#define __FPGA_PERF_COUNTER_H__


#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define PERF_DBG
#ifndef PERF_DBG
#define PERF_DBG(format,...)   \
                printf("%s:%d:%s() **DEBUG** :" format "\n",__FILE__, __LINE__, __func__)
#endif // PERF_DBG

#ifndef PERF_ERR
#define PERF_ERR(format,...)   \
                printf("%s:%d:%s() **ERROR** :" format "\n",__FILE__, __LINE__, __func__)
#endif // PERF_ERR

/*initilaize th eperf_counter structureand get the dfl_fme device*/
fpga_result fpgaperfcounterinit(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function);

 
/* Dynamically enumerate sysfs path and get the device type, cpumask, format and generic events
 * Reset the counter to 0 and enable the counters to get workload instructions */
fpga_result fpgaperfcounterstart();


/* Stops performance counter and get the counters values */
fpga_result fpgaperfcounterstop();

/*Print the perf counter values*/
void fpgaperfcounterprint();
 
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FPGA_PERF_COUNTER_H__ */
