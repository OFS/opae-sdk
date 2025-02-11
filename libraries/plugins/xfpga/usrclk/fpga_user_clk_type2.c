// Copyright(c) 2025, Intel Corporation
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

// Type 2 (from the DFH revision being set to 2) is a general masked
// read-modify-write pipeline. The FPGA family is discovered from
// the IOPLL_FREQ_STS1 register.
//
// The public entry point is set_userclock_type2(), which should
// be called only by set_userclock().

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <glob.h>
#include <opae/uio.h>

#include "fpga_user_clk.h"
#include "fpga_user_clk_int.h"
#include "mock/opae_std.h"

#define USRCLK_FAMILY_AGILEX5       1

#define PLL_AGX5_EN_REGS_ADDR       0x10
#define PLL_AGX5_CAL_STATUS_ADDR    0x58
#define PLL_AGX5_CAL_OVRD_ADDR      0x48
#define PLL_AGX5_CAL_REQ_ADDR       0x88
#define PLL_AGX5_MN_ADDR            0x40
#define PLL_AGX5_C0_ADDR            0x5c
#define PLL_AGX5_C1_ADDR            0x60
#define PLL_AGX5_CP_ADDR            0x44
#define PLL_AGX5_RESET_ADDR         0x80

#define PLL_AGX5_M                    GENMASK_ULL(28, 20)
#define PLL_AGX5_M_BYPASS_EN          BIT_ULL(31)
#define PLL_AGX5_N_HIGH               GENMASK_ULL(7, 0)
#define PLL_AGX5_N_LOW                GENMASK_ULL(16, 9)
#define PLL_AGX5_N_BYPASS_EN          BIT_ULL(8)
#define PLL_AGX5_N_EVEN_DUTY_EN       BIT_ULL(17)
#define PLL_AGX5_C_HIGH               GENMASK_ULL(7, 0)
#define PLL_AGX5_C_LOW                GENMASK_ULL(30, 23)
#define PLL_AGX5_C_BYPASS_EN          BIT_ULL(8)
#define PLL_AGX5_C_EVEN_DUTY_EN       BIT_ULL(31)
#define PLL_AGX5_CP                   GENMASK_ULL(15, 1)

// Sources of data to be written to the user clock registers.
typedef enum {
	PLL_PARAM_CONST,
	PLL_PARAM_MN,
	PLL_PARAM_C0,
	PLL_PARAM_C1,
	PLL_PARAM_CP
} e_pll_param;

// Descriptor for a single stage of the user clock configuration sequence.
typedef struct {
	uint32_t addr;
	uint32_t mask;
	e_pll_param param_type;
	uint32_t const_param;
	const char *msg;
} t_user_clk_type2_seq;

// Sequence of operations to configure an Agilex 5 user clock.
const t_user_clk_type2_seq agilex5_pll_seq[] = {
	{PLL_AGX5_EN_REGS_ADDR, 0x1, PLL_PARAM_CONST, 1, "Setting reconfiguration enable..."},
	{PLL_AGX5_CAL_STATUS_ADDR, 0x200080, PLL_PARAM_CONST, 0x0, "Clearing calibration status..."},
	{PLL_AGX5_MN_ADDR, 0xffffffff, PLL_PARAM_MN, 0x0, "Setting PLL M+N..."},
	{PLL_AGX5_C0_ADDR, 0xffb801ff, PLL_PARAM_C0, 0x0, "Setting PLL C0..."},
	{PLL_AGX5_C1_ADDR, 0xffb801ff, PLL_PARAM_C1, 0x0, "Setting PLL C1..."},
	{PLL_AGX5_CP_ADDR, 0xfffe, PLL_PARAM_CP, 0x0, "Setting PLL charge pump..."},
	{PLL_AGX5_RESET_ADDR, 0x4, PLL_PARAM_CONST, 0x4, "Asserting PLL reset..."},
	{PLL_AGX5_RESET_ADDR, 0x4, PLL_PARAM_CONST, 0x0, "Deasserting PLL reset..."},
	{PLL_AGX5_CAL_OVRD_ADDR, 0x4000, PLL_PARAM_CONST, 0x4000, "Permitting calibration override..."},
	{PLL_AGX5_CAL_REQ_ADDR, 0x800, PLL_PARAM_CONST, 0x0, "Clearing calibration request..."},
	{PLL_AGX5_CAL_REQ_ADDR, 0x800, PLL_PARAM_CONST, 0x800, "Requesting calibration..."},
	{0, 0, PLL_PARAM_CONST, 0, NULL}
};

// Table of charge pump settings for Agilex 5, indexed by M ranges.
typedef struct {
	uint32_t m_low;
	uint32_t m_high;
	uint32_t cp;
} t_user_clk_agilex5_cp;

#define N_AGILEX5_CP_ENTRIES        13
const t_user_clk_agilex5_cp agilex5_cp_table[N_AGILEX5_CP_ENTRIES] = {
	{2,   2,   0b000111010111111},
	{3,   5,   0b000111000111010},
	{6,   7,   0b000111000111010},
	{8,   10,  0b000101000011000},
	{11,  15,  0b000100110010010},
	{16,  20,  0b000010100101110},
	{21,  23,  0b000010100101110},
	{24,  43,  0b000000100001100},
	{44,  64,  0b000000011001001},
	{65,  85,  0b000000011000110},
	{86,  124, 0b000000010100101},
	{125, 160, 0b000000001100011},
	{161, 320, 0b000000001000010}
};

STATIC uint32_t user_clk_agilex5_cp_lookup(uint32_t m) 
{
	if (m < agilex5_cp_table[0].m_low) {
		return agilex5_cp_table[0].cp;
	}

	// Simple linear search. The table is small and the search is infrequent.
	for (int i = 0; i < N_AGILEX5_CP_ENTRIES; i++) {
		if (m >= agilex5_cp_table[i].m_low && m <= agilex5_cp_table[i].m_high) {
			return agilex5_cp_table[i].cp;
		}
	}

	return agilex5_cp_table[N_AGILEX5_CP_ENTRIES-1].cp;
}

// Type 2 user clock does a masked read-modify-write for every register update.
// Set the mask for the next write by writing it to CMD0.
STATIC fpga_result user_clk_type2_set_mask(uint8_t *uio_ptr, uint32_t mask)
{
	uint64_t v = 0;

	// Mask is stored in the data field
	v = FIELD_PREP(IOPLL_DATA, mask);
	v |= IOPLL_MGMT_DATA_MASK;
	*((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_CMD0)) = v;

	return FPGA_OK;
}

STATIC fpga_result usrclk_set_agilex5(uint8_t *uio_ptr,
	uint8_t *seq,
	uint64_t userclk_low)
{
	fpga_result res  = FPGA_OK;
	uint64_t v       = 0;
	uint32_t timeout = IOPLL_WRITE_POLL_TIMEOUT_US;
	uint32_t cp_from_table = 0;

	unsigned int iopll_max_freq        = IOPLL_AGILEX_MAX_FREQ;
	unsigned int iopll_min_freq        = IOPLL_AGILEX_MIN_FREQ;

	// Agilex 5 uses the same table for M, N, C0 and C1
	// as Agilex 7.
	const struct iopll_config *iopll_config =
		&iopll_agilex_freq_config[userclk_low];
	if ((iopll_config->pll_freq_khz > iopll_max_freq * 1000) ||
		(iopll_config->pll_freq_khz < iopll_min_freq * 1000))
		return FPGA_EXCEPTION;

	int i = 0;
	while (agilex5_pll_seq[i].msg) {
		const t_user_clk_type2_seq *cmd = &agilex5_pll_seq[i];

		// Mask -- bits that will be updated in the next write
		user_clk_type2_set_mask(uio_ptr, cmd->mask);

		// Construct the write, either with a constant value from the table or
		// with the PLL parameters.
		v = IOPLL_WRITE;
		v |= FIELD_PREP(IOPLL_ADDR, cmd->addr);

		*seq = (*seq + 1) & 0x3;
		v |= FIELD_PREP(IOPLL_SEQ, *seq);

		switch (cmd->param_type) {
		case PLL_PARAM_CONST:
			v |= FIELD_PREP(IOPLL_DATA, cmd->const_param);
			break;
		case PLL_PARAM_MN:
			v |= FIELD_PREP(PLL_AGX5_M, FIELD_GET(CFG_PLL_HIGH, iopll_config->pll_m) +
				FIELD_GET(CFG_PLL_LOW, iopll_config->pll_m));
			v |= FIELD_PREP(PLL_AGX5_M_BYPASS_EN, FIELD_GET(CFG_PLL_BYPASS_EN, iopll_config->pll_m));

			v |= FIELD_PREP(PLL_AGX5_N_HIGH, FIELD_GET(CFG_PLL_HIGH, iopll_config->pll_n));
			v |= FIELD_PREP(PLL_AGX5_N_LOW, FIELD_GET(CFG_PLL_LOW, iopll_config->pll_n));
			v |= FIELD_PREP(PLL_AGX5_N_BYPASS_EN, FIELD_GET(CFG_PLL_BYPASS_EN, iopll_config->pll_n));
			v |= FIELD_PREP(PLL_AGX5_N_EVEN_DUTY_EN, FIELD_GET(CFG_PLL_EVEN_DUTY_EN, iopll_config->pll_n));
			break;
		case PLL_PARAM_C0:
			v |= FIELD_PREP(PLL_AGX5_C_HIGH, FIELD_GET(CFG_PLL_HIGH, iopll_config->pll_c0));
			v |= FIELD_PREP(PLL_AGX5_C_LOW, FIELD_GET(CFG_PLL_LOW, iopll_config->pll_c0));
			v |= FIELD_PREP(PLL_AGX5_C_BYPASS_EN, FIELD_GET(CFG_PLL_BYPASS_EN, iopll_config->pll_c0));
			v |= FIELD_PREP(PLL_AGX5_C_EVEN_DUTY_EN, FIELD_GET(CFG_PLL_EVEN_DUTY_EN, iopll_config->pll_c0));
			break;
		case PLL_PARAM_C1:
			v |= FIELD_PREP(PLL_AGX5_C_HIGH, FIELD_GET(CFG_PLL_HIGH, iopll_config->pll_c1));
			v |= FIELD_PREP(PLL_AGX5_C_LOW, FIELD_GET(CFG_PLL_LOW, iopll_config->pll_c1));
			v |= FIELD_PREP(PLL_AGX5_C_BYPASS_EN, FIELD_GET(CFG_PLL_BYPASS_EN, iopll_config->pll_c1));
			v |= FIELD_PREP(PLL_AGX5_C_EVEN_DUTY_EN, FIELD_GET(CFG_PLL_EVEN_DUTY_EN, iopll_config->pll_c1));
			break;
		case PLL_PARAM_CP:
			cp_from_table = user_clk_agilex5_cp_lookup(
				FIELD_GET(CFG_PLL_HIGH, iopll_config->pll_m) +
				FIELD_GET(CFG_PLL_LOW, iopll_config->pll_m));
			v |= FIELD_PREP(PLL_AGX5_CP, cp_from_table);
			break;
		}

		*((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_CMD0)) = v;

		v = *((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_STS0));

		while (!(FIELD_GET(IOPLL_SEQ, v) == *seq)) {
			v = *((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_STS0));
			usleep(IOPLL_WRITE_POLL_INVL_US);
			if (--timeout == 0) {
				OPAE_ERR("Timeout on IOPLL write");
				res = FPGA_EXCEPTION;
				break;
			}
		}

		i += 1;
	}

	return res;
}


// set fpga user clock
fpga_result set_userclock_type2(const char *sysfs_path,
	uint64_t userclk_high,
	uint64_t userclk_low)
{
	uint8_t seq                        = 1;
	uint8_t *uio_ptr                   = NULL;
	fpga_result result                 = FPGA_OK;
	struct opae_uio uio;
	unsigned int fpga_family           = 0;
	uint64_t v                         = 0;

	memset(&uio, 0, sizeof(uio));

	if (sysfs_path == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	if (userclk_high < MIN_FPGA_FREQ) {
		OPAE_ERR("Invalid Input frequency");
		return FPGA_INVALID_PARAM;
	}

	result = get_usrclk_uio(sysfs_path,
		USRCLK_FEATURE_ID,
		&uio,
		&uio_ptr);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get user clock uio");
		return result;
	}

	// Initialize seq from the current sequence number in STS0. The
	// next command must start there.
	v = *((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_STS0));
	seq = FIELD_GET(IOPLL_SEQ, v) + 1;

	// Get target FPGA family
	v = *((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_STS1));
	fpga_family = FIELD_GET(IOPLL_STS1_FPGA_FAMILY, v);

	if (fpga_family == USRCLK_FAMILY_AGILEX5) {
		result = usrclk_set_agilex5(uio_ptr, &seq, userclk_low);
	} else {
		OPAE_ERR("Unsupported type 2 FPGA family: %d", fpga_family);
		result = FPGA_NOT_SUPPORTED;
	}
	if (result != FPGA_OK) {
		goto uio_close;
	}

	result = usrclk_reset(uio_ptr);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to reset user clock");
	}

uio_close:
	opae_uio_close(&uio);
	return result;
}
