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

#ifndef __PACD_RESET_BMC_H__
#define __PACD_RESET_BMC_H__

#include <semaphore.h>
#include "bitstream.h"
#include "config_int.h"
#include "bmc/bmc.h"

typedef struct {
	uint8_t *last_state; // bit vector
	uint8_t *tripped;    // bit vector
} sens_state_t;

typedef struct {
	struct bmc_thermal_context *c;
	fpga_token fme_token;
	opae_bitstream_info null_gbs_info;
	uint32_t gbs_found;
	uint32_t gbs_index;
	bmc_sdr_handle records;
	bmc_values_handle values;
	char **sensor_names;
	uint32_t num_sensors;
	sens_state_t s_state;
} pacd_bmc_reset_context;

fpga_result pacd_bmc_shutdown(pacd_bmc_reset_context *ctx);
fpga_result pacd_bmc_reinit(pacd_bmc_reset_context *ctx);


#endif // __PACD_RESET_BMC_H__
