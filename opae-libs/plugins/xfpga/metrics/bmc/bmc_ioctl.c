// Copyright(c) 2018-2020, Intel Corporation
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

#include <opae/fpga.h>
#include "bmc.h"
#include "bmcdata.h"
#include <sys/types.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/ioctl.h>
#else
#include <io.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include "bmc_ioctl.h"

#include <glob.h>

#define NULL_CHECK(x)                                                          \
	do {                                                                   \
		if (NULL == (x)) {                                             \
			return FPGA_INVALID_PARAM;                             \
		}                                                              \
	} while (0)

fpga_result rawFromDouble(Values *details, double dbl, uint8_t *raw)
{
	fpga_result res = FPGA_OK;

	NULL_CHECK(details);
	NULL_CHECK(raw);

	int32_t R_exp = -(details->result_exp);
	int32_t i;
	for (i = 0; i < abs(R_exp); i++) {
		if (R_exp < 0) {
			dbl /= 10.0;
		} else {
			dbl *= 10.0;
		}
	}

	dbl = (dbl - details->B) / details->M;

	*raw = dbl > (double)0xff ? (uint8_t)0xff : (uint8_t)dbl;

	return res;
}

void fill_set_request(Values *vals, threshold_list *thresh,
		     bmc_set_thresh_request *req)
{
	fpga_result res = FPGA_OK;
	uint8_t mask = 0;

	if (thresh->upper_nr_thresh.is_valid) {
		mask |= UNR_thresh;
		res += rawFromDouble(vals, thresh->upper_nr_thresh.value,
				     &req->UNR);
	} else {
		mask &= ~UNR_thresh;
	}

	if (thresh->upper_c_thresh.is_valid) {
		mask |= UC_thresh;
		res += rawFromDouble(vals, thresh->upper_c_thresh.value,
				     &req->UC);
	} else {
		mask &= ~UC_thresh;
	}

	if (thresh->upper_nc_thresh.is_valid) {
		mask |= UNC_thresh;
		res += rawFromDouble(vals, thresh->upper_nc_thresh.value,
				     &req->UNC);
	} else {
		mask &= ~UNC_thresh;
	}

	if (thresh->lower_nr_thresh.is_valid) {
		mask |= LNR_thresh;
		res += rawFromDouble(vals, thresh->lower_nr_thresh.value,
				     &req->LNR);
	} else {
		mask &= ~LNR_thresh;
	}

	if (thresh->lower_c_thresh.is_valid) {
		mask |= LC_thresh;
		res += rawFromDouble(vals, thresh->lower_c_thresh.value,
				     &req->LC);
	} else {
		mask &= ~LC_thresh;
	}

	if (thresh->lower_nc_thresh.is_valid) {
		mask |= LNC_thresh;
		res += rawFromDouble(vals, thresh->lower_nc_thresh.value,
				     &req->LNC);
	} else {
		mask &= ~LNC_thresh;
	}

	if (FPGA_OK == res) {
		req->mask = mask;
	}
}

fpga_result _bmcSetThreshold(int fd, uint32_t sensor,
		    bmc_set_thresh_request *req)
{
	bmc_xact xact = {0};
	bmc_set_thresh_response resp;
	fpga_result res = FPGA_OK;

	xact.argsz = sizeof(xact);
	xact.txlen = sizeof(bmc_set_thresh_request);
	xact.rxlen = sizeof(bmc_set_thresh_response);
	xact.txbuf = (uint64_t)req;
	xact.rxbuf = (uint64_t)&resp;

	req->sens_num = sensor;

	req->header[0] = BMC_THRESH_HEADER_0;
	req->header[1] = BMC_THRESH_HEADER_1;
	req->header[2] = BMC_SET_THRESH_CMD;

	if (ioctl(fd, _IOWR(AVMMI_BMC_MAGIC, 0, struct avmmi_bmc_xact), &xact)
	    != 0) {
		res = FPGA_INVALID_PARAM;
		goto out_close;
	}

	if (resp.cc) {
		res = FPGA_EXCEPTION;
	}

out_close:
	return res;
}

fpga_result _bmcGetThreshold(int fd, uint32_t sensor,
			    bmc_get_thresh_response *resp)
{
	bmc_xact xact = {0};
	bmc_get_thresh_request req;
	fpga_result res = FPGA_OK;

	xact.argsz = sizeof(xact);
	xact.txlen = sizeof(bmc_get_thresh_request);
	xact.rxlen = sizeof(bmc_get_thresh_response);
	xact.txbuf = (uint64_t)&req;
	xact.rxbuf = (uint64_t)resp;

	req.sens_num = sensor;

	req.header[0] = BMC_THRESH_HEADER_0;
	req.header[1] = BMC_THRESH_HEADER_1;
	req.header[2] = BMC_GET_THRESH_CMD;

	if (ioctl(fd, _IOWR(AVMMI_BMC_MAGIC, 0, struct avmmi_bmc_xact), &xact)
	    != 0) {
		res = FPGA_INVALID_PARAM;
		goto out_close;
	}

	if (resp->cc) {
		res = FPGA_EXCEPTION;
	}

out_close:
	return res;
}

fpga_result bmcSetHWThresholds(bmc_sdr_handle sdr_h, uint32_t sensor,
			       threshold_list *thresh)
{
	fpga_result res = FPGA_OK;
	char sysfspath[SYSFS_PATH_MAX] = { 0, };
	int fd = 0;
	bmc_set_thresh_request req;
	Values *vals;
	sensor_reading read;
	bmc_get_thresh_response resp;
	size_t len;

	NULL_CHECK(sdr_h);
	NULL_CHECK(thresh);
	struct _sdr_rec *sdr = (struct _sdr_rec *)sdr_h;

	if (BMC_SDR_MAGIC != sdr->magic) {
		return FPGA_INVALID_PARAM;
	}

	if (sensor >= sdr->num_records) {
		return FPGA_INVALID_PARAM;
	}

	len = strnlen(sdr->sysfs_path, sizeof(sysfspath) - 1);
	strncpy(sysfspath, sdr->sysfs_path, len + 1);
	strncat(sysfspath, "/", 2);
	len = strnlen(SYSFS_AVMMI_DIR, sizeof(sysfspath) - (len + 1));
	strncat(sysfspath, SYSFS_AVMMI_DIR, len + 1);

	glob_t pglob;
	int gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
	if ((gres) || (1 != pglob.gl_pathc)) {
		globfree(&pglob);
		return FPGA_NOT_FOUND;
	}

	char *avmmi = strrchr(pglob.gl_pathv[0], '/');
	if (NULL == avmmi) {
		globfree(&pglob);
		return FPGA_NOT_FOUND;
	}

	strncpy(sysfspath, "/dev/", 6);
	len = strnlen(&avmmi[1], sizeof(sysfspath) - 6);
	strncat(sysfspath, &avmmi[1], len + 1);

	fd = open(sysfspath, O_RDWR);
	globfree(&pglob);
	if (fd < 0) {
		return FPGA_NOT_FOUND;
	}

	memset(&req, 0, sizeof(req));
	memset(&read, 0, sizeof(read));

	vals = bmc_build_values(&read, &sdr->contents[sensor].header,
				&sdr->contents[sensor].key,
				&sdr->contents[sensor].body);

	if (NULL == vals) {
		close(fd);
		return FPGA_NO_MEMORY;
	}

	res = _bmcGetThreshold(fd, sensor, &resp);
	if (FPGA_OK != res) {
		fprintf(stderr, "Error returned from _bmcGetThreshold\n");
	}

	lseek(fd, 0, SEEK_SET);

	int sz = sizeof(bmc_get_thresh_response) - sizeof(resp.cc)
		 - sizeof(resp.header);
	memcpy(&req.mask, &resp.mask, sz);

	fill_set_request(vals, thresh, &req);

	if (vals->name)
		free(vals->name);

	if (vals)
		free(vals);

	res = _bmcSetThreshold(fd, sensor, &req);

	close(fd);

	return res;
}
