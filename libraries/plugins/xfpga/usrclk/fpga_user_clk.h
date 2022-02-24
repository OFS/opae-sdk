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

#ifndef FPGA_USER_CLK_H_
#define FPGA_USER_CLK_H_

#include <stdint.h>
#include "common_int.h"

#ifdef __cplusplus
extern "C" {
#endif

#define  BITS_PER_LONG_LONG 64
#define  UL(x) ((unsigned long) (x))
#define  ULL(x) ((unsigned long long) (x))
#define  BIT_ULL(nr) (ULL(1) << (nr))
#define  BIT(nr) (UL(1) << (nr))
#define  GENMASK_ULL(h, l) \
	(((~0ULL) << (l)) & (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))

#define __bf_shf(x) (__builtin_ffsll(x) - 1)

#define  FIELD_PREP(_mask, _val)                                    \
	({                                                             \
		((__typeof__(_mask))(_val) << __bf_shf(_mask)) & (_mask);  \
	})

#define  FIELD_GET(_mask, _reg)                                     \
	({                                                             \
		(__typeof__(_mask))(((_reg) & (_mask)) >> __bf_shf(_mask));\
	})

struct pll_config {
	unsigned int pll_freq_khz;
	unsigned int pll_m;
	unsigned int pll_n;
	unsigned int pll_c1;
	unsigned int pll_c0;
	unsigned int pll_lf;
	unsigned int pll_cp;
	unsigned int pll_rc;
};

fpga_result get_userclk_revision(const char *sysfs_path,
		uint64_t *revision);

/**
 * @brief Get fpga user clock
 *
 * @param sysfs_path  port sysfs path
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
 * @param sysfs_path  port sysfs path
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

#endif // end  FPGA_USER_CLK_H_
