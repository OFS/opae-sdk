//
// Copyright (c) 2014-2018, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once
#include "opae_svc_wrapper.h"

//
// Manage a standard collection of CSRS.  This is the software-side interface
// to the base cci_csrs RTL module.
//

class CSR_MGR
{
protected:
	enum {
		//
		// Offset of user CSRs in the CSR index space.  Offsets below this
		// are in the "common" (application-independent) CSR space,
		// defined below.
		//
		// This constant must match the RTL implementation in csr_mgr.sv!
		//
		USER_CSR_BASE = 32
	};

public:
	CSR_MGR(SVC_WRAPPER& svc) : svc(svc) {};

	~CSR_MGR() {};

	//
	// Write/read application-specific CSRs.  The maximum CSR index
	// is application-dependent.
	//
	void writeCSR(uint32_t idx, uint64_t v)	{
		svc.mmioWrite64(8 * (USER_CSR_BASE + idx), v);
	}

	uint64_t readCSR(uint32_t idx)	{
		return svc.mmioRead64(8 * (USER_CSR_BASE + idx));
	}

	//
	// Common CSRs available on in all programs using CSR_MGR.  These offsets
	// must match the implementation in csr_mgr.sv.
	//
	typedef enum {
		CSR_COMMON_DFH = 0,
		CSR_COMMON_ID_L = 1,
		CSR_COMMON_ID_H = 2,
		// AFU frequency
		CSR_COMMON_FREQ = 8,
		// Number of read/write hits in the FIU system memory cache
		CSR_COMMON_CACHE_RD_HITS = 9,
		CSR_COMMON_CACHE_WR_HITS = 10,
		// Lines read/written on the cached physical channel
		CSR_COMMON_VL0_RD_LINES = 11,
		CSR_COMMON_VL0_WR_LINES = 12,
		// Lines read or written on the non-cached physical channels
		CSR_COMMON_VH0_LINES = 13,
		CSR_COMMON_VH1_LINES = 14,
		// A collection of status signals from the FIU.  See "FIU state"
		// defined in csr_mgr.sv.
		CSR_COMMON_FIU_STATE = 15,
		CSR_COMMON_RD_ALMOST_FULL_CYCLES = 16,
		CSR_COMMON_WR_ALMOST_FULL_CYCLES = 17,

		CSR_COMMON__LAST = 18
	} t_csr_common;

	// Read one of the common CSRs
	uint64_t readCommonCSR(t_csr_common idx) {
		if (idx >= CSR_COMMON__LAST) return ~uint64_t(0);

		return svc.mmioRead64(8 * uint32_t(idx));
	}

	//
	// Return the frequency at which the AFU is running.  In the common
	// framework here, a CSR holds the frequency of the clock to which the
	// AFU is attached.  When the attached clock is the "user clock" the
	// frequency isn't known at compile time.
	//
	uint64_t getAFUMHz(uint64_t uClk_usr_mhz = 0) {
		// What's the AFU frequency (MHz)?
		uint64_t afu_mhz = readCommonCSR(CSR_COMMON_FREQ);

		if (afu_mhz == 2) {
			// 2 indicates uClk_usr
			afu_mhz = uClk_usr_mhz;
		} else if (afu_mhz == 1) {
			// 1 indicates uClk_usrDiv2
			afu_mhz = uClk_usr_mhz >> 1;
		}
		return afu_mhz;
	}

protected:
	SVC_WRAPPER& svc;
};
