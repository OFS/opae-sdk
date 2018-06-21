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

//TODO: Remove metadata parsing code duplication by using
//metadata parsing code in FPGA API

#include <stdio.h>
#include <stdlib.h>
#include <uuid/uuid.h>
#include <json-c/json.h>

#ifndef __BITSTREAM_H__
#define __BITSTREAM_H__

#define METADATA_GUID "58656F6E-4650-4741-B747-425376303031"
#define METADATA_GUID_LEN 16
#define GBS_AFU_IMAGE "afu-image"
#define BBS_INTERFACE_ID "interface-uuid"

#define PRINT_MSG printf
#define PRINT_ERR(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

static fpga_result string_to_guid(const char *guid, fpga_guid *result)
{
	if (uuid_parse(guid, *result) < 0) {
		PRINT_MSG("Error parsing guid %s\n", guid);
		return FPGA_INVALID_PARAM;
	}

	return FPGA_OK;
}

static uint64_t read_int_from_bitstream(const uint8_t *bitstream, uint8_t size)
{
	uint64_t ret = 0;
	switch (size) {
	case sizeof(uint8_t):
		ret = *((uint8_t *) bitstream);
		break;
	case sizeof(uint16_t):
		ret = *((uint16_t *) bitstream);
		break;
	case sizeof(uint32_t):
		ret = *((uint32_t *) bitstream);
		break;
	case sizeof(uint64_t):
		ret = *((uint64_t *) bitstream);
		break;
	default:
		PRINT_ERR("Unknown integer size");
	}

	return ret;
}

void fpga_guid_to_fpga(uint64_t guidh, uint64_t guidl, uint8_t *guid)
{
	uint32_t i;
	uint32_t s;

	// The function expects the MSB of the GUID at [0] and the LSB at [15].
	s = 64;
	for (i = 0; i < 8; ++i) {
		s -= 8;
		guid[i] = (uint8_t) ((guidh >> s) & 0xff);
	}

	s = 64;
	for (i = 0; i < 8; ++i) {
		s -= 8;
		guid[8 + i] = (uint8_t) ((guidl >> s) & 0xff);
	}
}

#endif
