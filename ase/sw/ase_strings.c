// Copyright(c) 2014-2017, Intel Corporation
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
// **************************************************************************

/*
 *
 * ASE string library.  The ASE RTL simulator-side compilation lacks the
 * context to find the OPAE SDK's copy of the safe_string library.  ASE
 * uses only a few string methods, which are implemented here using safety
 * checks similar to the safe_string checks.
 *
 */

#include "ase_common.h"

/*
 * Internal method: detect buffer overlap.
 */
static int buffers_overlap(const void *dest, const void *src, size_t n)
{
	// char* equivalents
	const char* d = (char*)dest;
	const char* s = (char*)src;

	// Is dest completely before or completely after src?  If so, they
	// don't overlap.
	if (((d + n) <= s) || (d >= (s + n))) {
		return 0;
	}

	// Buffers overlap
	return 1;
}


/*
 * ase_memcpy - Secure memcpy abstraction.  Return 0 on success.
 */
int ase_memcpy(void *dest, const void *src, size_t n)
{
	// No NULL pointers or long strings, no copies larger than 256MB.
	if ((dest == NULL) || (src == NULL) || (n == 0) || (n > (256UL << 20))) {
		ASE_DBG("Illegal parameter to ase_memcpy");
		return -1;
	}

	// Strings must not overlap
	if (buffers_overlap(dest, src, n)) {
		ASE_DBG("Illegal buffer overlap in ase_memcpy");
		return -1;
	}

	memcpy(dest, src, n);
	return 0;
}


/*
 * ASE string copy.  Returns 0 on success.
 */
int ase_strncpy(char *dest, const char *src, size_t n)
{
	// Validate parameters
	if ((dest == NULL) || (src == NULL) || (n == 0) || (n > 4096)) {
		ASE_DBG("Illegal parameter to ase_strncpy");
		return -1;
	}

	// Strings must not overlap
	if (buffers_overlap(dest, src, n)) {
		ASE_DBG("Illegal buffer overlap in ase_strncpy");
		return -1;
	}

	strncpy(dest, src, n);
	return 0;
}


/*
 * ASE string compare
 */
int ase_strncmp(const char *s1, const char *s2, size_t n)
{
	// Validate parameters
	if ((s1 == NULL) || (s2 == NULL) || (n == 0) || (n > 4096)) {
		ASE_DBG("Illegal parameter to ase_strncmp");
		return -1;
	}

	return strncmp(s1, s2, n);
}
