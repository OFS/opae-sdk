// Copyright(c) 2018, Intel Corporation
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
/*
 * @file bmc_ioctl.h
 *
 * @brief
 */
#ifndef BMC_IOCTL_H
#define BMC_IOCTL_H

#include <opae/fpga.h>
#include <wchar.h>
#include "bmcdata.h"
#ifdef __cplusplus
extern "C" {
#endif

//#define BMC_IOCTL_MAGIC (0xc0187600)
#define AVMMI_BMC_MAGIC (0x76)

#define BMC_THRESH_HEADER_0 (0x4 << 2)
#define BMC_THRESH_HEADER_1 (0)
#define BMC_SET_THRESH_CMD (0x26)
#define BMC_GET_THRESH_CMD (0x27)

#pragma pack(push, 1)

typedef struct avmmi_bmc_xact {
	uint32_t argsz;
	uint16_t txlen;
	uint16_t rxlen;
	uint64_t txbuf;
	uint64_t rxbuf;
} bmc_xact;

typedef struct {
	uint8_t header[3];
	uint8_t sens_num;
	uint8_t mask;
	uint8_t LNC;
	uint8_t LC;
	uint8_t LNR;
	uint8_t UNC;
	uint8_t UC;
	uint8_t UNR;
} bmc_set_thresh_request;

typedef struct {
	uint8_t header[3];
	uint8_t cc;
} bmc_set_thresh_response;

typedef struct {
	uint8_t header[3];
	uint8_t sens_num;
} bmc_get_thresh_request;

typedef struct {
	uint8_t header[3];
	uint8_t cc;
	uint8_t mask;
	uint8_t LNC;
	uint8_t LC;
	uint8_t LNR;
	uint8_t UNC;
	uint8_t UC;
	uint8_t UNR;
} bmc_get_thresh_response;

#pragma pack(pop)

typedef enum {
	LNC_thresh = 0x01,
	LC_thresh = 0x02,
	LNR_thresh = 0x04,
	UNC_thresh = 0x08,
	UC_thresh = 0x10,
	UNR_thresh = 0x20,
} Thresh;

fpga_result rawFromDouble(Values *details, double dbl, uint8_t *raw);

void fill_set_request(Values *vals, threshold_list *thresh,
	bmc_set_thresh_request *req);

fpga_result _bmcSetThreshold(int fd, uint32_t sensor,
	bmc_set_thresh_request *req);

fpga_result _bmcGetThreshold(int fd, uint32_t sensor,
	bmc_get_thresh_response *resp);

fpga_result bmcSetHWThresholds(bmc_sdr_handle sdr_h, uint32_t sensor,
	threshold_list *thresh);

#ifdef __cplusplus
}
#endif

#endif /* !BMC_IOCTL_H */
