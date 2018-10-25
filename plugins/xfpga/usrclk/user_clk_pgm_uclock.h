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
//****************************************************************************
// Arthur.Sheiman@Intel.com   Created: 03-31-16
// Revised: 10-26-16  18:28
//
// User Clock header file
//
//****************************************************************************

#ifndef USER_CLK_PGM_UCLK_H_
#define USER_CLK_PGM_UCLK_H_

#include <stdint.h>

// .h include, defines
#include "user_clk_pgm_uclock_freq_template_D.h"
// Errors, decimal code
#include "user_clk_pgm_uclock_eror_messages_D.h"
// Private member variables and objects
#include "user_clk_pgm_uclock_freq_template_A.h"
#include "user_clk_pgm_uclock_eror_messages_A.h"

#include "common_int.h"

// qph_user_clk.sv Equates
#if defined(DEF_BDX_P)
// BDX-P:
#define QUCPU_UI64_PRT_UCLK_CMD_0                                              \
	((uint64_t)0x0000000000000605LLU) // 0x03028 / 8 = 0x00605
#define QUCPU_UI64_PRT_UCLK_CMD_1                                              \
	((uint64_t)0x0000000000000606LLU) // 0x03030 / 8 = 0x00606
#define QUCPU_UI64_PRT_UCLK_STS_0                                              \
	((uint64_t)0x0000000000000607LLU) // 0x03038 / 8 = 0x00607
#define QUCPU_UI64_PRT_UCLK_STS_1                                              \
	((uint64_t)0x0000000000000608LLU) // 0x03040 / 8 = 0x00608

#elif defined(DEF_SKX_P)
// SKX-P:
#define QUCPU_UI64_PRT_UCLK_CMD_0                                              \
	((uint64_t)0x000000000000000aLLU) // 0x00050 / 8 = 0x0060a
#define QUCPU_UI64_PRT_UCLK_CMD_1                                              \
	((uint64_t)0x000000000000000bLLU) // 0x00058 / 8 = 0x0060b
#define QUCPU_UI64_PRT_UCLK_STS_0                                              \
	((uint64_t)0x000000000000000cLLU) // 0x00060 / 8 = 0x0060c
#define QUCPU_UI64_PRT_UCLK_STS_1                                              \
	((uint64_t)0x000000000000000dLLU) // 0x00068 / 8 = 0x0060d
#endif

#define QUCPU_UI64_AFU_MMIO_PRT_OFFSET_QW                                      \
	((uint64_t)0x0000000000000000LLU) // 0x00000 / 8 = 0x00000

#define QUCPU_UI64_CMD_0_SR1_b58                                               \
	((uint64_t)0x0400000000000000LLU) // fPLL Select: 0=refclk0, 1=refclk1
#define QUCPU_UI64_CMD_0_PDN_b57                                               \
	((uint64_t)0x0200000000000000LLU) // fPLL Powerdoen
#define QUCPU_UI64_CMD_0_PRS_b56                                               \
	((uint64_t)0x0100000000000000LLU) // fPLL management reset
#define QUCPU_UI64_CMD_0_MRN_b52                                               \
	((uint64_t)0x0010000000000000LLU) // mmmach machine reset_n
#define QUCPU_UI64_CMD_0_SEQ_b49t48                                            \
	((uint64_t)0x0003000000000000LLU) // mmmach sequence
#define QUCPU_UI64_CMD_0_WRT_b44                                               \
	((uint64_t)0x0000100000000000LLU) // mmmach write
#define QUCPU_UI64_CMD_0_ADR_b41t32                                            \
	((uint64_t)0x000003ff00000000LLU) // mmmach MM address
#define QUCPU_UI64_CMD_0_DAT_b31t00                                            \
	((uint64_t)0x00000000ffffffffLLU) // mmmach MM write data

#define QUCPU_UI64_CMD_0_AMM_b51t00                                            \
	((uint64_t)0x000fffffffffffffLLU) // Avmm mmmach portion

#define QUCPU_UI64_STS_0_ERR_b63                                               \
	((uint64_t)0x8000000000000000LLU) // mmmach error
#define QUCPU_UI64_STS_0_RCK_b62                                               \
	((uint64_t)0x4000000000000000LLU) // 0=refclk0, 1=refclk1
#define QUCPU_UI64_STS_0_BSY_b61                                               \
	((uint64_t)0x2000000000000000LLU) // fPLL cal busy
#define QUCPU_UI64_STS_0_LCK_b60                                               \
	((uint64_t)0x1000000000000000LLU) // fPLL locked
#define QUCPU_UI64_STS_0_SR1_b58                                               \
	((uint64_t)0x0400000000000000LLU) // fPLL Select: 0=refclk0, 1=refclk1
#define QUCPU_UI64_STS_0_PDN_b57                                               \
	((uint64_t)0x0200000000000000LLU) // fPLL Powerdoen
#define QUCPU_UI64_STS_0_PRS_b56                                               \
	((uint64_t)0x0100000000000000LLU) // fPLL management reset
#define QUCPU_UI64_STS_0_MRN_b52                                               \
	((uint64_t)0x0010000000000000LLU) // mmmach machine reset_n
#define QUCPU_UI64_STS_0_SEQ_b49t48                                            \
	((uint64_t)0x0003000000000000LLU) // mmmach sequence
#define QUCPU_UI64_STS_0_WRT_b44                                               \
	((uint64_t)0x0000100000000000LLU) // mmmach write
#define QUCPU_UI64_STS_0_ADR_b41t32                                            \
	((uint64_t)0x000003ff00000000LLU) // mmmach MM address
#define QUCPU_UI64_STS_0_DAT_b31t00                                            \
	((uint64_t)0x00000000ffffffffLLU) // mmmach MM read  data

#define QUCPU_UI64_CMD_1_MEA_b32                                               \
	((uint64_t)0x0000000100000000LLU) // 1: measure user clock; 0: measure
					  // 2nd clock, div2

#define QUCPU_UI64_STS_1_VER_b63t60                                            \
	((uint64_t)0xf000000000000000LLU) // frequency in 10 kHz units
#define QUCPU_UI64_STS_1_MEA_b32                                               \
	((uint64_t)0x0000000100000000LLU) // 1: measure user clock; 0: measure
					  // 2nd clock, div2
#define QUCPU_UI64_STS_1_FRQ_b16t00                                            \
	((uint64_t)0x000000000001ffffLLU) // frequency in 10 kHz units

#define QUCPU_UI64_STS_1_VER_version                                           \
	((uint64_t)0x03LLU) // Expected version number
#define QUCPU_UI64_STS_1_VER_version_legacy                                    \
	((uint64_t)0x01LLU) // Expected version number on legacy systems


#define QUCPU_UI64_AVMM_FPLL_IPI_200 ((uint64_t)0x200LLU) // IP identifer
#define QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RFDUAL                                \
	((uint64_t)0x05LLU) // Expected ID, RF=100 MHz & RF=322.265625 MHz
#define QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RF100M                                \
	((uint64_t)0x06LLU) // Expected ID, RF=100 MHz
#define QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RF322M                                \
	((uint64_t)0x07LLU) // Expected ID,              RF=322.265625 MHz

#define QUCPU_UI64_AVMM_FPLL_GPR_280 ((uint64_t)0x280LLU)
#define QUCPU_UI64_AVMM_FPLL_GPR_280_PDN_b00                                   \
	((uint64_t)0x0000000000000001LLU) // Powerdown when override set
#define QUCPU_UI64_AVMM_FPLL_GPR_280_ADM_b01                                   \
	((uint64_t)0x0000000000000001LLU) // 1: Override listen to ADME; 0:
					  // listen to powerdown port

// Bugs, decimal code
#define QUCPU_INT_UCLOCK_BUG_SLEEP_SHORT ((int)1) // Bug in fv_SleepShort

// Structures and Types
struct QUCPU_sInitz {
	uint64_t u64i_Version;	 // Version of clock user
	uint64_t u64i_PLL_ID;	  // PLL ID
	uint64_t u64i_NumFrq_Intg_End; // Integer/exact fPLL indices [0  ..End]
	uint64_t u64i_NumFrq_Frac_Beg; // Fractional    fPLL indices [Beg..End]
	uint64_t u64i_NumFrq_Frac_End;
	uint64_t u64i_NumFrq; // Array frequency  # of elements
	uint64_t u64i_NumReg; // Array registers  # of elements
	uint64_t u64i_NumRck; // Array ref-clocks # of elements
};

struct QUCPU_sFreqs {
	uint64_t u64i_Frq_ClkUsr; // Read user clock frequency (Hz)
	uint64_t u64i_Frq_DivBy2; // Read user clock frequency (Hz) divided-by-2
				  // output
};

typedef struct QUCPU_sInitz QUCPU_tInitz;

typedef struct QUCPU_sFreqs QUCPU_tFreqs;


struct QUCPU_Uclock {
	char sysfs_path[SYSFS_PATH_MAX];   // Port sysfs path
	int i_Bug_First;		   // First bug
	int i_Bug_Last;			   // Lasr bug
	int i_InitzState;		   // Initialization state
	QUCPU_tInitz tInitz_InitialParams; // Initialization parameters
	uint64_t u64i_cmd_reg_0;	   // Command register 0
	uint64_t u64i_cmd_reg_1;	   // Command register 1
	uint64_t u64i_AVMM_seq;		   // Sequence ID
};

int fi_GetFreqs(QUCPU_tFreqs *ptFreqs_retFreqs);

int fi_SetFreqs(uint64_t u64i_Refclk, uint64_t u64i_FrqInx);

int fi_RunInitz(const char *sysfs_path);

int sysfs_read_file(const char *sysfs_path, const char *csr_path,
		    uint64_t *value);

int sysfs_write_file(const char *sysfs_path, const char *csr_path,
		     uint64_t value);

int fi_WaitCalDone(void);

void fv_BugLog(int i_BugID);

int fi_AvmmReadModifyWrite(uint64_t u64i_AvmmAdr, uint64_t u64i_AvmmDat,
			   uint64_t u64i_AvmmMsk);

int fi_AvmmReadModifyWriteVerify(uint64_t u64i_AvmmAdr, uint64_t u64i_AvmmDat,
				 uint64_t u64i_AvmmMsk);

void fv_SleepShort(long int li_sleep_nanoseconds);

int fi_AvmmWrite(uint64_t u64i_AvmmAdr, uint64_t u64i_WriteData);

int fi_AvmmRead(uint64_t u64i_AvmmAdr, uint64_t *pu64i_ReadData);

const char *fpac_GetErrMsg(int i_ErrMsgInx);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get fpga user clock
 *
 * @param syfs_path  port sysfs path
 * @parm  pointer to  high user clock
 * @parm  pointer to  low user clock
 *
 * @return error code
 */
fpga_result get_userclock(const char *sysfs_path, uint64_t *userclk_high,
			  uint64_t *userclk_low);

/**
 * @brief set fpga user clock
 *
 * @param syfs_path  port sysfs path
 * @parm  high user clock
 * @parm  low user clock
 *
 * @return error code
 */
fpga_result set_userclock(const char *sysfs_path, uint64_t userclk_high,
			  uint64_t userclk_low);

#ifdef __cplusplus
}
#endif

#endif // end  USER_CLK_PGM_UCLK_H_
