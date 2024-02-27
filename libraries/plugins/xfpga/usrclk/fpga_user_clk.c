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
#include "fpga_user_clk_freq.h"
#include "mock/opae_std.h"

// user clock sysfs
#define  IOPLL_CLOCK_FREQ             "dfl*/userclk/frequency"
#define  IOPLL_REVISION               "dfl*/feature_rev"
#define  MAX_FPGA_FREQ                1200
#define  MIN_FPGA_FREQ                25
#define  AGILEX_USRCLK_REV            1

/*
 * USER CLK CSR register definitions
 */
 /* Field definitions for both USERCLK_FREQ_CMD0 and USERCLK_FREQ_STS0 */
#define IOPLL_FREQ_CMD0               0x8
#define IOPLL_DATA                    GENMASK_ULL(31, 0)
#define IOPLL_ADDR                    GENMASK_ULL(41, 32)
#define IOPLL_WRITE                   BIT_ULL(44)
#define IOPLL_SEQ                     GENMASK_ULL(49, 48)
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
#define IOPLL_VERSION                 GENMASK_ULL(63, 60)

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

#define IOPLL_MEASURE_LOW             0
#define IOPLL_MEASURE_HIGH            1
#define IOPLL_MEASURE_DELAY_US        8000
#define IOPLL_RESET_DELAY_US          1000
#define IOPLL_CAL_DELAY_US            1000

#define IOPLL_WRITE_POLL_INVL_US      10 /* Write poll interval */
#define IOPLL_WRITE_POLL_TIMEOUT_US   1000000 /* Write poll timeout */

#define USRCLK_FEATURE_ID             0x14
const char USRCLK_GUID[]              = "45AC5052C481412582380B8FF6C2F943";


 // DFHv0
struct dfh {
	union {
		uint64_t csr;
		struct {
			uint64_t id : 12;
			uint64_t feature_rev : 4;
			uint64_t next : 24;
			uint64_t eol : 1;
			uint64_t reserved41 : 7;
			uint64_t feature_minor_rev : 4;
			uint64_t dfh_version : 8;
			uint64_t type : 4;
		};
	};
	char guid_l[64];
	char guid_h[64];
};

static int using_iopll(char *sysfs_usrpath, const char *sysfs_path);

bool is_file_empty(char* file_path) {
	FILE *fp = fopen(file_path, "r");
	if(fp == NULL){
		printf("error opening file\n");
		return 1;
	}
	int c = fgetc(fp);
	if(c == EOF) {
		fclose(fp);
		return 1;
	} else {
		ungetc(c, fp);
		fclose(fp);
		return 0;
	}
}

fpga_result usrclk_reset(uint8_t *uio_ptr)
{
	uint64_t v      = 0;
	fpga_result res = FPGA_OK;

	if (uio_ptr == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	/* Assert all resets. IOPLL_AVMM_RESET_N is asserted implicitly */
	v = IOPLL_MGMT_RESET | IOPLL_RESET;
	*((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_CMD0)) = v;

	usleep(IOPLL_RESET_DELAY_US);

	/* De-assert the iopll reset only */
	v = IOPLL_MGMT_RESET;
	*((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_CMD0)) = v;

	usleep(IOPLL_RESET_DELAY_US);

	/* De-assert the remaining resets */
	v = IOPLL_AVMM_RESET_N;
	*((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_CMD0)) = v;

	usleep(IOPLL_RESET_DELAY_US);

	v = *((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_STS0));
	if (!(v & IOPLL_LOCKED)) {
		OPAE_ERR("IOPLL NOT locked after reset");;
		res = FPGA_BUSY;
	}

	return res;
}

fpga_result usrclk_read_freq(uint8_t *uio_ptr,
	uint8_t clock_sel, uint32_t *freq)
{
	uint64_t v = 0;

	if (uio_ptr == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	v = *((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_STS0));
	if (!(v & IOPLL_LOCKED)) {
		OPAE_ERR("IOPLL is NOT locked!");;
		return FPGA_BUSY;
	}

	v = FIELD_PREP(IOPLL_CLK_MEASURE, clock_sel);
	*((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_CMD1)) = v;

	usleep(IOPLL_MEASURE_DELAY_US);

	v = *((volatile uint64_t *)(uio_ptr + IOPLL_FREQ_STS1));

	*freq = FIELD_GET(IOPLL_FREQUENCY, v);

	return FPGA_OK;
}

fpga_result usrclk_write(uint8_t *uio_ptr, uint16_t address,
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

fpga_result usrclk_read(uint8_t *uio_ptr, uint16_t address,
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

fpga_result usrclk_update_bits(uint8_t *uio_ptr, uint16_t address,
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

fpga_result usrclk_m_write(uint8_t *uio_ptr,
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

fpga_result usrclk_n_write(uint8_t *uio_ptr, uint32_t cfg_pll_n,
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

fpga_result usrclk_c0_write(uint8_t *uio_ptr,
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

fpga_result usrclk_c1_write(uint8_t *uio_ptr,
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

fpga_result usrclk_set_freq(uint8_t *uio_ptr,
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

fpga_result usrclk_calibrate(uint8_t *uio_ptr, uint8_t *seq)
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

fpga_result get_usrclk_uio(const char *sysfs_path,
	uint32_t feature_id,
	const char* guid,
	struct opae_uio *uio,
	uint8_t **uio_ptr)
{
	fpga_result res                   = FPGA_NOT_FOUND;
	char feature_path[SYSFS_PATH_MAX] = { 0 };
	char guid_path[SYSFS_PATH_MAX]    = { 0 };
	char dfl_dev_str[SYSFS_PATH_MAX]  = { 0 };
	int feature_gres, guid_gres,ret                     = 0;
	size_t i                          = 0;
	uint64_t value                    = 0;
	char userclk_guid[128]            = { 0 };
	glob_t feature_pglob;
	glob_t guid_pglob;
	glob_t pglob;

	if (sysfs_path == NULL) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	if (snprintf(feature_path, sizeof(feature_path),
		"%s/dfl_dev*/feature_id",
		sysfs_path) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

    if (snprintf(guid_path, sizeof(guid_path),
		"%s/dfl_dev*/guid",
		sysfs_path) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

	feature_gres = opae_glob(feature_path, GLOB_NOSORT, NULL, &feature_pglob);
	/* Loop over sysfs feature_id file paths to check if any file is not empty, if all files are empty then we need non empty guid file or it's an error;
	   We need to do this because I see empty guid file in dfhv0 sysfs path eg - /sys/class/fpga_region/region0/dfl-port.0/dfl_dev.0/guid
	   It is possible feature_id can be empty by some mistake which would need to be flagged
	*/
	for (i = 0; i < feature_pglob.gl_pathc; i++) {
		if(!is_file_empty(feature_pglob.gl_pathv[i])){
			feature_gres = 0;
			break;
		} else{
			feature_gres = 1;
		}
	}
	
	guid_gres    = opae_glob(guid_path, GLOB_NOSORT, NULL, &guid_pglob);
	/* Loop over sysfs guid file paths to check if any file is not empty, if all files are empty then we need non empty feature_id file or it's an error;
	   We need to do this because I see empty guid file in dfhv0 sysfs path eg - /sys/class/fpga_region/region0/dfl-port.0/dfl_dev.0/guid
	*/
	for (i = 0; i < guid_pglob.gl_pathc; i++) {
		if(!is_file_empty(guid_pglob.gl_pathv[i])){
			guid_gres = 0;
			break;
		} else{
			guid_gres = 1;
		}
	}

	/* If we don't find guid or feature_id files in sysfs paths then error out
	*/
	if ((feature_gres) && (guid_gres)) {
		OPAE_ERR("Failed pattern match feature id - %s: %s ,guid - %s: %s ",
			feature_path, strerror(errno), guid_path, strerror(errno));
		opae_globfree(&feature_pglob);
        opae_globfree(&guid_pglob);
		return FPGA_INVALID_PARAM;
	} 

	/* guid gets priority over feature_id
	   non-empty guid file being present in sysfs path (eg /sys/class/fpga_region/region0/dfl-port.0/dfl_dev.0/guid) implies user is using dfhv1 IP
	*/
	if (!guid_gres) {
		pglob = guid_pglob;
	} else {
		pglob = feature_pglob;
	}

	for (i = 0; i < pglob.gl_pathc; i++) {
		if (!guid_gres) {
			int fd = open(pglob.gl_pathv[i],O_RDONLY);
			if(fd == -1) {
				OPAE_MSG("Could not open file %s\n", pglob.gl_pathv[i]);
				continue;
			}
			ssize_t bytes_read = read(fd, userclk_guid, sizeof(userclk_guid));
			if (bytes_read == -1) {
				OPAE_MSG("Could not read from file %s\n",pglob.gl_pathv[i]);
				continue;
			}
			if(close(fd) == -1) {
				OPAE_MSG("Could not close file %s\n", pglob.gl_pathv[i]);
				goto free;
			}
		} else {
			res = sysfs_read_u64(pglob.gl_pathv[i], &value);
			if (res != FPGA_OK) {
				OPAE_MSG("Failed to read sysfs value");
				continue;
			}
		}
		
		if ((value == feature_id) || (*guid == *userclk_guid)) {
			res = FPGA_OK;
			char *p = strstr(pglob.gl_pathv[i], "dfl_dev");
			if (p == NULL) {
				res = FPGA_NOT_FOUND;
				goto free;
			}
			char *end = strchr(p, '/');
			if (end == NULL) {
				res = FPGA_NOT_FOUND;
				goto free;
			}
			memcpy(dfl_dev_str, p, end - p);
			*(dfl_dev_str + (end - p)) = '\0';

			ret = opae_uio_open(uio, dfl_dev_str);
			if (ret) {
				res = FPGA_EXCEPTION;
				OPAE_ERR("Failed to open uio");
				break;
			}

			ret = opae_uio_region_get(uio, 0, (uint8_t **)uio_ptr, NULL);
			if (ret) {
				res = FPGA_EXCEPTION;
				OPAE_ERR("Failed to get uio region");
				opae_uio_close(uio);
				break;
			}
		}
		break;
	}

free:
	opae_globfree(&feature_pglob);
	opae_globfree(&guid_pglob);
	return res;
}

//Get fpga user clock
fpga_result get_userclock(const char *sysfs_path,
	uint64_t *userclk_high,
	uint64_t *userclk_low)
{
	char sysfs_usrpath[SYSFS_PATH_MAX]  = {0};
	fpga_result result                  = FPGA_OK;
	uint32_t high, low                  = 0;
	int ret                             = 0;
	uint8_t *uio_ptr                    = NULL;
	struct opae_uio uio;

	memset(&uio, 0, sizeof(uio));

	if ((sysfs_path == NULL) ||
		(userclk_high == NULL) ||
		(userclk_low == NULL)) {
		OPAE_ERR("Invalid input parameters");
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
		return FPGA_NO_ACCESS;
	}

	result = get_usrclk_uio(sysfs_path,
		USRCLK_FEATURE_ID,
		USRCLK_GUID,
		&uio,
		&uio_ptr);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get user clock uio");
		return result;
	}

	result = usrclk_read_freq(uio_ptr, IOPLL_MEASURE_HIGH, &high);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get user clock High");
		opae_uio_close(&uio);
		return result;
	}

	result = usrclk_read_freq(uio_ptr, IOPLL_MEASURE_LOW, &low);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get user clock Low");
		opae_uio_close(&uio);
		return result;
	}

	*userclk_high = high * 10000;	// Adjust to Hz
	*userclk_low = low * 10000;

	opae_uio_close(&uio);
	return FPGA_OK;
}

// set fpga user clock
fpga_result set_userclock(const char *sysfs_path,
	uint64_t userclk_high,
	uint64_t userclk_low)
{
	char sysfs_usrpath[SYSFS_PATH_MAX] = { 0 };
	int fd, ret                        = 0;
	char *bufp                         = NULL;
	ssize_t cnt                        = 0;
	uint64_t revision                  = 0;
	uint8_t seq                        = 1;
	uint8_t *uio_ptr                   = NULL;
	fpga_result result                 = FPGA_OK;
	ssize_t bytes_written              = 0;
	struct opae_uio uio;

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
	result = get_userclk_revision(sysfs_path, &revision);
	if (result == FPGA_OK && revision == AGILEX_USRCLK_REV) {
		iopll_max_freq = IOPLL_AGILEX_MAX_FREQ;
		iopll_min_freq = IOPLL_AGILEX_MIN_FREQ;

		// Enforce 1x clock within valid range
		if ((userclk_low > iopll_max_freq) ||
			(userclk_low < iopll_min_freq)) {
			OPAE_ERR("Invalid Input frequency");
			return FPGA_INVALID_PARAM;
		}

		bufp = (char *)&iopll_agilex_freq_config[userclk_low];
	} else {
		// S10 & A10 user clock
		// Enforce 1x clock within valid range
		if ((userclk_low > iopll_max_freq) ||
			(userclk_low < iopll_min_freq)) {
			OPAE_ERR("Invalid Input frequency");
			return FPGA_INVALID_PARAM;
		}
		bufp = (char *)&iopll_freq_config[userclk_low];
	}

	// Transitions from a currently configured very high frequency
	// or very low frequency to another extreme frequency sometimes
	// fails to stabilize. Start by forcing the fast clock to half
	// speed.
	slow_freq = iopll_max_freq / 4;
	if (userclk_low != slow_freq) {
		result = set_userclock(sysfs_path, slow_freq * 2, slow_freq);
	}

	ret = using_iopll(sysfs_usrpath, sysfs_path);
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
		USRCLK_GUID,
		&uio,
		&uio_ptr);
	if (result != FPGA_OK) {
		OPAE_ERR("Failed to get user clock uio");
		return result;
	}

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

// Determine whether or not the IOPLL is serving as the source of
// the user clock.
static int using_iopll(char *sysfs_usrpath, const char *sysfs_path)
{
	glob_t iopll_glob;
	size_t len        = 0;
	int res           = 0;

	// Test for the existence of the userclk_frequency file
	// which indicates an S10 driver

	if (snprintf(sysfs_usrpath, SYSFS_PATH_MAX,
		"%s/%s", sysfs_path, IOPLL_CLOCK_FREQ) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

	res = opae_glob(sysfs_usrpath, 0, NULL, &iopll_glob);
	if (res) {
		opae_globfree(&iopll_glob);
		return FPGA_NOT_FOUND;
	}

	if (iopll_glob.gl_pathc > 1)
		OPAE_MSG("WARNING: Port has multiple sysfs frequency files");

	len = strnlen(iopll_glob.gl_pathv[0], SYSFS_PATH_MAX - 1);
	memcpy(sysfs_usrpath, iopll_glob.gl_pathv[0], len);
	sysfs_usrpath[len] = '\0';

	opae_globfree(&iopll_glob);

	if (opae_access(sysfs_usrpath, F_OK | R_OK | W_OK) != 0) {
		OPAE_ERR("Unable to access sysfs frequency file");
		return FPGA_NO_ACCESS;
	}

	return FPGA_OK;
}


//Get fpga user clock
fpga_result get_userclk_revision(const char *sysfs_path,
	uint64_t *revision)
{
	char path[SYSFS_PATH_MAX] = { 0 };
	fpga_result result        = FPGA_OK;
	uint8_t* uio_ptr          = NULL;
	struct opae_uio uio;
	struct dfh dfh_csr;

	dfh_csr.csr = 0;
	// get user clock dfh revision from UIO
	result = get_usrclk_uio(sysfs_path,
		USRCLK_FEATURE_ID,
		USRCLK_GUID,
		&uio,
		&uio_ptr);
	if (result == FPGA_OK) {
		dfh_csr.csr = *((volatile uint64_t*)(uio_ptr + 0x0));
		*revision = dfh_csr.feature_rev;
		opae_uio_close(&uio);
		return result;
	}

	// get user clock dfh revision from sysfs
	if (snprintf(path, SYSFS_PATH_MAX,
		"%s/%s", sysfs_path, IOPLL_REVISION) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}

	result = opae_glob_path(path, SYSFS_PATH_MAX - 1);
	if (result != FPGA_OK) {
		OPAE_MSG("Failed to get sysfs path");
		return result;
	}

	result = sysfs_read_u64(path, revision);
	if (result != FPGA_OK) {
		OPAE_MSG("Failed to read value");
		return result;
	}

	return result;
}
