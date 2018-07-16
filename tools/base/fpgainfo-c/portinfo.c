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
#include "portinfo.h"
#include <opae/fpga.h>
#include <uuid/uuid.h>

#define MODEL_SIZE 64

struct port_info {
	fpga_guid guid;
	uint64_t object_id;
	uint8_t bus;
	uint8_t device;
	uint8_t function;
	uint8_t socket_id;
	uint16_t device_id;
	char model[MODEL_SIZE];
};

fpga_result get_port_info(fpga_token tok, struct port_info *finfo)
{
	fpga_result res = FPGA_OK;
	fpga_properties props;
	res = fpgaGetProperties(tok, &props);
	ON_FPGAINFO_ERR_GOTO(res, out, "reading properties from token");

	fpgaPropertiesGetObjectID(props, &finfo->object_id);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "reading object_id from properties");

	fpgaPropertiesGetGUID(props, &finfo->guid);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "reading guid from properties");

	fpgaPropertiesGetBus(props, &finfo->bus);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "reading bus from properties");

	fpgaPropertiesGetDevice(props, &finfo->device);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "reading device from properties");

	fpgaPropertiesGetFunction(props, &finfo->function);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "reading function from properties");

	fpgaPropertiesGetSocketID(props, &finfo->socket_id);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "reading socket_id from properties");

	fpgaPropertiesGetDeviceID(props, &finfo->device_id);
	ON_FPGAINFO_ERR_GOTO(res, out_destroy, "reading device_id from properties");

	// fpgaPropertiesGetModel(props, &finfo->model);
	// ON_FPGAINFO_ERR_GOTO(res, out_destroy, "reading model from properties");

out_destroy:
	fpgaDestroyProperties(&props);
out:
	return res;
}

void print_port_info(struct port_info *info)
{
	char guid_str[38];
	uuid_unparse(info->guid, guid_str);
	printf("//****** PORT ******//\n");
	printf("%-24s : 0x%2lX\n", "Object ID", info->object_id);
	printf("%-24s : 0x%02X\n", "Bus", info->bus);
	printf("%-24s : 0x%02X\n", "Device", info->device);
	printf("%-24s : 0x%02X\n", "Function", info->function);
	printf("%-24s : 0x%02X\n", "Socket ID", info->socket_id);
	printf("%-24s : %s\n", "Accelerator GUID", guid_str);
}

fpga_result port_filter(fpga_properties *filter, int argc, char *argv[])
{
	(void)argc;
	(void)argv;
	fpga_result res = FPGA_OK;
	res = fpgaPropertiesSetObjectType(*filter, FPGA_ACCELERATOR);
	fpgainfo_print_err("setting type to FPGA_ACCELERATOR", res);
	return res;
}

fpga_result port_command(fpga_token *tokens, int num_tokens, int argc,
			 char *argv[])
{
	(void)tokens;
	(void)num_tokens;
	(void)argc;
	(void)argv;

	fpga_result res = FPGA_OK;
	struct port_info info;

	int i = 0;
	for (i = 0; i < num_tokens; ++i) {
		res = get_port_info(tokens[i], &info);
		ON_FPGAINFO_ERR_GOTO(res, out, 0);
		print_port_info(&info);
	}
out:
	return res;
}
