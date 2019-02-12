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

#ifndef __FPGA_BITSTREAM_INT_H__
#define __FPGA_BITSTREAM_INT_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <opae/types.h>

#include "log_sys.h"
#include "safe_string/safe_string.h"
#include "opae/utils.h"
#include "types_sys.h"

#define GUID_LEN       36
#define AFU_NAME_LEN   512
#define SYSFS_PATH_MAX 256
#define DEV_PATH_MAX   256

// Max power values
#define FPGA_BBS_IDLE_POWER   30            // watts
#define FPGA_MAX_POWER        90            // watts
#define FPGA_GBS_MAX_POWER    60            // watts
#define FPGA_THRESHOLD2(x)    ((x*10)/100)  // threshold1 + 10%
#define PWRMGMT_THRESHOLD1    "power_mgmt/threshold1"
#define  MAX_FPGA_FREQ        1200
#define  MIN_FPGA_FREQ          100
#define USER_CLOCK_CMD0 "userclk_freqcmd"

#define QUCPU_INT_UCLOCK_RUNINITZ_ERR_FPLL_ID_ILLEGAL            ((int)   2)                       // Check PLL:   identifier  illegal
#define QUCPU_UI64_AVMM_FPLL_IPI_200                             ((uint64_t)0x200LLU)              // IP identifer
#define QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RFDUAL                  ((uint64_t)0x05LLU)               // Expected ID, RF=100 MHz & RF=322.265625 MHz
#define QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RF100M                  ((uint64_t)0x06LLU)               // Expected ID, RF=100 MHz
#define QUCPU_UI64_AVMM_FPLL_IPI_200_IDI_RF322M                  ((uint64_t)0x07LLU)               // Expected ID,              RF=322.265625 MHz
#define QUCPU_UI64_CMD_0_MRN_b52                                 ((uint64_t)0x0010000000000000LLU) // mmmach machine reset_n
#define QUCPU_UI64_CMD_0_PRS_b56                                 ((uint64_t)0x0100000000000000LLU) // fPLL management reset
#define QUCPU_INT_UCLOCK_RUNINITZ_ERR_VER                        ((int)   1)                       // Wrong Uclock version error
#define QUCPU_UI64_STS_1_VER_version                             ((uint64_t)0x03LLU)               // Expected version number
#define QUCPU_UI64_STS_1_VER_b63t60                              ((uint64_t)0xf000000000000000LLU) // frequency in 10 kHz units
#define  USER_CLOCK_STS1       "userclk_freqcntrsts"
#define QUCPU_INT_UCLOCK_AVMMRMWV_ERR_VERIFY                     ((int)   4)                       // AvmmRMW:     verify failure
#define QUCPU_INT_NUMFRQ_INTG_END ((int)    2)
#define QUCPU_INT_NUMFRQ_FRAC_BEG ((int)  100)
#define QUCPU_INT_NUMFRQ_FRAC_END ((int) 1200)
#define QUCPU_INT_NUMFRQ          ((int) 1201)
#define QUCPU_INT_NUMREG          ((int)   14)
#define QUCPU_INT_NUMRCK          ((int)    2)
#define QUCPU_INT_UCLOCK_AVMMRWCOM_ERR_TIMEOUT                   ((int)   5)                       // AvmmRWcom:   timeout
#define QUCPU_INT_UCLOCK_SETFREQS_ERR_PLL_LOCK_TO                ((int)  15)                       // SetFreqs:    timed out waiting for lock
#define QUCPU_UI64_STS_0_LCK_b60                                 ((uint64_t)0x1000000000000000LLU) // fPLL locked
#define  USRCLK_SLEEEP_1MS           1000000
#define  USRCLK_SLEEEP_10MS          10000000
#define QUCPU_INT_UCLOCK_WAITCALDONE_ERR_BSY_TO                  ((int)   3)                       // WaitCalDone: timeout
#define QUCPU_UI64_STS_0_BSY_b61                                 ((uint64_t)0x2000000000000000LLU) // fPLL cal busy
#define  USER_CLOCK_STS0       "userclk_freqsts"
#define QUCPU_INT_UCLOCK_SETFREQS_ERR_INITZSTATE                 ((int)   7)                       // SetFreqs:    missing initialization
#define QUCPU_INT_UCLOCK_SETFREQS_ERR_REFCLK_100M_MISSING        ((int)   9)                       // SetFreqs:    100 MHz refclk missing from RTL
#define QUCPU_INT_UCLOCK_SETFREQS_ERR_REFCLK_322M_MISSING        ((int)  10)                       // SetFreqs:    322 MHz refclk missing from RTL
#define QUCPU_INT_UCLOCK_SETFREQS_ERR_REFCLK_ILLEGAL             ((int)   8)                       // SetFreqs:    illegal refclk index
#define QUCPU_INT_UCLOCK_SETFREQS_ERR_FINDEX_OVERRANGE           ((int)  11)                       // SetFreqs:    f-index > END
#define QUCPU_INT_UCLOCK_SETFREQS_ERR_FINDEX_INTG_RANGE_BAD      ((int)  12)                       // SetFreqs:    integer-PLL mode f-index invalid
#define QUCPU_INT_UCLOCK_SETFREQS_ERR_FINDEX_INTG_NEEDS_322M     ((int)  13)                       // SetFreqs:    integer-PLL mode needs 322 MHz ref
#define QUCPU_INT_UCLOCK_SETFREQS_ERR_PLL_NO_UNLOCK              ((int)  14)                       // SetFreqs:    PLL would not unlock
#define QUCPU_INT_UCLOCK_BUG_SLEEP_SHORT                         ((int)   1)                       // Bug in fv_SleepShort
#define QUCPU_INT_UCLOCK_GETFREQS_ERR_INITZSTATE                 ((int)   6)                       // GetFreqs:    missing initialization
#define QUCPU_UI64_CMD_1_MEA_b32                                 ((uint64_t)0x0000000100000000LLU) // 1: measure user clock; 0: measure 2nd clock, div2
#define  USER_CLOCK_CMD1       "userclk_freqcntrcmd"


// GBS Metadata format /json
struct gbs_metadata {

	double version;                             // version

	struct afu_image_content {
		uint64_t magic_num;                 // Magic number
		char interface_uuid[GUID_LEN + 1];  // Interface id
		int clock_frequency_high;            // user clock frequency hi
		int clock_frequency_low;             // user clock frequency low
		int power;                           // power

		struct afu_clusters_content {
			char name[AFU_NAME_LEN];     // AFU Name
			int  total_contexts;         // total contexts
			char afu_uuid[GUID_LEN + 1]; // afu guid
		} afu_clusters;

	} afu_image;

};

// Structures and Types
struct QUCPU_sInitz {
    uint64_t u64i_Version;          // Version of clock user
    uint64_t u64i_PLL_ID;           // PLL ID
    uint64_t u64i_NumFrq_Intg_End;  // Integer/exact fPLL indices [0  ..End]
    uint64_t u64i_NumFrq_Frac_Beg;  // Fractional    fPLL indices [Beg..End]
    uint64_t u64i_NumFrq_Frac_End;
    uint64_t u64i_NumFrq;           // Array frequency  # of elements
    uint64_t u64i_NumReg;           // Array registers  # of elements
    uint64_t u64i_NumRck;           // Array ref-clocks # of elements
};

typedef struct QUCPU_sInitz QUCPU_tInitz;

struct  QUCPU_Uclock
{
    char          sysfs_path[SYSFS_PATH_MAX];          // Port sysfs path
    int           i_Bug_First;                         // First bug
    int           i_Bug_Last;                          // Lasr bug
    int           i_InitzState;                        // Initialization state
    QUCPU_tInitz  tInitz_InitialParams;                // Initialization parameters
    uint64_t     u64i_cmd_reg_0;                       // Command register 0
    uint64_t     u64i_cmd_reg_1;                       // Command register 1
    uint64_t     u64i_AVMM_seq ;                       // Sequence ID
};

typedef struct  QUCPU_Uclock gQUCPU_Uclock;

struct QUCPU_sFreqs {
    uint64_t u64i_Frq_ClkUsr;       // Read user clock frequency (Hz)
    uint64_t u64i_Frq_DivBy2;       // Read user clock frequency (Hz) divided-by-2 output
};


typedef struct QUCPU_sFreqs QUCPU_tFreqs;


/**
 * Check the validity of GUID
 *
 *Extracts the 128 bit guid from passed bitstream
 *converts it to fpga_guid type anc checks it against
 *expected value
 *
 *
 * @param[in] bitstream   Pointer to the bitstream
 * @returns               FPGA_OK on success
 */
fpga_result check_bitstream_guid(const uint8_t *bitstream);

/**
 * Get total length of bitstream header
 *
 * Returns the total length of header which is
 * GUID + size of variable describing length of metadata + length of metadata
 *
 *
 * @param[in] bitstream   Pointer to the bitstream
 * @returns               int value of length, -1 on failure
 */
int get_bitstream_header_len(const uint8_t *bitstream);

/**
 * Get total length of json metadata in bitstream
 *
 * Returns the length of the json metadata from the
 * bitstream which is represented by a uint32 after the
 * GUID
 *
 *
 * @param[in] bitstream   Pointer to the bitstream
 * @returns               int value of length, -1 on failure
 */
int32_t get_bitstream_json_len(const uint8_t *bitstream);


/**
 * Check bitstream magic no and interface id
 *
 * Checks the bitstream magic no and interface id
 * with expected values
 *
 * @param[in] handle			Handle to previously opened FPGA object
 * @param[in] bitstream_magic_no	magic no. to be checked
 * @param[in] ifid_l			lower 64 bits of interface id
 * @param[in] ifid_h			higher 64 bits of interface id
 * @returns				FPGA_OK on success
 */
fpga_result check_interface_id(fpga_handle handle, uint32_t bitstream_magic_no,
				uint64_t ifid_l, uint64_t ifid_h);

/**
 * Check if the JSON metadata is valid
 *
 * Reads the bitstream magic no and interface
 * id values from the metadata and compares them
 * with expected values
 *
 * @param[in] handle	  Handle to previously opened FPGA object
 * @param[in] bitstream   Pointer to the bitstream
 * @returns		  FPGA_OK on success
 */
fpga_result validate_bitstream_metadata(fpga_handle handle,
					const uint8_t *bitstream);

/**
 * Reads GBS metadata
 *
 * Parses GBS JSON metadata.
 *
 * @param[in] bitstream    Pointer to the bitstream
 * @param[in] gbs_metadata Pointer to gbs metadata struct
 * @returns                FPGA_OK on success
 */
fpga_result read_gbs_metadata(const uint8_t *bitstream,
			      struct gbs_metadata *gbs_metadata);

/**
* @brief Sets FPGA power threshold values
*
* @param fpga handle
* @param gbs_power gbs power value
*
* @return error code
*/
fpga_result set_fpga_pwr_threshold(fpga_handle handle,
                uint64_t gbs_power);

/**
* @brief set afu user clock
*
* @param handle
* @param usrlclock_high user clock low frequency
* @param usrlclock_low user clock high frequency
*
* @return error code
*/
fpga_result set_afu_userclock(fpga_handle handle,
                uint64_t usrlclock_high,
                uint64_t usrlclock_low);

fpga_result sysfs_write_u64(const char *path, uint64_t u);
fpga_result get_port_sysfs(fpga_handle handle, char *sysfs_port);

/**
* @brief set fpga user clock
*
* @param sysfs_path  port sysfs path
* @parm  high user clock
* @parm  low user clock
*
* @return error code
*/
fpga_result set_userclock(const char* sysfs_path, uint64_t userclk_high, uint64_t userclk_low);


/**
* @brief Get fpga user clock
*
* @param sysfs_path  port sysfs path
* @parm  pointer to  high user clock
* @parm  pointer to  low user clock
*
* @return error code
*/
fpga_result get_userclock(const char* sysfs_path, uint64_t *userclk_high, uint64_t *userclk_low);

int fi_RunInitz(const char* sysfs_path);

fpga_result sysfs_read_u64(const char *path, uint64_t *u);

int fi_WaitCalDone(void);

int fi_AvmmRead(uint64_t u64i_AvmmAdr, uint64_t *pu64i_ReadData);

int fi_SetFreqs(uint64_t u64i_Refclk, uint64_t u64i_FrqInx);

void fv_SleepShort(long int li_sleep_nanoseconds);

int fi_GetFreqs(QUCPU_tFreqs *ptFreqs_retFreqs);

int fi_AvmmRWcom(int i_CmdWrite, uint64_t u64i_AvmmAdr, uint64_t u64i_WriteData, uint64_t *pu64i_ReadData);

int fi_AvmmReadModifyWriteVerify(uint64_t u64i_AvmmAdr,
                uint64_t u64i_AvmmDat,
                uint64_t u64i_AvmmMsk);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __COMMON_SYS_H__
