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
#include "fpgainfo.h"
#include "tempinfo.h"
#include <opae/fpga.h>
//#include <uuid/uuid.h>
#include <wchar.h>

#include "bmcinfo.h"
#include "sysinfo.h"
#include "safe_string/safe_string.h"

static void print_temp_info(fpga_properties props, struct dev_list *dev)
{
	fpga_result res = FPGA_OK;
	char path[SYSFS_PATH_MAX];
	uint32_t temperature = -1;

	fpgainfo_print_common("//****** TEMP ******//", props);

	if (NULL == dev) {
		printf("  NO THERMAL DATA AVAILABLE\n");
		return;
	}

	snprintf_s_ss(path, sizeof(path), "%s/%s", dev->fme->sysfspath,
		      "thermal_mgmt/temperature");
	res = fpgainfo_sysfs_read_u32(path, &temperature);
	fpgainfo_print_err("Failure reading temperature value", res);

	printf("%-24s : %02d%ls\n", "Temperature", temperature, L"\x00b0\x0043");

	res = bmc_print_values(dev->fme->sysfspath, BMC_THERMAL);
	fpgainfo_print_err("Cannot read BMC telemetry", res);
}

fpga_result temp_filter(fpga_properties *filter, int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	fpga_result res = FPGA_OK;
	res = fpgaPropertiesSetObjectType(*filter, FPGA_DEVICE);
	fpgainfo_print_err("setting type to FPGA_DEVICE", res);
	return res;
}

fpga_result temp_command(fpga_token *tokens, int num_tokens, int argc,
			 char *argv[])
{
	(void)tokens;
	(void)num_tokens;
	(void)argc;
	(void)argv;

	fpga_result res = FPGA_OK;
	struct dev_list head;
	struct dev_list *lptr;
	fpga_properties props;

	memset_s(&head, sizeof(head), 0);

	res = fpgainfo_enumerate_devices(&head);
	ON_FPGAINFO_ERR_GOTO(res, out, "Cannot enumerate devices");

	int i = 0;
	for (i = 0; i < num_tokens; ++i) {
		uint16_t segment;
		uint8_t bus;
		uint8_t device;
		uint8_t function;

		res = fpgaGetProperties(tokens[i], &props);
		ON_FPGAINFO_ERR_GOTO(res, out_destroy,
				     "reading properties from token");

		res = fpgaPropertiesGetSegment(props, &segment);
		fpgainfo_print_err("reading segment from properties", res);

		res = fpgaPropertiesGetBus(props, &bus);
		fpgainfo_print_err("reading bus from properties", res);

		res = fpgaPropertiesGetDevice(props, &device);
		fpgainfo_print_err("reading device from properties", res);

		res = fpgaPropertiesGetFunction(props, &function);
		fpgainfo_print_err("reading function from properties", res);

		for (lptr = &head; NULL != lptr; lptr = lptr->next) {
			if ((lptr->bus == bus) && (lptr->device == device)
			    && (lptr->function == function)
			    && (lptr->segment == segment)) {
				break;
			}
		}

		print_temp_info(props, lptr);
		fpgaDestroyProperties(&props);
	}

out:
	return res;

out_destroy:
	fpgaDestroyProperties(&props);
	return res;
}
