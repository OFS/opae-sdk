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

// Agilex 7 and Stratix 10 user clock frequency management. Named type1
// because the user clock feature header revision is 1 for Agilex 7.
// Legacy support for Stratix 10 (revision 0) is maintained here as
// well.
//
// The public entry point is set_userclock_type1(), which should
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

/*
 * Control and status registers for the IOPLL
 * https://www.altera.com/en_US/pdfs/literature/hb/stratix-10/ug-s10-clkpll.pdf
 * Section 7.2
 */

#define CFG_PLL_LOW                   GENMASK_ULL(7, 0)
#define CFG_PLL_HIGH                  GENMASK_ULL(15, 8)
#define CFG_PLL_BYPASS_EN             BIT_ULL(16)
#define CFG_PLL_EVEN_DUTY_EN          BIT_ULL(17)

#define PLL_EVEN_DUTY_EN_SHIFT        7

#define PLL_N_HIGH_ADDR               0x100
#define PLL_N_BYPASS_EN_ADDR          0x101
#define PLL_N_EVEN_DUTY_EN_ADDR       0x101
#define PLL_N_LOW_ADDR                0x102

#define PLL_M_HIGH_ADDR               0x104
#define PLL_M_BYPASS_EN_ADDR          0x105
#define PLL_M_EVEN_DUTY_EN_ADDR       0x106
#define PLL_M_LOW_ADDR                0x107

#define PLL_C0_HIGH_ADDR              0x11b
#define PLL_C0_BYPASS_EN_ADDR         0x11c
#define PLL_C0_EVEN_DUTY_EN_ADDR      0x11d
#define PLL_C0_LOW_ADDR               0x11e

#define PLL_C1_HIGH_ADDR              0x11f
#define PLL_C1_BYPASS_EN_ADDR         0x120
#define PLL_C1_EVEN_DUTY_EN_ADDR      0x121
#define PLL_C1_LOW_ADDR               0x122

#define CFG_PLL_CP1                   GENMASK_ULL(2, 0)
#define PLL_CP1_ADDR                  0x101
#define PLL_CP1_SHIFT                 4

#define CFG_PLL_LF                    GENMASK_ULL(13, 6)
#define PLL_LF_ADDR                   0x10a
#define PLL_LF_SHIFT                  3

#define CFG_PLL_CP2                   GENMASK_ULL(5, 3)
#define PLL_CP2_ADDR                  0x10d
#define PLL_CP2_SHIFT                 5

#define CFG_PLL_RC                    GENMASK_ULL(1, 0)
#define PLL_RC_SHIFT                  1

#define PLL_REQUEST_CAL_ADDR          0x149
#define PLL_REQUEST_CALIBRATION       BIT(6)

#define PLL_ENABLE_CAL_ADDR           0x14a
#define PLL_ENABLE_CALIBRATION        0x03


STATIC fpga_result usrclk_write(uint8_t *uio_ptr, uint16_t address,
	uint32_t data, uint8_t seq)
{
	fpga_result res   = FPGA_OK;
	uint64_t v        = 0;
	uint32_t timeout  = IOPLL_WRITE_POLL_TIMEOUT_US;

	if (uio_ptr == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}
	seq &= 0x3;

	v = FIELD_PREP(IOPLL_DATA, data);
	v |= FIELD_PREP(IOPLL_ADDR, address);
	v |= IOPLL_WRITE;
	v |= FIELD_PREP(IOPLL_SEQ, seq);
	v |= IOPLL_AVMM_RESET_N;
	*((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_CMD0)) = v;

	v = *((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_STS0));

	while (!(FIELD_GET(IOPLL_SEQ, v) == seq)) {
		v = *((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_STS0));
		usleep(IOPLL_WRITE_POLL_INVL_US);
		if (--timeout == 0) {
			OPAE_ERR("Timeout on IOPLL write");
			res = FPGA_EXCEPTION;
			break;
		}
	}

	return res;
}

STATIC fpga_result usrclk_read(uint8_t *uio_ptr, uint16_t address,
	uint32_t *data, uint8_t seq)
{
	uint64_t v       = 0;
	uint32_t timeout = IOPLL_WRITE_POLL_TIMEOUT_US;

	if (uio_ptr == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}
	seq &= 0x3;

	v = FIELD_PREP(IOPLL_ADDR, address);
	v |= FIELD_PREP(IOPLL_SEQ, seq);
	v |= IOPLL_AVMM_RESET_N;
	*((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_CMD0)) = v;

	v = *((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_STS0));

	while (!(FIELD_GET(IOPLL_SEQ, v) == seq)) {
		v = *((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_STS0));
		usleep(IOPLL_WRITE_POLL_INVL_US);
		if (--timeout == 0) {
			OPAE_ERR("Timeout on IOPLL write");
			return FPGA_EXCEPTION;
		}
	}

	*data = FIELD_GET(IOPLL_DATA, v);
	return FPGA_OK;
}

STATIC fpga_result usrclk_update_bits(uint8_t *uio_ptr, uint16_t address,
	uint32_t mask, uint32_t bits, uint8_t *seq)
{
	uint32_t data   = 0;
	fpga_result res = FPGA_OK;

	if (uio_ptr == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = usrclk_read(uio_ptr, address, &data, (*seq)++);
	if (res)
		return res;

	data &= ~mask;
	data |= (bits & mask);

	return usrclk_write(uio_ptr, PLL_REQUEST_CAL_ADDR,
		data | PLL_REQUEST_CALIBRATION, (*seq)++);

	return res;
}

STATIC fpga_result usrclk_m_write(uint8_t *uio_ptr,
	uint32_t cfg_pll_m, uint8_t *seq)
{
	uint32_t high, low, bypass_en, even_duty_en = 0;
	fpga_result res                             = FPGA_OK;

	if (uio_ptr == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	high = FIELD_GET(CFG_PLL_HIGH, cfg_pll_m);
	res = usrclk_write(uio_ptr, PLL_M_HIGH_ADDR, high, (*seq)++);
	if (res)
		return res;

	low = FIELD_GET(CFG_PLL_LOW, cfg_pll_m);
	res = usrclk_write(uio_ptr, PLL_M_LOW_ADDR, low, (*seq)++);
	if (res)
		return res;

	bypass_en = FIELD_GET(CFG_PLL_BYPASS_EN, cfg_pll_m);
	res = usrclk_write(uio_ptr, PLL_M_BYPASS_EN_ADDR, bypass_en, (*seq)++);
	if (res)
		return res;

	even_duty_en = FIELD_GET(CFG_PLL_EVEN_DUTY_EN, cfg_pll_m) <<
		PLL_EVEN_DUTY_EN_SHIFT;
	return usrclk_write(uio_ptr, PLL_M_EVEN_DUTY_EN_ADDR,
		even_duty_en, (*seq)++);
}

STATIC fpga_result usrclk_n_write(uint8_t *uio_ptr, uint32_t cfg_pll_n,
	uint32_t cfg_pll_cp, uint8_t *seq)
{
	uint32_t high, low, bypass_en, even_duty_en, cp1 = 0;
	fpga_result res                                  = FPGA_OK;

	if (uio_ptr == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	high = FIELD_GET(CFG_PLL_HIGH, cfg_pll_n);
	res = usrclk_write(uio_ptr, PLL_N_HIGH_ADDR, high, (*seq)++);
	if (res)
		return res;

	low = FIELD_GET(CFG_PLL_LOW, cfg_pll_n);
	res = usrclk_write(uio_ptr, PLL_N_LOW_ADDR, low, (*seq)++);
	if (res)
		return res;

	even_duty_en = FIELD_GET(CFG_PLL_EVEN_DUTY_EN, cfg_pll_n) <<
		PLL_EVEN_DUTY_EN_SHIFT;
	cp1 = FIELD_GET(CFG_PLL_CP1, cfg_pll_cp) << PLL_CP1_SHIFT;
	bypass_en = FIELD_GET(CFG_PLL_BYPASS_EN, cfg_pll_n);
	return usrclk_write(uio_ptr, PLL_N_BYPASS_EN_ADDR,
		even_duty_en | cp1 | bypass_en, (*seq)++);
}

STATIC fpga_result usrclk_c0_write(uint8_t *uio_ptr,
	uint32_t cfg_pll_c0, uint8_t *seq)
{
	uint32_t high, low, bypass_en, even_duty_en = 0;
	fpga_result res                             = FPGA_OK;

	if (uio_ptr == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	high = FIELD_GET(CFG_PLL_HIGH, cfg_pll_c0);
	res = usrclk_write(uio_ptr, PLL_C0_HIGH_ADDR, high, (*seq)++);
	if (res)
		return res;

	low = FIELD_GET(CFG_PLL_LOW, cfg_pll_c0);
	res = usrclk_write(uio_ptr, PLL_C0_LOW_ADDR, low, (*seq)++);
	if (res)
		return res;

	bypass_en = FIELD_GET(CFG_PLL_BYPASS_EN, cfg_pll_c0);
	res = usrclk_write(uio_ptr, PLL_C0_BYPASS_EN_ADDR, bypass_en, (*seq)++);
	if (res)
		return res;

	even_duty_en = FIELD_GET(CFG_PLL_EVEN_DUTY_EN, cfg_pll_c0) <<
		PLL_EVEN_DUTY_EN_SHIFT;
	return usrclk_write(uio_ptr, PLL_C0_EVEN_DUTY_EN_ADDR,
		even_duty_en, (*seq)++);
}

STATIC fpga_result usrclk_c1_write(uint8_t *uio_ptr,
	uint32_t cfg_pll_c1, uint8_t *seq)
{
	uint32_t high, low, bypass_en, even_duty_en = 0;
	fpga_result res                             = FPGA_OK;

	if ((uio_ptr == NULL) ||
		(seq == NULL)) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	high = FIELD_GET(CFG_PLL_HIGH, cfg_pll_c1);
	res = usrclk_write(uio_ptr, PLL_C1_HIGH_ADDR, high, (*seq)++);
	if (res)
		return res;

	low = FIELD_GET(CFG_PLL_LOW, cfg_pll_c1);
	res = usrclk_write(uio_ptr, PLL_C1_LOW_ADDR, low, (*seq)++);
	if (res)
		return res;

	bypass_en = FIELD_GET(CFG_PLL_BYPASS_EN, cfg_pll_c1);
	res = usrclk_write(uio_ptr, PLL_C1_BYPASS_EN_ADDR, bypass_en, (*seq)++);
	if (res)
		return res;

	even_duty_en = FIELD_GET(CFG_PLL_EVEN_DUTY_EN, cfg_pll_c1) <<
		PLL_EVEN_DUTY_EN_SHIFT;
	return usrclk_write(uio_ptr, PLL_C1_EVEN_DUTY_EN_ADDR,
		even_duty_en, (*seq)++);
}

STATIC fpga_result usrclk_set_freq(uint8_t *uio_ptr,
	struct pll_config *c, uint8_t *seq)
{
	uint32_t cp2, lf, rc  = 0;
	fpga_result res       = FPGA_OK;

	if ((uio_ptr == NULL) ||
		(seq == NULL)) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	res = usrclk_m_write(uio_ptr, c->pll_m, seq);
	if (res)
		return res;

	res = usrclk_n_write(uio_ptr, c->pll_n, c->pll_cp, seq);
	if (res)
		return res;

	res = usrclk_c0_write(uio_ptr, c->pll_c0, seq);
	if (res)
		return res;

	res = usrclk_c1_write(uio_ptr, c->pll_c1, seq);
	if (res)
		return res;

	cp2 = FIELD_GET(CFG_PLL_CP2, c->pll_cp) << PLL_CP2_SHIFT;
	res = usrclk_write(uio_ptr, PLL_CP2_ADDR, cp2, (*seq)++);
	if (res)
		return res;

	lf = FIELD_GET(CFG_PLL_LF, c->pll_lf) << PLL_LF_SHIFT;
	rc = FIELD_GET(CFG_PLL_RC, c->pll_rc) << PLL_RC_SHIFT;
	return usrclk_write(uio_ptr, PLL_LF_ADDR, lf | rc, (*seq)++);
}

STATIC fpga_result usrclk_calibrate(uint8_t *uio_ptr, uint8_t *seq)
{
	fpga_result res = FPGA_OK;

	if ((uio_ptr == NULL) ||
		(seq == NULL)) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	/* Request IOPLL Calibration */
	res = usrclk_update_bits(uio_ptr, PLL_REQUEST_CAL_ADDR,
		PLL_REQUEST_CALIBRATION,
		PLL_REQUEST_CALIBRATION, seq);
	if (res)
		return res;

	/* Enable calibration interface */
	res = usrclk_write(uio_ptr, PLL_ENABLE_CAL_ADDR, PLL_ENABLE_CALIBRATION,
		(*seq)++);
	usleep(IOPLL_CAL_DELAY_US);
	return res;
}

// set fpga user clock
fpga_result set_userclock_type1(const char *sysfs_path,
	uint64_t revision,
	uint64_t userclk_high,
	uint64_t userclk_low)
{
	char sysfs_usrpath[SYSFS_PATH_MAX] = { 0 };
	int fd, ret                        = 0;
	char *bufp                         = NULL;
	ssize_t cnt                        = 0;
	uint8_t seq                        = 1;
	uint8_t *uio_ptr                   = NULL;
	fpga_result result                 = FPGA_OK;
	ssize_t bytes_written              = 0;
	struct opae_uio uio;
	uint64_t v                         = 0;

	unsigned int iopll_max_freq        = IOPLL_MAX_FREQ;
	unsigned int iopll_min_freq        = IOPLL_MIN_FREQ;
	unsigned int slow_freq             = MIN_FPGA_FREQ;

	memset(&uio, 0, sizeof(uio));

	if (sysfs_path == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	if (userclk_high < MIN_FPGA_FREQ) {
		OPAE_ERR("Invalid Input frequency");
		return FPGA_INVALID_PARAM;
	}

	// Agilex user clock DFH revision 1
	// S10 & A10 user clock DFH revision 0
	if (revision == AGILEX_USRCLK_REV) {
		iopll_max_freq = IOPLL_AGILEX_MAX_FREQ;
		iopll_min_freq = IOPLL_AGILEX_MIN_FREQ;

		// Enforce 1x clock within valid range
		if ((userclk_low > iopll_max_freq) ||
			(userclk_low < iopll_min_freq)) {
			OPAE_ERR("Invalid Input frequency");
			return FPGA_INVALID_PARAM;
		}

		bufp = (char *)&iopll_agilex_freq_config[userclk_low];
	} else if (revision == 0) {
		// S10 & A10 user clock
		// Enforce 1x clock within valid range
		if ((userclk_low > iopll_max_freq) ||
			(userclk_low < iopll_min_freq)) {
			OPAE_ERR("Invalid Input frequency");
			return FPGA_INVALID_PARAM;
		}
		bufp = (char *)&iopll_freq_config[userclk_low];
	} else {
		OPAE_ERR("Unsupported FPGA device revision: %d", revision);
		return FPGA_NOT_SUPPORTED;
	}

	// Transitions from a currently configured very high frequency
	// or very low frequency to another extreme frequency sometimes
	// fails to stabilize. Start by forcing the fast clock to half
	// speed.
	slow_freq = iopll_max_freq / 4;
	if (userclk_low != slow_freq) {
		result = set_userclock(sysfs_path, slow_freq * 2, slow_freq);
	}

	ret = usrclk_using_iopll(sysfs_usrpath, sysfs_path);
	if (ret == FPGA_OK) {

		fd = opae_open(sysfs_usrpath, O_WRONLY);
		if (fd < 0) {
			OPAE_MSG("open(%s) failed: %s",
				sysfs_usrpath, strerror(errno));
			return FPGA_NOT_FOUND;
		}
		cnt = sizeof(struct iopll_config);

		bytes_written = eintr_write(fd, bufp, cnt);
		if (bytes_written != cnt) {
			OPAE_ERR("Failed to write: %s", strerror(errno));
			opae_close(fd);
			return FPGA_EXCEPTION;
		}
		opae_close(fd);

		return FPGA_OK;
	} else if (ret == FPGA_NO_ACCESS) {
		return FPGA_NO_ACCESS;
	}

	struct pll_config *iopll_config = (struct pll_config *)bufp;
	if ((iopll_config->pll_freq_khz > iopll_max_freq * 1000) ||
		(iopll_config->pll_freq_khz < iopll_min_freq * 1000))
		return FPGA_EXCEPTION;

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

	result = usrclk_set_freq(uio_ptr, iopll_config, &seq);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to set user clock");
		goto uio_close;
	}

	result = usrclk_reset(uio_ptr);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to reset user clock");
		goto uio_close;
	}

	result = usrclk_calibrate(uio_ptr, &seq);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to calibrate user clock");
		goto uio_close;
	}

uio_close:
	opae_uio_close(&uio);
	return result;
}
