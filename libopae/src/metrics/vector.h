// Copyright(c) 2017, Intel Corporation
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

/**
* \file vector.h
* \brief fpga metrics vector
*/

#ifndef __FPGA_METRICS_VECTOR_H__
#define __FPGA_METRICS_VECTOR_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <opae/fpga.h>
#include <opae/types.h>


typedef struct fpga_vector {
	void **fpga_items;
	uint64_t capacity;
	uint64_t total;
} fpga_vector;



fpga_result fpga_vector_init(fpga_vector *vector);

fpga_result fpga_vector_free(fpga_vector *vector);

uint64_t fpga_vector_total(fpga_vector *vector);

fpga_result fpga_vector_resize(fpga_vector *vector, uint64_t capacity);

fpga_result fpga_vector_push(fpga_vector *vector, void *fpga_items);

void *fpga_vector_pop(fpga_vector *vector, uint64_t index);

void *fpga_vector_get(fpga_vector *vector, uint64_t value);

fpga_result fpga_vector_delete(fpga_vector *v, uint64_t index);



fpga_result fpga_metrics_vector_init(fpga_metrics *vector);

fpga_result fpga_metrics_vector_free(fpga_metrics *vector);

uint64_t fpga_metrics_vector_total(fpga_metrics *vector);

fpga_result fpga_metrics_vector_resize(fpga_metrics *vector, uint64_t capacity);

fpga_result fpga_metrics_vector_push(fpga_metrics *vector, void *fpga_metrics_val);

void *fpga_metrics_vector_pop(fpga_metrics *vector, uint64_t index);

void *fpga_metrics_vector_get(fpga_metrics *vector, uint64_t value);

fpga_result fpga_metrics_vector_delete(fpga_metrics *vector, uint64_t index);


#endif // __FPGA_METRICS_VECTOR_H__