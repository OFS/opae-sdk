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
#include <opae/log.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif // __USE_GNU
#include <pthread.h>

/* Macro for defining symbol visibility */
//#define __FPGA_API__ __attribute__((visibility("default")))
//#define __FIXME_MAKE_VISIBLE__ __attribute__((visibility("default")))


#define ASSERT_NOT_NULL_MSG_RESULT(__arg, __msg, __result)                     \
	do {                                                                   \
		if (!__arg) {                                                  \
			OPAE_ERR(__msg);                                       \
			return __result;                                       \
		}                                                              \
	} while (0)

/*
 * Check if argument is NULL and return FPGA_INVALID_PARAM and a message
 */
#define ASSERT_NOT_NULL_MSG(__arg, __msg)                                      \
	ASSERT_NOT_NULL_MSG_RESULT(__arg, __msg, FPGA_INVALID_PARAM)

#define ASSERT_NOT_NULL(__arg) ASSERT_NOT_NULL_MSG(__arg, #__arg " is NULL")

#define ASSERT_NOT_NULL_RESULT(__arg, __result)                                \
	ASSERT_NOT_NULL_MSG_RESULT(__arg, #__arg "is NULL", __result)

#define ASSERT_RESULT(__result)                                                \
	do {                                                                   \
		if ((__result) != FPGA_OK)                                     \
			return __result;                                       \
	} while (0)


#define UNUSED_PARAM(x) ((void)x)


#define opae_mutex_lock(__res, __mtx_ptr)                                      \
	({                                                                     \
		(__res) = pthread_mutex_lock(__mtx_ptr);                       \
		if (__res)                                                     \
			OPAE_ERR("pthread_mutex_lock failed: %s",              \
				 strerror(errno));                             \
		__res;                                                         \
	})

#define opae_mutex_unlock(__res, __mtx_ptr)                                    \
	({                                                                     \
		(__res) = pthread_mutex_unlock(__mtx_ptr);                     \
		if (__res)                                                     \
			OPAE_ERR("pthread_mutex_unlock failed: %s",            \
				 strerror(errno));                             \
		__res;                                                         \
	})


typedef struct _opae_api_adapter_table opae_api_adapter_table;

//                                  k o t w
#define OPAE_WRAPPED_TOKEN_MAGIC 0x6b6f7477

typedef struct _opae_wrapped_token {
	uint32_t magic;
	fpga_token opae_token;
	opae_api_adapter_table *adapter_table;
} opae_wrapped_token;

opae_wrapped_token *
opae_allocate_wrapped_token(fpga_token token,
			    const opae_api_adapter_table *adapter);

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

opae_wrapped_handle *
opae_allocate_wrapped_handle(opae_wrapped_token *wt, fpga_handle opae_handle,
			     opae_api_adapter_table *adapter);

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

//                                         e v e w
#define OPAE_WRAPPED_EVENT_HANDLE_MAGIC 0x65766577

#define OPAE_WRAPPED_EVENT_HANDLE_CREATED 0x00000001

typedef struct _opae_wrapped_event_handle {
	uint32_t magic;
	pthread_mutex_t lock;
	uint32_t flags;
	fpga_event_handle opae_event_handle;
	opae_api_adapter_table *adapter_table;
} opae_wrapped_event_handle;

opae_wrapped_event_handle *
opae_allocate_wrapped_event_handle(fpga_event_handle opae_event_handle,
				   opae_api_adapter_table *adapter);

static inline opae_wrapped_event_handle *
opae_validate_wrapped_event_handle(fpga_event_handle h)
{
	opae_wrapped_event_handle *we;
	if (!h)
		return NULL;
	we = (opae_wrapped_event_handle *)h;
	return (we->magic == OPAE_WRAPPED_EVENT_HANDLE_MAGIC) ? we : NULL;
}

static inline void
opae_destroy_wrapped_event_handle(opae_wrapped_event_handle *we)
{
	int err;
	opae_mutex_lock(err, &we->lock);
	we->magic = 0;
	opae_mutex_unlock(err, &we->lock);
	if (pthread_mutex_destroy(&we->lock))
		OPAE_ERR("pthread_mutex_destroy() failed");
	free(we);
}

//                                   j b o w
#define OPAE_WRAPPED_OBJECT_MAGIC 0x6a626f77

typedef struct _opae_wrapped_object {
	uint32_t magic;
	fpga_object opae_object;
	opae_api_adapter_table *adapter_table;
} opae_wrapped_object;

opae_wrapped_object *
opae_allocate_wrapped_object(fpga_object opae_object,
			     opae_api_adapter_table *adapter);

static inline opae_wrapped_object *opae_validate_wrapped_object(fpga_object o)
{
	opae_wrapped_object *wo;
	if (!o)
		return NULL;
	wo = (opae_wrapped_object *)o;
	return (wo->magic == OPAE_WRAPPED_OBJECT_MAGIC) ? wo : NULL;
}

static inline void opae_destroy_wrapped_object(opae_wrapped_object *wo)
{
	wo->magic = 0;
	free(wo);
}

//                                         f e a t
#define OPAE_WRAPPED_FEATURE_TOKEN_MAGIC 0x66656174

typedef struct _opae_wrapped_feature_token {
	uint64_t magic;
	fpga_feature_token feature_token;
	opae_api_adapter_table *adapter_table;
} opae_wrapped_feature_token;

opae_wrapped_feature_token *
opae_allocate_wrapped_feature_token(fpga_feature_token token,
			    const opae_api_adapter_table *adapter);

static inline opae_wrapped_feature_token *opae_validate_wrapped_feature_token(fpga_feature_token t)
{
	opae_wrapped_feature_token *wt;
	if (!t)
		return NULL;
	wt = (opae_wrapped_feature_token *)t;
	return (wt->magic == OPAE_WRAPPED_FEATURE_TOKEN_MAGIC) ? wt : NULL;
}

static inline void opae_destroy_wrapped_feature_token(opae_wrapped_feature_token *wt)
{
	wt->magic = 0;
	free(wt);
}
//                                          f e a h
#define OPAE_WRAPPED_FEATURE_HANDLE_MAGIC 0x66656168

/** Process-wide unique FPGA feature handle */
typedef struct _opae_wrapped_feature_handle {
	uint32_t magic;
	opae_wrapped_feature_token *wrapped_feature_token;
	fpga_feature_handle feature_handle;
	opae_api_adapter_table *adapter_table;
} opae_wrapped_feature_handle;

opae_wrapped_feature_handle *
opae_allocate_wrapped_feature_handle(opae_wrapped_feature_token *t, fpga_feature_handle opae_feature_handle,
			     opae_api_adapter_table *adapter);

static inline opae_wrapped_feature_handle *opae_validate_wrapped_feature_handle(fpga_feature_handle h)
{
	opae_wrapped_feature_handle *wh;
	if (!h)
		return NULL;
	wh = (opae_wrapped_feature_handle *)h;
	return (wh->magic == OPAE_WRAPPED_FEATURE_HANDLE_MAGIC) ? wh : NULL;
}

static inline void opae_destroy_wrapped_feature_handle(opae_wrapped_feature_handle *wh)
{
	wh->magic = 0;
	free(wh);
}

#endif // ___OPAE_OPAE_INT_H__
