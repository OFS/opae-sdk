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
// Arthur.Sheiman@Intel.com   Created: 09-08-16
// Revision: 10-18-16  18:06


#include <errno.h>
#include <malloc.h>    /* malloc */
#include <stdlib.h>    /* exit */
#include <stdio.h>     /* printf */
#include <string.h>    /* memcpy */
#include <unistd.h>    /* getpid */
#include <stdint.h>
#include <ctype.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <time.h>
#include <glob.h>

#include "safe_string/safe_string.h"

#include "user_clk_pgm_uclock.h"
#include "user_clk_pgm_uclock_freq_template.h"
#include "user_clk_pgm_uclock_freq_template_322.h"
#include "user_clk_pgm_uclock_eror_messages.h"
#include "user_clk_iopll_freq.h"


// user clock sysfs
#define  USER_CLOCK_CMD0       "userclk_freqcmd"
#define  USER_CLOCK_CMD1       "userclk_freqcntrcmd"
#define  USER_CLOCK_STS0       "userclk_freqsts"
#define  USER_CLOCK_STS1       "userclk_freqcntrsts"
#define  IOPLL_CLOCK_FREQ      "intel-pac-iopll.*.auto/userclk/frequency"
#define  MAX_FPGA_FREQ          1200
#define  MIN_FPGA_FREQ          25

// User clock sleep
#define  USRCLK_SLEEEP_1MS           1000000
#define  USRCLK_SLEEEP_10MS          10000000

struct  QUCPU_Uclock   gQUCPU_Uclock;

static int using_iopll(char* sysfs_usrpath, const char* sysfs_path);

//Get fpga user clock
fpga_result __FIXME_MAKE_VISIBLE__ get_userclock(const char* sysfs_path,
					uint64_t* userclk_high,
					uint64_t* userclk_low)
{
	char sysfs_usrpath[SYSFS_PATH_MAX];
	QUCPU_tFreqs userClock;
	fpga_result result;
	uint32_t high, low;
	int ret;

	if ((sysfs_path == NULL) ||
		(userclk_high == NULL) ||
		(userclk_low == NULL)) {
		FPGA_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	// Test for the existence of the userclk_frequency file
	// which indicates an S10 driver
	ret = using_iopll(sysfs_usrpath, sysfs_path);
	if (ret == FPGA_OK) {
		result = sysfs_read_u32_pair(sysfs_usrpath, &low, &high, ' ');
		if (FPGA_OK != result)
			return result;

		*userclk_high = high * 1000;	// Adjust to Hz
		*userclk_low = low * 1000;
		return FPGA_OK;
	} else if (ret == FPGA_NO_ACCESS) {
		return ret;
	}

	// Initialize
	if (fi_RunInitz(sysfs_path) != 0) {
		FPGA_ERR("Failed to initialize user clock ");
		return FPGA_NOT_SUPPORTED;
	}

	// get user clock
	if (fi_GetFreqs(&userClock) != 0) {
		FPGA_ERR("Failed to get user clock Frequency ");
		return FPGA_NOT_SUPPORTED;
	}

	*userclk_high = userClock.u64i_Frq_ClkUsr;
	*userclk_low = userClock.u64i_Frq_DivBy2;

	return FPGA_OK;
}

// set fpga user clock
fpga_result __FIXME_MAKE_VISIBLE__ set_userclock(const char* sysfs_path,
					uint64_t userclk_high,
					uint64_t userclk_low)
{
	char sysfs_usrpath[SYSFS_PATH_MAX] = { 0, };
	uint64_t freq = userclk_high;
	uint64_t refClk = 0;
	int fd, res;
	char *bufp;
	size_t cnt;
	int ret;

	if (sysfs_path == NULL) {
		FPGA_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	ret = using_iopll(sysfs_usrpath, sysfs_path);
	if (ret == FPGA_OK) {
		// Enforce 1x clock within valid range
		if ((userclk_low > IOPLL_MAX_FREQ) ||
		    (userclk_low < IOPLL_MIN_FREQ)) {
			FPGA_ERR("Invalid Input frequency");
			return FPGA_INVALID_PARAM;
		}

		fd = open(sysfs_usrpath, O_WRONLY);
		if (fd < 0) {
			FPGA_MSG("open(%s) failed: %s",
				 sysfs_usrpath, strerror(errno));
			return FPGA_NOT_FOUND;
		}
		bufp = (char *)&iopll_freq_config[userclk_low];
		cnt = sizeof(struct iopll_config);
		do {
			res = write(fd, bufp, cnt);
			if (res < 0) {
				FPGA_ERR("Failed to write");
				break;
			}
			bufp += res;
			cnt -= res;
		} while (cnt > 0);
		close(fd);

		return FPGA_OK;
	} else if (ret == FPGA_NO_ACCESS) {
		return ret;
	}

	// verify user clock freq range (100hz to 1200hz)
	if ((userclk_high > MAX_FPGA_FREQ) ||
		(userclk_low > MAX_FPGA_FREQ)) {
		FPGA_ERR("Invalid Input frequency");
		return FPGA_INVALID_PARAM;
	}

	if (userclk_low != 0 && userclk_high != 0
		&& userclk_low > userclk_high) {
		FPGA_ERR("Invalid Input low frequency");
		return FPGA_INVALID_PARAM;
	}

	if (freq < MIN_FPGA_FREQ){
		FPGA_ERR("Invalid Input frequency");
		return FPGA_INVALID_PARAM;
	}

	// Initialize
	if (fi_RunInitz(sysfs_path) != 0) {
		FPGA_ERR("Failed to initialize user clock ");
		return FPGA_NOT_SUPPORTED;
	}

	if ((gQUCPU_Uclock.tInitz_InitialParams.u64i_Version == QUCPU_UI64_STS_1_VER_version_legacy) &&
		(gQUCPU_Uclock.tInitz_InitialParams.u64i_PLL_ID == QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RF322M))
	{ // Use the 322.265625 MHz REFCLK
		refClk = 1;
	}

	FPGA_DBG("User clock: %ld \n", freq);

	// set user clock
	if (fi_SetFreqs(refClk, freq) != 0) {
		FPGA_ERR("Failed to set user clock frequency ");
		return FPGA_NOT_SUPPORTED;
	}

	return FPGA_OK;
}

//fi_RunInitz
int fi_RunInitz(const char* sysfs_path)
{
	// fi_RunInitz
	// Initialize
	// Reinitialization okay too, since will issue machine reset

	uint64_t u64i_PrtData;
	uint64_t u64i_AvmmAdr, u64i_AvmmDat;
	int      i_ReturnErr;
	char syfs_usrpath[SYSFS_PATH_MAX];

	gQUCPU_Uclock.i_InitzState = 0;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_Version = (uint64_t) 0;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_PLL_ID = (uint64_t) 0;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_NumFrq_Intg_End = (uint64_t) 0;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_NumFrq_Frac_Beg = (uint64_t) 0;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_NumFrq_Frac_End = (uint64_t) 0;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_NumFrq = (uint64_t) 0;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_NumReg = (uint64_t) 0;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_NumRck = (uint64_t) 0;
	gQUCPU_Uclock.u64i_cmd_reg_0 = (uint64_t) 0x0LLU;
	gQUCPU_Uclock.u64i_cmd_reg_1 = (uint64_t) 0x0LLU;
	gQUCPU_Uclock.u64i_AVMM_seq = (uint64_t) 0x0LLU;
	gQUCPU_Uclock.i_Bug_First = 0;
	gQUCPU_Uclock.i_Bug_Last = 0;


	if (sysfs_path == NULL) {
		printf(" Invalid input sysfs path \n");
		return -1;
	}
	snprintf_s_s(gQUCPU_Uclock.sysfs_path, sizeof(gQUCPU_Uclock.sysfs_path), "%s", sysfs_path);

	// Assume return error okay, for now
	i_ReturnErr = 0;

	// Initialize default values (for error abort)
	gQUCPU_Uclock.tInitz_InitialParams.u64i_Version = 0;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_PLL_ID = 0;

	// Initialize command shadow registers
	gQUCPU_Uclock.u64i_cmd_reg_0 = ((uint64_t) 0x0LLU);
	gQUCPU_Uclock.u64i_cmd_reg_1 = ((uint64_t) 0x0LLU);

	// Initialize sequence IO
	gQUCPU_Uclock.u64i_AVMM_seq = ((uint64_t) 0x0LLU);

	// Static values
	gQUCPU_Uclock.tInitz_InitialParams.u64i_NumFrq_Intg_End = (uint64_t) QUCPU_INT_NUMFRQ_INTG_END;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_NumFrq_Frac_Beg = (uint64_t) QUCPU_INT_NUMFRQ_FRAC_BEG;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_NumFrq_Frac_End = (uint64_t) QUCPU_INT_NUMFRQ_FRAC_END;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_NumFrq = (uint64_t) QUCPU_INT_NUMFRQ;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_NumReg = (uint64_t) QUCPU_INT_NUMREG;
	gQUCPU_Uclock.tInitz_InitialParams.u64i_NumRck = (uint64_t) QUCPU_INT_NUMRCK;


	// Read version number
	if (i_ReturnErr == 0) // This always true; added for future safety
	{
		// Verifying User Clock version number
		snprintf_s_ss(syfs_usrpath, sizeof(syfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS1);
		sysfs_read_u64(syfs_usrpath, &u64i_PrtData);
		//printf(" fi_RunInitz u64i_PrtData %llx  \n", u64i_PrtData);

		gQUCPU_Uclock.tInitz_InitialParams.u64i_Version = (u64i_PrtData & QUCPU_UI64_STS_1_VER_b63t60) >> 60;
		if ((gQUCPU_Uclock.tInitz_InitialParams.u64i_Version != QUCPU_UI64_STS_1_VER_version) &&
		    (gQUCPU_Uclock.tInitz_InitialParams.u64i_Version != QUCPU_UI64_STS_1_VER_version_legacy))
		{ // User Clock wrong version number
			i_ReturnErr = QUCPU_INT_UCLOCK_RUNINITZ_ERR_VER;

		} // User Clock wrong version number
	} // Verifying User Clock version number

	FPGA_DBG("User clock version = %lx \n", gQUCPU_Uclock.tInitz_InitialParams.u64i_Version);

	// Read PLL ID
	if (i_ReturnErr == 0)
	{ // Waiting for fcr PLL calibration not to be busy
		i_ReturnErr = fi_WaitCalDone();
	} // Waiting for fcr PLL calibration not to be busy

	if (i_ReturnErr == 0)
	{
		// Cycle reset and wait for any calibration to finish
		// Activating management & machine reset

		gQUCPU_Uclock.u64i_cmd_reg_0 |= (QUCPU_UI64_CMD_0_PRS_b56);
		gQUCPU_Uclock.u64i_cmd_reg_0 &= ~(QUCPU_UI64_CMD_0_MRN_b52);
		u64i_PrtData = gQUCPU_Uclock.u64i_cmd_reg_0;

		snprintf_s_ss(syfs_usrpath, sizeof(syfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_CMD0);
		sysfs_write_u64(syfs_usrpath, u64i_PrtData);

		// Deasserting management & machine reset
		gQUCPU_Uclock.u64i_cmd_reg_0 |= (QUCPU_UI64_CMD_0_MRN_b52);
		gQUCPU_Uclock.u64i_cmd_reg_0 &= ~(QUCPU_UI64_CMD_0_PRS_b56);
		u64i_PrtData = gQUCPU_Uclock.u64i_cmd_reg_0;

		sysfs_write_u64(syfs_usrpath, u64i_PrtData);
		//printf(" fi_RunInitz u64i_PrtData %llx  \n", u64i_PrtData);

		// Waiting for fcr PLL calibration not to be busy
		i_ReturnErr = fi_WaitCalDone();
	} // Cycle reset and wait for any calibration to finish

	if (i_ReturnErr == 0)
	{ // Checking fPLL ID
		u64i_AvmmAdr = QUCPU_UI64_AVMM_FPLL_IPI_200;
		i_ReturnErr = fi_AvmmRead(u64i_AvmmAdr, &u64i_AvmmDat);
		if (i_ReturnErr == 0)
		{ // Check identifier
			gQUCPU_Uclock.tInitz_InitialParams.u64i_PLL_ID = u64i_AvmmDat & 0xffLLU;
			if (!(gQUCPU_Uclock.tInitz_InitialParams.u64i_PLL_ID == QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RFDUAL
				|| gQUCPU_Uclock.tInitz_InitialParams.u64i_PLL_ID == QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RF100M
				|| gQUCPU_Uclock.tInitz_InitialParams.u64i_PLL_ID == QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RF322M))
			{ // ERROR: Wrong fPLL ID Identifer
				printf(" ERROR  \n");
				i_ReturnErr = QUCPU_INT_UCLOCK_RUNINITZ_ERR_FPLL_ID_ILLEGAL;
			} // ERROR: Wrong fPLL ID Identifer
		} // Check identifier
	} // Checking fPLL ID

	// Copy structure, initialize, and return based on error status
	//*ptInitz_retInitz = gQUCPU_Uclock.tInitz_InitialParams;
	gQUCPU_Uclock.i_InitzState = !i_ReturnErr; // Set InitzState to 0 or 1

	return  (i_ReturnErr);
} // fi_RunInitz

//fu64i_GetAVMM_seq
uint64_t fu64i_GetAVMM_seq()
{
	// fu64i_GetAVMM_seq
	// Increment seq
	gQUCPU_Uclock.u64i_AVMM_seq++;
	gQUCPU_Uclock.u64i_AVMM_seq &= 0x03LLU;

	return(gQUCPU_Uclock.u64i_AVMM_seq);
} // fu64i_GetAVMM_seq


//fi_AvmmRWcom
int fi_AvmmRWcom(int i_CmdWrite,
		uint64_t   u64i_AvmmAdr,
		uint64_t   u64i_WriteData,
		uint64_t *pu64i_ReadData)
{
	// fi_AvmmRWcom
	uint64_t u64i_SeqCmdAddrData, u64i_SeqCmdAddrData_seq_2, u64i_SeqCmdAddrData_wrt_1;
	uint64_t u64i_SeqCmdAddrData_adr_10, u64i_SeqCmdAddrData_dat_32;
	uint64_t u64i_PrtData;
	uint64_t u64i_DataX;
	uint64_t u64i_FastPoll, u64i_SlowPoll;
	long int li_sleep_nanoseconds;
	int      i_ReturnErr;
	char syfs_usrpath[SYSFS_PATH_MAX];

	// Assume return error okay, for now
	i_ReturnErr = 0;

	// Common portion
	u64i_SeqCmdAddrData_seq_2 = fu64i_GetAVMM_seq();
	u64i_SeqCmdAddrData_adr_10 = u64i_AvmmAdr;

	if (i_CmdWrite == 1)
	{
		// Write data
		u64i_SeqCmdAddrData_wrt_1 = 0x1LLU;
		u64i_SeqCmdAddrData_dat_32 = u64i_WriteData;
	} // Write data
	else
	{ // Read data
		u64i_SeqCmdAddrData_wrt_1 = 0x0LLU;
		u64i_SeqCmdAddrData_dat_32 = 0x0LLU;
	} // Read data

	u64i_SeqCmdAddrData = (u64i_SeqCmdAddrData_seq_2 & 0x00000003LLU) << 48  // [49:48]
							| (u64i_SeqCmdAddrData_wrt_1 & 0x00000001LLU) << 44  // [   44]
							| (u64i_SeqCmdAddrData_adr_10 & 0x000003ffLLU) << 32  // [41:32]
							| (u64i_SeqCmdAddrData_dat_32 & 0xffffffffLLU) << 0; // [31:00]

	gQUCPU_Uclock.u64i_cmd_reg_0 &= ~QUCPU_UI64_CMD_0_AMM_b51t00;
	gQUCPU_Uclock.u64i_cmd_reg_0 |= u64i_SeqCmdAddrData;

	// Write register 0 to kick it off

	u64i_PrtData = gQUCPU_Uclock.u64i_cmd_reg_0;
	snprintf_s_ss(syfs_usrpath, sizeof(syfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_CMD0);
	sysfs_write_u64(syfs_usrpath, u64i_PrtData);

	li_sleep_nanoseconds = USRCLK_SLEEEP_1MS;
	fv_SleepShort(li_sleep_nanoseconds);

	snprintf_s_ss(syfs_usrpath, sizeof(syfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS0);

	// Poll register 0 for completion.
	// CCI is synchronous and needs only 1 read with matching sequence.

	for (u64i_SlowPoll = 0; u64i_SlowPoll<100; ++u64i_SlowPoll)  // 100 ms
	{ // Poll 0, slow outer loop with 1 ms sleep
		for (u64i_FastPoll = 0; u64i_FastPoll<100; ++u64i_FastPoll)
		{
			// Poll 0, fast inner loop with no sleep
			sysfs_read_u64(syfs_usrpath, &u64i_DataX);

			if ((u64i_DataX & QUCPU_UI64_STS_0_SEQ_b49t48) == (u64i_SeqCmdAddrData & QUCPU_UI64_STS_0_SEQ_b49t48))
			{ // Have result
				goto GOTO_LABEL_HAVE_RESULT;
			} // Have result
		} // Poll 0, fast inner loop with no sleep

		// Sleep 1 ms
		li_sleep_nanoseconds = USRCLK_SLEEEP_1MS;
		fv_SleepShort(li_sleep_nanoseconds);
	} // Poll 0, slow outer loop with 1 ms sleep

	i_ReturnErr = QUCPU_INT_UCLOCK_AVMMRWCOM_ERR_TIMEOUT; // Error

GOTO_LABEL_HAVE_RESULT: // No error

	if (i_CmdWrite == 0) *pu64i_ReadData = u64i_DataX;
	return(i_ReturnErr);

} // fi_AvmmRWcom


//fi_AvmmRead
int fi_AvmmRead(uint64_t u64i_AvmmAdr, uint64_t *pu64i_ReadData)
{
	// fi_AvmmRead
	int         i_CmdWrite    = 0;
	uint64_t u64i_WriteData   = 0;
	int         res           = 0;

	// Perform read with common code
	i_CmdWrite = 0;
	u64i_WriteData = 0; // Not used for read
	res = fi_AvmmRWcom(i_CmdWrite, u64i_AvmmAdr, u64i_WriteData, pu64i_ReadData);

	// Return error status
	return(res);
} // fi_AvmmRead

//fi_AvmmWrite
int fi_AvmmWrite(uint64_t u64i_AvmmAdr, uint64_t u64i_WriteData)
{
	// fi_AvmmWrite
	int         i_CmdWrite   = 0;
	uint64_t u64i_ReadData   = 0;  // Read data is not used
	int         res          = 0;

	// Perform write with common code
	i_CmdWrite = 1;
	res = fi_AvmmRWcom(i_CmdWrite, u64i_AvmmAdr, u64i_WriteData, &u64i_ReadData);

	// Return error status
	return(res);
} // fi_AvmmWrite


//Sleep for nanoseconds
void fv_SleepShort(long int li_sleep_nanoseconds)
{
	// fv_SleepShort
	// Sleep for nanoseconds
	struct timespec timespecRemaining     = {0};
	struct timespec timespecWait          = {0};
	int res                               = 0;

	timespecRemaining.tv_nsec = li_sleep_nanoseconds; timespecRemaining.tv_sec = 0;

	do
	{ // Wait, and retry if wait ended early
		timespecWait = timespecRemaining;
		res = (int) nanosleep(&timespecWait, &timespecRemaining);
		if (res != 0 && res != -1)
		{ // BUG: unexpected nanosleep return value
			fv_BugLog((int) QUCPU_INT_UCLOCK_BUG_SLEEP_SHORT);
		} // BUG: unexpected nanosleep return value
	} // Wait, and retry if wait ended early
	while (res != 0);

	return;
} // fv_SleepShort

// get user clock
// Read the frequency for the User clock and div2 clock
int fi_GetFreqs(QUCPU_tFreqs *ptFreqs_retFreqs)
{
	// fi_GetFreqs
	// Read the frequency for the User clock and div2 clock

	uint64_t u64i_PrtData                 = 0;
	long int li_sleep_nanoseconds         = 0;
	int      res                          = 0;
	char syfs_usrpath[SYSFS_PATH_MAX]     = {0};

	// Assume return error okay, for now
	res                           = 0;

	if (!gQUCPU_Uclock.i_InitzState) res = QUCPU_INT_UCLOCK_GETFREQS_ERR_INITZSTATE;

	if (res == 0)
	{ // Read div2 and 1x user clock frequency
		// Low frequency
		gQUCPU_Uclock.u64i_cmd_reg_1 &= ~QUCPU_UI64_CMD_1_MEA_b32;

		u64i_PrtData = gQUCPU_Uclock.u64i_cmd_reg_1;
		snprintf_s_ss(syfs_usrpath, sizeof(syfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_CMD1);
		sysfs_write_u64(syfs_usrpath, u64i_PrtData);


		li_sleep_nanoseconds = USRCLK_SLEEEP_10MS;            // 10 ms for frequency counter
		fv_SleepShort(li_sleep_nanoseconds);

		snprintf_s_ss(syfs_usrpath, sizeof(syfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS1);
		sysfs_read_u64(syfs_usrpath,  &u64i_PrtData);


		ptFreqs_retFreqs->u64i_Frq_DivBy2 = (u64i_PrtData & QUCPU_UI64_STS_1_FRQ_b16t00) * 10000; // Hz
		//printf(" ptFreqs_retFreqs->u64i_Frq_ClkUsr %llx \n", ptFreqs_retFreqs->u64i_Frq_DivBy2);
		li_sleep_nanoseconds = USRCLK_SLEEEP_10MS;
		fv_SleepShort(li_sleep_nanoseconds);

		// High frequency
		gQUCPU_Uclock.u64i_cmd_reg_1 |= QUCPU_UI64_CMD_1_MEA_b32;

		u64i_PrtData = gQUCPU_Uclock.u64i_cmd_reg_1;

		snprintf_s_ss(syfs_usrpath, sizeof(syfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_CMD1);
		sysfs_write_u64(syfs_usrpath,  u64i_PrtData);

		li_sleep_nanoseconds = USRCLK_SLEEEP_10MS; // 10 ms for frequency counter
		fv_SleepShort(li_sleep_nanoseconds);

		snprintf_s_ss(syfs_usrpath, sizeof(syfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS1);
		sysfs_read_u64(syfs_usrpath,  &u64i_PrtData);
		ptFreqs_retFreqs->u64i_Frq_ClkUsr = (u64i_PrtData & QUCPU_UI64_STS_1_FRQ_b16t00) * 10000; // Hz
		//printf(" ptFreqs_retFreqs->u64i_Frq_ClkUsr %llx \n", ptFreqs_retFreqs->u64i_Frq_ClkUsr);

		fv_SleepShort(li_sleep_nanoseconds);

	} // Read div2 and 1x user clock frequency

	FPGA_DBG("\nApproximate frequency:\n"
		"High clock = %5.1f MHz\n"
		"Low clock  = %5.1f MHz\n \n",
		ptFreqs_retFreqs->u64i_Frq_ClkUsr / 1.0e6, (ptFreqs_retFreqs->u64i_Frq_DivBy2) / 1.0e6);


	return (res);
} // fi_GetFreqs

// set user clock
int fi_SetFreqs(uint64_t u64i_Refclk,
		uint64_t u64i_FrqInx)
{
	// fi_SetFreqs
	// Set the user clock frequency
	uint64_t u64i_I, u64i_MifReg, u64i_PrtData;
	uint64_t u64i_AvmmAdr, u64i_AvmmDat, u64i_AvmmMsk;
	long int li_sleep_nanoseconds;
	int      i_ReturnErr;
	char syfs_usrpath[SYSFS_PATH_MAX];

	// Assume return error okay, for now
	i_ReturnErr = 0;

	if (!gQUCPU_Uclock.i_InitzState) i_ReturnErr = QUCPU_INT_UCLOCK_SETFREQS_ERR_INITZSTATE;

	if (i_ReturnErr == 0)
	{ // Check REFCLK
		if (u64i_Refclk == 0)
		{ // 100 MHz REFCLK requested
			if (!(gQUCPU_Uclock.tInitz_InitialParams.u64i_PLL_ID == QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RFDUAL
				|| gQUCPU_Uclock.tInitz_InitialParams.u64i_PLL_ID == QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RF100M))
				i_ReturnErr = QUCPU_INT_UCLOCK_SETFREQS_ERR_REFCLK_100M_MISSING;
		} // 100 MHz REFCLK requested
		else if (u64i_Refclk == 1)
		{ // 322.265625 MHz REFCLK requested
			if (!(gQUCPU_Uclock.tInitz_InitialParams.u64i_PLL_ID == QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RFDUAL
				|| gQUCPU_Uclock.tInitz_InitialParams.u64i_PLL_ID == QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RF322M))
				i_ReturnErr = QUCPU_INT_UCLOCK_SETFREQS_ERR_REFCLK_322M_MISSING;
		} // 322.265625 MHz REFCLK requested
		else i_ReturnErr = QUCPU_INT_UCLOCK_SETFREQS_ERR_REFCLK_ILLEGAL;
	} // Check REFCLK

	if (i_ReturnErr == 0)
	{ // Check frequency index
		if (u64i_FrqInx > gQUCPU_Uclock.tInitz_InitialParams.u64i_NumFrq_Frac_End)
			i_ReturnErr = QUCPU_INT_UCLOCK_SETFREQS_ERR_FINDEX_OVERRANGE;
		else if (u64i_FrqInx   < gQUCPU_Uclock.tInitz_InitialParams.u64i_NumFrq_Frac_Beg
			&& u64i_FrqInx   > gQUCPU_Uclock.tInitz_InitialParams.u64i_NumFrq_Intg_End)
			i_ReturnErr = QUCPU_INT_UCLOCK_SETFREQS_ERR_FINDEX_INTG_RANGE_BAD;
		else if (u64i_FrqInx   < gQUCPU_Uclock.tInitz_InitialParams.u64i_NumFrq_Frac_Beg
			&& u64i_Refclk != 1) // Integer-PLL mode, exact requires 322.265625 MHz
			i_ReturnErr = QUCPU_INT_UCLOCK_SETFREQS_ERR_FINDEX_INTG_NEEDS_322M;
	} // Check frequency index


	if (i_ReturnErr == 0)
	{ // Power down PLL
		// Altera bug. Power down pin doesn't work  SR #11229652.
		// WORKAROUND: Use power down port
		u64i_AvmmAdr = 0x2e0LLU;
		u64i_AvmmDat = 0x03LLU;
		u64i_AvmmMsk = 0x03LLU;

		i_ReturnErr = fi_AvmmReadModifyWriteVerify(u64i_AvmmAdr, u64i_AvmmDat, u64i_AvmmMsk);

		// Sleep 1 ms
		li_sleep_nanoseconds = USRCLK_SLEEEP_1MS;
		fv_SleepShort(li_sleep_nanoseconds);
	} // Power down PLL

	if (i_ReturnErr == 0)
	{ // Verifying fcr PLL not locking

		snprintf_s_ss(syfs_usrpath, sizeof(syfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS0);
		sysfs_read_u64(syfs_usrpath,  &u64i_PrtData);
		//sysfs_read_uint64(gQUCPU_Uclock.sys_path, USER_CLOCK_STS0, &u64i_PrtData);

		if ((u64i_PrtData & QUCPU_UI64_STS_0_LCK_b60) != 0)
		{ // fcr PLL is locked but should be unlocked
			i_ReturnErr = QUCPU_INT_UCLOCK_SETFREQS_ERR_PLL_NO_UNLOCK;
		} // fcr PLL is locked but should be unlocked
	} // Verifying fcr PLL not locking

	if (i_ReturnErr == 0)
	{ // Select reference and push table
		// Selecting desired reference clock
		gQUCPU_Uclock.u64i_cmd_reg_0 &= ~QUCPU_UI64_CMD_0_SR1_b58;
		if (u64i_Refclk) gQUCPU_Uclock.u64i_cmd_reg_0 |= QUCPU_UI64_CMD_0_SR1_b58;
		u64i_PrtData = gQUCPU_Uclock.u64i_cmd_reg_0;

		snprintf_s_ss(syfs_usrpath, sizeof(syfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_CMD0);
		sysfs_write_u64(syfs_usrpath,  u64i_PrtData);

		// Sleep 1 ms
		li_sleep_nanoseconds = USRCLK_SLEEEP_1MS;
		fv_SleepShort(li_sleep_nanoseconds);

		// Pushing the table
		for (u64i_MifReg = 0; u64i_MifReg<gQUCPU_Uclock.tInitz_InitialParams.u64i_NumReg; u64i_MifReg++)
		{ // Write each register in the diff mif

			uint32_t tbl_entry;
			if (u64i_Refclk == 0)
			{ // 100 MHz table
				tbl_entry = scu32ia3d_DiffMifTbl[(int) u64i_FrqInx][(int) u64i_MifReg][(int) u64i_Refclk];
			}
			else
			{ // 322.265625 MHz table
				tbl_entry = scu32ia3d_DiffMifTbl_322[(int) u64i_FrqInx][(int) u64i_MifReg][(int) u64i_Refclk];
			}

			u64i_AvmmAdr = (uint64_t) (tbl_entry) >> 16;
			u64i_AvmmDat = (uint64_t) (tbl_entry & 0x000000ff);
			u64i_AvmmMsk = (uint64_t) (tbl_entry & 0x0000ff00) >> 8;
			i_ReturnErr = fi_AvmmReadModifyWriteVerify(u64i_AvmmAdr, u64i_AvmmDat, u64i_AvmmMsk);

			if (i_ReturnErr) break;
		} // Write each register in the diff mif
	} // Select reference and push table

	if (i_ReturnErr == 0)
	{ // Waiting for fcr PLL calibration not to be busy
		i_ReturnErr = fi_WaitCalDone();
	} // Waiting for fcr PLL calibration not to be busy

	if (i_ReturnErr == 0)
	{ // Recalibrating

		// "Request user access to the internal configuration bus"
		// and "Wait for reconfig_waitrequest to be deasserted."
		// Note that the Verify operation performs the post "wait."

		u64i_AvmmAdr = 0x000LLU;
		u64i_AvmmDat = 0x02LLU;
		u64i_AvmmMsk = 0xffLLU;
		i_ReturnErr = fi_AvmmReadModifyWriteVerify(u64i_AvmmAdr, u64i_AvmmDat, u64i_AvmmMsk);

		if (i_ReturnErr == 0)
		{ // "To calibrate the fPLL, Read-Modify-Write:" set B1 of 0x100 high
			u64i_AvmmAdr = 0x100LLU;
			u64i_AvmmDat = 0x02LLU;
			u64i_AvmmMsk = 0x02LLU;
			i_ReturnErr = fi_AvmmReadModifyWrite(u64i_AvmmAdr, u64i_AvmmDat, u64i_AvmmMsk);
		} // "To calibrate the fPLL, Read-Modify-Write:" set B1 of 0x100 high

		if (i_ReturnErr == 0)
		{ // "Release the internal configuraiton bus to PreSICE to perform recalibration"
			u64i_AvmmAdr = 0x000LLU;
			u64i_AvmmDat = 0x01LLU;
			i_ReturnErr = fi_AvmmWrite(u64i_AvmmAdr, u64i_AvmmDat);

			// Sleep 1 ms
			li_sleep_nanoseconds = USRCLK_SLEEEP_1MS;
			fv_SleepShort(li_sleep_nanoseconds);
		} // "Release the internal configuraiton bus to PreSICE to perform recalibration"
	} // Recalibrating

	if (i_ReturnErr == 0)
	{ // Waiting for fcr PLL calibration not to be busy
		i_ReturnErr = fi_WaitCalDone();
	} // Waiting for fcr PLL calibration not to be busy

	if (i_ReturnErr == 0)
	{ // Power up PLL
		// Altera bug. Power down pin doesn't work  SR #11229652.
		// WORKAROUND: Use power down port
		u64i_AvmmAdr = 0x2e0LLU;
		u64i_AvmmDat = 0x02LLU;
		u64i_AvmmMsk = 0x03LLU;
		i_ReturnErr = fi_AvmmReadModifyWriteVerify(u64i_AvmmAdr, u64i_AvmmDat, u64i_AvmmMsk);
	} // Power up PLL

	if (i_ReturnErr == 0)
	{ // Wait for PLL to lock

		for (u64i_I = 0; u64i_I<100; u64i_I++)
		{ // Poll with 100 ms timeout

			snprintf_s_ss(syfs_usrpath, sizeof(syfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS0);
			sysfs_read_u64(syfs_usrpath,  &u64i_PrtData);

			if ((u64i_PrtData & QUCPU_UI64_STS_0_LCK_b60) != 0) break;

			// Sleep 1 ms
			li_sleep_nanoseconds = USRCLK_SLEEEP_1MS;
			fv_SleepShort(li_sleep_nanoseconds);
		} // Poll with 100 ms timeout

		if ((u64i_PrtData & QUCPU_UI64_STS_0_LCK_b60) == 0)
		{ // fcr PLL lock error

			i_ReturnErr = QUCPU_INT_UCLOCK_SETFREQS_ERR_PLL_LOCK_TO;
		} // fcr PLL lock error
	} // Verifying fcr PLL is locking

	return (i_ReturnErr);
} // fi_SetFreqs

// get error message
//Read the frequency for the User clock and div2 clock
const char * fpac_GetErrMsg(int i_ErrMsgInx)
{
	// fpac_GetErrMsg
	// Read the frequency for the User clock and div2 clock
	const char * pac_ErrMsgStr    =  NULL;

	// Extra "+1" message has index range error message
	pac_ErrMsgStr = pac_UclockErrorMsg[QUCPU_INT_UCLOCK_NUM_ERROR_MESSAGES + 1 - 1];

	// Check index range
	if (i_ErrMsgInx >= 0
		&& i_ErrMsgInx  < QUCPU_INT_UCLOCK_NUM_ERROR_MESSAGES) {
	// All okay, set the message string
		pac_ErrMsgStr = pac_UclockErrorMsg[i_ErrMsgInx];
	} // All okay, set the message string

	return (pac_ErrMsgStr);
} // fpac_GetErrMsg

// fi_AvmmReadModifyWriteVerify
int fi_AvmmReadModifyWriteVerify(uint64_t u64i_AvmmAdr,
				uint64_t u64i_AvmmDat,
				uint64_t u64i_AvmmMsk)
{
	// fi_AvmmReadModifyWriteVerify
	int      res                 = 0;
	uint64_t u64i_VerifyData     = 0;

	res = fi_AvmmReadModifyWrite(u64i_AvmmAdr, u64i_AvmmDat, u64i_AvmmMsk);

	if (res == 0)
	{ // Read back the data and verify mask-enabled bits

		res = fi_AvmmRead(u64i_AvmmAdr, &u64i_VerifyData);

		if (res == 0)
		{ // Perform verify
			if ((u64i_VerifyData & u64i_AvmmMsk) != (u64i_AvmmDat & u64i_AvmmMsk))
			{ // Verify failure
				res = QUCPU_INT_UCLOCK_AVMMRMWV_ERR_VERIFY;
			} // Verify failure
		} // Perform verify
	} // Read back the data and verify mask-enabled bits

	return(res);
} // fi_AvmmReadModifyWriteVerify


// fi_AvmmReadModifyWrite
int fi_AvmmReadModifyWrite(uint64_t u64i_AvmmAdr,
			uint64_t u64i_AvmmDat,
			uint64_t u64i_AvmmMsk)
{
	uint64_t u64i_ReadData    = 0;
	uint64_t u64i_WriteData   = 0;
	int      res              = 0;

	// Read data
	res = fi_AvmmRead(u64i_AvmmAdr, &u64i_ReadData);

	if (res == 0)
	{ // Modify the read data and write it
		u64i_WriteData = (u64i_ReadData & ~u64i_AvmmMsk) | (u64i_AvmmDat & u64i_AvmmMsk);
		res = fi_AvmmWrite(u64i_AvmmAdr, u64i_WriteData);
	} // Modify the read data and write it

	return(res);
} // fi_AvmmReadModifyWrite

// fv_BugLog
// Logs first and last bugs
void fv_BugLog(int i_BugID)
{
	if (gQUCPU_Uclock.i_Bug_First)
	{ // This is not the first bug
		gQUCPU_Uclock.i_Bug_Last = i_BugID;
	} // This is not the first bug
	else
	{ // This is the first bug
		gQUCPU_Uclock.i_Bug_First = i_BugID;
	} // This is the first bug

	return;
} // fv_BugLog

// wait caldone
// Wait for calibration to be done
int fi_WaitCalDone(void)
{
	// fi_WaitCalDone
	// Wait for calibration to be done
	uint64_t u64i_PrtData                = 0;
	uint64_t u64i_I                      = 0;
	long int li_sleep_nanoseconds        = 0;
	int      res                         = 0;
	char syfs_usrpath[SYSFS_PATH_MAX]    = {0};

	// Waiting for fcr PLL calibration not to be busy
	for (u64i_I = 0; u64i_I<1000; u64i_I++)
	{ // Poll with 1000 ms timeout

		snprintf_s_ss(syfs_usrpath, sizeof(syfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS0);
		sysfs_read_u64(syfs_usrpath,  &u64i_PrtData);

		if ((u64i_PrtData & QUCPU_UI64_STS_0_BSY_b61) == 0) break;

		// Sleep 1 ms
		li_sleep_nanoseconds = USRCLK_SLEEEP_1MS;
		fv_SleepShort(li_sleep_nanoseconds);
	} // Poll with 1000 ms timeout


	if ((u64i_PrtData & QUCPU_UI64_STS_0_BSY_b61) != 0)
	{ // ERROR: calibration busy too long
		res = QUCPU_INT_UCLOCK_WAITCALDONE_ERR_BSY_TO;
	} // ERROR: calibration busy too long

	return(res);
} // fi_WaitCalDone

// Determine whether or not the IOPLL is serving as the source of
// the user clock.
static int using_iopll(char* sysfs_usrpath, const char* sysfs_path)
{
	glob_t iopll_glob;

	// Test for the existence of the userclk_frequency file
	// which indicates an S10 driver
	snprintf_s_ss(sysfs_usrpath, SYSFS_PATH_MAX, "%s/%s",
		      sysfs_path, IOPLL_CLOCK_FREQ);

	if (glob(sysfs_usrpath, 0, NULL, &iopll_glob))
		return FPGA_NOT_FOUND;

	if (iopll_glob.gl_pathc > 1)
		FPGA_MSG("WARNING: Port has multiple sysfs frequency files");

	strcpy_s(sysfs_usrpath, SYSFS_PATH_MAX, iopll_glob.gl_pathv[0]);

	globfree(&iopll_glob);

	if (access(sysfs_usrpath, F_OK | R_OK | W_OK) != 0) {
		FPGA_ERR("Unable to access sysfs frequency file");
		return FPGA_NO_ACCESS;
	}

	return FPGA_OK;
}
