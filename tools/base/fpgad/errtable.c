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

/*
 * errtable.c : list of errors to monitor.
 */

#include "errtable.h"
#include "evt.h"
#include "log.h"
#include "config_int.h"

#include "safe_string/safe_string.h"

#define PORT_ERR(fil, field, lo, hi)        { fil, field, lo, hi, evt_notify_error        }
#define  FME_ERR(fil, field, lo, hi)        { fil, field, lo, hi, evt_notify_error        }
#define  AP6_ERR(fil, field, lo, hi)        { fil, field, lo, hi, evt_notify_ap6          }
#define  AP6_NULL_ERR(fil, field, lo, hi)   { fil, field, lo, hi, evt_notify_ap6_and_null }

struct fpga_err mcp_port_error_table_rev_1[] = {
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].VfFlrAccessError",                   51, 51),
    AP6_NULL_ERR("errors/errors", "PORT_ERROR[0x1010].Ap6Event",                           50, 50),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].PMRError",                           49, 49),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].PageFault",                          48, 48),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].VgaMemRangeError",                   47, 47),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].LegRangeHighError",                  46, 46),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].LegRangeLowError",                   45, 45),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].GenProtRangeError",                  44, 44),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].L1prMesegError",                     43, 43),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].L1prSmrr2Error",                     42, 42),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].L1prSmrrError",                      41, 41),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxReqCounterOverflow",               40, 40),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].UnexpMMIOResp",                      34, 34),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh2FifoOverflow",                  33, 33),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].MMIOTimedOut",                       32, 32),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh1NonZeroSOP",                    24, 24),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh1IncorrectAddr",                 23, 23),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh1DataPayloadOverrun",            22, 22),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh1InsufficientData",              21, 21),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh1Len4NotAligned",                20, 20),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh1Len2NotAligned",                19, 19),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh1Len3NotSupported",              18, 18),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh1InvalidReqEncoding",            17, 17),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh1Overflow",                      16, 16),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].MMIOWrWhileRst",                     10, 10),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].MMIORdWhileRst",                      9,  9),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh0Len4NotAligned",                 4,  4),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh0Len2NotAligned",                 3,  3),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh0Len3NotSupported",               2,  2),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh0InvalidReqEncoding",             1,  1),
	PORT_ERR("errors/errors", "PORT_ERROR[0x1010].TxCh0Overflow",                       0,  0),

	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxReqCounterOverflow",    40, 40),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh2FifoOverflow",       33, 33),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].MMIOTimedOut",            32, 32),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1IllegalVCsel",       25, 25),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1NonZeroSOP",         24, 24),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1IncorrectAddr",      23, 23),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1DataPayloadOverrun", 22, 22),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1InsufficientData",   21, 21),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Len4NotAligned",     20, 20),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Len2NotAligned",     19, 19),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Len3NotSupported",   18, 18),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1InvalidReqEncoding", 17, 17),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Overflow",           16, 16),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].MMIOWrWhileRst",          10, 10),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].MMIORdWhileRst",           9,  9),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Len4NotAligned",      4,  4),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Len2NotAligned",      3,  3),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Len3NotSupported",    2,  2),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0InvalidReqEncoding",  1,  1),
	PORT_ERR("errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Overflow",            0,  0),

	TABLE_TERMINATOR
};

struct fpga_err mcp_fme_error_table_rev_1[] = {
	FME_ERR("errors/fme-errors/errors", "FME_ERROR0[0x4010].CvlCdcParErro0",           17, 19),
	FME_ERR("errors/fme-errors/errors", "FME_ERROR0[0x4010].Pcie1CdcParErr",           12, 16),
	FME_ERR("errors/fme-errors/errors", "FME_ERROR0[0x4010].Pcie0CdcParErr",            7, 11),
	FME_ERR("errors/fme-errors/errors", "FME_ERROR0[0x4010].MBPErr",                    6,  6),
	FME_ERR("errors/fme-errors/errors", "FME_ERROR0[0x4010].AfuAccessModeErr",          5,  5),
	FME_ERR("errors/fme-errors/errors", "FME_ERROR0[0x4010].IommuParityErr",            4,  4),
	FME_ERR("errors/fme-errors/errors", "FME_ERROR0[0x4010].KtiCdcParityErr",           2,  3),
	FME_ERR("errors/fme-errors/errors", "FME_ERROR0[0x4010].FabricFifoUOflow",          1,  1),
	FME_ERR("errors/fme-errors/errors", "FME_ERROR0[0x4010].FabricErr",                 0,  0),

	FME_ERR("errors/pcie0_errors", "PCIE0_ERROR[0x4020].FunctTypeErr",                 63, 63),
	FME_ERR("errors/pcie0_errors", "PCIE0_ERROR[0x4020].VFNumb",                       62, 62),
	FME_ERR("errors/pcie0_errors", "PCIE0_ERROR[0x4020].RxPoisonTlpErr",                9,  9),
	FME_ERR("errors/pcie0_errors", "PCIE0_ERROR[0x4020].ParityErr",                     8,  8),
	FME_ERR("errors/pcie0_errors", "PCIE0_ERROR[0x4020].CompTimeOutErr",                7,  7),
	FME_ERR("errors/pcie0_errors", "PCIE0_ERROR[0x4020].CompStatErr",                   6,  6),
	FME_ERR("errors/pcie0_errors", "PCIE0_ERROR[0x4020].CompTagErr",                    5,  5),
	FME_ERR("errors/pcie0_errors", "PCIE0_ERROR[0x4020].MRLengthErr",                   4,  4),
	FME_ERR("errors/pcie0_errors", "PCIE0_ERROR[0x4020].MRAddrErr",                     3,  3),
	FME_ERR("errors/pcie0_errors", "PCIE0_ERROR[0x4020].MWLengthErr",                   2,  2),
	FME_ERR("errors/pcie0_errors", "PCIE0_ERROR[0x4020].MWAddrErr",                     1,  1),
	FME_ERR("errors/pcie0_errors", "PCIE0_ERROR[0x4020].FormatTypeErr",                 0,  0),

	FME_ERR("errors/pcie1_errors", "PCIE1_ERROR[0x4030].RxPoisonTlpErr",                9,  9),
	FME_ERR("errors/pcie1_errors", "PCIE1_ERROR[0x4030].ParityErr",                     8,  8),
	FME_ERR("errors/pcie1_errors", "PCIE1_ERROR[0x4030].CompTimeOutErr",                7,  7),
	FME_ERR("errors/pcie1_errors", "PCIE1_ERROR[0x4030].CompStatErr",                   6,  6),
	FME_ERR("errors/pcie1_errors", "PCIE1_ERROR[0x4030].CompTagErr",                    5,  5),
	FME_ERR("errors/pcie1_errors", "PCIE1_ERROR[0x4030].MRLengthErr",                   4,  4),
	FME_ERR("errors/pcie1_errors", "PCIE1_ERROR[0x4030].MRAddrErr",                     3,  3),
	FME_ERR("errors/pcie1_errors", "PCIE1_ERROR[0x4030].MWLengthErr",                   2,  2),
	FME_ERR("errors/pcie1_errors", "PCIE1_ERROR[0x4030].MWAddrErr",                     1,  1),
	FME_ERR("errors/pcie1_errors", "PCIE1_ERROR[0x4030].FormatTypeErr",                 0,  0),

	FME_ERR("errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].MBPErr",            12, 12),
	FME_ERR("errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PowerThreshAP2",    11, 11),
	FME_ERR("errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PowerThreshAP1",    10, 10),
	AP6_ERR("errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP6",      9,  9),
	FME_ERR("errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].InjectedWarningErr", 6,  6),
	FME_ERR("errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].AfuAccessModeErr",   5,  5),
	FME_ERR("errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].ProcHot",            4,  4),
	FME_ERR("errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PortFatalErr",       3,  3),
	FME_ERR("errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PcieError",          2,  2),
	FME_ERR("errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP2",      1,  1),
	FME_ERR("errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP1",      0,  0),

	FME_ERR("errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].InjectedCatastErr",  11, 11),
	FME_ERR("errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].ThermCatastErr",     10, 10),
	FME_ERR("errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].CrcCatastErr",        9,  9),
	FME_ERR("errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].InjectedFatalErr",    8,  8),
	FME_ERR("errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].PciePoisonErr",       7,  7),
	FME_ERR("errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].FabricFatalErr",      6,  6),
	FME_ERR("errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].IommuFatalErr",       5,  5),
	FME_ERR("errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].DramFatalErr",        4,  4),
	FME_ERR("errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].KtiProtoFatalErr",    3,  3),
	FME_ERR("errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].CciFatalErr",         2,  2),
	FME_ERR("errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].TagCchFatalErr",      1,  1),
	FME_ERR("errors/catfatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].KtiLinkFatalErr",     0,  0),

	TABLE_TERMINATOR
};

supported_device supported_port_devices[] = {
	{ 0x8086, 0xbcc0, 1, mcp_port_error_table_rev_1 },
	{ 0x8086, 0xbcc1, 1, mcp_port_error_table_rev_1 },
};

supported_device supported_fme_devices[] = {
	{ 0x8086, 0xbcc0, 1, mcp_fme_error_table_rev_1 },
	{ 0x8086, 0xbcc1, 1, mcp_fme_error_table_rev_1 },
};

monitored_device *monitored_device_list;

monitored_device * add_monitored_device(fpga_token token,
					uint8_t socket_id,
					uint64_t object_id,
					supported_device *device)
{
	monitored_device *md = malloc(sizeof(monitored_device));

	if (md) {
		md->token = token;
		md->socket_id = socket_id;
		md->object_id = object_id;
		md->device = device;
		md->num_error_occurrences = 0;

		/* Add it to the list */
		md->next = monitored_device_list;
		monitored_device_list = md;
	}
	
	return md;
}

/*
** Use the supported_port_devices and supported_fme_devices
** tables to determine whether the Port/FME represented by
** token should be monitored for events. If so, then add
** an entry to monitored_device_list.
*/
void consider_device(fpga_token token)
{
	uint32_t i;
	uint16_t vendor_id;
	uint16_t device_id;
	uint64_t error_revision;
	uint8_t socket_id;
	uint64_t object_id;
	fpga_objtype objtype;
	fpga_properties props = NULL;
	fpga_object rev_obj = NULL;
	fpga_result res;
	bool added;

	res = fpgaGetProperties(token, &props);
	if (res != FPGA_OK) {
		dlog("logger: failed to get properties\n");
		return;
	}

	vendor_id = 0;
	res = fpgaPropertiesGetVendorID(props, &vendor_id);
	if (res != FPGA_OK) {
		dlog("logger: failed to get vendor ID\n");
		goto out_destroy_props;
	}

	device_id = 0;
	res = fpgaPropertiesGetDeviceID(props, &device_id);
	if (res != FPGA_OK) {
		dlog("logger: failed to get device ID\n");
		goto out_destroy_props;
	}

	objtype = FPGA_DEVICE;
	res = fpgaPropertiesGetObjectType(props, &objtype);
	if (res != FPGA_OK) {
		dlog("logger: failed to get object type\n");
		goto out_destroy_props;
	}

	socket_id = 0;
	res = fpgaPropertiesGetSocketID(props, &socket_id);
	if (res != FPGA_OK) {
		dlog("logger: failed to get socket ID\n");
		goto out_destroy_props;
	}

	object_id = 0;
	res = fpgaPropertiesGetObjectID(props, &object_id);
	if (res != FPGA_OK) {
		dlog("logger: failed to get object ID\n");
		goto out_destroy_props;
	}

	res = fpgaTokenGetObject(token, "errors/revision",
				 &rev_obj, 0);
	if (res != FPGA_OK) {
		dlog("logger: failed to determine errors revision\n");
		goto out_destroy_props;
	}

	error_revision = 0;
	res = fpgaObjectRead64(rev_obj, &error_revision, 0);
	if (res != FPGA_OK) {
		dlog("logger: failed to read errors revision value\n");
		goto out_destroy_obj;
	}

	added = false;

	if (objtype == FPGA_ACCELERATOR) {
		/* Determine if the Port matches */
		for (i = 0 ;
		      i < sizeof(supported_port_devices) / sizeof(supported_port_devices[0]) ;
		       ++i) {
			supported_device *d = &supported_port_devices[i];

			if ((vendor_id == d->vendor_id) &&
			    (device_id == d->device_id) &&
			    (error_revision == d->error_revision)) {

				if (add_monitored_device(token,
							 socket_id,
							 object_id,
							 d)) {
					added = true;
				}
			}

		}

		if (added) {
			dlog("logger: monitoring Port device 0x%04x:0x%04x on socket %d\n",
				vendor_id, device_id, (int) socket_id);
		} else {
			dlog("logger: skipping unsupported Port device 0x%04x:0x%04x\n",
				vendor_id, device_id);
		}
	} else {
		/* Determine if the FME matches */
		for (i = 0 ;
		      i < sizeof(supported_fme_devices) / sizeof(supported_fme_devices[0]) ;
		       ++i) {
			supported_device *d = &supported_fme_devices[i];

			if ((vendor_id == d->vendor_id) &&
			    (device_id == d->device_id) &&
			    (error_revision == d->error_revision)) {

				if (add_monitored_device(token,
							 socket_id,
							 object_id,
							 d)) {
					added = true;
				}
			}

		}

		if (added) {
			dlog("logger: monitoring FME device 0x%04x:0x%04x on socket %d\n",
				vendor_id, device_id, (int) socket_id);
		} else {
			dlog("logger: skipping unsupported FME device 0x%04x:0x%04x\n",
				vendor_id, device_id);
		}
	}

out_destroy_obj:
	fpgaDestroyObject(&rev_obj);
out_destroy_props:
	fpgaDestroyProperties(&props);
}

bool error_already_occurred(monitored_device *d, struct fpga_err *e)
{
	uint32_t i;
	for (i = 0 ; i < d->num_error_occurrences ; ++i) {
		if (d->error_occurrences[i] == e)
			return true;
	}
	return false;
}

void error_just_occurred(monitored_device *d, struct fpga_err *e)
{
	if (d->num_error_occurrences == MAX_ERROR_COUNT) {
		dlog("logger: internal error - exceeded MAX_ERROR_COUNT\n");
	} else {
		d->error_occurrences[d->num_error_occurrences++] = e;
	}
}

void clear_occurrences_of(monitored_device *d, struct fpga_err *e)
{
	uint32_t i, j;
	uint32_t count = 0;
	for (i = j = 0 ; i < d->num_error_occurrences ; ++i) {
		if (d->error_occurrences[i] != e)
			d->error_occurrences[j++] = d->error_occurrences[i];
		else
			++count;
	}
	d->num_error_occurrences -= count;
}

int log_fpga_error(monitored_device *d, struct fpga_err *e)
{
	if (error_already_occurred(d, e))
		return 0;

	error_just_occurred(d, e);

	dlog("socket %d, object 0x%" PRIx64 ": %s\n",
		d->socket_id, d->object_id, e->reg_field);

	if (e->callback)
		e->callback(d->socket_id,
			    d->object_id,
			    e);

	return 1;
}

int poll_error(monitored_device *d, struct fpga_err *e)
{
	int i;
	int count = 0;

	uint64_t err = 0;
	uint64_t mask;

	fpga_result res;
	fpga_object err_obj = NULL;

	res = fpgaTokenGetObject(d->token, e->sysfsfile, &err_obj, 0);
	if (res != FPGA_OK) {
		dlog("logger: failed to get error object\n");
		return -1;
	}

	res = fpgaObjectRead64(err_obj, &err, 0);
	if (res != FPGA_OK) {
		dlog("logger: failed to read error object\n");
		fpgaDestroyObject(&err_obj);
		return -1;
	}

	fpgaDestroyObject(&err_obj);

	mask = 0;
	for (i = e->lowbit ; i <= e->highbit ; ++i)
		mask |= 1ULL << i;

	if (err & mask) {
		count += log_fpga_error(d, e);
	} else {
		clear_occurrences_of(d, e);
	}

	return count;
}

/*
 * Poll all known error registers
 *
 * @returns number of (new) errors that were found
 */
int poll_errors(monitored_device *d)
{
	unsigned i;
	int errors = 0;
	int res;

	for (i = 0 ; d->device->error_table[i].sysfsfile ; ++i) {
		res = poll_error(d, &d->device->error_table[i]);
		if (-1 == res)
			return res;
		errors += res;
	}

	return errors;
}

void *logger_thread(void *thread_context)
{
	struct config *c = (struct config *)thread_context;

	fpga_result res;
	uint32_t i;
	uint32_t num_matches = 0;
	fpga_token *tokens = NULL;
	monitored_device *d;

	monitored_device_list = NULL;

	dlog("logger: starting\n");

	/* Enumerate all devices */
	res = fpgaEnumerate(NULL, 0, NULL, 0, &num_matches);
	if (res != FPGA_OK) {
		dlog("logger: enumeration failed\n");
		goto out_exit;
	}

	if (!num_matches) {
		dlog("logger: no devices present. Nothing to do.\n");
		goto out_exit;
	}

	tokens = calloc(num_matches, sizeof(fpga_token));
	if (!tokens) {
		dlog("logger: calloc failed.\n");
		goto out_exit;
	}

	res = fpgaEnumerate(NULL, 0, tokens, num_matches, &num_matches);
	if (res != FPGA_OK) {
		dlog("logger: enumeration failed\n");
		free(tokens);
		goto out_exit;
	}

	/*
	** Determine if we support this device. If so,
        ** then add it to monitored_device_list.
	*/
	for (i = 0 ; i < num_matches ; ++i) {
		consider_device(tokens[i]);
	}

	if (!monitored_device_list) {
		dlog("logger: no matching devices\n");
		goto out_destroy_tokens;
	}

	while (c->running) {

		for (d = monitored_device_list ; d ; d = d->next) {
			if (poll_errors(d) < 0) {
				dlog("logger: error polling errors. Aborting!\n");
				goto out_destroy_list;
			}
		}

		usleep(c->poll_interval_usec);
	}

out_destroy_list:
	for (d = monitored_device_list ; d ; ) {
		monitored_device *trash = d;
		d = d->next;
		free(trash);
	}
	monitored_device_list = NULL;

out_destroy_tokens:
	for (i = 0 ; i < num_matches ; ++i) {
		fpgaDestroyToken(&tokens[i]);
	}
	free(tokens);

out_exit:
	dlog("logger: thread exiting\n");
	return NULL;
}
