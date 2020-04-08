// Copyright(c) 2018-2020, Intel Corporation
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
* \file vector.c
* \brief fpga metrics vector
*/

#include "vector.h"
#include "opae/access.h"
#include "opae/utils.h"
#include "opae/manage.h"
#include "opae/enum.h"
#include "opae/properties.h"
#include "common_int.h"

#define FPGA_VECTOR_CAPACITY     20

fpga_result fpga_vector_init(fpga_metric_vector *vector)
{
	fpga_result result = FPGA_OK;

	if (vector == NULL)
		return FPGA_INVALID_PARAM;

	vector->capacity = FPGA_VECTOR_CAPACITY;
	vector->total = 0;
	vector->fpga_metric_item = malloc(sizeof(void *) * vector->capacity);

	if (vector->fpga_metric_item == NULL)
		return FPGA_NO_MEMORY;

	return result;
}

fpga_result fpga_vector_free(fpga_metric_vector *vector)
{
	fpga_result result = FPGA_OK;
	uint64_t i = 0;
	if (vector == NULL) {
		OPAE_ERR("Invalid parm");
		return FPGA_INVALID_PARAM;
	}
	for (i = 0; i < vector->total; i++) {
		if (vector->fpga_metric_item[i]) {
			free(vector->fpga_metric_item[i]);
			vector->fpga_metric_item[i] = NULL;
		}
	}
	if (vector->fpga_metric_item)
		free(vector->fpga_metric_item);

	vector->fpga_metric_item = NULL;

	return result;
}

fpga_result fpga_vector_total(fpga_metric_vector *vector, uint64_t *total)
{

	if (vector == NULL ||
		total == NULL) {
		OPAE_ERR("Invalid parm");
		return FPGA_EXCEPTION;
	}
	*total =  vector->total;

	return FPGA_OK;
}

fpga_result fpga_vector_resize(fpga_metric_vector *vector, uint64_t capacity)
{
	fpga_result result = FPGA_OK;

	if (vector == NULL) {
		OPAE_ERR("Invalid parm");
		return FPGA_INVALID_PARAM;
	}

	void **fpga_metric_item = realloc(vector->fpga_metric_item, sizeof(void *) * capacity);

	if (fpga_metric_item == NULL) {
		OPAE_ERR("Invalid parm");
		return FPGA_NO_MEMORY;
	}

	if (fpga_metric_item) {
		vector->fpga_metric_item = fpga_metric_item;
		vector->capacity = capacity;
	}
	return result;
}

fpga_result fpga_vector_push(fpga_metric_vector *vector, void *fpga_metric_item)
{
	fpga_result result = FPGA_OK;

	if ((vector == NULL) ||
		(fpga_metric_item == NULL)) {
		OPAE_ERR("Invalid parm");
		return FPGA_INVALID_PARAM;
	}

	if (vector->capacity == vector->total) {

		result = fpga_vector_resize(vector, vector->capacity * 2);
		if (result != FPGA_OK)
			return result;
	}
	vector->fpga_metric_item[vector->total++] = fpga_metric_item;


	return result;
}


fpga_result fpga_vector_delete(fpga_metric_vector *vector, uint64_t index)
{
	fpga_result result = FPGA_OK;
	uint64_t i = 0;

	if (vector == NULL) {
		OPAE_ERR("Invalid parm");
		return FPGA_INVALID_PARAM;
	}
	if (index >= vector->total)
		return FPGA_INVALID_PARAM;

	if (vector->fpga_metric_item[index])
		free(vector->fpga_metric_item[index]);

	vector->fpga_metric_item[index] = NULL;

	for (i = index; i < vector->total - 1; i++) {
		vector->fpga_metric_item[i] = vector->fpga_metric_item[i + 1];
		vector->fpga_metric_item[i + 1] = NULL;
	}

	vector->total--;

	if (vector->total > 0 && vector->total == vector->capacity / 4)
		fpga_vector_resize(vector, vector->capacity / 2);

	return result;
}

void *fpga_vector_get(fpga_metric_vector *vector, uint64_t index)
{
	if (vector == NULL) {
		OPAE_ERR("Invalid parm");
		return NULL;
	}

	if (index < vector->total)
		return vector->fpga_metric_item[index];
	return NULL;
}
