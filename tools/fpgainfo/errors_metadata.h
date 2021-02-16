// Copyright(c) 2021, Intel Corporation
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
 * @file errors_metadata.h
 *
 * @brief fpga errors description
 *
 */
#ifndef FPGA_ERRORS_METADATA_H
#define FPGA_ERRORS_METADATA_H

#include <opae/fpga.h>
#include <opae/error.h>

enum fapg_error_type {
	FPGA_FME_ERROR,
	FPGA_PCIE0_ERROR,
	FPGA_PCIE1_ERROR,
	FPGA_NONFATAL_ERROR,
	FPGA_CATFATAL_ERROR,
	FPGA_INJECT_ERROR,
	FPGA_PORT_ERROR,
	FPGA_ERROR_UNKNOWN
};

#define FPGA_ERR_METADATA_COUNT  12

typedef  struct FPGA_ERRORS {
	uint64_t revision;
	enum fapg_error_type error_type;
	uint64_t arry_size_max;
	struct {
		char err_str[256];
	} str_err[64];
} fpga_errors;



fpga_errors fpga_errors_metadata[] = {

	// FME ERROR  REVISION 0
	{
		.revision = 0,
		.error_type = FPGA_FME_ERROR,
		.arry_size_max = 7,
		.str_err = {
			{ .err_str = "Fabric error detected" },
			{ .err_str = "Fabric fifo under / overflow error detected" },
			{ .err_str = "KTI CDC Parity Error detected" },
			{ .err_str = "KTI CDC Parity Error detected" },
			{ .err_str = "IOMMU Parity error detected" },
			{ .err_str = "AFU PF/VF access mismatch detected" },
			{ .err_str = "Indicates an MBP event error detected" }
		}

	},

	// FME ERROR  REVISION 1
	{
		.revision = 1,
		.error_type = FPGA_FME_ERROR,
		.arry_size_max = 6,
		.str_err = {
			{.err_str = "Error from Partial Reconfiguration Block reporting \
			 a FIFO Parity Error has occurred." },
			{.err_str = "Error from Remote SignalTap Block reporting a \
			Parity Error detected" },
			{.err_str = "Reserved" },
			{.err_str = "Reserved" },
			{.err_str = "Reserved" },
			{.err_str = "AFU PF/VF access mismatch detected" }
		}

	},

	// PCI0 ERROR  REVISION 0
	{
		.revision = 0,
		.error_type = FPGA_PCIE0_ERROR,
		.arry_size_max = 10,
		.str_err = {
			{.err_str = "TLP format / type error detected." },
			{.err_str = "TTLP MW address error detected." },
			{.err_str = "TLP MW length error detected." },
			{.err_str = "TLP MR address error detected." },
			{.err_str = "TLP MR length error detected." },
			{.err_str = "TLP CPL tag error detected." },
			{.err_str = "TLP CPL status error detected." },
			{.err_str = "TLP CPL timeout error detected." },
			{.err_str = "CCI bridge parity error detected." },
			{.err_str = "TLP with EP  error  detected." }
		}

	},

	// PCI0 ERROR  REVISION 1
	{
		.revision = 1,
		.error_type = FPGA_PCIE0_ERROR,
		.arry_size_max = 13,
		.str_err = {
			{.err_str = "TLP format/type error detected." },
			{.err_str = "TTLP MW address error detected." },
			{.err_str = "TLP MW length error detected." },
			{.err_str = "TLP MR address error detected." },
			{.err_str = "TLP MR length error detected." },
			{.err_str = "TLP CPL tag error detected." },
			{.err_str = "TLP CPL status error detected." },
			{.err_str = "TLP CPL timeout error detected." },
			{.err_str = "CCI bridge parity error detected." },
			{.err_str = "TLP with EP  error detected." },
			// 10
			{.err_str = "Malformed TLP, Start-of-packet is receved in the \
			middle of a TLP packet error detected." },
			{.err_str = "Malformed TLP, End-of-packet is received earlier \
			or later than expected error detected." },
			{.err_str = "RX FIFO overflow error detected." }
		}

	},

	// PCI1 ERROR  REVISION 0
	{
		.revision = 0,
		.error_type = FPGA_PCIE1_ERROR,
		.arry_size_max = 10,
		.str_err = {
			{.err_str = "TLP format / type error detected." },
			{.err_str = "TTLP MW address error detected." },
			{.err_str = "TLP MW length error detected." },
			{.err_str = "TLP MR address error detected." },
			{.err_str = "TLP MR length error detected." },
			{.err_str = "TLP CPL tag error detected." },
			{.err_str = "TLP CPL status error detected." },
			{.err_str = "TLP CPL timeout error detected." },
			{.err_str = "CCI bridge parity error detected." },
			{.err_str = "TLP with EP  error detected." }
		}

	},

	// NON FATAL ERROR  REVISION 0
	{
		.revision = 0,
		.error_type = FPGA_NONFATAL_ERROR,
		.arry_size_max = 13,
		.str_err = {
			{.err_str = "Temperature threshold triggered AP1 detected." },
			{.err_str = "Temperature threshold triggered AP2 detected." },
			{.err_str = "PCIe error detected." },
			{.err_str = "AFU port Fatal error detected." },
			{.err_str = "ProcHot event error detected." },
			{.err_str = "AFU PF/VF access mismatch error detected." },
			{.err_str = "Injected Warning Error detected." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Temperature threshold triggered AP6 detected." },
			// 10
			{.err_str = "Power threshold triggered AP1 error detecte." },
			{.err_str = "Power threshold triggered AP2 error detecte." },
			{.err_str = "MBP event error detected." }
		}

	},

	// NON FATAL ERROR  REVISION 1
	{
		.revision = 1,
		.error_type = FPGA_NONFATAL_ERROR,
		.arry_size_max = 7,
		.str_err = {
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "PCIe error detected." },
			{.err_str = "AFU port Fatal error detected." },
			{.err_str = "Reserved." },
			{.err_str = "AFU PF/VF access mismatch error detected." },
			{.err_str = "Injected Warning Error detected." }
		}

	},

	// CATFATAL ERROR  REVISION 0
	{
		.revision = 0,
		.error_type = FPGA_CATFATAL_ERROR,
		.arry_size_max = 12,
		.str_err = {
			{.err_str = "KTI link layer error detected." },
			{.err_str = "tag-n-cache error detected." },
			{.err_str = "CCI error detected." },
			{.err_str = "KTI protocol error detected." },
			{.err_str = "Fatal DRAM error detected." },
			{.err_str = "IOMMU fatal parity error detected." },
			{.err_str = "Fabric fatal error detected." },
			{.err_str = "Poison error from any of PCIe ports detected." },
			{.err_str = "Injected Fatal Error detected." },
			{.err_str = "Catastrophic CRC error detected." },
			// 10
			{.err_str = "Catastrophic thermal runaway event detected." },
			{.err_str = "Injected Catastrophic Error detected." }
		}

	},

	// CATFATAL ERROR  REVISION 1
	{
		.revision = 1,
		.error_type = FPGA_CATFATAL_ERROR,
		.arry_size_max = 12,
		.str_err = {
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Fabric fatal error detected." },
			{.err_str = "Poison error from any of PCIe ports detected." },
			{.err_str = "Injected Fatal Error detected." },
			{.err_str = "Catastrophic CRC error detected." },
			// 10
			{.err_str = "Reserved." },
			{.err_str = "Injected Catastrophic Error detected." }
		}

	},
	// INJECT ERROR  REVISION 0
	{
		.revision = 0,
		.error_type = FPGA_INJECT_ERROR,
		.arry_size_max = 3,
		.str_err = {
			{.err_str = "Set Catastrophic error." },
			{.err_str = "Set Fatal error." },
			{.err_str = "Sett Non-fatal error." }
		}

	},
	// INJECT ERROR  REVISION 1
	{
		.revision = 1,
		.error_type = FPGA_INJECT_ERROR,
		.arry_size_max = 3,
		.str_err = {
			{.err_str = "Set Catastrophic error." },
			{.err_str = "Set Fatal error." },
			{.err_str = "Sett Non-fatal error." }
		}

	},
	// PORT ERROR  REVISION 0
	{
		.revision = 0,
		.error_type = FPGA_PORT_ERROR,
		.arry_size_max = 61,
		.str_err = {
			{.err_str = "Tx Channel 0 overflow error detected." },
			{.err_str = "Tx Channel 0 invalid request encoding \
			error detected." },
			{.err_str = "Tx Channel 0 cl_len=3 not supported \
			error detected." },
			{.err_str = "Tx Channel 0 request with cl_len=2 does NOT have \
			a 2CL aligned address error detected." },
			{.err_str = "Tx Channel 0 request with cl_len=4 does NOT have a \
			 4CL aligned address error detected." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "AFU MMIO RD received while PORT is in \
			reset error detected." },
			//10
			{.err_str = "AFU MMIO WR received while PORT is in \
			reset error detected." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Tx Channel 1 overflow error detected." },
			{.err_str = "Tx Channel 1 invalid request encoding error detected." },
			{.err_str = "Tx Channel 1 cl_len=3 not supported error detecte." },
			{.err_str = "Tx Channel 1 request with cl_len=2 does NOT have a \
			2CL aligned address error detected." },
			//20
			{.err_str = "Tx Channel 1 request with cl_len=4 does NOT have a \
			4CL aligned address error detected." },
			{.err_str = "Tx Channel 1 insufficient data payload \
			Error detected." },
			{.err_str = "Tx Channel 1 data payload overrun error detected." },
			{.err_str = "Tx Channel 1 incorrect address on subsequent \
			payloads error detected." },
			{.err_str = "Tx Channel 1 Non-zero SOP detected for \
			requests!=WrLine_* error detected." },
			{.err_str = "Tx Channel 1 SOP expected to be 0 for \
			req_type!=WrLine_." },
			{.err_str = "Tx Channel 1 Illegal VC_SEL. Atomic request is \
			only supported on VL0 error detected." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			// 30
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },

			{.err_str = "MMIO TimedOut error detected." },
			{.err_str = "Tx Channel 2 fifo overflo error detected." },
			{.err_str = "MMIO Read response received, with no matching \
			request pending error detected." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			// 40
			{.err_str = "Reserved." },
			{.err_str = "Number of pending requests: counter overflow \
			error detected." },
			{.err_str = "Request with Address violating SMM range \
			error detected." },
			{.err_str = "Request with Address violating second SMM range \
			error detected." },
			{.err_str = "Request with Address violating ME stolen range." },
			{.err_str = "Request with Address violating Generic protected \
			range error detected." },
			{.err_str = "Request with Address violating Legacy Range \
			Low error detected." },
			{.err_str = "Request with Address violating Legacy Range  \
			High error detected." },
			{.err_str = "Request with Address violating VGA memory range \
			 error detected." },
			{.err_str = "Page Fault error detected." },
			// 50
			{.err_str = "PMR Erro error detected." },
			{.err_str = "AP6 event detected." },
			{.err_str = "VF FLR detected on port when PORT configured \
			in PF access mode error detected." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Reserved." },
			{.err_str = "Tx Channel 1 byte_len cannot be zer." },
			{.err_str = "Tx Channel 1 illegal operation: sum of byte_len and \
			byte_start should be less than or equal to 64." },
			{.err_str = "Tx Channel 1 illegal operation: cl_len cannot be \
			non-zero when mode is eMOD_BYTE." },
			// 60
			{.err_str = "Tx Channel 1 byte_len and byte_start should be \
			zero when mode is not eMOD_BYTE." }
		}

	},	// PORT ERROR  REVISION 1
	{
		.revision = 1,
		.error_type = FPGA_PORT_ERROR,
		.arry_size_max = 16,
		.str_err = {
			{.err_str = "Tx valid violation error detected." },
			{.err_str = "Tx mwr insufficient data error detected." },
			{.err_str = "Tx mwr data payload overrun error detected." },
			{.err_str = "mmio insufficient data error detected." },
			{.err_str = "mmio data payload overrun error detected." },
			{.err_str = "mmio read while reset error detected." },
			{.err_str = "mmio write while reset error detected." },
			{.err_str = "mmio timeout error detected." },
			{.err_str = "unexpected mmio response error detected." },
			{.err_str = "tag occupiied error detected." },
			// 10
			{.err_str = "unaligned address error detected." },
			{.err_str = "max tag error detected." },
			{.err_str = "max read request size error detected." },
			{.err_str = "max payload error detected." },
			{.err_str = "malformed TLP error detected." },
			{.err_str = "Tx request couter overflow error detected." }
		}

	},

};

#endif /* !FPGA_ERRORS_METADATA_H */
