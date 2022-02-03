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
#ifndef OFS_LOG_H
#define OFS_LOG_H
#include <string.h>
#include <errno.h>

#ifdef __SHORT_FILE__
#undef __SHORT_FILE__
#endif // __SHORT_FILE__
#define __SHORT_FILE__                                   \
({                                                       \
	const char *file = __FILE__;                     \
	const char *p = file;                            \
	while (*p)                                       \
		++p;                                     \
	while((p > file) && ('/' != *p) && ('\\' != *p)) \
		--p;                                     \
	if (p > file)                                    \
		++p;                                     \
	p;                                               \
})

#ifdef OFS_MSG
#undef OFS_MSG
#endif // OFS_MSG
#define OFS_MSG(__fmt, ...) \
ofs_print(OFS_LOG_MESSAGE, "%s:%u:%s() : " __fmt "\n", \
	  __SHORT_FILE__, __LINE__, __func__, ##__VA_ARGS__)

#ifdef OFS_ERR
#undef OFS_ERR
#endif // OFS_ERR
#define OFS_ERR(__fmt, ...) \
ofs_print(OFS_LOG_ERROR, "%s:%u:%s() **ERROR** : [errno=%s] " __fmt "\n", \
	  __SHORT_FILE__, __LINE__, __func__, strerror(errno), ##__VA_ARGS__)

#ifdef OFS_DBG
#undef OFS_DBG
#endif // OFS_DBG
#ifdef LIBOFS_DEBUG
#define OFS_DBG(__fmt, ...) \
ofs_print(OFS_LOG_DEBUG, "%s:%u:%s() *DEBUG* : " __fmt "\n", \
	  __SHORT_FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define OFS_DBG(__fmt, ...)
#endif // LIBOFS_DEBUG

enum ofs_log_level {
	OFS_LOG_ERROR = 0,
	OFS_LOG_MESSAGE,
	OFS_LOG_DEBUG
};

#define OFS_DEFAULT_LOG_LEVEL OFS_LOG_ERROR

#ifdef __cplusplus
extern "C" {
#endif

void ofs_print(int level, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /* !OFS_LOG_H */
