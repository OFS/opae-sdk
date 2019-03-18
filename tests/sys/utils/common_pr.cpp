// Copyright(c) 2017-2018, Intel Corporation
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

#include <uuid/uuid.h>
#include <json-c/json.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "common_sys.h"

#define METADATA_GUID "58656F6E-4650-4741-B747-425376303031"
#define METADATA_GUID_LEN 16
#define FPGA_GBS_6_3_0_MAGIC	0x1d1f8680 // dec: 488605312
#define PR_INTERFACE_ID 	"pr/interface_id"
#define INTFC_ID_LOW_LEN	16
#define INTFC_ID_HIGH_LEN	16
#define BUFFER_SIZE		32



//fi_RunInitz
int fi_RunInitz(const char* sysfs_path)
{
    // fi_RunInitz
    // Initialize
    // Reinitialization okay too, since will issue machine reset

    uint64_t u64i_PrtData;
    uint64_t u64i_AvmmAdr, u64i_AvmmDat;
    int      i_ReturnErr;
    char sysfs_usrpath[SYSFS_PATH_MAX];

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
        snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS1);
        sysfs_read_u64(sysfs_usrpath, &u64i_PrtData);
        //printf(" fi_RunInitz u64i_PrtData %llx  \n", u64i_PrtData);

        gQUCPU_Uclock.tInitz_InitialParams.u64i_Version = (u64i_PrtData & QUCPU_UI64_STS_1_VER_b63t60) >> 60;
        if (gQUCPU_Uclock.tInitz_InitialParams.u64i_Version != QUCPU_UI64_STS_1_VER_version)
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

        snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_CMD0);
        sysfs_write_u64(sysfs_usrpath, u64i_PrtData);

        // Deasserting management & machine reset
        gQUCPU_Uclock.u64i_cmd_reg_0 |= (QUCPU_UI64_CMD_0_MRN_b52);
        gQUCPU_Uclock.u64i_cmd_reg_0 &= ~(QUCPU_UI64_CMD_0_PRS_b56);
        u64i_PrtData = gQUCPU_Uclock.u64i_cmd_reg_0;

        sysfs_write_u64(sysfs_usrpath, u64i_PrtData);
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



// Sets FPGA threshold power values
fpga_result set_fpga_pwr_threshold(fpga_handle handle,
                    uint64_t gbs_power)
{
    char sysfs_path[SYSFS_PATH_MAX]   = {0};
    fpga_result result                = FPGA_OK;
    uint64_t fpga_power               = 0;
    struct _fpga_token  *_token       = NULL;
    struct _fpga_handle *_handle      = (struct _fpga_handle *)handle;

    if (_handle == NULL) {
        FPGA_ERR("Invalid handle");
        return FPGA_INVALID_PARAM;
    }

    _token = (struct _fpga_token *)_handle->token;
    if (_token == NULL) {
        FPGA_ERR("Invalid token within handle");
        return FPGA_INVALID_PARAM;
    }

    // Set max power if not specified by gbs
    if (gbs_power == 0) {
        gbs_power = FPGA_GBS_MAX_POWER;
    }

    // verify gbs power limits
    if (gbs_power > FPGA_GBS_MAX_POWER) {
        FPGA_ERR("Invalid GBS power value");
        result = FPGA_NOT_SUPPORTED;
        return result;
    }

    // FPGA threshold1 = BBS Idle power + GBS power
    fpga_power = gbs_power + FPGA_BBS_IDLE_POWER;
    if (fpga_power > FPGA_MAX_POWER) {
        FPGA_ERR("Total power requirements exceed FPGA maximum");
        result = FPGA_NOT_SUPPORTED;
        return result;
    }

    // set fpga threshold 1
    snprintf_s_ss(sysfs_path, sizeof(sysfs_path), "%s/%s",  _token->sysfspath, PWRMGMT_THRESHOLD1);
    FPGA_DBG(" FPGA Threshold1             :%ld watts\n", fpga_power);

    result = sysfs_write_u64(sysfs_path, fpga_power);
    if (result != FPGA_OK) {
        FPGA_ERR("Failed to write power threshold 1");
        return result;
    }

    return result;
}


// set afu user clock
fpga_result set_afu_userclock(fpga_handle handle,
                uint64_t usrlclock_high,
                uint64_t usrlclock_low)
{
    char sysfs_path[SYSFS_PATH_MAX]    = {0};
    fpga_result result                = FPGA_OK;
    uint64_t userclk_high             = 0;
    uint64_t userclk_low              = 0;

    // Read port sysfs path
    result = get_port_sysfs(handle, sysfs_path);
    if (result != FPGA_OK) {
        FPGA_ERR("Failed to get port syfs path");
        return result;
    }

    // set user clock
    result = set_userclock(sysfs_path, usrlclock_high, usrlclock_low);
    if (result != FPGA_OK) {
        FPGA_ERR("Failed to set user clock");
        return result;
    }

    // read user clock
    result = get_userclock(sysfs_path, &userclk_high, &userclk_low);
    if (result != FPGA_OK) {
        FPGA_ERR("Failed to get user clock");
        return result;
    }

    return result;
}


// set fpga user clock
fpga_result set_userclock(const char* sysfs_path,
                    uint64_t userclk_high,
                    uint64_t userclk_low)
{
    uint64_t refClock = userclk_high;

    if (sysfs_path == NULL) {
        FPGA_ERR("Invalid Input parameters");
        return FPGA_INVALID_PARAM;
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

    // set low refclk if only one clock is avalible
    if (userclk_high == 0 && userclk_low != 0) {
        refClock = userclk_low;
    }

    if (refClock < MIN_FPGA_FREQ){
        FPGA_ERR("Invalid Input frequency");
        return FPGA_INVALID_PARAM;
    }

    // Initialize
    if (fi_RunInitz(sysfs_path) != 0) {
        FPGA_ERR("Failed to initialize user clock ");
        return FPGA_NOT_SUPPORTED;
    }

    FPGA_DBG("User clock: %ld \n", refClock);

    // set user clock
    if (fi_SetFreqs(0, refClock) != 0) {
        FPGA_ERR("Failed to set user clock frequency ");
        return FPGA_NOT_SUPPORTED;
    }

    return FPGA_OK;
}


//Get fpga user clock
fpga_result get_userclock(const char* sys_path,
                    uint64_t* userclk_high,
                    uint64_t* userclk_low)
{
    QUCPU_tFreqs userClock;

    if ((sys_path == NULL) ||
        (userclk_high == NULL) ||
        (userclk_low == NULL)) {
        FPGA_ERR("Invalid input parameters");
        return FPGA_INVALID_PARAM;
    }

    // Initialize
    if (fi_RunInitz(sys_path) != 0) {
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
    char sysfs_usrpath[SYSFS_PATH_MAX]     = {0};

    // Assume return error okay, for now
    res                           = 0;

    if (!gQUCPU_Uclock.i_InitzState) res = QUCPU_INT_UCLOCK_GETFREQS_ERR_INITZSTATE;

    if (res == 0)
    { // Read div2 and 1x user clock frequency
        // Low frequency
        gQUCPU_Uclock.u64i_cmd_reg_1 &= ~QUCPU_UI64_CMD_1_MEA_b32;

        u64i_PrtData = gQUCPU_Uclock.u64i_cmd_reg_1;
        snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_CMD1);
        sysfs_write_u64(sysfs_usrpath, u64i_PrtData);


        li_sleep_nanoseconds = USRCLK_SLEEEP_10MS;            // 10 ms for frequency counter
        fv_SleepShort(li_sleep_nanoseconds);

        snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS1);
        sysfs_read_u64(sysfs_usrpath,  &u64i_PrtData);


        ptFreqs_retFreqs->u64i_Frq_DivBy2 = (u64i_PrtData & QUCPU_UI64_STS_1_FRQ_b16t00) * 10000; // Hz
        //printf(" ptFreqs_retFreqs->u64i_Frq_ClkUsr %llx \n", ptFreqs_retFreqs->u64i_Frq_DivBy2);
        li_sleep_nanoseconds = USRCLK_SLEEEP_10MS;
        fv_SleepShort(li_sleep_nanoseconds);

        // High frequency
        gQUCPU_Uclock.u64i_cmd_reg_1 |= QUCPU_UI64_CMD_1_MEA_b32;

        u64i_PrtData = gQUCPU_Uclock.u64i_cmd_reg_1;

        snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_CMD1);
        sysfs_write_u64(sysfs_usrpath,  u64i_PrtData);

        li_sleep_nanoseconds = USRCLK_SLEEEP_10MS; // 10 ms for frequency counter
        fv_SleepShort(li_sleep_nanoseconds);

        snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS1);
        sysfs_read_u64(sysfs_usrpath,  &u64i_PrtData);
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
    char sysfs_usrpath[SYSFS_PATH_MAX];

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

        snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS0);
        sysfs_read_u64(sysfs_usrpath,  &u64i_PrtData);
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

        snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_CMD0);
        sysfs_write_u64(sysfs_usrpath,  u64i_PrtData);

        // Sleep 1 ms
        li_sleep_nanoseconds = USRCLK_SLEEEP_1MS;
        fv_SleepShort(li_sleep_nanoseconds);

        // Pushing the table
        for (u64i_MifReg = 0; u64i_MifReg<gQUCPU_Uclock.tInitz_InitialParams.u64i_NumReg; u64i_MifReg++)
        { // Write each register in the diff mif

            u64i_AvmmAdr = (uint64_t) (scu32ia3d_DiffMifTbl[(int) u64i_FrqInx][(int) u64i_MifReg][(int) u64i_Refclk]) >> 16;
            u64i_AvmmDat = (uint64_t) (scu32ia3d_DiffMifTbl[(int) u64i_FrqInx][(int) u64i_MifReg][(int) u64i_Refclk] & 0x000000ff);
            u64i_AvmmMsk = (uint64_t) (scu32ia3d_DiffMifTbl[(int) u64i_FrqInx][(int) u64i_MifReg][(int) u64i_Refclk] & 0x0000ff00) >> 8;
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

            snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS0);
            sysfs_read_u64(sysfs_usrpath,  &u64i_PrtData);

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

// get user clock
// Read the frequency for the User clock and div2 clock
int fi_GetFreqs(QUCPU_tFreqs *ptFreqs_retFreqs)
{
    // fi_GetFreqs
    // Read the frequency for the User clock and div2 clock

    uint64_t u64i_PrtData                 = 0;
    long int li_sleep_nanoseconds         = 0;
    int      res                          = 0;
    char sysfs_usrpath[SYSFS_PATH_MAX]     = {0};

    // Assume return error okay, for now
    res                           = 0;

    if (!gQUCPU_Uclock.i_InitzState) res = QUCPU_INT_UCLOCK_GETFREQS_ERR_INITZSTATE;

    if (res == 0)
    { // Read div2 and 1x user clock frequency
        // Low frequency
        gQUCPU_Uclock.u64i_cmd_reg_1 &= ~QUCPU_UI64_CMD_1_MEA_b32;

        u64i_PrtData = gQUCPU_Uclock.u64i_cmd_reg_1;
        snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_CMD1);
        sysfs_write_u64(sysfs_usrpath, u64i_PrtData);


        li_sleep_nanoseconds = USRCLK_SLEEEP_10MS;            // 10 ms for frequency counter
        fv_SleepShort(li_sleep_nanoseconds);

        snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS1);
        sysfs_read_u64(sysfs_usrpath,  &u64i_PrtData);


        ptFreqs_retFreqs->u64i_Frq_DivBy2 = (u64i_PrtData & QUCPU_UI64_STS_1_FRQ_b16t00) * 10000; // Hz
        //printf(" ptFreqs_retFreqs->u64i_Frq_ClkUsr %llx \n", ptFreqs_retFreqs->u64i_Frq_DivBy2);
        li_sleep_nanoseconds = USRCLK_SLEEEP_10MS;
        fv_SleepShort(li_sleep_nanoseconds);

        // High frequency
        gQUCPU_Uclock.u64i_cmd_reg_1 |= QUCPU_UI64_CMD_1_MEA_b32;

        u64i_PrtData = gQUCPU_Uclock.u64i_cmd_reg_1;

        snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_CMD1);
        sysfs_write_u64(sysfs_usrpath,  u64i_PrtData);

        li_sleep_nanoseconds = USRCLK_SLEEEP_10MS; // 10 ms for frequency counter
        fv_SleepShort(li_sleep_nanoseconds);

        snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS1);
        sysfs_read_u64(sysfs_usrpath,  &u64i_PrtData);
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
    char sysfs_usrpath[SYSFS_PATH_MAX];

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
    snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_CMD0);
    sysfs_write_u64(sysfs_usrpath, u64i_PrtData);

    li_sleep_nanoseconds = USRCLK_SLEEEP_1MS;
    fv_SleepShort(li_sleep_nanoseconds);

    snprintf_s_ss(sysfs_usrpath, sizeof(sysfs_usrpath), "%s/%s", gQUCPU_Uclock.sysfs_path, USER_CLOCK_STS0);

    // Poll register 0 for completion.
    // CCI is synchronous and needs only 1 read with matching sequence.

    for (u64i_SlowPoll = 0; u64i_SlowPoll<100; ++u64i_SlowPoll)  // 100 ms
    { // Poll 0, slow outer loop with 1 ms sleep
        for (u64i_FastPoll = 0; u64i_FastPoll<100; ++u64i_FastPoll)
        {
            // Poll 0, fast inner loop with no sleep
            sysfs_read_u64(sysfs_usrpath, &u64i_DataX);

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
