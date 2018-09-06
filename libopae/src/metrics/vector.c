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

#include "safe_string/safe_string.h"

#define FPGA_VECTOR_CAPACITY 20

fpga_result fpga_vector_init(fpga_vector *vector)
{
	fpga_result result = FPGA_OK;
	vector->capacity = FPGA_VECTOR_CAPACITY;
	vector->total = 0;
	vector->fpga_items = malloc(sizeof(void *) * vector->capacity);

	if (vector->fpga_items == NULL)
		return FPGA_NO_MEMORY;

	return result;
}

fpga_result fpga_vector_free(fpga_vector *vector)
{
	fpga_result result = FPGA_OK;
	if (vector == NULL) {
		FPGA_ERR("Invalid parm");
		return FPGA_INVALID_PARAM;
	}
	free(vector->fpga_items);

	return result;
}
uint64_t fpga_vector_total(fpga_vector *vector)
{
	if (vector == NULL) {
		FPGA_ERR("Invalid parm");
		return 0;
	}
	return vector->total;
}

fpga_result fpga_vector_resize(fpga_vector *vector, uint64_t capacity)
{
	fpga_result result = FPGA_OK;

	if (vector == NULL) {
		FPGA_ERR("Invalid parm");
		return FPGA_INVALID_PARAM;
	}

	void **fpga_items = realloc(vector->fpga_items, sizeof(void *) * capacity);

	if (fpga_items == NULL) {
		FPGA_ERR("Invalid parm");
		return FPGA_NO_MEMORY;
	}

	if (fpga_items) {
		vector->fpga_items = fpga_items;
		vector->capacity = capacity;
	}
	return result;

}

fpga_result fpga_vector_push(fpga_vector *vector, void *fpga_items)
{
	fpga_result result = FPGA_OK;

	if (vector->capacity == vector->total) {

		result = fpga_vector_resize(vector, vector->capacity * 2);
		if (result != FPGA_OK)
			return result;
	}
	vector->fpga_items[vector->total++] = fpga_items;


	return result;

}

void *fpga_vector_pop(fpga_vector *vector, uint64_t index)
{
	if ((index < vector->total) && (index > 0))
		return vector->fpga_items[index];

	return NULL;
}

fpga_result fpga_vector_delete(fpga_vector *vector, uint64_t index)
{
	fpga_result result = FPGA_OK;
	uint64_t i = 0;
	if (index >= vector->total)
		return FPGA_INVALID_PARAM;

	vector->fpga_items[index] = NULL;

	for (i = index; i < vector->total - 1; i++) {
		vector->fpga_items[i] = vector->fpga_items[i + 1];
		vector->fpga_items[i + 1] = NULL;
	}

	vector->total--;

	if (vector->total > 0 && vector->total == vector->capacity / 4)
		fpga_vector_resize(vector, vector->capacity / 2);

	return result;
}

void *fpga_vector_get(fpga_vector *vector, uint64_t index)
{
	if (index < vector->total)
		return vector->fpga_items[index];
	return NULL;
}

///////////////////



fpga_result fpga_metrics_vector_init(fpga_metrics *vector)
{
	fpga_result result = FPGA_OK;
	vector->capacity = FPGA_VECTOR_CAPACITY;
	vector->total = 0;
	vector->fpga_metrics_val = malloc(sizeof(void *) * vector->capacity);

	if (vector->fpga_metrics_val == NULL)
		return FPGA_NO_MEMORY;

	return result;

}

fpga_result fpga_metrics_vector_free(fpga_metrics *vector)
{
	fpga_result result = FPGA_OK;
	if (vector == NULL) {
		FPGA_ERR("Invalid parm");
		return FPGA_INVALID_PARAM;
	}
	free(vector->fpga_metrics_val);

	return result;
}
uint64_t fpga_metrics_vector_total(fpga_metrics *vector)
{
	if (vector == NULL) {
		FPGA_ERR("Invalid parm");
		return 0;
	}
	return vector->total;
}
fpga_result fpga_metrics_vector_resize(fpga_metrics  *vector, uint64_t capacity)
{
	fpga_result result = FPGA_OK;

	if (vector == NULL) {
		FPGA_ERR("Invalid parm");
		return FPGA_INVALID_PARAM;
	}

	fpga_metrics_values **fpga_metrics_val = realloc(vector->fpga_metrics_val, sizeof(void *) * capacity);

	if (fpga_metrics_val == NULL) {
		FPGA_ERR("Invalid parm");
		return FPGA_NO_MEMORY;
	}

	if (fpga_metrics_val) {
		vector->fpga_metrics_val = fpga_metrics_val;
		vector->capacity = capacity;
	}
	return result;

}

fpga_result fpga_metrics_vector_push(fpga_metrics *vector, void *fpga_metrics_val)
{
	fpga_result result = FPGA_OK;

	if (vector->capacity == vector->total) {

		result = fpga_metrics_vector_resize(vector, vector->capacity * 2);
		if (result != FPGA_OK)
			return result;
	}
	vector->fpga_metrics_val[vector->total++] = fpga_metrics_val;


	return result;

}

void *fpga_metrics_vector_pop(fpga_metrics *vector, uint64_t index)
{
	if ((index < vector->total) && (index > 0))
		return vector->fpga_metrics_val[index];


	return NULL;
}

fpga_result fpga_metrics_vector_delete(fpga_metrics *vector, uint64_t index)
{
	fpga_result result = FPGA_OK;
	uint64_t i = 0;
	if (index >= vector->total)
		return FPGA_INVALID_PARAM;

	vector->fpga_metrics_val[index] = NULL;

	for (i = index; i < vector->total - 1; i++) {
		vector->fpga_metrics_val[i] = vector->fpga_metrics_val[i + 1];
		vector->fpga_metrics_val[i + 1] = NULL;
	}

	vector->total--;

	if (vector->total > 0 && vector->total == vector->capacity / 4)
		fpga_metrics_vector_resize(vector, vector->capacity / 2);

	return result;
}

void *fpga_metrics_vector_get(fpga_metrics *vector, uint64_t index)
{
	if (index < vector->total)
		return vector->fpga_metrics_val[index];
	return NULL;
}