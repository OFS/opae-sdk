// Copyright(c) 2018, Intel Corporation
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

#ifndef __OPAE_OPAE_INT_H__
#define __OPAE_OPAE_INT_H__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <opae/types.h>

/*
 * Convenience macros for printing messages and errors.
 */
#ifdef __SHORT_FILE__
#undef __SHORT_FILE__
#endif // __SHORT_FILE__
#define __SHORT_FILE__                                                         \
	({                                                                     \
		const char *file = __FILE__;                                   \
		const char *p = file;                                          \
		while (*p)                                                     \
			++p;                                                   \
		while ((p > file) && ('/' != *p) && ('\\' != *p))              \
			--p;                                                   \
		if (p > file)                                                  \
			++p;                                                   \
		p;                                                             \
	})

#ifdef OPAE_MSG
#undef OPAE_MSG
#endif // OPAE_MSG
#define OPAE_MSG(format, ...)                                                  \
	opae_print(OPAE_LOG_MESSAGE, "libopae-c %s:%u:%s() : " format "\n",    \
		   __SHORT_FILE__, __LINE__, __func__, ##__VA_ARGS__)

#ifdef OPAE_ERR
#undef OPAE_ERR
#endif // OPAE_ERR
#define OPAE_ERR(format, ...)                                                  \
	opae_print(OPAE_LOG_ERROR,                                             \
		   "libopae-c %s:%u:%s() **ERROR** : " format "\n",            \
		   __SHORT_FILE__, __LINE__, __func__, ##__VA_ARGS__)

#ifdef OPAE_DBG
#undef OPAE_DBG
#endif // OPAE_DBG
#ifdef LIBOPAE_DEBUG
#define OPAE_DBG(format, ...)                                                  \
	opae_print(OPAE_LOG_DEBUG,                                             \
		   "libopae-c %s:%u:%s() *DEBUG* : " format "\n",              \
		   __SHORT_FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define OPAE_DBG(format, ...)                                                  \
	{                                                                      \
	}
#endif // LIBOPAE_DEBUG

/*
 * Logging functions
 */
enum opae_loglevel {
	OPAE_LOG_ERROR = 0, /* critical errors (always print) */
	OPAE_LOG_MESSAGE,   /* information (i.e. explain return code */
	OPAE_LOG_DEBUG      /* debugging (also needs #define DEBUG 1) */
};
#define OPAE_DEFAULT_LOGLEVEL OPAE_LOG_ERROR

void opae_print(int loglevel, char *fmt, ...);



/* Macro for defining symbol visibility */
//#define __FPGA_API__ __attribute__((visibility("default")))
//#define __FIXME_MAKE_VISIBLE__ __attribute__((visibility("default")))


#define ASSERT_NOT_NULL_MSG_RESULT(__arg, __msg, __result) \
	do {                                               \
		if (!__arg) {                              \
			OPAE_ERR(__msg);                   \
			return __result;                   \
		}                                          \
	} while (0)


/*
 * Check if argument is NULL and return FPGA_INVALID_PARAM and a message
 */
#define ASSERT_NOT_NULL_MSG(__arg, __msg) \
ASSERT_NOT_NULL_MSG_RESULT(__arg, __msg, FPGA_INVALID_PARAM)

#define ASSERT_NOT_NULL(__arg) \
ASSERT_NOT_NULL_MSG(__arg, #__arg " is NULL")

#define ASSERT_NOT_NULL_RESULT(__arg, __result) \
ASSERT_NOT_NULL_MSG_RESULT(__arg, #__arg "is NULL", __result)

#define ASSERT_RESULT(__result)    \
	if ((__result) != FPGA_OK) \
		return __result



#define UNUSED_PARAM(x) ((void)x)


#define opae_mutex_lock(__res, __mtx_ptr)                         \
	({                                                        \
		(__res) = pthread_mutex_lock(__mtx_ptr);          \
		if (__res)                                        \
			OPAE_ERR("pthread_mutex_lock failed: %s", \
					strerror(errno));         \
		__res;                                            \
	})

#define opae_mutex_unlock(__res, __mtx_ptr)                         \
	({                                                          \
		(__res) = pthread_mutex_unlock(__mtx_ptr);          \
		if (__res)                                          \
			OPAE_ERR("pthread_mutex_unlock failed: %s", \
					strerror(errno));           \
		__res;                                              \
	})




typedef struct _opae_api_adapter_table opae_api_adapter_table;

//                                  k o t w
#define OPAE_WRAPPED_TOKEN_MAGIC 0x6b6f7477

typedef struct _opae_wrapped_token {
	uint32_t magic;
	fpga_token opae_token;
	opae_api_adapter_table *adapter_table;
} opae_wrapped_token;

static inline opae_wrapped_token * opae_allocate_wrapped_token(fpga_token token,
					const opae_api_adapter_table *adapter)
{
	opae_wrapped_token *wtok = (opae_wrapped_token *)
				malloc(sizeof(opae_wrapped_token));

	if (wtok) {
		wtok->magic = OPAE_WRAPPED_TOKEN_MAGIC;
		wtok->opae_token = token;
		wtok->adapter_table = (opae_api_adapter_table *) adapter;
	}

	return wtok;
}

static inline opae_wrapped_token *opae_validate_wrapped_token(fpga_token t)
{
	opae_wrapped_token *wt;
	if (!t)
		return NULL;
	wt = (opae_wrapped_token *)t;
	return (wt->magic == OPAE_WRAPPED_TOKEN_MAGIC) ? wt : NULL;
}

static inline void opae_destroy_wrapped_token(opae_wrapped_token *wt)
{
	wt->magic = 0;
	free(wt);
}

//                                   n a h w
#define OPAE_WRAPPED_HANDLE_MAGIC 0x6e616877

typedef struct _opae_wrapped_handle {
	uint32_t magic;
	opae_wrapped_token *wrapped_token;
	fpga_handle opae_handle;
	opae_api_adapter_table *adapter_table;
} opae_wrapped_handle;

static inline opae_wrapped_handle * opae_allocate_wrapped_handle(
	opae_wrapped_token *wt,
	fpga_handle opae_handle,
	opae_api_adapter_table *adapter)
{
	opae_wrapped_handle *whan = (opae_wrapped_handle *)
				malloc(sizeof(opae_wrapped_handle));

	if (whan) {
		whan->magic = OPAE_WRAPPED_HANDLE_MAGIC;
		whan->wrapped_token = wt;
		whan->opae_handle = opae_handle;
		whan->adapter_table = adapter;
	}

	return whan;
}

static inline opae_wrapped_handle *opae_validate_wrapped_handle(fpga_handle h)
{
	opae_wrapped_handle *wh;
	if (!h)
		return NULL;
	wh = (opae_wrapped_handle *)h;
	return (wh->magic == OPAE_WRAPPED_HANDLE_MAGIC) ? wh : NULL;
}

static inline void opae_destroy_wrapped_handle(opae_wrapped_handle *wh)
{
	wh->magic = 0;
	free(wh);
}

//                                       o r p w
#define OPAE_WRAPPED_PROPERTIES_MAGIC 0x6f727077

typedef struct _opae_wrapped_properties {
	uint32_t magic;
	fpga_properties opae_properties;
	opae_api_adapter_table *adapter_table;
} opae_wrapped_properties;

static inline opae_wrapped_properties * opae_allocate_wrapped_properties(
	fpga_properties opae_properties,
	opae_api_adapter_table *adapter)
{
	opae_wrapped_properties *wprop = (opae_wrapped_properties *)
					malloc(sizeof(opae_wrapped_properties));

	if (wprop) {
		wprop->magic = OPAE_WRAPPED_PROPERTIES_MAGIC;
		wprop->opae_properties = opae_properties;
		wprop->adapter_table = adapter;
	}

	return wprop;
}

static inline opae_wrapped_properties *opae_validate_wrapped_properties(
					fpga_properties p)
{
	opae_wrapped_properties *wp;
	if (!p)
		return NULL;
	wp = (opae_wrapped_properties *)p;
	return (wp->magic == OPAE_WRAPPED_PROPERTIES_MAGIC) ? wp : NULL;
}

static inline void opae_destroy_wrapped_properties(opae_wrapped_properties *wp)
{
	wp->magic = 0;
	free(wp);
}

//                                         e v e w
#define OPAE_WRAPPED_EVENT_HANDLE_MAGIC 0x65766577

#define OPAE_WRAPPED_EVENT_HANDLE_CREATED 0x00000001

typedef struct _opae_wrapped_event_handle {
	uint32_t magic;
	uint32_t flags;
	fpga_event_handle opae_event_handle;
	opae_api_adapter_table *adapter_table;
} opae_wrapped_event_handle;

static inline opae_wrapped_event_handle * opae_allocate_wrapped_event_handle(
	fpga_event_handle opae_event_handle,
	opae_api_adapter_table *adapter)
{
	opae_wrapped_event_handle *wevent = (opae_wrapped_event_handle *)
				malloc(sizeof(opae_wrapped_event_handle));

	if (wevent) {
		wevent->magic = OPAE_WRAPPED_EVENT_HANDLE_MAGIC;
		wevent->flags = 0;
		wevent->opae_event_handle = opae_event_handle;
		wevent->adapter_table = adapter;
	}

	return wevent;
}

static inline opae_wrapped_event_handle *opae_validate_wrapped_event_handle(
					fpga_event_handle h)
{
	opae_wrapped_event_handle *we;
	if (!h)
		return NULL;
	we = (opae_wrapped_event_handle *)h;
	return (we->magic == OPAE_WRAPPED_EVENT_HANDLE_MAGIC) ? we : NULL;
}

static inline void opae_destroy_wrapped_event_handle(opae_wrapped_event_handle *we)
{
	we->magic = 0;
	free(we);
}

#endif // ___OPAE_OPAE_INT_H__
