// Copyright(c) 2014-2018, Intel Corporation
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

#define FMT_CHAR    'c'
#define FMT_WCHAR   'C'
#define FMT_SHORT   'h'
#define FMT_INT		'd'
#define FMT_LONG	'l'
#define FMT_STRING	's'
#define FMT_WSTRING	'S'
#define FMT_DOUBLE	'g'
#define FMT_LDOUBLE	'G'
#define FMT_VOID    'p'
#define FMT_PCHAR	'1'
#define FMT_PSHORT	'2'
#define FMT_PINT	'3'
#define FMT_PLONG	'4'

#define MAX_FORMAT_ELEMENTS    16

#define CHK_FORMAT(X, Y)   (((X) == (Y))?1:0)

#define RSIZE_MAX_STR 4096
#define ESBADFMT -1
#define ESFMTTYP -2

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
 * ase_memset - Secure memset abstraction.  Return 0 on success.
 */
int ase_memset_s(void *dest, size_t dmax, int ch, size_t count)
{
	int ret = 0;
	volatile unsigned char *s = dest;
	size_t n = count;

	/* Fatal runtime-constraint violations. */
	if (s == NULL || dmax > (256UL << 20) || count > dmax) {
		ASE_DBG("Illegal parameter to ase_memset_s");
		return -1;
	}

	/* Updating through a volatile pointer should not be optimized away. */
	while (n--)
		*s++ = (unsigned char) ch;
	return ret;
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


unsigned int
parse_format(const char *format, char pformatList[], unsigned int maxFormats)
{
	unsigned int  numFormats = 0;
	unsigned int  index = 0;
	unsigned int  start = 0;
	char		  lmod = 0;

	while (index < RSIZE_MAX_STR && format[index] != '\0' && numFormats < maxFormats) {
		if (format[index] == '%') {
			start = index; // remember where the format string started
						   // Check for flags
			switch (format[++index]) {
			case '\0':
				continue; // skip - end of format string
			case '%':
				continue; // skip - actually a percent character
			case '#': // convert to alternate form
			case '0': // zero pad
			case '-': // left adjust
			case ' ': // pad with spaces
			case '+': // force a sign be used
				index++; // skip the flag character
				break;
			}
			// check for and skip the optional field width
			while (format[index] != '\0' && format[index] >= '0' && format[index] <= '9') {
				index++;
			}
			// Check for an skip the optional precision
			if (format[index] != '\0' && format[index] == '.') {
				index++; // skip the period
				while (format[index] != '\0' && format[index] >= '0' && format[index] <= '9') {
					index++;
				}
			}
			// Check for and skip the optional length modifiers
			lmod = ' ';
			switch (format[index]) {
			case 'h':
				if (format[++index] == 'h') {
				    ++index; //also recognize the 'hh' modifier
				    lmod = 'H'; // for char
			    }
					  else {
						  lmod = 'h'; // for short
					  }
					  break;
			case 'l':
				if (format[++index] == 'l') {
				    ++index; //also recognize the 'll' modifier
				    lmod = 'd'; // for long long
			    }
					  else {
						  lmod = 'l'; // for long
					  }
					  break;
			case 'L':
				lmod = 'L'; break;
			case 'j':
			case 'z':
			case 't':
				index++;
				break;
			}

			// Recognize and record the actual modifier
			switch (format[index]) {
			case 'c':
				if (lmod == 'l') {
					pformatList[numFormats] = FMT_WCHAR; // store the format character
				} else {
					pformatList[numFormats] = FMT_CHAR;
				}
				numFormats++;
				index++; // skip the format character
				break;

			case 'd': case 'i': // signed
			case 'o': case 'u': // unsigned
			case 'x': case 'X': // unsigned
				if (lmod == 'H') {
					pformatList[numFormats] = FMT_CHAR; // store the format character
				} else if (lmod == 'l') {
					pformatList[numFormats] = FMT_LONG; // store the format character
				} else if (lmod == 'h') {
					pformatList[numFormats] = FMT_SHORT; // store the format character
				} else {
					pformatList[numFormats] = FMT_INT;
				}
				numFormats++;
				index++; // skip the format character
				break;

			case 'e': case 'E':
			case 'f': case 'F':
			case 'g': case 'G':
			case 'a': case 'A':
				if (lmod == 'L') {
					pformatList[numFormats] = FMT_LDOUBLE; // store the format character
				} else {
					pformatList[numFormats] = FMT_DOUBLE;
				}
				numFormats++;
				index++; // skip the format character
				break;

			case 's':
				if (lmod == 'l' || lmod == 'L') {
					pformatList[numFormats] = FMT_WSTRING; // store the format character
				} else {
					pformatList[numFormats] = FMT_STRING;
				}
				numFormats++;
				index++; // skip the format character
				break;

			case 'p':
				pformatList[numFormats] = FMT_VOID;
				numFormats++;
				index++; // skip the format character
				break;

			case 'n':
				if (lmod == 'H') {
					pformatList[numFormats] = FMT_PCHAR; // store the format character
				} else if (lmod == 'l') {
					pformatList[numFormats] = FMT_PLONG; // store the format character
				} else if (lmod == 'h') {
					pformatList[numFormats] = FMT_PSHORT; // store the format character
				} else {
					pformatList[numFormats] = FMT_PINT;
				}
				numFormats++;
				index++; // skip the format character
				break;
			case 'm':
				// Does not represent an argument in the call stack
				index++; // skip the format character
				continue;
			default:
				printf("failed to recognize format string [");
				for (; start < index; start++) {
					printf("%c", format[start]);
				}
				puts("]");
				break;
			}
		} else {
			index++; // move past this character
		}
	}

	return numFormats;
}

unsigned int
check_integer_format(const char format)
{
	unsigned int  retValue = 0; // default failure
	switch (format) {
	case FMT_CHAR:
	case FMT_SHORT:
	case FMT_INT:
		retValue = 1;
		break;
	}
	return retValue;
}


int sscanf_s_u(const char *src, const char *format, unsigned *a)
{
	char pformatList[MAX_FORMAT_ELEMENTS];
	unsigned int index = 0;

	// Determine the number of format options in the format string
	unsigned int  nfo = parse_format(format, &pformatList[0], MAX_FORMAT_ELEMENTS);

	// Check that there are not too many format options
	if (nfo != 1) {
		return ESBADFMT;
	}
	// Check that the format is for an integer type
	if (check_integer_format(pformatList[index]) == 0) {
		return ESFMTTYP;
	}

	return sscanf(src, format, a);
}


int sscanf_s_ii(const char *src, const char *format, int *a, int *b)
{
	char pformatList[MAX_FORMAT_ELEMENTS];

	// Determine the number of format options in the format string
	unsigned int  nfo = parse_format(format, &pformatList[0], MAX_FORMAT_ELEMENTS);

	// Check that there are not too many format options
	if (nfo != 2) {
		return ESBADFMT;
	}
	// Check that the format is for an integer type
	if (check_integer_format(pformatList[0]) == 0
		|| check_integer_format(pformatList[1]) == 0) {
		return ESFMTTYP;
	}

	return sscanf(src, format, a, b);
}


int fscanf_s_i(FILE *file, const char *format, int *a)
{
	char pformatList[MAX_FORMAT_ELEMENTS];
	unsigned int index = 0;

	// Determine the number of format options in the format string
	unsigned int  nfo = parse_format(format, &pformatList[0], MAX_FORMAT_ELEMENTS);

	// Check that there are not too many format options
	if (nfo != 1) {
		return ESBADFMT;
	}
	// Check that the format is for an integer type
	if (check_integer_format(pformatList[index]) == 0) {
		return ESFMTTYP;
	}

	return fscanf(file, format, a);
}
