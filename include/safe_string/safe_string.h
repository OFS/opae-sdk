
/*------------------------------------------------------------------
 * safe_types.h - C99 std types & defs or Linux kernel equivalents
 *
 * March 2007, Bo Berry
 * Modified 2012, Jonathan Toppins <jtoppins@users.sourceforge.net>
 *
 * Copyright (c) 2007-2013 by Cisco Systems, Inc
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *------------------------------------------------------------------
 */

#ifndef __SAFE_TYPES_H__
#define __SAFE_TYPES_H__

#ifdef __KERNEL__
/* linux kernel environment */

#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/errno.h>

/* errno_t isn't defined in the kernel */
typedef int errno_t;

#else

#include <stdio.h>
#include <sys/types.h>
#include <inttypes.h>
#include <stdint.h>
#include <errno.h>

typedef int errno_t;

#include <stdbool.h>

#endif /* __KERNEL__ */
#endif /* __SAFE_TYPES_H__ */



/*------------------------------------------------------------------
 * safe_lib_errno.h -- Safe C Lib Error codes
 *
 * October 2008, Bo Berry
 * Modified 2012, Jonathan Toppins <jtoppins@users.sourceforge.net>
 *
 * Copyright (c) 2008-2013 by Cisco Systems, Inc
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *------------------------------------------------------------------
 */

#ifndef __SAFE_LIB_ERRNO_H__
#define __SAFE_LIB_ERRNO_H__

#ifdef __KERNEL__
# include <linux/errno.h>
#else
#include <errno.h>
#endif /* __KERNEL__ */

/*
 * Safe Lib specific errno codes.  These can be added to the errno.h file
 * if desired.
 */
#ifndef ESNULLP
#define ESNULLP         ( 400 )       /* null ptr                    */
#endif

#ifndef ESZEROL
#define ESZEROL         ( 401 )       /* length is zero              */
#endif

#ifndef ESLEMIN
#define ESLEMIN         ( 402 )       /* length is below min         */
#endif

#ifndef ESLEMAX
#define ESLEMAX         ( 403 )       /* length exceeds max          */
#endif

#ifndef ESOVRLP
#define ESOVRLP         ( 404 )       /* overlap undefined           */
#endif

#ifndef ESEMPTY
#define ESEMPTY         ( 405 )       /* empty string                */
#endif

#ifndef ESNOSPC
#define ESNOSPC         ( 406 )       /* not enough space for s2     */
#endif

#ifndef ESUNTERM
#define ESUNTERM        ( 407 )       /* unterminated string         */
#endif

#ifndef ESNODIFF
#define ESNODIFF        ( 408 )       /* no difference               */
#endif

#ifndef ESNOTFND
#define ESNOTFND        ( 409 )       /* not found                   */
#endif

/* Additional for safe snprintf_s interfaces                         */
#ifndef ESBADFMT
#define ESBADFMT        ( 410 )       /* bad format string           */
#endif

#ifndef ESFMTTYP
#define ESFMTTYP        ( 411 )       /* bad format type             */
#endif

/* EOK may or may not be defined in errno.h */
#ifndef EOK
#define EOK             ( 0 )
#endif

#endif /* __SAFE_LIB_ERRNO_H__ */





/*------------------------------------------------------------------
 * safe_lib.h -- Safe C Library
 *
 * October 2008, Bo Berry
 * Modified 2012, Jonathan Toppins <jtoppins@users.sourceforge.net>
 *
 * Copyright (c) 2008-2013 by Cisco Systems, Inc
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *------------------------------------------------------------------
 */

#ifndef __SAFE_LIB_H__
#define __SAFE_LIB_H__

/* #include "safe_types.h"     */
/* #include "safe_lib_errno.h" */

/* C11 appendix K types - specific for bounds checking */
typedef size_t  rsize_t;

/*
 * We depart from the standard and allow memory and string operations to
 * have different max sizes. See the respective safe_mem_lib.h or
 * safe_str_lib.h files.
 */
/* #define RSIZE_MAX (~(rsize_t)0)  - leave here for completeness */

typedef void (*constraint_handler_t) (const char * /* msg */,
                                      void *       /* ptr */,
                                      errno_t      /* error */);

/*
extern void abort_handler_s(const char *msg, void *ptr, errno_t error);
extern void ignore_handler_s(const char *msg, void *ptr, errno_t error);
*/

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

void  opae_safestr_abort_handler_s(const char *msg, void *ptr, errno_t error);
void opae_safestr_ignore_handler_s(const char *msg, void *ptr, errno_t error);

#if defined(__cplusplus)
}
#endif // __cplusplus

/* #define sl_default_handler ignore_handler_s */
#define sl_default_handler opae_safestr_ignore_handler_s

/* #include "safe_mem_lib.h" */
/* #include "safe_str_lib.h" */

#endif /* __SAFE_LIB_H__ */





/*------------------------------------------------------------------
 * safe_mem_lib.h -- Safe C Library Memory APIs
 *
 * October 2008, Bo Berry
 * Modified 2012, Jonathan Toppins <jtoppins@users.sourceforge.net>
 *
 * Copyright (c) 2008-2012 by Cisco Systems, Inc.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *------------------------------------------------------------------
 */

#ifndef __SAFE_MEM_LIB_H__
#define __SAFE_MEM_LIB_H__

/* #include "safe_lib.h" */
#include <wchar.h>

#define RSIZE_MAX_MEM      ( 256UL << 20 )     /* 256MB */
#define RSIZE_MAX_MEM16    ( RSIZE_MAX_MEM/2 )
#define RSIZE_MAX_MEM32    ( RSIZE_MAX_MEM/4 )

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

/* set memory constraint handler */
/* extern constraint_handler_t
set_mem_constraint_handler_s(constraint_handler_t handler);
*/

constraint_handler_t
opae_set_mem_constraint_handler_s(constraint_handler_t handler);

/* compare memory */
extern errno_t memcmp_s(const void *dest, rsize_t dmax,
                        const void *src, rsize_t slen, int *diff);

/* compare uint16_t memory */
extern errno_t memcmp16_s(const uint16_t *dest, rsize_t dmax,
                          const uint16_t *src, rsize_t slen, int *diff);

/* compare uint32_t memory */
extern errno_t memcmp32_s(const uint32_t *dest, rsize_t dmax,
                          const uint32_t *src, rsize_t slen, int *diff);


/* copy memory */
extern errno_t memcpy_s(void *dest, rsize_t dmax,
                        const void *src, rsize_t slen);

/* copy uint16_t memory */
extern errno_t memcpy16_s(uint16_t *dest, rsize_t dmax,
                          const uint16_t *src, rsize_t slen);

/* copy uint32_t memory */
extern errno_t memcpy32_s(uint32_t *dest, rsize_t dmax,
                          const uint32_t *src, rsize_t slen);

/* copy wchar_t memory */
extern errno_t wmemcpy_s(wchar_t *dest, rsize_t dmax,
                          const wchar_t *src, rsize_t slen);


/* move memory, including overlapping memory */
extern errno_t memmove_s(void *dest, rsize_t  dmax,
                          const void *src, rsize_t slen);

/* uint16_t move memory, including overlapping memory */
extern errno_t memmove16_s(uint16_t *dest, rsize_t dmax,
                            const uint16_t *src, rsize_t slen);

/* uint32_t move memory, including overlapping memory */
extern errno_t memmove32_s(uint32_t *dest, rsize_t dmax,
                            const uint32_t *src, rsize_t slen);

/* copy wchar_t memory, including overlapping memory */
extern errno_t wmemmove_s(wchar_t *dest, rsize_t dmax,
                          const wchar_t *src, rsize_t slen);


/* set bytes */
extern errno_t memset_s(void *dest, rsize_t dmax, uint8_t value);

/* set uint16_t */
extern errno_t memset16_s(uint16_t *dest, rsize_t dmax, uint16_t value);

/* set uint32_t */
extern errno_t memset32_s(uint32_t *dest, rsize_t dmax, uint32_t value);


/* byte zero */
extern errno_t memzero_s(void *dest, rsize_t dmax);

/* uint16_t zero */
extern errno_t memzero16_s(uint16_t *dest, rsize_t dmax);

/* uint32_t zero */
extern errno_t memzero32_s(uint32_t *dest, rsize_t dmax);

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif  /* __SAFE_MEM_LIB_H__ */





/*------------------------------------------------------------------
 * safe_str_lib.h -- Safe C Library String APIs
 *
 * October 2008, Bo Berry
 *
 * Copyright (c) 2008-2011, 2013 by Cisco Systems, Inc.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *------------------------------------------------------------------
 */

#ifndef __SAFE_STR_LIB_H__
#define __SAFE_STR_LIB_H__

/* #include "safe_lib.h" */

/*
 * The shortest string is a null string!!
 */
#define RSIZE_MIN_STR      ( 1 )

/* maximum sring length */
#define RSIZE_MAX_STR      ( 4UL << 10 )      /* 4KB */


/* The makeup of a password */
#define SAFE_STR_MIN_LOWERCASE     ( 2 )
#define SAFE_STR_MIN_UPPERCASE     ( 2 )
#define SAFE_STR_MIN_NUMBERS       ( 1 )
#define SAFE_STR_MIN_SPECIALS      ( 1 )

#define SAFE_STR_PASSWORD_MIN_LENGTH   ( 6 )
#define SAFE_STR_PASSWORD_MAX_LENGTH   ( 32 )

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

/* set string constraint handler */
/* extern constraint_handler_t
set_str_constraint_handler_s(constraint_handler_t handler);
*/

constraint_handler_t
opae_set_str_constraint_handler_s(constraint_handler_t handler);

/* string compare */
extern errno_t
strcasecmp_s(const char *dest, rsize_t dmax,
             const char *src, int *indicator);


/* find a substring _ case insensitive */
extern errno_t
strcasestr_s(char *dest, rsize_t dmax,
             const char *src, rsize_t slen, char **substring);


/* string concatenate */
extern errno_t
strcat_s(char *dest, rsize_t dmax, const char *src);


/* string compare */
extern errno_t
strcmp_s(const char *dest, rsize_t dmax,
         const char *src, int *indicator);


/* fixed field string compare */
extern errno_t
strcmpfld_s(const char *dest, rsize_t dmax,
            const char *src, int *indicator);


/* string copy */
extern errno_t
strcpy_s(char *dest, rsize_t dmax, const char *src);

/* string copy */
extern char *
stpcpy_s(char *dest, rsize_t dmax, const char *src, errno_t *err);

/* fixed char array copy */
extern errno_t
strcpyfld_s(char *dest, rsize_t dmax, const char *src, rsize_t slen);


/* copy from a null terminated string to fixed char array */
extern errno_t
strcpyfldin_s(char *dest, rsize_t dmax, const char *src, rsize_t slen);


/* copy from a char array to null terminated string */
extern errno_t
strcpyfldout_s(char *dest, rsize_t dmax, const char *src, rsize_t slen);


/* computes excluded prefix length */
extern errno_t
strcspn_s(const char *dest, rsize_t dmax,
          const char *src,  rsize_t slen, rsize_t *count);


/* returns a pointer to the first occurrence of c in dest */
extern errno_t
strfirstchar_s(char *dest, rsize_t dmax, char c, char **first);


/* returns index of first difference */
extern  errno_t
strfirstdiff_s(const char *dest, rsize_t dmax,
               const char *src, rsize_t *index);


/* validate alphanumeric string */
extern bool
strisalphanumeric_s(const char *str, rsize_t slen);


/* validate ascii string */
extern bool
strisascii_s(const char *str, rsize_t slen);


/* validate string of digits */
extern bool
strisdigit_s(const char *str, rsize_t slen);


/* validate hex string */
extern bool
strishex_s(const char *str, rsize_t slen);


/* validate lower case */
extern bool
strislowercase_s(const char *str, rsize_t slen);


/* validate mixed case */
extern bool
strismixedcase_s(const char *str, rsize_t slen);


/* validate password */
extern bool
strispassword_s(const char *str, rsize_t slen);


/* validate upper case */
extern bool
strisuppercase_s(const char *str, rsize_t slen);


/* returns  a pointer to the last occurrence of c in s1 */
extern errno_t
strlastchar_s(char *str, rsize_t smax, char c, char **first);


/* returns index of last difference */
extern  errno_t
strlastdiff_s(const char *dest, rsize_t dmax,
              const char *src, rsize_t *index);


/* left justify */
extern errno_t
strljustify_s(char *dest, rsize_t dmax);


/* fitted string concatenate */
extern errno_t
strncat_s(char *dest, rsize_t dmax, const char *src, rsize_t slen);


/* fitted string copy */
extern errno_t
strncpy_s(char *dest, rsize_t dmax, const char *src, rsize_t slen);


/* string length */
extern rsize_t
strnlen_s (const char *s, rsize_t smax);


/* string terminate */
extern rsize_t
strnterminate_s (char *s, rsize_t smax);


/* get pointer to first occurrence from set of char */
extern errno_t
strpbrk_s(char *dest, rsize_t dmax,
          char *src,  rsize_t slen, char **first);


extern errno_t
strfirstsame_s(const char *dest, rsize_t dmax,
               const char *src,  rsize_t *index);

extern errno_t
strlastsame_s(const char *dest, rsize_t dmax,
              const char *src, rsize_t *index);


/* searches for a prefix */
extern errno_t
strprefix_s(const char *dest, rsize_t dmax, const char *src);


/* removes leading and trailing white space */
extern errno_t
strremovews_s(char *dest, rsize_t dmax);


/* computes inclusive prefix length */
extern errno_t
strspn_s(const char *dest, rsize_t dmax,
         const char *src,  rsize_t slen, rsize_t *count);


/* find a substring */
extern errno_t
strstr_s(char *dest, rsize_t dmax,
         const char *src, rsize_t slen, char **substring);


/* string tokenizer */
extern char *
strtok_s(char *s1, rsize_t *s1max, const char *src, char **ptr);


/* convert string to lowercase */
extern errno_t
strtolowercase_s(char *str, rsize_t slen);


/* convert string to uppercase */
extern errno_t
strtouppercase_s(char *str, rsize_t slen);


/* zero an entire string with nulls */
extern errno_t
strzero_s(char *dest, rsize_t dmax);

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif   /* __SAFE_STR_LIB_H__ */





/*------------------------------------------------------------------
 * sprintf_s.h -- Safe Sprintf Interfaces
 *
 * August 2014, D Wheeler
 *
 * Copyright (c) 2014 by Intel Corp
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *------------------------------------------------------------------
 */
#ifndef SPRINTF_S_H_
#define SPRINTF_S_H_

/* #include <safe_lib_errno.h> */


#define SNPRFNEGATE(x) (-1*(x))

#if defined(__cplusplus)
extern "C" {
#endif // __cplusplus

int snprintf_s_i(char *dest, rsize_t dmax, const char *format, int a);
int sscanf_s_i(const char *src, const char *format, int *a);
int sscanf_s_u(const char *src, const char *format, unsigned *a);
int snprintf_s_l(char *dest, rsize_t dmax, const char *format, long a);
int snprintf_s_s(char *dest, rsize_t dmax, const char *format, const char *s);

int snprintf_s_ss(char *dest, rsize_t dmax, const char *format, const char *s, const char *t);
int snprintf_s_ii(char *dest, rsize_t dmax, const char *format, int a, int b);
int sscanf_s_ii(const char *src, const char *format, int *a, int *b);
int snprintf_s_si(char *dest, rsize_t dmax, const char *format, const char *s, int a);
int snprintf_s_is(char *dest, rsize_t dmax, const char *format, int a, const char *s);
int snprintf_s_sl(char *dest, rsize_t dmax, const char *format, const char *s, long a);
int snprintf_s_il(char *dest, rsize_t dmax, const char *format, int a, long b);

int snprintf_s_iis(char *dest, rsize_t dmax, const char *format, int a, int b, const char *s);

int snprintf_s_iiii(char *dest, rsize_t dmax, const char *format, int a, int b, int c, int d);

int snprintf_s_ciii(char *dest, rsize_t dmax, const char *format, char s, int a, int b, int c);

#if defined(__cplusplus)
}
#endif // __cplusplus

#endif /* SPRINTF_S_H_ */

