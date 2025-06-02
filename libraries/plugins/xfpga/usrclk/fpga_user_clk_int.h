// Copyright(c) 2017-2022, Intel Corporation
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

#ifndef FPGA_USER_CLK_INT_H_
#define FPGA_USER_CLK_INT_H_

#include "fpga_user_clk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  USRCLK_FEATURE_ID            0x14

#define  MAX_FPGA_FREQ                1200
#define  MIN_FPGA_FREQ                25

/*
 * USER CLK CSR register definitions
 */
 /* Field definitions for both USERCLK_FREQ_CMD0 and USERCLK_FREQ_STS0 */
#define IOPLL_FREQ_CMD0               0x8
#define IOPLL_DATA                    GENMASK_ULL(31, 0)
#define IOPLL_ADDR                    GENMASK_ULL(41, 32)
#define IOPLL_WRITE                   BIT_ULL(44)
#define IOPLL_SEQ                     GENMASK_ULL(49, 48)
#define IOPLL_MGMT_DATA_MASK          BIT_ULL(51)
#define IOPLL_AVMM_RESET_N            BIT_ULL(52)
#define IOPLL_MGMT_RESET              BIT_ULL(56)
#define IOPLL_RESET                   BIT_ULL(57)

#define IOPLL_FREQ_CMD1               0x10
/* Field definitions for both USERCLKL_FREQ_CMD1 and USERCLK_FREQ_STS1 */
#define IOPLL_CLK_MEASURE             BIT_ULL(32)
#define IOPLL_FREQ_STS0               0x18
#define IOPLL_LOCKED                  BIT_ULL(60)
#define IOPLL_AVMM_ERROR              BIT_ULL(63)

#define IOPLL_FREQ_STS1               0x20
#define IOPLL_FREQUENCY               GENMASK_ULL(16, 0)
#define IOPLL_REF_FREQ                GENMASK_ULL(50, 33)
#define IOPLL_STS1_FPGA_FAMILY        GENMASK_ULL(59, 54)
#define IOPLL_VERSION                 GENMASK_ULL(63, 60)

#define IOPLL_CAL_DELAY_US            1000
#define IOPLL_WRITE_POLL_INVL_US      10 /* Write poll interval */
#define IOPLL_WRITE_POLL_TIMEOUT_US   1000000 /* Write poll timeout */

#define  IOPLL_MAX_FREQ             600
#define  IOPLL_MIN_FREQ             10

// Revision stored in the user clock DFH indicates the CSR protocol
#define  TYPE2_USRCLK_REV           2
#define  AGILEX_USRCLK_REV          1

#define  IOPLL_AGILEX_MAX_FREQ      800
#define  IOPLL_AGILEX_MIN_FREQ      10

#define CFG_PLL_LOW                   GENMASK_ULL(7, 0)
#define CFG_PLL_HIGH                  GENMASK_ULL(15, 8)
#define CFG_PLL_BYPASS_EN             BIT_ULL(16)
#define CFG_PLL_EVEN_DUTY_EN          BIT_ULL(17)

struct iopll_config {
	unsigned int pll_freq_khz;
	unsigned int pll_m;
	unsigned int pll_n;
	unsigned int pll_c1;
	unsigned int pll_c0;
	unsigned int pll_lf;
	unsigned int pll_cp;
	unsigned int pll_rc;
};


// Configuration tables are initialized in fpga_user_clk_freq.h, included
// only by fpga_user_clk.c.

//   S10 reference frequency: 100MHz
extern const struct iopll_config iopll_freq_config[];
//   Agilex reference frequency: 100MHz
extern const struct iopll_config iopll_agilex_freq_config[];


fpga_result usrclk_reset(uint8_t *uio_ptr);
int usrclk_using_iopll(char *sysfs_usrpath, const char *sysfs_path);

/**
 * @brief open UIO handle to fpga user clock manager
 *
 * @param sysfs_path  port sysfs path
 * @param feature_id
 * @param uio         returned OPAE uio struct
 * @param uio_ptr     returned handle to open UIO device
 *
 * @return error code
 */
fpga_result get_usrclk_uio(const char *sysfs_path,
			  uint32_t feature_id,
			  struct opae_uio *uio,
			  uint8_t **uio_ptr);

/**
 * @brief set fpga user clock for Agilex 7 and Stratix 10
 *
 * @param sysfs_path  port sysfs path
 * @param revision    DFH revision number
 * @param high user clock
 * @param low user clock
 *
 * @return error code
 */
fpga_result set_userclock_type1(const char *sysfs_path, uint64_t revision,
			  uint64_t userclk_high, uint64_t userclk_low);

/**
 * @brief set fpga user clock for read-modify-write pipeline (e.g. Agilex 5)
 *
 * @param sysfs_path  port sysfs path
 * @param high user clock
 * @param low user clock
 *
 * @return error code
 */
fpga_result set_userclock_type2(const char *sysfs_path,
			  uint64_t userclk_high, uint64_t userclk_low);

#ifdef __cplusplus
}
#endif

#endif // end  FPGA_USER_CLK_INT_H_
