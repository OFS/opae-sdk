// Copyright(c) 2018-2019, Intel Corporation
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

#include "fpgad/api/opae_events_api.h"
#include "fpgad/api/device_monitoring.h"

#include "logging.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("fpgad-xfpga: " format, ##__VA_ARGS__)

#ifndef UNUSED_PARAM
#define UNUSED_PARAM(x) ((void)x)
#endif // UNUSED_PARAM

enum fpga_power_state {
	FPGAD_NORMAL_PWR = 0,
	FPGAD_AP1_STATE,
	FPGAD_AP2_STATE,
	FPGAD_AP6_STATE
};

typedef struct _fpgad_xfpga_AP_context {
	const char *sysfs_file;
	const char *message;
	int low_bit;
	int high_bit;
} fpgad_xfpga_AP_context;

fpgad_xfpga_AP_context fpgad_xfpga_AP_contexts[] = {
	{ "ap1_event",   "AP1 Triggered!",         0, 0 },
	{ "ap2_event",   "AP2 Triggered!",         0, 0 },
	{ "power_state", "Power state changed to", 0, 1 },
};

fpgad_detection_status
fpgad_xfpga_detect_AP1_or_AP2(fpgad_monitored_device *d,
			      void *context)
{
	fpgad_xfpga_AP_context *c =
		(fpgad_xfpga_AP_context *)context;
	fpga_object obj = NULL;
	fpga_result res;
	uint64_t err = 0;
	uint64_t mask;
	uint64_t value;
	int i;
	bool detected = false;

	res = fpgaTokenGetObject(d->token, c->sysfs_file,
				 &obj, 0);
	if (res != FPGA_OK) {
		LOG("failed to get error object\n");
		return FPGAD_STATUS_NOT_DETECTED;
	}

	res = fpgaObjectRead64(obj, &err, 0);
	if (res != FPGA_OK) {
		LOG("failed to read error object\n");
		fpgaDestroyObject(&obj);
		return FPGAD_STATUS_NOT_DETECTED;
	}

	fpgaDestroyObject(&obj);

	mask = 0;
	for (i = c->low_bit ; i <= c->high_bit ; ++i)
		mask |= 1ULL << i;

	value = mask & err;

	if (value != 0 && !mon_has_error_occurred(d, context)) {
		detected = mon_add_device_error(d, context);
	}

	if (value == 0 && mon_has_error_occurred(d, context)) {
		mon_remove_device_error(d, context);
	}

	return detected ? FPGAD_STATUS_DETECTED : FPGAD_STATUS_NOT_DETECTED;
}

void fpgad_xfpga_respond_AP1_or_AP2(fpgad_monitored_device *d,
				    void *context)
{
	fpgad_xfpga_AP_context *c =
		(fpgad_xfpga_AP_context *)context;

	LOG("%s\n", c->message);

	// Signal OPAE events API
	opae_api_send_EVENT_POWER_THERMAL(d);
}

fpgad_detection_status
fpgad_xfpga_detect_PowerStateChange(fpgad_monitored_device *d,
				    void *context)
{
	fpgad_xfpga_AP_context *c =
		(fpgad_xfpga_AP_context *)context;
	fpga_object obj = NULL;
	fpga_result res;
	uint64_t err = 0;
	uint64_t mask;
	uint64_t value;
	int i;
	bool detected = false;

	res = fpgaTokenGetObject(d->token, c->sysfs_file,
				 &obj, 0);
	if (res != FPGA_OK) {
		LOG("failed to get error object\n");
		return FPGAD_STATUS_NOT_DETECTED;
	}

	res = fpgaObjectRead64(obj, &err, 0);
	if (res != FPGA_OK) {
		LOG("failed to read error object\n");
		fpgaDestroyObject(&obj);
		return FPGAD_STATUS_NOT_DETECTED;
	}

	fpgaDestroyObject(&obj);

	mask = 0;
	for (i = c->low_bit ; i <= c->high_bit ; ++i)
		mask |= 1ULL << i;

	value = mask & err;

	if (value != d->scratchpad[0]) {
		detected = true;
	}

	d->scratchpad[0] = value;

	return detected ? FPGAD_STATUS_DETECTED : FPGAD_STATUS_NOT_DETECTED;
}

void fpgad_xfpga_respond_PowerStateChange(fpgad_monitored_device *d,
					  void *context)
{
	const char *power_states[] = {
		"Normal Power",
		"AP1 Power State",
		"AP2 Power State",
		"AP6 Power State"
	};

	fpgad_xfpga_AP_context *c =
		(fpgad_xfpga_AP_context *)context;

	LOG("%s %s\n", c->message,
			d->scratchpad[0] < 4 ?
			power_states[d->scratchpad[0]] :
			"unknown");
}

typedef struct _fpgad_xfpga_Error_context {
	const char *sysfs_file;
	const char *message;
	int low_bit;
	int high_bit;
} fpgad_xfpga_Error_context;

fpgad_xfpga_Error_context fpgad_xfpga_Error_contexts[] = {
	/*  0 */ { "errors/errors", "PORT_ERROR[0x1010].VfFlrAccessError",                   51, 51 },
	/*  1 */ { "errors/errors", "PORT_ERROR[0x1010].Ap6Event",                           50, 50 }, /* AP6 NULL GBS */
	/*  2 */ { "errors/errors", "PORT_ERROR[0x1010].PMRError",                           49, 49 },
	/*  3 */ { "errors/errors", "PORT_ERROR[0x1010].PageFault",                          48, 48 },
	/*  4 */ { "errors/errors", "PORT_ERROR[0x1010].VgaMemRangeError",                   47, 47 },
	/*  5 */ { "errors/errors", "PORT_ERROR[0x1010].LegRangeHighError",                  46, 46 },
	/*  6 */ { "errors/errors", "PORT_ERROR[0x1010].LegRangeLowError",                   45, 45 },
	/*  7 */ { "errors/errors", "PORT_ERROR[0x1010].GenProtRangeError",                  44, 44 },
	/*  8 */ { "errors/errors", "PORT_ERROR[0x1010].L1prMesegError",                     43, 43 },
	/*  9 */ { "errors/errors", "PORT_ERROR[0x1010].L1prSmrr2Error",                     42, 42 },
	/* 10 */ { "errors/errors", "PORT_ERROR[0x1010].L1prSmrrError",                      41, 41 },
	/* 11 */ { "errors/errors", "PORT_ERROR[0x1010].TxReqCounterOverflow",               40, 40 },
	/* 12 */ { "errors/errors", "PORT_ERROR[0x1010].UnexpMMIOResp",                      34, 34 },
	/* 13 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh2FifoOverflow",                  33, 33 },
	/* 14 */ { "errors/errors", "PORT_ERROR[0x1010].MMIOTimedOut",                       32, 32 },
	/* 15 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh1NonZeroSOP",                    24, 24 },
	/* 16 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh1IncorrectAddr",                 23, 23 },
	/* 17 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh1DataPayloadOverrun",            22, 22 },
	/* 18 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh1InsufficientData",              21, 21 },
	/* 19 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh1Len4NotAligned",                20, 20 },
	/* 20 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh1Len2NotAligned",                19, 19 },
	/* 21 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh1Len3NotSupported",              18, 18 },
	/* 22 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh1InvalidReqEncoding",            17, 17 },
	/* 23 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh1Overflow",                      16, 16 },
	/* 24 */ { "errors/errors", "PORT_ERROR[0x1010].MMIOWrWhileRst",                     10, 10 },
	/* 25 */ { "errors/errors", "PORT_ERROR[0x1010].MMIORdWhileRst",                      9,  9 },
	/* 26 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh0Len4NotAligned",                 4,  4 },
	/* 27 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh0Len2NotAligned",                 3,  3 },
	/* 28 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh0Len3NotSupported",               2,  2 },
	/* 29 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh0InvalidReqEncoding",             1,  1 },
	/* 30 */ { "errors/errors", "PORT_ERROR[0x1010].TxCh0Overflow",                       0,  0 },

	/* 31 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxReqCounterOverflow",    40, 40 },
	/* 32 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh2FifoOverflow",       33, 33 },
	/* 33 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].MMIOTimedOut",            32, 32 },
	/* 34 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1IllegalVCsel",       25, 25 },
	/* 35 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1NonZeroSOP",         24, 24 },
	/* 36 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1IncorrectAddr",      23, 23 },
	/* 37 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1DataPayloadOverrun", 22, 22 },
	/* 38 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1InsufficientData",   21, 21 },
	/* 39 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Len4NotAligned",     20, 20 },
	/* 40 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Len2NotAligned",     19, 19 },
	/* 41 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Len3NotSupported",   18, 18 },
	/* 42 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1InvalidReqEncoding", 17, 17 },
	/* 43 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Overflow",           16, 16 },
	/* 44 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].MMIOWrWhileRst",          10, 10 },
	/* 45 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].MMIORdWhileRst",           9,  9 },
	/* 46 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Len4NotAligned",      4,  4 },
	/* 47 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Len2NotAligned",      3,  3 },
	/* 48 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Len3NotSupported",    2,  2 },
	/* 49 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0InvalidReqEncoding",  1,  1 },
	/* 50 */ { "errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Overflow",            0,  0 },

	/* 51 */ { "errors/fme-errors/errors", "FME_ERROR0[0x4010].CvlCdcParErro0",           17, 19 },
	/* 52 */ { "errors/fme-errors/errors", "FME_ERROR0[0x4010].Pcie1CdcParErr",           12, 16 },
	/* 53 */ { "errors/fme-errors/errors", "FME_ERROR0[0x4010].Pcie0CdcParErr",            7, 11 },
	/* 54 */ { "errors/fme-errors/errors", "FME_ERROR0[0x4010].MBPErr",                    6,  6 },
	/* 55 */ { "errors/fme-errors/errors", "FME_ERROR0[0x4010].AfuAccessModeErr",          5,  5 },
	/* 56 */ { "errors/fme-errors/errors", "FME_ERROR0[0x4010].IommuParityErr",            4,  4 },
	/* 57 */ { "errors/fme-errors/errors", "FME_ERROR0[0x4010].KtiCdcParityErr",           2,  3 },
	/* 58 */ { "errors/fme-errors/errors", "FME_ERROR0[0x4010].FabricFifoUOflow",          1,  1 },
	/* 59 */ { "errors/fme-errors/errors", "FME_ERROR0[0x4010].FabricErr",                 0,  0 },

	/* 60 */ { "errors/pcie0_errors", "PCIE0_ERROR[0x4020].FunctTypeErr",                 63, 63 },
	/* 61 */ { "errors/pcie0_errors", "PCIE0_ERROR[0x4020].VFNumb",                       62, 62 },
	/* 62 */ { "errors/pcie0_errors", "PCIE0_ERROR[0x4020].RxPoisonTlpErr",                9,  9 },
	/* 63 */ { "errors/pcie0_errors", "PCIE0_ERROR[0x4020].ParityErr",                     8,  8 },
	/* 64 */ { "errors/pcie0_errors", "PCIE0_ERROR[0x4020].CompTimeOutErr",                7,  7 },
	/* 65 */ { "errors/pcie0_errors", "PCIE0_ERROR[0x4020].CompStatErr",                   6,  6 },
	/* 66 */ { "errors/pcie0_errors", "PCIE0_ERROR[0x4020].CompTagErr",                    5,  5 },
	/* 67 */ { "errors/pcie0_errors", "PCIE0_ERROR[0x4020].MRLengthErr",                   4,  4 },
	/* 68 */ { "errors/pcie0_errors", "PCIE0_ERROR[0x4020].MRAddrErr",                     3,  3 },
	/* 69 */ { "errors/pcie0_errors", "PCIE0_ERROR[0x4020].MWLengthErr",                   2,  2 },
	/* 70 */ { "errors/pcie0_errors", "PCIE0_ERROR[0x4020].MWAddrErr",                     1,  1 },
	/* 71 */ { "errors/pcie0_errors", "PCIE0_ERROR[0x4020].FormatTypeErr",                 0,  0 },

	/* 72 */ { "errors/pcie1_errors", "PCIE1_ERROR[0x4030].RxPoisonTlpErr",                9,  9 },
	/* 73 */ { "errors/pcie1_errors", "PCIE1_ERROR[0x4030].ParityErr",                     8,  8 },
	/* 74 */ { "errors/pcie1_errors", "PCIE1_ERROR[0x4030].CompTimeOutErr",                7,  7 },
	/* 75 */ { "errors/pcie1_errors", "PCIE1_ERROR[0x4030].CompStatErr",                   6,  6 },
	/* 76 */ { "errors/pcie1_errors", "PCIE1_ERROR[0x4030].CompTagErr",                    5,  5 },
	/* 77 */ { "errors/pcie1_errors", "PCIE1_ERROR[0x4030].MRLengthErr",                   4,  4 },
	/* 78 */ { "errors/pcie1_errors", "PCIE1_ERROR[0x4030].MRAddrErr",                     3,  3 },
	/* 79 */ { "errors/pcie1_errors", "PCIE1_ERROR[0x4030].MWLengthErr",                   2,  2 },
	/* 80 */ { "errors/pcie1_errors", "PCIE1_ERROR[0x4030].MWAddrErr",                     1,  1 },
	/* 81 */ { "errors/pcie1_errors", "PCIE1_ERROR[0x4030].FormatTypeErr",                 0,  0 },

	/* 82 */ { "errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].MBPErr",            12, 12 },
	/* 83 */ { "errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PowerThreshAP2",    11, 11 },
	/* 84 */ { "errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PowerThreshAP1",    10, 10 },
	/* 85 */ { "errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP6",      9,  9 }, /* AP6 */
	/* 86 */ { "errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].InjectedWarningErr", 6,  6 },
	/* 87 */ { "errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].AfuAccessModeErr",   5,  5 },
	/* 88 */ { "errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].ProcHot",            4,  4 },
	/* 89 */ { "errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PortFatalErr",       3,  3 },
	/* 90 */ { "errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PcieError",          2,  2 },
	/* 91 */ { "errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP2",      1,  1 },
	/* 92 */ { "errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP1",      0,  0 },

	/* 93 */ { "errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].InjectedCatastErr",  11, 11 },
	/* 94 */ { "errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].ThermCatastErr",     10, 10 },
	/* 95 */ { "errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].CrcCatastErr",        9,  9 },
	/* 96 */ { "errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].InjectedFatalErr",    8,  8 },
	/* 97 */ { "errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].PciePoisonErr",       7,  7 },
	/* 98 */ { "errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].FabricFatalErr",      6,  6 },
	/* 99 */ { "errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].IommuFatalErr",       5,  5 },
	/*100 */ { "errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].DramFatalErr",        4,  4 },
	/*101 */ { "errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].KtiProtoFatalErr",    3,  3 },
	/*102 */ { "errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].CciFatalErr",         2,  2 },
	/*103 */ { "errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].TagCchFatalErr",      1,  1 },
	/*104 */ { "errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].KtiLinkFatalErr",     0,  0 },
};

fpgad_detection_status
fpgad_xfpga_detect_Error(fpgad_monitored_device *d,
			 void *context)
{
	fpgad_xfpga_Error_context *c =
		(fpgad_xfpga_Error_context *)context;
	fpga_object obj = NULL;
	fpga_result res;
	uint64_t err = 0;
	uint64_t mask;
	uint64_t value;
	int i;
	bool detected = false;

	res = fpgaTokenGetObject(d->token, c->sysfs_file,
				 &obj, 0);
	if (res != FPGA_OK) {
		LOG("failed to get error object\n");
		return FPGAD_STATUS_NOT_DETECTED;
	}

	res = fpgaObjectRead64(obj, &err, 0);
	if (res != FPGA_OK) {
		LOG("failed to read error object\n");
		fpgaDestroyObject(&obj);
		return FPGAD_STATUS_NOT_DETECTED;
	}

	fpgaDestroyObject(&obj);

	mask = 0;
	for (i = c->low_bit ; i <= c->high_bit ; ++i)
		mask |= 1ULL << i;

	value = mask & err;

	if (value != 0 && !mon_has_error_occurred(d, context)) {
		detected = mon_add_device_error(d, context);
	}

	if (value == 0 && mon_has_error_occurred(d, context)) {
		mon_remove_device_error(d, context);
	}

	return detected ? FPGAD_STATUS_DETECTED : FPGAD_STATUS_NOT_DETECTED;
}

fpgad_detection_status
fpgad_xfpga_detect_High_Priority_Error(fpgad_monitored_device *d,
				       void *context)
{
	fpgad_detection_status status;

	status = fpgad_xfpga_detect_Error(d, context);

	if (status == FPGAD_STATUS_DETECTED)
		return FPGAD_STATUS_DETECTED_HIGH;

	return status;
}

void fpgad_xfpga_respond_LogError(fpgad_monitored_device *d,
				  void *context)
{
	fpgad_xfpga_Error_context *c =
		(fpgad_xfpga_Error_context *)context;

	LOG("%s\n", c->message);

	// signal OPAE events API
	opae_api_send_EVENT_ERROR(d);
}

void fpgad_xfpga_respond_AP6_and_Null_GBS(fpgad_monitored_device *d,
					  void *context)
{
	fpgad_xfpga_Error_context *c =
		(fpgad_xfpga_Error_context *)context;

	LOG("%s\n", c->message);

	// Program NULL GBS

	// d will be the Port device. We need to find the parent
	// (FME) device to perform the PR.

	if (d->bitstr) {
		fpga_result res;
		fpga_properties prop = NULL;
		fpga_token fme_tok = NULL;
		fpga_handle fme_h = NULL;
		const uint32_t slot = 0;

		res = fpgaGetProperties(d->token, &prop);
		if (res != FPGA_OK) {
			LOG("(AP6) failed to get properties! : %s\n",
			    fpgaErrStr(res));
			goto out_signal;
		}

		res = fpgaPropertiesGetParent(prop, &fme_tok);
		if (res != FPGA_OK || !fme_tok) {
			LOG("(AP6) failed to get FME token! : %s\n",
			    fpgaErrStr(res));
			goto out_destroy_props;
		}

		res = fpgaOpen(fme_tok, &fme_h, 0);
		if (res != FPGA_OK) {
			LOG("(AP6) failed to get FME handle! : %s\n",
			    fpgaErrStr(res));
			goto out_destroy_fme_tok;
		}

		LOG("programming \"%s\": ", d->bitstr->filename);

		res = fpgaReconfigureSlot(fme_h,
					  slot,
					  d->bitstr->data,
					  d->bitstr->data_len,
					  FPGA_RECONF_FORCE);
		if (res != FPGA_OK)
			LOG("SUCCESS\n");
		else
			LOG("FAILED : %s\n", fpgaErrStr(res));

		fpgaClose(fme_h);
out_destroy_fme_tok:
		fpgaDestroyToken(&fme_tok);
out_destroy_props:
		fpgaDestroyProperties(&prop);
	} else
		LOG("no bitstream to program for AP6!\n");

	// Signal OPAE events API
out_signal:
	opae_api_send_EVENT_POWER_THERMAL(d);
}

void fpgad_xfpga_respond_AP6(fpgad_monitored_device *d,
			     void *context)
{
	fpgad_xfpga_Error_context *c =
		(fpgad_xfpga_Error_context *)context;

	LOG("%s\n", c->message);

	// Signal OPAE events API
	opae_api_send_EVENT_POWER_THERMAL(d);
}

// Port detections
STATIC fpgad_detect_event_t fpgad_xfpga_port_detections[] = {
	fpgad_xfpga_detect_AP1_or_AP2,
	fpgad_xfpga_detect_AP1_or_AP2,
	fpgad_xfpga_detect_PowerStateChange,

	fpgad_xfpga_detect_Error, //  0
	fpgad_xfpga_detect_High_Priority_Error, // 1 (AP6 and NULL GBS)
	fpgad_xfpga_detect_Error, //  2
	fpgad_xfpga_detect_Error, //  3
	fpgad_xfpga_detect_Error, //  4
	fpgad_xfpga_detect_Error, //  5
	fpgad_xfpga_detect_Error, //  6
	fpgad_xfpga_detect_Error, //  7
	fpgad_xfpga_detect_Error, //  8
	fpgad_xfpga_detect_Error, //  9
	fpgad_xfpga_detect_Error, // 10
	fpgad_xfpga_detect_Error, // 11
	fpgad_xfpga_detect_Error, // 12
	fpgad_xfpga_detect_Error, // 13
	fpgad_xfpga_detect_Error, // 14
	fpgad_xfpga_detect_Error, // 15
	fpgad_xfpga_detect_Error, // 16
	fpgad_xfpga_detect_Error, // 17
	fpgad_xfpga_detect_Error, // 18
	fpgad_xfpga_detect_Error, // 19
	fpgad_xfpga_detect_Error, // 20
	fpgad_xfpga_detect_Error, // 21
	fpgad_xfpga_detect_Error, // 22
	fpgad_xfpga_detect_Error, // 23
	fpgad_xfpga_detect_Error, // 24
	fpgad_xfpga_detect_Error, // 25
	fpgad_xfpga_detect_Error, // 26
	fpgad_xfpga_detect_Error, // 27
	fpgad_xfpga_detect_Error, // 28
	fpgad_xfpga_detect_Error, // 29
	fpgad_xfpga_detect_Error, // 30

	fpgad_xfpga_detect_Error, // 31
	fpgad_xfpga_detect_Error, // 32
	fpgad_xfpga_detect_Error, // 33
	fpgad_xfpga_detect_Error, // 34
	fpgad_xfpga_detect_Error, // 35
	fpgad_xfpga_detect_Error, // 36
	fpgad_xfpga_detect_Error, // 37
	fpgad_xfpga_detect_Error, // 38
	fpgad_xfpga_detect_Error, // 39
	fpgad_xfpga_detect_Error, // 40
	fpgad_xfpga_detect_Error, // 41
	fpgad_xfpga_detect_Error, // 42
	fpgad_xfpga_detect_Error, // 43
	fpgad_xfpga_detect_Error, // 44
	fpgad_xfpga_detect_Error, // 45
	fpgad_xfpga_detect_Error, // 46
	fpgad_xfpga_detect_Error, // 47
	fpgad_xfpga_detect_Error, // 48
	fpgad_xfpga_detect_Error, // 49
	fpgad_xfpga_detect_Error, // 50

	NULL
};

STATIC void *fpgad_xfpga_port_detection_contexts[] = {
	&fpgad_xfpga_AP_contexts[0],
	&fpgad_xfpga_AP_contexts[1],
	&fpgad_xfpga_AP_contexts[2],

	&fpgad_xfpga_Error_contexts[0],
	&fpgad_xfpga_Error_contexts[1],
	&fpgad_xfpga_Error_contexts[2],
	&fpgad_xfpga_Error_contexts[3],
	&fpgad_xfpga_Error_contexts[4],
	&fpgad_xfpga_Error_contexts[5],
	&fpgad_xfpga_Error_contexts[6],
	&fpgad_xfpga_Error_contexts[7],
	&fpgad_xfpga_Error_contexts[8],
	&fpgad_xfpga_Error_contexts[9],
	&fpgad_xfpga_Error_contexts[10],
	&fpgad_xfpga_Error_contexts[11],
	&fpgad_xfpga_Error_contexts[12],
	&fpgad_xfpga_Error_contexts[13],
	&fpgad_xfpga_Error_contexts[14],
	&fpgad_xfpga_Error_contexts[15],
	&fpgad_xfpga_Error_contexts[16],
	&fpgad_xfpga_Error_contexts[17],
	&fpgad_xfpga_Error_contexts[18],
	&fpgad_xfpga_Error_contexts[19],
	&fpgad_xfpga_Error_contexts[20],
	&fpgad_xfpga_Error_contexts[21],
	&fpgad_xfpga_Error_contexts[22],
	&fpgad_xfpga_Error_contexts[23],
	&fpgad_xfpga_Error_contexts[24],
	&fpgad_xfpga_Error_contexts[25],
	&fpgad_xfpga_Error_contexts[26],
	&fpgad_xfpga_Error_contexts[27],
	&fpgad_xfpga_Error_contexts[28],
	&fpgad_xfpga_Error_contexts[29],
	&fpgad_xfpga_Error_contexts[30],

	&fpgad_xfpga_Error_contexts[31],
	&fpgad_xfpga_Error_contexts[32],
	&fpgad_xfpga_Error_contexts[33],
	&fpgad_xfpga_Error_contexts[34],
	&fpgad_xfpga_Error_contexts[35],
	&fpgad_xfpga_Error_contexts[36],
	&fpgad_xfpga_Error_contexts[37],
	&fpgad_xfpga_Error_contexts[38],
	&fpgad_xfpga_Error_contexts[39],
	&fpgad_xfpga_Error_contexts[40],
	&fpgad_xfpga_Error_contexts[41],
	&fpgad_xfpga_Error_contexts[42],
	&fpgad_xfpga_Error_contexts[43],
	&fpgad_xfpga_Error_contexts[44],
	&fpgad_xfpga_Error_contexts[45],
	&fpgad_xfpga_Error_contexts[46],
	&fpgad_xfpga_Error_contexts[47],
	&fpgad_xfpga_Error_contexts[48],
	&fpgad_xfpga_Error_contexts[49],
	&fpgad_xfpga_Error_contexts[50],

	NULL
};

// Port responses
STATIC fpgad_respond_event_t fpgad_xfpga_port_responses[] = {
	fpgad_xfpga_respond_AP1_or_AP2,
	fpgad_xfpga_respond_AP1_or_AP2,
	fpgad_xfpga_respond_PowerStateChange,

	fpgad_xfpga_respond_LogError, //  0
	fpgad_xfpga_respond_AP6_and_Null_GBS, // 1
	fpgad_xfpga_respond_LogError, //  2
	fpgad_xfpga_respond_LogError, //  3
	fpgad_xfpga_respond_LogError, //  4
	fpgad_xfpga_respond_LogError, //  5
	fpgad_xfpga_respond_LogError, //  6
	fpgad_xfpga_respond_LogError, //  7
	fpgad_xfpga_respond_LogError, //  8
	fpgad_xfpga_respond_LogError, //  9
	fpgad_xfpga_respond_LogError, // 10
	fpgad_xfpga_respond_LogError, // 11
	fpgad_xfpga_respond_LogError, // 12
	fpgad_xfpga_respond_LogError, // 13
	fpgad_xfpga_respond_LogError, // 14
	fpgad_xfpga_respond_LogError, // 15
	fpgad_xfpga_respond_LogError, // 16
	fpgad_xfpga_respond_LogError, // 17
	fpgad_xfpga_respond_LogError, // 18
	fpgad_xfpga_respond_LogError, // 19
	fpgad_xfpga_respond_LogError, // 20
	fpgad_xfpga_respond_LogError, // 21
	fpgad_xfpga_respond_LogError, // 22
	fpgad_xfpga_respond_LogError, // 23
	fpgad_xfpga_respond_LogError, // 24
	fpgad_xfpga_respond_LogError, // 25
	fpgad_xfpga_respond_LogError, // 26
	fpgad_xfpga_respond_LogError, // 27
	fpgad_xfpga_respond_LogError, // 28
	fpgad_xfpga_respond_LogError, // 29
	fpgad_xfpga_respond_LogError, // 30

	fpgad_xfpga_respond_LogError, // 31
	fpgad_xfpga_respond_LogError, // 32
	fpgad_xfpga_respond_LogError, // 33
	fpgad_xfpga_respond_LogError, // 34
	fpgad_xfpga_respond_LogError, // 35
	fpgad_xfpga_respond_LogError, // 36
	fpgad_xfpga_respond_LogError, // 37
	fpgad_xfpga_respond_LogError, // 38
	fpgad_xfpga_respond_LogError, // 39
	fpgad_xfpga_respond_LogError, // 40
	fpgad_xfpga_respond_LogError, // 41
	fpgad_xfpga_respond_LogError, // 42
	fpgad_xfpga_respond_LogError, // 43
	fpgad_xfpga_respond_LogError, // 44
	fpgad_xfpga_respond_LogError, // 45
	fpgad_xfpga_respond_LogError, // 46
	fpgad_xfpga_respond_LogError, // 47
	fpgad_xfpga_respond_LogError, // 48
	fpgad_xfpga_respond_LogError, // 49
	fpgad_xfpga_respond_LogError, // 50

	NULL
};

STATIC void *fpgad_xfpga_port_response_contexts[] = {
	&fpgad_xfpga_AP_contexts[0],
	&fpgad_xfpga_AP_contexts[1],
	&fpgad_xfpga_AP_contexts[2],

	&fpgad_xfpga_Error_contexts[0],
	&fpgad_xfpga_Error_contexts[1],
	&fpgad_xfpga_Error_contexts[2],
	&fpgad_xfpga_Error_contexts[3],
	&fpgad_xfpga_Error_contexts[4],
	&fpgad_xfpga_Error_contexts[5],
	&fpgad_xfpga_Error_contexts[6],
	&fpgad_xfpga_Error_contexts[7],
	&fpgad_xfpga_Error_contexts[8],
	&fpgad_xfpga_Error_contexts[9],
	&fpgad_xfpga_Error_contexts[10],
	&fpgad_xfpga_Error_contexts[11],
	&fpgad_xfpga_Error_contexts[12],
	&fpgad_xfpga_Error_contexts[13],
	&fpgad_xfpga_Error_contexts[14],
	&fpgad_xfpga_Error_contexts[15],
	&fpgad_xfpga_Error_contexts[16],
	&fpgad_xfpga_Error_contexts[17],
	&fpgad_xfpga_Error_contexts[18],
	&fpgad_xfpga_Error_contexts[19],
	&fpgad_xfpga_Error_contexts[20],
	&fpgad_xfpga_Error_contexts[21],
	&fpgad_xfpga_Error_contexts[22],
	&fpgad_xfpga_Error_contexts[23],
	&fpgad_xfpga_Error_contexts[24],
	&fpgad_xfpga_Error_contexts[25],
	&fpgad_xfpga_Error_contexts[26],
	&fpgad_xfpga_Error_contexts[27],
	&fpgad_xfpga_Error_contexts[28],
	&fpgad_xfpga_Error_contexts[29],
	&fpgad_xfpga_Error_contexts[30],

	&fpgad_xfpga_Error_contexts[31],
	&fpgad_xfpga_Error_contexts[32],
	&fpgad_xfpga_Error_contexts[33],
	&fpgad_xfpga_Error_contexts[34],
	&fpgad_xfpga_Error_contexts[35],
	&fpgad_xfpga_Error_contexts[36],
	&fpgad_xfpga_Error_contexts[37],
	&fpgad_xfpga_Error_contexts[38],
	&fpgad_xfpga_Error_contexts[39],
	&fpgad_xfpga_Error_contexts[40],
	&fpgad_xfpga_Error_contexts[41],
	&fpgad_xfpga_Error_contexts[42],
	&fpgad_xfpga_Error_contexts[43],
	&fpgad_xfpga_Error_contexts[44],
	&fpgad_xfpga_Error_contexts[45],
	&fpgad_xfpga_Error_contexts[46],
	&fpgad_xfpga_Error_contexts[47],
	&fpgad_xfpga_Error_contexts[48],
	&fpgad_xfpga_Error_contexts[49],
	&fpgad_xfpga_Error_contexts[50],

	NULL
};

// FME detections
STATIC fpgad_detect_event_t fpgad_xfpga_fme_detections[] = {
	fpgad_xfpga_detect_Error, // 51
	fpgad_xfpga_detect_Error, // 52
	fpgad_xfpga_detect_Error, // 53
	fpgad_xfpga_detect_Error, // 54
	fpgad_xfpga_detect_Error, // 55
	fpgad_xfpga_detect_Error, // 56
	fpgad_xfpga_detect_Error, // 57
	fpgad_xfpga_detect_Error, // 58
	fpgad_xfpga_detect_Error, // 59

	fpgad_xfpga_detect_Error, // 60
	fpgad_xfpga_detect_Error, // 61
	fpgad_xfpga_detect_Error, // 62
	fpgad_xfpga_detect_Error, // 63
	fpgad_xfpga_detect_Error, // 64
	fpgad_xfpga_detect_Error, // 65
	fpgad_xfpga_detect_Error, // 66
	fpgad_xfpga_detect_Error, // 67
	fpgad_xfpga_detect_Error, // 68
	fpgad_xfpga_detect_Error, // 69
	fpgad_xfpga_detect_Error, // 70
	fpgad_xfpga_detect_Error, // 71

	fpgad_xfpga_detect_Error, // 72
	fpgad_xfpga_detect_Error, // 73
	fpgad_xfpga_detect_Error, // 74
	fpgad_xfpga_detect_Error, // 75
	fpgad_xfpga_detect_Error, // 76
	fpgad_xfpga_detect_Error, // 77
	fpgad_xfpga_detect_Error, // 78
	fpgad_xfpga_detect_Error, // 79
	fpgad_xfpga_detect_Error, // 80
	fpgad_xfpga_detect_Error, // 81

	fpgad_xfpga_detect_Error, // 82
	fpgad_xfpga_detect_Error, // 83
	fpgad_xfpga_detect_Error, // 84
	fpgad_xfpga_detect_Error, // 85
	fpgad_xfpga_detect_Error, // 86
	fpgad_xfpga_detect_Error, // 87
	fpgad_xfpga_detect_Error, // 88
	fpgad_xfpga_detect_Error, // 89
	fpgad_xfpga_detect_Error, // 90
	fpgad_xfpga_detect_Error, // 91
	fpgad_xfpga_detect_Error, // 92

	fpgad_xfpga_detect_Error, // 93
	fpgad_xfpga_detect_Error, // 94
	fpgad_xfpga_detect_Error, // 95
	fpgad_xfpga_detect_Error, // 96
	fpgad_xfpga_detect_Error, // 97
	fpgad_xfpga_detect_Error, // 98
	fpgad_xfpga_detect_Error, // 99
	fpgad_xfpga_detect_Error, //100
	fpgad_xfpga_detect_Error, //101
	fpgad_xfpga_detect_Error, //102
	fpgad_xfpga_detect_Error, //103
	fpgad_xfpga_detect_Error, //104

	NULL
};

STATIC void *fpgad_xfpga_fme_detection_contexts[] = {
	&fpgad_xfpga_Error_contexts[51],
	&fpgad_xfpga_Error_contexts[52],
	&fpgad_xfpga_Error_contexts[53],
	&fpgad_xfpga_Error_contexts[54],
	&fpgad_xfpga_Error_contexts[55],
	&fpgad_xfpga_Error_contexts[56],
	&fpgad_xfpga_Error_contexts[57],
	&fpgad_xfpga_Error_contexts[58],
	&fpgad_xfpga_Error_contexts[59],

	&fpgad_xfpga_Error_contexts[60],
	&fpgad_xfpga_Error_contexts[61],
	&fpgad_xfpga_Error_contexts[62],
	&fpgad_xfpga_Error_contexts[63],
	&fpgad_xfpga_Error_contexts[64],
	&fpgad_xfpga_Error_contexts[65],
	&fpgad_xfpga_Error_contexts[66],
	&fpgad_xfpga_Error_contexts[67],
	&fpgad_xfpga_Error_contexts[68],
	&fpgad_xfpga_Error_contexts[69],
	&fpgad_xfpga_Error_contexts[70],
	&fpgad_xfpga_Error_contexts[71],

	&fpgad_xfpga_Error_contexts[72],
	&fpgad_xfpga_Error_contexts[73],
	&fpgad_xfpga_Error_contexts[74],
	&fpgad_xfpga_Error_contexts[75],
	&fpgad_xfpga_Error_contexts[76],
	&fpgad_xfpga_Error_contexts[77],
	&fpgad_xfpga_Error_contexts[78],
	&fpgad_xfpga_Error_contexts[79],
	&fpgad_xfpga_Error_contexts[80],
	&fpgad_xfpga_Error_contexts[81],

	&fpgad_xfpga_Error_contexts[82],
	&fpgad_xfpga_Error_contexts[83],
	&fpgad_xfpga_Error_contexts[84],
	&fpgad_xfpga_Error_contexts[85],
	&fpgad_xfpga_Error_contexts[86],
	&fpgad_xfpga_Error_contexts[87],
	&fpgad_xfpga_Error_contexts[88],
	&fpgad_xfpga_Error_contexts[89],
	&fpgad_xfpga_Error_contexts[90],
	&fpgad_xfpga_Error_contexts[91],
	&fpgad_xfpga_Error_contexts[92],

	&fpgad_xfpga_Error_contexts[93],
	&fpgad_xfpga_Error_contexts[94],
	&fpgad_xfpga_Error_contexts[95],
	&fpgad_xfpga_Error_contexts[96],
	&fpgad_xfpga_Error_contexts[97],
	&fpgad_xfpga_Error_contexts[98],
	&fpgad_xfpga_Error_contexts[99],
	&fpgad_xfpga_Error_contexts[100],
	&fpgad_xfpga_Error_contexts[101],
	&fpgad_xfpga_Error_contexts[102],
	&fpgad_xfpga_Error_contexts[103],
	&fpgad_xfpga_Error_contexts[104],

	NULL
};

// FME responses
STATIC fpgad_respond_event_t fpgad_xfpga_fme_responses[] = {
	fpgad_xfpga_respond_LogError, // 51
	fpgad_xfpga_respond_LogError, // 52
	fpgad_xfpga_respond_LogError, // 53
	fpgad_xfpga_respond_LogError, // 54
	fpgad_xfpga_respond_LogError, // 55
	fpgad_xfpga_respond_LogError, // 56
	fpgad_xfpga_respond_LogError, // 57
	fpgad_xfpga_respond_LogError, // 58
	fpgad_xfpga_respond_LogError, // 59

	fpgad_xfpga_respond_LogError, // 60
	fpgad_xfpga_respond_LogError, // 61
	fpgad_xfpga_respond_LogError, // 62
	fpgad_xfpga_respond_LogError, // 63
	fpgad_xfpga_respond_LogError, // 64
	fpgad_xfpga_respond_LogError, // 65
	fpgad_xfpga_respond_LogError, // 66
	fpgad_xfpga_respond_LogError, // 67
	fpgad_xfpga_respond_LogError, // 68
	fpgad_xfpga_respond_LogError, // 69
	fpgad_xfpga_respond_LogError, // 70
	fpgad_xfpga_respond_LogError, // 71

	fpgad_xfpga_respond_LogError, // 72
	fpgad_xfpga_respond_LogError, // 73
	fpgad_xfpga_respond_LogError, // 74
	fpgad_xfpga_respond_LogError, // 75
	fpgad_xfpga_respond_LogError, // 76
	fpgad_xfpga_respond_LogError, // 77
	fpgad_xfpga_respond_LogError, // 78
	fpgad_xfpga_respond_LogError, // 79
	fpgad_xfpga_respond_LogError, // 80
	fpgad_xfpga_respond_LogError, // 81

	fpgad_xfpga_respond_LogError, // 82
	fpgad_xfpga_respond_LogError, // 83
	fpgad_xfpga_respond_LogError, // 84
	fpgad_xfpga_respond_AP6,      // 85
	fpgad_xfpga_respond_LogError, // 86
	fpgad_xfpga_respond_LogError, // 87
	fpgad_xfpga_respond_LogError, // 88
	fpgad_xfpga_respond_LogError, // 89
	fpgad_xfpga_respond_LogError, // 90
	fpgad_xfpga_respond_LogError, // 91
	fpgad_xfpga_respond_LogError, // 92

	fpgad_xfpga_respond_LogError, // 93
	fpgad_xfpga_respond_LogError, // 94
	fpgad_xfpga_respond_LogError, // 95
	fpgad_xfpga_respond_LogError, // 96
	fpgad_xfpga_respond_LogError, // 97
	fpgad_xfpga_respond_LogError, // 98
	fpgad_xfpga_respond_LogError, // 99
	fpgad_xfpga_respond_LogError, //100
	fpgad_xfpga_respond_LogError, //101
	fpgad_xfpga_respond_LogError, //102
	fpgad_xfpga_respond_LogError, //103
	fpgad_xfpga_respond_LogError, //104

	NULL
};

STATIC void *fpgad_xfpga_fme_response_contexts[] = {
	&fpgad_xfpga_Error_contexts[51],
	&fpgad_xfpga_Error_contexts[52],
	&fpgad_xfpga_Error_contexts[53],
	&fpgad_xfpga_Error_contexts[54],
	&fpgad_xfpga_Error_contexts[55],
	&fpgad_xfpga_Error_contexts[56],
	&fpgad_xfpga_Error_contexts[57],
	&fpgad_xfpga_Error_contexts[58],
	&fpgad_xfpga_Error_contexts[59],

	&fpgad_xfpga_Error_contexts[60],
	&fpgad_xfpga_Error_contexts[61],
	&fpgad_xfpga_Error_contexts[62],
	&fpgad_xfpga_Error_contexts[63],
	&fpgad_xfpga_Error_contexts[64],
	&fpgad_xfpga_Error_contexts[65],
	&fpgad_xfpga_Error_contexts[66],
	&fpgad_xfpga_Error_contexts[67],
	&fpgad_xfpga_Error_contexts[68],
	&fpgad_xfpga_Error_contexts[69],
	&fpgad_xfpga_Error_contexts[70],
	&fpgad_xfpga_Error_contexts[71],

	&fpgad_xfpga_Error_contexts[72],
	&fpgad_xfpga_Error_contexts[73],
	&fpgad_xfpga_Error_contexts[74],
	&fpgad_xfpga_Error_contexts[75],
	&fpgad_xfpga_Error_contexts[76],
	&fpgad_xfpga_Error_contexts[77],
	&fpgad_xfpga_Error_contexts[78],
	&fpgad_xfpga_Error_contexts[79],
	&fpgad_xfpga_Error_contexts[80],
	&fpgad_xfpga_Error_contexts[81],

	&fpgad_xfpga_Error_contexts[82],
	&fpgad_xfpga_Error_contexts[83],
	&fpgad_xfpga_Error_contexts[84],
	&fpgad_xfpga_Error_contexts[85],
	&fpgad_xfpga_Error_contexts[86],
	&fpgad_xfpga_Error_contexts[87],
	&fpgad_xfpga_Error_contexts[88],
	&fpgad_xfpga_Error_contexts[89],
	&fpgad_xfpga_Error_contexts[90],
	&fpgad_xfpga_Error_contexts[91],
	&fpgad_xfpga_Error_contexts[92],

	&fpgad_xfpga_Error_contexts[93],
	&fpgad_xfpga_Error_contexts[94],
	&fpgad_xfpga_Error_contexts[95],
	&fpgad_xfpga_Error_contexts[96],
	&fpgad_xfpga_Error_contexts[97],
	&fpgad_xfpga_Error_contexts[98],
	&fpgad_xfpga_Error_contexts[99],
	&fpgad_xfpga_Error_contexts[100],
	&fpgad_xfpga_Error_contexts[101],
	&fpgad_xfpga_Error_contexts[102],
	&fpgad_xfpga_Error_contexts[103],
	&fpgad_xfpga_Error_contexts[104],

	NULL
};

int fpgad_plugin_configure(fpgad_monitored_device *d,
			   const char *cfg)
{
	UNUSED_PARAM(cfg);

	LOG("monitoring vid=0x%04x did=0x%04x objid=0x%x (%s)\n",
			d->supported->vendor_id,
			d->supported->device_id,
			d->object_id,
			d->object_type == FPGA_ACCELERATOR ?
			"accelerator" : "device");

	d->type = FPGAD_PLUGIN_TYPE_CALLBACK;

	if (d->object_type == FPGA_ACCELERATOR) {
		d->detections = fpgad_xfpga_port_detections;
		d->detection_contexts = fpgad_xfpga_port_detection_contexts;
		d->responses = fpgad_xfpga_port_responses;
		d->response_contexts = fpgad_xfpga_port_response_contexts;
	} else {
		d->detections = fpgad_xfpga_fme_detections;
		d->detection_contexts = fpgad_xfpga_fme_detection_contexts;
		d->responses = fpgad_xfpga_fme_responses;
		d->response_contexts = fpgad_xfpga_fme_response_contexts;
	}

	return 0;
}

void fpgad_plugin_destroy(fpgad_monitored_device *d)
{
	LOG("stop monitoring vid=0x%04x did=0x%04x objid=0x%x (%s)\n",
			d->supported->vendor_id,
			d->supported->device_id,
			d->object_id,
			d->object_type == FPGA_ACCELERATOR ?
			"accelerator" : "device");
}
