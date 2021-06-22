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

#include <stdio.h>
#include <errno.h>
#ifndef __USE_GNU
#define __USE_GNU 1
#endif
#include <pthread.h>

#include <opae/types.h>
#include <opae/types_enum.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define FPGA_PERF_MAGIC 	0x46504741584c474c
#define DFL_PERF_STR_MAX	256

typedef struct  {
	char event_name[DFL_PERF_STR_MAX];
	uint64_t config;
	int fd;
	uint64_t id;
	uint64_t start_value;
	uint64_t stop_value;
} perf_events_type;

typedef struct {
	char format_name[DFL_PERF_STR_MAX];
	uint64_t shift;
} perf_format_type;

typedef struct {
	pthread_mutex_t lock;
	uint64_t magic;
	char dfl_fme_name[DFL_PERF_STR_MAX];
	uint64_t type;
	uint64_t cpumask;
	uint64_t num_format;
	perf_format_type *format_type;
	uint64_t num_perf_events;
	perf_events_type *perf_events;
} fpga_perf_counter;

/**
 * Initilaize the fpga_perf_counter structure. 
 *
 * Dynamically enumerate sysfs path 
 * and get the device type, cpumask, format and generic events.
 * Reset the counter to 0 and enable the counters to get workload instructions.
 *
 * @param[in] token Fpga_token object for device (FPGA_DEVICE type)
 * @param[inout] fpga_perf  Returns the fpga_perf_counter struct
 *
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to update the fpga_perf_counter struct.
 */
fpga_result fpgaPerfCounterGet(fpga_token token, fpga_perf_counter *fpga_perf);

/* 
 * Strat record the performance counter
 *
 * Enable the performance counter, read the start value and update
 * the fpga_perf_counter start value.
 * 
 * @param[inout] fpga_perf  Returns the fpga_perf_counter struct with
 * 				start counter values
 *
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to update the start value.
 */
fpga_result fpgaPerfCounterStartRecord(fpga_perf_counter *fpga_perf);

/*
 * Record the performance counter after Stop the workload
 *
 * Disable the performance counter, read the stop value and update
 * the fpga_perf_counter stop value.
 *
 * @param[inout] fpga_perf  Returns the fpga_perf_counter struct with
 * 				stop counter values
 *
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to update the stop value.
 */
fpga_result fpgaPerfCounterStopRecord(fpga_perf_counter *fpga_perf);

/*
 * Print the perf counter values
 *
 * Calculate the delta of fpga performance counter values and prints.
 * 
 * @param[in] file FILE * paramter
 *
 * @param[in] fpga_perf Print the values of counter from structure
 *
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to access the fpga_perf_counter struct variables.
 *
 */
fpga_result fpgaPerfCounterPrint(FILE *file, fpga_perf_counter *fpga_perf);

/*
 * Release the memory alloacted.
 *
 * Free the resource of format_type and perf_events allocated. Assign the
 * substructures to NULL.
 *
 * @param[in] fpga_perf Release the memory alloacted to substructures.
 *
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to destroy the memory allocated.
 *
 */
fpga_result fpgaPerfCounterDestroy(fpga_perf_counter *fpga_perf);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FPGA_PERF_COUNTER_H__ */
