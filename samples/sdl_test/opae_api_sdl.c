// Copyright(c) 2017-2020, Intel Corporation
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
 * @file opae_api_sdl.c
 * @brief A code sample illustrates the basic usage of the OPAE C Object API
 * to test SDL requirements.
 *
 */
#include <opae/fpga.h>
#include <stdlib.h>
#include <string.h>

void print_err(const char *s, fpga_result res)
{
	fprintf(stderr, "Error %s: %s\n", s, fpgaErrStr(res));
}

#define ON_ERR_GOTO(res, label, desc)                                          \
	do {                                                                   \
		if ((res) != FPGA_OK) {                                        \
			print_err((desc), (res));                              \
			goto label;                                            \
		}                                                              \
	} while (0)


typedef fpga_result (*destroy_f)(void *p);
#define MAX_CLEANUP 100
struct {
	destroy_f fn;
	void *param;
} cleanup[MAX_CLEANUP];

static int cleanup_size;

#define ADD_TO_CLEANUP(func, p)                                         \
do {									\
	if (cleanup_size < MAX_CLEANUP) {                               \
		cleanup[cleanup_size].fn = (destroy_f)func;             \
		cleanup[cleanup_size++].param = p;                      \
	}								\
} while (0)

#define INVALID_MIN_METRICS_PATH "a:b:b"
#define INVALID_MAX_METRICS_PATH "invalid_qualifier_name_with_maxmummmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm_length_for_sdl_testing:invalid_metrics_name_with_maximummmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm_metrics_name_with_maximummmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm_length_for_sdl_testing"
#define SYSFS_INVALID_MIN_LENGTH_PATH "a"
#define SYSFS_INVALID_MAX_LENGTH_PATH "This/is/invalid/path/with/maximim/255/\
		characterssssssssssssssssssssssssssssssssssssssssssssssssssss\
		/lengthhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh\
		/so/opaeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\
		/api/should/return/with/errorrrrrrrrrrrrrrrrr/for/SDL testing/"

int main( void )
{
	fpga_properties filter = NULL;
	fpga_token token = NULL;
	fpga_handle handle = NULL;
	fpga_result res = FPGA_OK;
	uint32_t num_matches = 0;
	uint64_t array_size = 2;
	fpga_object object = NULL;
	const char *metric_strings[2] = { INVALID_MIN_METRICS_PATH,
					  INVALID_MAX_METRICS_PATH };

	struct fpga_metric *metrics = (struct fpga_metric *)
		calloc(sizeof(struct fpga_metric), array_size);
	res = fpgaGetProperties(NULL, &filter);
	ON_ERR_GOTO(res, out_exit, "creating properties object");

	res = fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	ON_ERR_GOTO(res, out_destroy_prop, "setting object type");

	res = fpgaEnumerate(&filter, 1, &token, 1, &num_matches);
	ON_ERR_GOTO(res, out_destroy_prop, "enumerating accelerators");

	if (num_matches < 1) {
		fprintf(stderr, "accelerator not found.\n");
		res = fpgaDestroyProperties(&filter);
		return FPGA_INVALID_PARAM;
	}

	/* Open accelerator */
	res = fpgaOpen(token, &handle, FPGA_OPEN_SHARED);
	ON_ERR_GOTO(res, out_destroy_tok, "opening accelerator");

	//Test cases for CT181: Verify that too-long or too-short parameters are detected.
	res = fpgaGetMetricsByName(handle, (char **)metric_strings, array_size, metrics);

	if (res == FPGA_OK) {
		printf("fpgaGetMetricsByName() success\n");
	} else {
		printf("fpgaGetMetricsByName() failed. Error[%0d]\n", res);
	}

	// Validate the API by passing minimum length path.
	res = fpgaTokenGetObject(token, SYSFS_INVALID_MIN_LENGTH_PATH,
			object, FPGA_OBJECT_GLOB);

	if (res == FPGA_OK) {
		printf("fpgaTokenGetObject() failed for invalid minimum path for SDL-CT181.\n");
	} else {
		printf("fpgaTokenGetObject() passed for invalid minimum path for SDL-CT181.Error[%0d]\n", res);
	}

	// Validate the API by passing maximum length path.
	res = fpgaTokenGetObject(token, SYSFS_INVALID_MAX_LENGTH_PATH,
			object, FPGA_OBJECT_GLOB);

	if (res == FPGA_OK) {
		printf("fpgaTokenGetObject() failed for invalid maximum path for SDL-CT181.\n");
	} else {
		printf("fpgaTokenGetObject() passed for invalid maximum path for SDL-CT181.Error[%0d]\n", res);
	}

	// Validate the API by passing minimum length path.
	res = fpgaHandleGetObject(token, SYSFS_INVALID_MIN_LENGTH_PATH, object, 0);

	if (res == FPGA_OK) {
		printf("fpgaHandleGetObject() failed for invalid minimum path for SDL-CT181.\n");
		ADD_TO_CLEANUP(fpgaDestroyObject, object);
	} else {
		printf("fpgaHandleGetObject() passed for invalid minimum path for SDL-CT181.Error[%0d]\n", res);
	}

	// Validate the API by passing maximum length path.
	res = fpgaHandleGetObject(token, SYSFS_INVALID_MAX_LENGTH_PATH, object, 0);

	if (res == FPGA_OK) {
		printf("fpgaHandleGetObject() failed for invalid maximum path for SDL-CT181.\n");
		ADD_TO_CLEANUP(fpgaDestroyObject, object);
	} else {
		printf("fpgaHandleGetObject() passed for invalid maximum path for SDL-CT181.Error[%0d]\n", res);
	}
	
	printf("Done Test\n");

	/* Destroy token */
out_destroy_tok:
	res = fpgaDestroyToken(&token);
	ON_ERR_GOTO(res, out_destroy_prop, "destroying token");

	/* Destroy properties object */
out_destroy_prop:
	res = fpgaDestroyProperties(&filter);
	ON_ERR_GOTO(res, out_exit, "destroying properties object");

out_exit:
	return res;
}
