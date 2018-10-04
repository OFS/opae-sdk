// Copyright(c) 2018, Intel Corporation
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

#include <opae/fpga.h>
#include "bmc_thermal.h"
#include "log.h"

#define PRINT_ERR(desc, res)                                                   \
	do {                                                                   \
		if ((desc) && (FPGA_OK != (res))) {                            \
			fprintf(stderr, "Error %s: %s\n", (desc),              \
				fpgaErrStr((res)));                            \
		}                                                              \
	} while (0);

/*
 * macro to check FPGA return codes, print error message, and goto cleanup label
 * NOTE: this changes the program flow (uses goto)!
 */
#define ON_FPGAINFO_ERR_GOTO(res, label, desc)                                 \
	do {                                                                   \
		if ((res) != FPGA_OK) {                                        \
			PRINT_ERR((desc), (res));                              \
			goto label;                                            \
		}                                                              \
	} while (0)

int enumerate(struct bmc_thermal_context *context)
{
	fpga_result res = FPGA_OK;
	uint32_t matches = 0;
	fpga_properties filter = NULL;
	uint32_t result = 0;
	struct config *config = context->config;
	uint32_t i;

	// start a filter using the first level command line arguments
	res = fpgaGetProperties(NULL, &filter);
	ON_FPGAINFO_ERR_GOTO(res, out_err, "creating properties object");

	if ((uint16_t)-1 != context->segment) {
		res += fpgaPropertiesSetSegment(filter, context->segment);
		PRINT_ERR("setting segment", res);
	}

	if ((uint8_t)-1 != context->bus) {
		res += fpgaPropertiesSetBus(filter, context->bus);
		PRINT_ERR("setting bus", res);
	}
	if ((uint8_t)-1 != context->device) {
		res += fpgaPropertiesSetDevice(filter, context->device);
		PRINT_ERR("setting device", res);
	}

	if ((uint8_t)-1 != context->function) {
		res += fpgaPropertiesSetFunction(filter, context->function);
		PRINT_ERR("setting function", res);
	}

	res += fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);
	PRINT_ERR("setting type to FPGA_DEVICE", res);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy,
			     "failed to initialize filter for enumeration");

	uint32_t num_tokens = 0;

	res = fpgaEnumerate(&filter, 1, NULL, 0, &matches);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "enumerating resources");

	num_tokens = matches;
	res = fpgaEnumerate(&filter, 1, &config->tokens[0], num_tokens,
			    &matches);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "enumerating resources");

	if (num_tokens != matches) {
		fprintf(stderr,
			"token list changed in between enumeration calls\n");
		for (i = 0; i < matches; i++) {
			fpgaDestroyToken(&config->tokens[i]);
		}
		goto out_destroy;
	}

	result = num_tokens;
	config->num_PACs = num_tokens;

out_destroy:
	res = fpgaDestroyProperties(&filter);
	ON_FPGAINFO_ERR_GOTO(res, out_err, "destroying properties object");
out_err:
	return result;
}
