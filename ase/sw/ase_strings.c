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
static int buffers_overlap(const void *dest, size_t dmax, const void *src, size_t smax)
{
	// char* equivalents
	const char *d = (char *)dest;
	const char *s = (char *)src;

	// Is dest completely before or completely after src?  If so, they
	// don't overlap.
	if (((d + dmax) <= s) || (d >= (s + smax))) {
		return 0;
	}

	// Buffers overlap
	return 1;
}


/*
 * ase_memcpy - Secure memcpy abstraction.  Return 0 on success.
 */
int ase_memcpy_s(void *dest, size_t dmax, const void *src, size_t smax)
{
	// No NULL pointers, maxima must be non-zero, smax must be less than dmax
	// and dmax must be less than 256MB.
	if ((dest == NULL) || (src == NULL) ||
		(dmax == 0) || (smax == 0) || (smax > dmax) ||
		(dmax > (256UL << 20))) {
		ASE_DBG("Illegal parameter to ase_memcpy_s");
		return -1;
	}

	// Strings must not overlap
	if (buffers_overlap(dest, dmax, src, smax)) {
		ASE_DBG("Illegal buffer overlap in ase_memcpy_s");
		return -1;
	}

	memcpy(dest, src, smax);
	return 0;
}


/*
 * ASE string copy.  Returns 0 on success.
 */
int ase_strncpy_s(char *dest, size_t dmax, const char *src, size_t slen)
{
	// No NULL pointers, maxima must be non-zero, smax must be less than dmax
	// and dmax must be less than 4KB.
	if ((dest == NULL) || (src == NULL) ||
		(dmax == 0) || (slen == 0) || (slen > dmax) ||
		(dmax > 4096)) {
		ASE_DBG("Illegal parameter to ase_strncpy_s");
		return -1;
	}

	// Strings must not overlap
	if (buffers_overlap(dest, dmax, src, slen)) {
		ASE_DBG("Illegal buffer overlap in ase_strncpy_s");
		return -1;
	}

	strncpy(dest, src, slen);
	return 0;
}


/*
 * ASE string compare
 */
int ase_strcmp_s(const char *dest, size_t dmax, const char *src, int *indicator)
{
	// Validate parameters
	if ((dest == NULL) || (src == NULL) || (dmax == 0) || (dmax > 4096) ||
		(indicator == NULL)) {
		ASE_DBG("Illegal parameter to ase_strncmp_s");
		return -1;
	}

	*indicator = strncmp(dest, src, dmax);
	return 0;
}
