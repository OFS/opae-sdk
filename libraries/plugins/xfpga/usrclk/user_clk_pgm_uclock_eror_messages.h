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

// Arthur.Sheiman@Intel.com   Created: 09-08-16
// Revision: 10-18-16  18:06


const char *pac_UclockErrorMsg[] = {
	"QUCPU_Uclock: No error.\0",
	"RunInitz: RTL versions number incompatible.\0",
	"RunInitz: PLL RTL has illegal ID.\0",
	"Timeout waiting for calibration.\0",
	"AvmmRMW: Verify error.\0",
	"AvmmRWcom: Timeout with AVMM transaction.\0",
	"GetFreqs: Not initialized.\0",
	"SetFreqs: Not initialized.\0",
	"SetFreqs: Illegal reference clock index.\0",
	"SetFreqs: RTL not configured for 100 MHz SYSCLK reflk.\0",
	"SetFreqs: RTL not configured for 322.265625 MHz reflk.\0",
	"SetFreqs: Requested frequency too high.\0",
	"SetFreqs: Illegal ExactFreq mode requested.\0",
	"SetFreqs: Use 322.265625 MHz refclk for ExactFreq mode.\0",
	"SetFreqs: PLL did unlock during power down.\0",
	"SetFreqs: Timeout waiting for PLL to lock.\0",
	"ERROR: MSG INDEX OUT OF RANGE\0" // "+1" message
};
