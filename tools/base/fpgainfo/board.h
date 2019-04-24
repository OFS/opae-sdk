// Copyright(c) 2019, Intel Corporation
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

/*
 * @file board.h
 *
 * @brief
 */
#ifndef _FPGA_BOARD_H
#define _FPGA_BOARD_H

#include <opae/fpga.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _platform_data {
	uint16_t vendor_id;
	uint16_t device_id;
	char *board_plugin;
	void *dl_handle;
} platform_data;

fpga_result load_board_plugin(fpga_token token, void** dl_handle);
int unload_board_plugin(void);

// Board info
fpga_result fpgainfo_board_info(fpga_token token);

// mac info
fpga_result mac_filter(fpga_properties *filter, int argc, char *argv[]);
fpga_result mac_command(fpga_token *tokens, int num_tokens, int argc,
	char *argv[]);
void mac_help(void);
fpga_result mac_info(fpga_token token);

// phy group info
fpga_result phy_filter(fpga_properties *filter, int argc, char *argv[]);
fpga_result phy_command(fpga_token *tokens, int num_tokens, int argc,
	char *argv[]);
void phy_help(void);
fpga_result phy_group_info(fpga_token token);


#ifdef __cplusplus
}
#endif

#endif /* !_FPGA_BOARD_H */