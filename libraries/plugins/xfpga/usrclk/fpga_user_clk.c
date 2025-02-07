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
#include "fpga_user_clk_int.h"
#include "fpga_user_clk_freq.h"
#include "mock/opae_std.h"

// user clock sysfs
#define  IOPLL_CLOCK_FREQ             "dfl*/userclk/frequency"
#define  IOPLL_REVISION               "dfl*/feature_rev"

#define IOPLL_MEASURE_LOW             0
#define IOPLL_MEASURE_HIGH            1
#define IOPLL_MEASURE_DELAY_US        8000
#define IOPLL_RESET_DELAY_US          1000

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
};

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

STATIC fpga_result usrclk_read_freq(uint8_t *uio_ptr,
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

fpga_result get_usrclk_uio(const char *sysfs_path,
	uint32_t feature_id,
	struct opae_uio *uio,
	uint8_t **uio_ptr)
{
	fpga_result res                   = FPGA_NOT_FOUND;
	char feature_path[SYSFS_PATH_MAX] = { 0 };
	char dfl_dev_str[SYSFS_PATH_MAX]  = { 0 };
	int gres, ret                     = 0;
	size_t i                          = 0;
	uint64_t value                    = 0;
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

	gres = opae_glob(feature_path, GLOB_NOSORT, NULL, &pglob);
	if (gres) {
		OPAE_ERR("Failed pattern match %s: %s",
			feature_path, strerror(errno));
		opae_globfree(&pglob);
		return FPGA_NOT_FOUND;
	}

	for (i = 0; i < pglob.gl_pathc; i++) {
		res = sysfs_read_u64(pglob.gl_pathv[i], &value);
		if (res != FPGA_OK) {
			OPAE_MSG("Failed to read sysfs value");
			continue;
		}

		if (value == feature_id) {
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
	opae_globfree(&pglob);
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
	ret = usrclk_using_iopll(sysfs_usrpath, sysfs_path);
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
	uint64_t revision	= 0;
	fpga_result result	= FPGA_OK;

	if (sysfs_path == NULL) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	if (userclk_high < MIN_FPGA_FREQ) {
		OPAE_ERR("Invalid Input frequency");
		return FPGA_INVALID_PARAM;
	}

	// Revision determines the algorithm used for updating clocks
	result = get_userclk_revision(sysfs_path, &revision);
	if (result != FPGA_OK) {
		// Very old systems may use a sysfs path to directly update
		// clocks. This is handled in the type1 path.
		result = set_userclock_type1(sysfs_path, 0,
					     userclk_high, userclk_low);
	} else if (revision <= AGILEX_USRCLK_REV) {
		// Agilex user clock DFH revision 1
		// S10 & A10 user clock DFH revision 0
		result = set_userclock_type1(sysfs_path, revision,
					     userclk_high, userclk_low);
	} else {
		OPAE_ERR("Unsupported FPGA device revision: %d", revision);
		return FPGA_NOT_SUPPORTED;
	}

	return result;
}

// Determine whether or not the IOPLL is serving as the source of
// the user clock.
int usrclk_using_iopll(char *sysfs_usrpath, const char *sysfs_path)
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
