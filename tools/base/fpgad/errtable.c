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

/*
 * errtable.c : list of errors to monitor.
 */

#include "errtable.h"
#include "evt.h"
#include "log.h"
#include "config_int.h"

#include "safe_string/safe_string.h"

//#define  ACCELERATOR_IRQ(fil, field, l, hi)
#define PORT_ERR(sock, fil, field, lo, hi)        { sock, fil, field, lo, hi, false, evt_notify_error        }
#define  FME_ERR(sock, fil, field, lo, hi)        { sock, fil, field, lo, hi, false, evt_notify_error        }
#define  AP6_ERR(sock, fil, field, lo, hi)        { sock, fil, field, lo, hi, false, evt_notify_ap6          }
#define  AP6_NULL_ERR(sock, fil, field, lo, hi)   { sock, fil, field, lo, hi, false, evt_notify_ap6_and_null }
#define TABLE_TERMINATOR                          { 0,    NULL, NULL, 0,  0,  false, NULL                    }

struct fpga_err port_error_table_rev_1[] = {

	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].VfFlrAccessError",                   51, 51),
    AP6_NULL_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].Ap6Event",                           50, 50),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].PMRError",                           49, 49),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].PageFault",                          48, 48),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].VgaMemRangeError",                   47, 47),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].LegRangeHighError",                  46, 46),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].LegRangeLowError",                   45, 45),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].GenProtRangeError",                  44, 44),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].L1prMesegError",                     43, 43),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].L1prSmrr2Error",                     42, 42),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].L1prSmrrError",                      41, 41),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxReqCounterOverflow",               40, 40),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].UnexpMMIOResp",                      34, 34),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh2FifoOverflow",                  33, 33),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].MMIOTimedOut",                       32, 32),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh1NonZeroSOP",                    24, 24),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh1IncorrectAddr",                 23, 23),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh1DataPayloadOverrun",            22, 22),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh1InsufficientData",              21, 21),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh1Len4NotAligned",                20, 20),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh1Len2NotAligned",                19, 19),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh1Len3NotSupported",              18, 18),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh1InvalidReqEncoding",            17, 17),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh1Overflow",                      16, 16),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].MMIOWrWhileRst",                     10, 10),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].MMIORdWhileRst",                      9,  9),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh0Len4NotAligned",                 4,  4),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh0Len2NotAligned",                 3,  3),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh0Len3NotSupported",               2,  2),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh0InvalidReqEncoding",             1,  1),
	PORT_ERR(0, SYSFS_PORT0 "/errors/errors", "PORT_ERROR[0x1010].TxCh0Overflow",                       0,  0),

	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].VfFlrAccessError",                   51, 51),
    AP6_NULL_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].Ap6Event",                           50, 50),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].PMRError",                           49, 49),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].PageFault",                          48, 48),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].VgaMemRangeError",                   47, 47),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].LegRangeHighError",                  46, 46),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].LegRangeLowError",                   45, 45),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].GenProtRangeError",                  44, 44),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].L1prMesegError",                     43, 43),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].L1prSmrr2Error",                     42, 42),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].L1prSmrrError",                      41, 41),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxReqCounterOverflow",               40, 40),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].UnexpMMIOResp",                      34, 34),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh2FifoOverflow",                  33, 33),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].MMIOTimedOut",                       32, 32),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh1NonZeroSOP",                    24, 24),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh1IncorrectAddr",                 23, 23),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh1DataPayloadOverrun",            22, 22),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh1InsufficientData",              21, 21),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh1Len4NotAligned",                20, 20),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh1Len2NotAligned",                19, 19),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh1Len3NotSupported",              18, 18),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh1InvalidReqEncoding",            17, 17),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh1Overflow",                      16, 16),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].MMIOWrWhileRst",                     10, 10),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].MMIORdWhileRst",                      9,  9),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh0Len4NotAligned",                 4,  4),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh0Len2NotAligned",                 3,  3),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh0Len3NotSupported",               2,  2),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh0InvalidReqEncoding",             1,  1),
	PORT_ERR(1, SYSFS_PORT1 "/errors/errors", "PORT_ERROR[0x1010].TxCh0Overflow",                       0,  0),



	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxReqCounterOverflow",    40, 40),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh2FifoOverflow",       33, 33),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].MMIOTimedOut",            32, 32),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1IllegalVCsel",       25, 25),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1NonZeroSOP",         24, 24),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1IncorrectAddr",      23, 23),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1DataPayloadOverrun", 22, 22),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1InsufficientData",   21, 21),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Len4NotAligned",     20, 20),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Len2NotAligned",     19, 19),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Len3NotSupported",   18, 18),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1InvalidReqEncoding", 17, 17),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Overflow",           16, 16),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].MMIOWrWhileRst",          10, 10),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].MMIORdWhileRst",           9,  9),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Len4NotAligned",      4,  4),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Len2NotAligned",      3,  3),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Len3NotSupported",    2,  2),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0InvalidReqEncoding",  1,  1),
	PORT_ERR(0, SYSFS_PORT0 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Overflow",            0,  0),

	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxReqCounterOverflow",    40, 40),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh2FifoOverflow",       33, 33),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].MMIOTimedOut",            32, 32),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1IllegalVCsel",       25, 25),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1NonZeroSOP",         24, 24),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1IncorrectAddr",      23, 23),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1DataPayloadOverrun", 22, 22),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1InsufficientData",   21, 21),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Len4NotAligned",     20, 20),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Len2NotAligned",     19, 19),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Len3NotSupported",   18, 18),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1InvalidReqEncoding", 17, 17),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh1Overflow",           16, 16),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].MMIOWrWhileRst",          10, 10),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].MMIORdWhileRst",           9,  9),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Len4NotAligned",      4,  4),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Len2NotAligned",      3,  3),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Len3NotSupported",    2,  2),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0InvalidReqEncoding",  1,  1),
	PORT_ERR(1, SYSFS_PORT1 "/errors/first_error", "PORT_FIRST_ERROR[0x1018].TxCh0Overflow",            0,  0),


	TABLE_TERMINATOR
};

struct fpga_err fme_error_table_rev_1[] = {
	FME_ERR(0, SYSFS_FME0 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].CvlCdcParErro0",           17, 19),
	FME_ERR(0, SYSFS_FME0 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].Pcie1CdcParErr",           12, 16),
	FME_ERR(0, SYSFS_FME0 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].Pcie0CdcParErr",            7, 11),
	FME_ERR(0, SYSFS_FME0 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].MBPErr",                    6,  6),
	FME_ERR(0, SYSFS_FME0 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].AfuAccessModeErr",          5,  5),
	FME_ERR(0, SYSFS_FME0 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].IommuParityErr",            4,  4),
	FME_ERR(0, SYSFS_FME0 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].KtiCdcParityErr",           2,  3),
	FME_ERR(0, SYSFS_FME0 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].FabricFifoUOflow",          1,  1),
	FME_ERR(0, SYSFS_FME0 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].FabricErr",                 0,  0),

	FME_ERR(1, SYSFS_FME1 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].CvlCdcParErro0",           17, 19),
	FME_ERR(1, SYSFS_FME1 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].Pcie1CdcParErr",           12, 16),
	FME_ERR(1, SYSFS_FME1 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].Pcie0CdcParErr",            7, 11),
	FME_ERR(1, SYSFS_FME1 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].MBPErr",                    6,  6),
	FME_ERR(1, SYSFS_FME1 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].AfuAccessModeErr",          5,  5),
	FME_ERR(1, SYSFS_FME1 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].IommuParityErr",            4,  4),
	FME_ERR(1, SYSFS_FME1 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].KtiCdcParityErr",           2,  3),
	FME_ERR(1, SYSFS_FME1 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].FabricFifoUOflow",          1,  1),
	FME_ERR(1, SYSFS_FME1 "/errors/fme-errors/errors", "FME_ERROR0[0x4010].FabricErr",                 0,  0),



	FME_ERR(0, SYSFS_FME0 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].FunctTypeErr",                 63, 63),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].VFNumb",                       62, 62),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].RxPoisonTlpErr",                9,  9),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].ParityErr",                     8,  8),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].CompTimeOutErr",                7,  7),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].CompStatErr",                   6,  6),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].CompTagErr",                    5,  5),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].MRLengthErr",                   4,  4),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].MRAddrErr",                     3,  3),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].MWLengthErr",                   2,  2),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].MWAddrErr",                     1,  1),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].FormatTypeErr",                 0,  0),

	FME_ERR(1, SYSFS_FME1 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].FunctTypeErr",                 63, 63),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].VFNumb",                       62, 62),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].RxPoisonTlpErr",                9,  9),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].ParityErr",                     8,  8),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].CompTimeOutErr",                7,  7),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].CompStatErr",                   6,  6),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].CompTagErr",                    5,  5),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].MRLengthErr",                   4,  4),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].MRAddrErr",                     3,  3),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].MWLengthErr",                   2,  2),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].MWAddrErr",                     1,  1),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie0_errors", "PCIE0_ERROR[0x4020].FormatTypeErr",                 0,  0),



	FME_ERR(0, SYSFS_FME0 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].RxPoisonTlpErr",                9,  9),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].ParityErr",                     8,  8),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].CompTimeOutErr",                7,  7),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].CompStatErr",                   6,  6),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].CompTagErr",                    5,  5),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].MRLengthErr",                   4,  4),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].MRAddrErr",                     3,  3),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].MWLengthErr",                   2,  2),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].MWAddrErr",                     1,  1),
	FME_ERR(0, SYSFS_FME0 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].FormatTypeErr",                 0,  0),

	FME_ERR(1, SYSFS_FME1 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].RxPoisonTlpErr",                9,  9),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].ParityErr",                     8,  8),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].CompTimeOutErr",                7,  7),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].CompStatErr",                   6,  6),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].CompTagErr",                    5,  5),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].MRLengthErr",                   4,  4),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].MRAddrErr",                     3,  3),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].MWLengthErr",                   2,  2),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].MWAddrErr",                     1,  1),
	FME_ERR(1, SYSFS_FME1 "/errors/pcie1_errors", "PCIE1_ERROR[0x4030].FormatTypeErr",                 0,  0),



	FME_ERR(0, SYSFS_FME0 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].MBPErr",            12, 12),
	FME_ERR(0, SYSFS_FME0 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PowerThreshAP2",    11, 11),
	FME_ERR(0, SYSFS_FME0 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PowerThreshAP1",    10, 10),
	AP6_ERR(0, SYSFS_FME0 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP6",      9,  9),
	FME_ERR(0, SYSFS_FME0 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].InjectedWarningErr", 6,  6),
	FME_ERR(0, SYSFS_FME0 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].AfuAccessModeErr",   5,  5),
	FME_ERR(0, SYSFS_FME0 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].ProcHot",            4,  4),
	FME_ERR(0, SYSFS_FME0 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PortFatalErr",       3,  3),
	FME_ERR(0, SYSFS_FME0 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PcieError",          2,  2),
	FME_ERR(0, SYSFS_FME0 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP2",      1,  1),
	FME_ERR(0, SYSFS_FME0 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP1",      0,  0),

	FME_ERR(1, SYSFS_FME1 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].MBPErr",            12, 12),
	FME_ERR(1, SYSFS_FME1 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PowerThreshAP2",    11, 11),
	FME_ERR(1, SYSFS_FME1 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PowerThreshAP1",    10, 10),
	AP6_ERR(1, SYSFS_FME1 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP6",      9,  9),
	FME_ERR(1, SYSFS_FME1 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].InjectedWarningErr", 6,  6),
	FME_ERR(1, SYSFS_FME1 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].AfuAccessModeErr",   5,  5),
	FME_ERR(1, SYSFS_FME1 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].ProcHot",            4,  4),
	FME_ERR(1, SYSFS_FME1 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PortFatalErr",       3,  3),
	FME_ERR(1, SYSFS_FME1 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].PcieError",          2,  2),
	FME_ERR(1, SYSFS_FME1 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP2",      1,  1),
	FME_ERR(1, SYSFS_FME1 "/errors/nonfatal_errors",  "RAS_NOFAT_ERR_STAT[0x4050].TempThreshAP1",      0,  0),



	FME_ERR(0, SYSFS_FME0 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].InjectedCatastErr",  11, 11),
	FME_ERR(0, SYSFS_FME0 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].ThermCatastErr",     10, 10),
	FME_ERR(0, SYSFS_FME0 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].CrcCatastErr",        9,  9),
	FME_ERR(0, SYSFS_FME0 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].InjectedFatalErr",    8,  8),
	FME_ERR(0, SYSFS_FME0 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].PciePoisonErr",       7,  7),
	FME_ERR(0, SYSFS_FME0 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].FabricFatalErr",      6,  6),
	FME_ERR(0, SYSFS_FME0 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].IommuFatalErr",       5,  5),
	FME_ERR(0, SYSFS_FME0 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].DramFatalErr",        4,  4),
	FME_ERR(0, SYSFS_FME0 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].KtiProtoFatalErr",    3,  3),
	FME_ERR(0, SYSFS_FME0 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].CciFatalErr",         2,  2),
	FME_ERR(0, SYSFS_FME0 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].TagCchFatalErr",      1,  1),
	FME_ERR(0, SYSFS_FME0 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].KtiLinkFatalErr",     0,  0),

	FME_ERR(1, SYSFS_FME1 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].InjectedCatastErr",  11, 11),
	FME_ERR(1, SYSFS_FME1 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].ThermCatastErr",     10, 10),
	FME_ERR(1, SYSFS_FME1 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].CrcCatastErr",        9,  9),
	FME_ERR(1, SYSFS_FME1 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].InjectedFatalErr",    8,  8),
	FME_ERR(1, SYSFS_FME1 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].PciePoisonErr",       7,  7),
	FME_ERR(1, SYSFS_FME1 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].FabricFatalErr",      6,  6),
	FME_ERR(1, SYSFS_FME1 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].IommuFatalErr",       5,  5),
	FME_ERR(1, SYSFS_FME1 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].DramFatalErr",        4,  4),
	FME_ERR(1, SYSFS_FME1 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].KtiProtoFatalErr",    3,  3),
	FME_ERR(1, SYSFS_FME1 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].CciFatalErr",         2,  2),
	FME_ERR(1, SYSFS_FME1 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].TagCchFatalErr",      1,  1),
	FME_ERR(1, SYSFS_FME1 "/errors/fatal_errors",  "RAS_CATFAT_ERROR_STAT[0x4060].KtiLinkFatalErr",     0,  0),


	TABLE_TERMINATOR
};

int log_fpga_error(struct fpga_err *e)
{
	if (!e->occurred) {
		e->occurred = true;

		dlog("socket %i: %s\n", e->socket, e->reg_field);

		if (e->callback)
			e->callback(e);

		return 1;
	}

	return 0;
}

int poll_error(struct fpga_err *e)
{
	int i;
	int count = 0;

	uint64_t err;
	uint64_t mask;

	struct stat st;

	fpga_result res;

	i = stat(e->sysfsfile, &st);
	if (i != 0) // file may not exist on single-socket.
		return 0;

	err = 0;
	res = fpgad_sysfs_read_u64(e->sysfsfile, &err);
	if (res != FPGA_OK) {
		return -1;
	}

	mask = 0;
	for (i = e->lowbit ; i <= e->highbit ; ++i)
		mask |= 1ULL << i;

	if (err & mask) {
		count += log_fpga_error(e);
	} else {
		e->occurred = false;
	}

	return count;
}

/*
 * Poll all known error registers
 *
 * @returns number of (new) errors that were found
 */
int poll_errors(struct fpga_err error_table[])
{
	unsigned i;
	int errors = 0;
	int res;

	for (i = 0 ; error_table[i].sysfsfile ; ++i) {
		res = poll_error(&error_table[i]);
		if (-1 == res)
			return res;
		errors += res;
	}

	return errors;
}

void *logger_thread(void *thread_context)
{
	struct config *c = (struct config *)thread_context;

	char sysfspath[SYSFS_PATH_MAX];
	struct stat stats;
	uint64_t err_rev = 0;
	struct fpga_err *port_error_table = port_error_table_rev_1;
	struct fpga_err *fme_error_table  = fme_error_table_rev_1;

	/*
	 * Check the errors/revision sysfs file to determine
	 * which table to use.
	 */
	snprintf_s_s(sysfspath, sizeof(sysfspath),
			"%s/errors/revision", SYSFS_PORT0);

	if (!stat(sysfspath, &stats)) {
		if (FPGA_OK == fpgad_sysfs_read_u64(sysfspath, &err_rev)) {
			switch (err_rev) {
			case 1ULL:
				port_error_table = port_error_table_rev_1;
				break;
			default:
				dlog("logger: invalid port error revision: %llu\n", err_rev);
				goto out_exit;
				break;
			}
		} else {
			dlog("logger: couldn't parse %s\n", sysfspath);
			goto out_exit;
		}
	} else {
		dlog("logger: couldn't find %s\n", sysfspath);
		goto out_exit;
	}

	/*
	 * Check the errors/revision sysfs file to determine
	 * which table to use.
	 */
	snprintf_s_s(sysfspath, sizeof(sysfspath),
			"%s/errors/revision", SYSFS_FME0);

	if (!stat(sysfspath, &stats)) {
		if (FPGA_OK == fpgad_sysfs_read_u64(sysfspath, &err_rev)) {
			switch (err_rev) {
			case 1ULL:
				fme_error_table = fme_error_table_rev_1;
				break;
			default:
				dlog("logger: invalid fme error revision: %llu\n", err_rev);
				goto out_exit;
				break;
			}
		} else {
			dlog("logger: couldn't parse %s\n", sysfspath);
			goto out_exit;
		}
	} else {
		dlog("logger: couldn't find %s\n", sysfspath);
		goto out_exit;
	}

	while (c->running) {
		/* read port error */
		if ((poll_errors(port_error_table) < 0) ||
		    (poll_errors(fme_error_table) < 0))
			return NULL;
		usleep(c->poll_interval_usec);
	}

out_exit:
	return NULL;
}

#define SYSFS_PATH_MAX 256
fpga_result fpgad_sysfs_read_u64(const char *path, uint64_t *u)
{
	int fd;
	int res;
	char buf[SYSFS_PATH_MAX];
	int b;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "open(%s) failed \n", path);
		return FPGA_NOT_FOUND;
	}

	if ((off_t)-1 == lseek(fd, 0, SEEK_SET)) {
		fprintf(stderr, "seek(%s) failed \n", path);
		goto out_close;
	}

	b = 0;

	do {
		res = read(fd, buf+b, sizeof(buf)-b);
		if (res <= 0) {
			fprintf(stderr, "Read from %s failed \n", path);
			goto out_close;
		}
		b += res;
		if (((unsigned)b > sizeof(buf)) || (b <= 0)) {
			fprintf(stderr, "Unexpected size reading from %s", path);
			goto out_close;
		}
	} while (buf[b-1] != '\n' && buf[b-1] != '\0' && (unsigned)b < sizeof(buf));

	// erase \n
	buf[b-1] = 0;

	*u = strtoull(buf, NULL, 0);

	close(fd);
	return FPGA_OK;

out_close:
	close(fd);
	return FPGA_NOT_FOUND;
}
