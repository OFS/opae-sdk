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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

#include <opae/properties.h>

#include "safe_string/safe_string.h"

#include "opae_int.h"
#include "pluginmgr.h"
#include "props.h"


opae_wrapped_token *
opae_allocate_wrapped_token(fpga_token token,
			    const opae_api_adapter_table *adapter)
{
	opae_wrapped_token *wtok =
		(opae_wrapped_token *)malloc(sizeof(opae_wrapped_token));

	if (wtok) {
		wtok->magic = OPAE_WRAPPED_TOKEN_MAGIC;
		wtok->opae_token = token;
		wtok->adapter_table = (opae_api_adapter_table *)adapter;
	}

	return wtok;
}

opae_wrapped_handle *
opae_allocate_wrapped_handle(opae_wrapped_token *wt, fpga_handle opae_handle,
			     opae_api_adapter_table *adapter)
{
	opae_wrapped_handle *whan =
		(opae_wrapped_handle *)malloc(sizeof(opae_wrapped_handle));

	if (whan) {
		whan->magic = OPAE_WRAPPED_HANDLE_MAGIC;
		whan->wrapped_token = wt;
		whan->opae_handle = opae_handle;
		whan->adapter_table = adapter;
	}

	return whan;
}

opae_wrapped_event_handle *
opae_allocate_wrapped_event_handle(fpga_event_handle opae_event_handle,
				   opae_api_adapter_table *adapter)
{
	pthread_mutexattr_t mattr;
	opae_wrapped_event_handle *wevent = (opae_wrapped_event_handle *)malloc(
		sizeof(opae_wrapped_event_handle));

	if (wevent) {
		if (pthread_mutexattr_init(&mattr)) {
			OPAE_ERR("pthread_mutexattr_init() failed");
			goto out_free;
		}
		if (pthread_mutexattr_settype(&mattr,
					      PTHREAD_MUTEX_RECURSIVE)) {
			OPAE_ERR("pthread_mutexattr_settype() failed");
			goto out_destroy;
		}
		if (pthread_mutex_init(&wevent->lock, &mattr)) {
			OPAE_ERR("pthread_mutex_init() failed");
			goto out_destroy;
		}

		pthread_mutexattr_destroy(&mattr);

		wevent->magic = OPAE_WRAPPED_EVENT_HANDLE_MAGIC;
		wevent->flags = 0;
		wevent->opae_event_handle = opae_event_handle;
		wevent->adapter_table = adapter;
	}

	return wevent;

out_destroy:
	pthread_mutexattr_destroy(&mattr);
out_free:
	free(wevent);
	return NULL;
}

opae_wrapped_object *
opae_allocate_wrapped_object(fpga_object opae_object,
			     opae_api_adapter_table *adapter)
{
	opae_wrapped_object *wobj =
		(opae_wrapped_object *)malloc(sizeof(opae_wrapped_object));

	if (wobj) {
		wobj->magic = OPAE_WRAPPED_OBJECT_MAGIC;
		wobj->opae_object = opae_object;
		wobj->adapter_table = adapter;
	}

	return wobj;
}

opae_wrapped_feature_token *
opae_allocate_wrapped_feature_token(fpga_feature_token token,
			    const opae_api_adapter_table *adapter)
{
	opae_wrapped_feature_token *wtok =
		(opae_wrapped_feature_token *)malloc(sizeof(opae_wrapped_feature_token));

	if (wtok) {
		wtok->magic = OPAE_WRAPPED_FEATURE_TOKEN_MAGIC;
		wtok->feature_token = token;
		wtok->adapter_table = (opae_api_adapter_table *)adapter;
	}

	return wtok;
}

opae_wrapped_feature_handle *
opae_allocate_wrapped_feature_handle(opae_wrapped_feature_token *wt, fpga_feature_handle feature_handle,
			     opae_api_adapter_table *adapter)
{
	opae_wrapped_feature_handle *whan =
		(opae_wrapped_feature_handle *)malloc(sizeof(opae_wrapped_feature_handle));

	if (whan) {
		whan->magic = OPAE_WRAPPED_FEATURE_HANDLE_MAGIC;
		whan->wrapped_feature_token = wt;
		whan->feature_handle = feature_handle;
		whan->adapter_table = adapter;
	}

	return whan;
}

fpga_result fpgaInitialize(const char *config_file)
{
	return opae_plugin_mgr_initialize(config_file) ? FPGA_EXCEPTION
						       : FPGA_OK;
}

fpga_result fpgaFinalize(void)
{
	return opae_plugin_mgr_finalize_all() ? FPGA_EXCEPTION
					      : FPGA_OK;
}

fpga_result fpgaOpen(fpga_token token, fpga_handle *handle, int flags)
{
	fpga_result res;
	fpga_result cres = FPGA_OK;
	opae_wrapped_token *wrapped_token;
	fpga_handle opae_handle = NULL;
	opae_wrapped_handle *wrapped_handle;

	wrapped_token = opae_validate_wrapped_token(token);

	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL(handle);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaOpen,
			       FPGA_NOT_SUPPORTED);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaClose,
			       FPGA_NOT_SUPPORTED);

	res = wrapped_token->adapter_table->fpgaOpen(wrapped_token->opae_token,
						     &opae_handle, flags);

	ASSERT_RESULT(res);

	wrapped_handle = opae_allocate_wrapped_handle(
		wrapped_token, opae_handle, wrapped_token->adapter_table);

	if (!wrapped_handle) {
		OPAE_ERR("malloc failed");
		res = FPGA_NO_MEMORY;
		cres = wrapped_token->adapter_table->fpgaClose(opae_handle);
	}

	*handle = wrapped_handle;

	return res != FPGA_OK ? res : cres;
}

fpga_result fpgaClose(fpga_handle handle)
{
	fpga_result res;
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaClose,
			       FPGA_NOT_SUPPORTED);

	res = wrapped_handle->adapter_table->fpgaClose(
		wrapped_handle->opae_handle);

	opae_destroy_wrapped_handle(wrapped_handle);

	return res;
}

fpga_result fpgaReset(fpga_handle handle)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaReset,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaReset(
		wrapped_handle->opae_handle);
}

fpga_result fpgaGetPropertiesFromHandle(fpga_handle handle,
					fpga_properties *prop)
{
	fpga_result res;
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);
	struct _fpga_properties *p;
	int err;

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(prop);
	ASSERT_NOT_NULL_RESULT(
		wrapped_handle->adapter_table->fpgaGetPropertiesFromHandle,
		FPGA_NOT_SUPPORTED);

	res = wrapped_handle->adapter_table->fpgaGetPropertiesFromHandle(
		wrapped_handle->opae_handle, prop);

	ASSERT_RESULT(res);

	// If the output properties has a parent token set,
	// then it will be a raw token. We need to wrap it.

	p = opae_validate_and_lock_properties(*prop);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_PARENT)) {
		opae_wrapped_token *wrapped_parent =
			opae_allocate_wrapped_token(
				p->parent, wrapped_handle->adapter_table);

		if (wrapped_parent) {
			p->parent = wrapped_parent;
			p->flags |= OPAE_PROPERTIES_FLAG_PARENT_ALLOC;
		} else {
			OPAE_ERR("malloc failed");
			res = FPGA_NO_MEMORY;
		}
	}

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result fpgaGetProperties(fpga_token token, fpga_properties *prop)
{
	fpga_result res = FPGA_OK;
	opae_wrapped_token *wrapped_token = opae_validate_wrapped_token(token);

	ASSERT_NOT_NULL(prop);

	if (!token) {
		fpga_properties pr;

		pr = opae_properties_create();

		if (!pr) {
			OPAE_ERR("malloc failed");
			return FPGA_NO_MEMORY;
		}

		*prop = pr;

	} else {
		struct _fpga_properties *p;
		int err;

		ASSERT_NOT_NULL(wrapped_token);

		ASSERT_NOT_NULL_RESULT(
			wrapped_token->adapter_table->fpgaGetProperties,
			FPGA_NOT_SUPPORTED);

		res = wrapped_token->adapter_table->fpgaGetProperties(
			wrapped_token->opae_token, prop);

		ASSERT_RESULT(res);

		// If the output properties has a parent token set,
		// then it will be a raw token. We need to wrap it.

		p = opae_validate_and_lock_properties(*prop);

		ASSERT_NOT_NULL(p);

		if (FIELD_VALID(p, FPGA_PROPERTY_PARENT)) {
			opae_wrapped_token *wrapped_parent =
				opae_allocate_wrapped_token(
					p->parent,
					wrapped_token->adapter_table);

			if (wrapped_parent) {
				p->parent = wrapped_parent;
				p->flags |= OPAE_PROPERTIES_FLAG_PARENT_ALLOC;
			} else {
				OPAE_ERR("malloc failed");
				res = FPGA_NO_MEMORY;
			}
		}

		opae_mutex_unlock(err, &p->lock);
	}

	return res;
}

fpga_result fpgaUpdateProperties(fpga_token token, fpga_properties prop)
{
	fpga_result res;
	struct _fpga_properties *p;
	int err;
	opae_wrapped_token *wrapped_token = opae_validate_wrapped_token(token);
	opae_wrapped_token *wrapped_parent = NULL;

	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL_RESULT(
		wrapped_token->adapter_table->fpgaUpdateProperties,
		FPGA_NOT_SUPPORTED);

	// If the input properties already has a parent token
	// set, then it will be wrapped. If we allocated the wrapper,
	// Save the wrapper, and reuse it below.

	p = opae_validate_and_lock_properties(prop);

	ASSERT_NOT_NULL(p);

	if (FIELD_VALID(p, FPGA_PROPERTY_PARENT)
	    && (p->flags & OPAE_PROPERTIES_FLAG_PARENT_ALLOC)) {
		wrapped_parent = opae_validate_wrapped_token(p->parent);
		if (wrapped_parent)
			p->parent = wrapped_parent->opae_token;
	}

	res = wrapped_token->adapter_table->fpgaUpdateProperties(
		wrapped_token->opae_token, prop);

	if (res != FPGA_OK) {
		opae_mutex_unlock(err, &p->lock);
		return res;
	}

	// If the output properties has a parent token set,
	// then it will be a raw token. We need to wrap it.

	if (FIELD_VALID(p, FPGA_PROPERTY_PARENT)) {
		if (!wrapped_parent) {
			// We need to allocate a wrapper.
			wrapped_parent = opae_allocate_wrapped_token(
				p->parent, wrapped_token->adapter_table);

			if (wrapped_parent) {
				p->parent = wrapped_parent;
				p->flags |= OPAE_PROPERTIES_FLAG_PARENT_ALLOC;
			} else {
				OPAE_ERR("malloc failed");
				res = FPGA_NO_MEMORY;
			}
		} else {
			// We are re-using the wrapper from above.
			wrapped_parent->opae_token = p->parent;
			wrapped_parent->adapter_table =
				wrapped_token->adapter_table;
			p->parent = wrapped_parent;
			p->flags |= OPAE_PROPERTIES_FLAG_PARENT_ALLOC;
		}
	} else if (wrapped_parent)
		opae_destroy_wrapped_token(wrapped_parent);

	opae_mutex_unlock(err, &p->lock);

	return res;
}

fpga_result fpgaWriteMMIO64(fpga_handle handle, uint32_t mmio_num,
			    uint64_t offset, uint64_t value)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaWriteMMIO64,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaWriteMMIO64(
		wrapped_handle->opae_handle, mmio_num, offset, value);
}

fpga_result fpgaReadMMIO64(fpga_handle handle, uint32_t mmio_num,
			   uint64_t offset, uint64_t *value)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaReadMMIO64,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaReadMMIO64(
		wrapped_handle->opae_handle, mmio_num, offset, value);
}

fpga_result fpgaWriteMMIO32(fpga_handle handle, uint32_t mmio_num,
			    uint64_t offset, uint32_t value)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaWriteMMIO32,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaWriteMMIO32(
		wrapped_handle->opae_handle, mmio_num, offset, value);
}

fpga_result fpgaReadMMIO32(fpga_handle handle, uint32_t mmio_num,
			   uint64_t offset, uint32_t *value)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaReadMMIO32,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaReadMMIO32(
		wrapped_handle->opae_handle, mmio_num, offset, value);
}

fpga_result fpgaMapMMIO(fpga_handle handle, uint32_t mmio_num,
			uint64_t **mmio_ptr)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaMapMMIO,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaMapMMIO(
		wrapped_handle->opae_handle, mmio_num, mmio_ptr);
}

fpga_result fpgaUnmapMMIO(fpga_handle handle, uint32_t mmio_num)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaUnmapMMIO,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaUnmapMMIO(
		wrapped_handle->opae_handle, mmio_num);
}

typedef struct _opae_enumeration_context {
	// <verbatim from fpgaEnumerate>
	const fpga_properties *filters;
	uint32_t num_filters;
	fpga_token *wrapped_tokens;
	uint32_t max_wrapped_tokens;
	uint32_t *num_matches;
	// </verbatim from fpgaEnumerate>

	fpga_token *adapter_tokens;
	uint32_t num_wrapped_tokens;
	uint32_t errors;
} opae_enumeration_context;

static int opae_enumerate(const opae_api_adapter_table *adapter, void *context)
{
	opae_enumeration_context *ctx = (opae_enumeration_context *)context;
	fpga_result res;
	uint32_t num_matches = 0;
	uint32_t i;
	uint32_t space_remaining;

	// TODO: accept/reject this adapter, based on device support
	if (adapter->supports_device) {
	}

	// TODO: accept/reject this adapter, based on host support
	if (adapter->supports_host) {
	}

	space_remaining = ctx->max_wrapped_tokens - ctx->num_wrapped_tokens;

	if (ctx->wrapped_tokens && !space_remaining)
		return OPAE_ENUM_STOP;

	if (!adapter->fpgaEnumerate) {
		OPAE_MSG("NULL fpgaEnumerate in adapter \"%s\"",
			 adapter->plugin.path);
		return OPAE_ENUM_CONTINUE;
	}

	res = adapter->fpgaEnumerate(ctx->filters, ctx->num_filters,
				     ctx->adapter_tokens, space_remaining,
				     &num_matches);

	if (res != FPGA_OK) {
		OPAE_ERR("fpgaEnumerate() failed for \"%s\"",
			 adapter->plugin.path);
		++ctx->errors;
		return OPAE_ENUM_CONTINUE;
	}

	*ctx->num_matches += num_matches;

	if (!ctx->adapter_tokens) {
		// requesting token count, only.
		return OPAE_ENUM_CONTINUE;
	}

	if (space_remaining > num_matches)
		space_remaining = num_matches;

	for (i = 0; i < space_remaining; ++i) {
		opae_wrapped_token *wt = opae_allocate_wrapped_token(
			ctx->adapter_tokens[i], adapter);
		if (!wt) {
			++ctx->errors;
			return OPAE_ENUM_STOP;
		}

		ctx->wrapped_tokens[ctx->num_wrapped_tokens++] = wt;
	}

	return ctx->num_wrapped_tokens == ctx->max_wrapped_tokens
		       ? OPAE_ENUM_STOP
		       : OPAE_ENUM_CONTINUE;
}

fpga_result fpgaEnumerate(const fpga_properties *filters, uint32_t num_filters,
			  fpga_token *tokens, uint32_t max_tokens,
			  uint32_t *num_matches)
{
	fpga_result res = FPGA_EXCEPTION;
	fpga_token *adapter_tokens = NULL;

	opae_enumeration_context enum_context;

	typedef struct _parent_token_fixup {
		struct _parent_token_fixup *next;
		fpga_properties prop;
		opae_wrapped_token *wrapped_token;
	} parent_token_fixup;

	parent_token_fixup *ptf_list = NULL;
	uint32_t i;

	ASSERT_NOT_NULL(num_matches);

	if ((max_tokens > 0) && !tokens) {
		OPAE_ERR("max_tokens > 0 with NULL tokens");
		return FPGA_INVALID_PARAM;
	}

	if ((num_filters > 0) && !filters) {
		OPAE_ERR("num_filters > 0 with NULL filters");
		return FPGA_INVALID_PARAM;
	}

	if ((num_filters == 0) && (filters != NULL)) {
		OPAE_ERR("num_filters == 0 with non-NULL filters");
		return FPGA_INVALID_PARAM;
	}

	*num_matches = 0;

	enum_context.filters = filters;
	enum_context.num_filters = num_filters;
	enum_context.wrapped_tokens = tokens;
	enum_context.max_wrapped_tokens = max_tokens;
	enum_context.num_matches = num_matches;

	if (tokens) {
		adapter_tokens =
			(fpga_token *)calloc(max_tokens, sizeof(fpga_token));
		if (!adapter_tokens) {
			OPAE_ERR("out of memory");
			return FPGA_NO_MEMORY;
		}
	}

	enum_context.adapter_tokens = adapter_tokens;
	enum_context.num_wrapped_tokens = 0;
	enum_context.errors = 0;

	// If any of the input filters has a parent token set,
	// then it will be wrapped. We need to unwrap it here,
	// then re-wrap below.
	for (i = 0; i < num_filters; ++i) {
		int err;
		struct _fpga_properties *p =
			opae_validate_and_lock_properties(filters[i]);

		if (!p) {
			OPAE_ERR("Invalid input filter");
			res = FPGA_INVALID_PARAM;
			goto out_free_tokens;
		}

		if (FIELD_VALID(p, FPGA_PROPERTY_PARENT)) {
			parent_token_fixup *fixup;
			opae_wrapped_token *wrapped_parent =
				opae_validate_wrapped_token(p->parent);

			if (!wrapped_parent) {
				OPAE_ERR("Invalid wrapped parent in filter");
				res = FPGA_INVALID_PARAM;
				opae_mutex_unlock(err, &p->lock);
				goto out_free_tokens;
			}

			fixup = (parent_token_fixup *)malloc(
				sizeof(parent_token_fixup));

			if (!fixup) {
				OPAE_ERR("malloc failed");
				res = FPGA_NO_MEMORY;
				opae_mutex_unlock(err, &p->lock);
				goto out_free_tokens;
			}

			fixup->next = NULL;
			fixup->prop = filters[i];
			fixup->wrapped_token = wrapped_parent;

			if (!ptf_list)
				ptf_list = fixup;
			else {
				fixup->next = ptf_list;
				ptf_list = fixup;
			}

			// Set the unwrapped parent token.
			p->parent = wrapped_parent->opae_token;
		}

		opae_mutex_unlock(err, &p->lock);
	}

	// perform the enumeration.
	opae_plugin_mgr_for_each_adapter(opae_enumerate, &enum_context);

	res = (enum_context.errors > 0) ? FPGA_EXCEPTION : FPGA_OK;

out_free_tokens:
	if (adapter_tokens)
		free(adapter_tokens);

	// Re-establish any wrapped parent tokens.
	while (ptf_list) {
		int err;
		parent_token_fixup *trash = ptf_list;
		struct _fpga_properties *p =
			opae_validate_and_lock_properties(trash->prop);
		ptf_list = ptf_list->next;

		if (p) {
			p->parent = trash->wrapped_token;
			opae_mutex_unlock(err, &p->lock);
		}

		free(trash);
	}

	return res;
}

fpga_result fpgaCloneToken(fpga_token src, fpga_token *dst)
{
	fpga_result res;
	fpga_result dres = FPGA_OK;
	fpga_token cloned_token = NULL;
	opae_wrapped_token *wrapped_dst_token;
	opae_wrapped_token *wrapped_src_token =
		opae_validate_wrapped_token(src);

	ASSERT_NOT_NULL(wrapped_src_token);
	ASSERT_NOT_NULL(dst);
	ASSERT_NOT_NULL_RESULT(wrapped_src_token->adapter_table->fpgaCloneToken,
			       FPGA_NOT_SUPPORTED);
	ASSERT_NOT_NULL_RESULT(
		wrapped_src_token->adapter_table->fpgaDestroyToken,
		FPGA_NOT_SUPPORTED);

	res = wrapped_src_token->adapter_table->fpgaCloneToken(
		wrapped_src_token->opae_token, &cloned_token);

	ASSERT_RESULT(res);

	wrapped_dst_token = opae_allocate_wrapped_token(
		cloned_token, wrapped_src_token->adapter_table);

	if (!wrapped_dst_token) {
		OPAE_ERR("malloc failed");
		res = FPGA_NO_MEMORY;
		dres = wrapped_src_token->adapter_table->fpgaDestroyToken(
			&cloned_token);
	}

	*dst = wrapped_dst_token;

	return res != FPGA_OK ? res : dres;
}

fpga_result fpgaDestroyToken(fpga_token *token)
{
	fpga_result res;
	opae_wrapped_token *wrapped_token;

	ASSERT_NOT_NULL(token);

	wrapped_token = opae_validate_wrapped_token(*token);

	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaDestroyToken,
			       FPGA_NOT_SUPPORTED);

	res = wrapped_token->adapter_table->fpgaDestroyToken(
		&wrapped_token->opae_token);

	opae_destroy_wrapped_token(wrapped_token);

	return res;
}

fpga_result fpgaGetNumUmsg(fpga_handle handle, uint64_t *value)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(value);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaGetNumUmsg,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaGetNumUmsg(
		wrapped_handle->opae_handle, value);
}

fpga_result fpgaSetUmsgAttributes(fpga_handle handle, uint64_t value)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(
		wrapped_handle->adapter_table->fpgaSetUmsgAttributes,
		FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaSetUmsgAttributes(
		wrapped_handle->opae_handle, value);
}

fpga_result fpgaTriggerUmsg(fpga_handle handle, uint64_t value)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaTriggerUmsg,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaTriggerUmsg(
		wrapped_handle->opae_handle, value);
}

fpga_result fpgaGetUmsgPtr(fpga_handle handle, uint64_t **umsg_ptr)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(umsg_ptr);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaGetUmsgPtr,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaGetUmsgPtr(
		wrapped_handle->opae_handle, umsg_ptr);
}

fpga_result fpgaPrepareBuffer(fpga_handle handle, uint64_t len, void **buf_addr,
			      uint64_t *wsid, int flags)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	// A special case: allow each plugin to respond FPGA_OK
	// when !buf_addr and !len as an indication that
	// FPGA_BUF_PREALLOCATED is supported by the plugin.
	if (!(flags & FPGA_BUF_PREALLOCATED) || (len > 0)) {
		// Assert only if not the special case described above.
		ASSERT_NOT_NULL(buf_addr);
	}

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(wsid);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaPrepareBuffer,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaPrepareBuffer(
		wrapped_handle->opae_handle, len, buf_addr, wsid, flags);
}

fpga_result fpgaReleaseBuffer(fpga_handle handle, uint64_t wsid)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaReleaseBuffer,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaReleaseBuffer(
		wrapped_handle->opae_handle, wsid);
}

fpga_result fpgaGetIOAddress(fpga_handle handle, uint64_t wsid,
			     uint64_t *ioaddr)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(ioaddr);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaGetIOAddress,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaGetIOAddress(
		wrapped_handle->opae_handle, wsid, ioaddr);
}

fpga_result fpgaGetOPAECVersion(fpga_version *version)
{
	ASSERT_NOT_NULL(version);

	version->major = INTEL_FPGA_API_VER_MAJOR;
	version->minor = INTEL_FPGA_API_VER_MINOR;
	version->patch = INTEL_FPGA_API_VER_REV;

	return FPGA_OK;
}

fpga_result fpgaGetOPAECVersionString(char *version_str, size_t len)
{
	errno_t err;

	ASSERT_NOT_NULL(version_str);

	err = strncpy_s(version_str, len, INTEL_FPGA_API_VERSION,
			sizeof(INTEL_FPGA_API_VERSION));

	if (err) {
		OPAE_ERR("strncpy_s failed with error %d", err);
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

fpga_result fpgaGetOPAECBuildString(char *build_str, size_t len)
{
	errno_t err;

	ASSERT_NOT_NULL(build_str);

	err = strncpy_s(build_str, len, INTEL_FPGA_API_HASH,
			sizeof(INTEL_FPGA_API_HASH));

	if (err) {
		OPAE_ERR("strncpy_s failed with error %d", err);
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

fpga_result fpgaReadError(fpga_token token, uint32_t error_num, uint64_t *value)
{
	opae_wrapped_token *wrapped_token = opae_validate_wrapped_token(token);

	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL(value);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaReadError,
			       FPGA_NOT_SUPPORTED);

	return wrapped_token->adapter_table->fpgaReadError(
		wrapped_token->opae_token, error_num, value);
}

fpga_result fpgaClearError(fpga_token token, uint32_t error_num)
{
	opae_wrapped_token *wrapped_token = opae_validate_wrapped_token(token);

	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaClearError,
			       FPGA_NOT_SUPPORTED);

	return wrapped_token->adapter_table->fpgaClearError(
		wrapped_token->opae_token, error_num);
}

fpga_result fpgaClearAllErrors(fpga_token token)
{
	opae_wrapped_token *wrapped_token = opae_validate_wrapped_token(token);

	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaClearAllErrors,
			       FPGA_NOT_SUPPORTED);

	return wrapped_token->adapter_table->fpgaClearAllErrors(
		wrapped_token->opae_token);
}

fpga_result fpgaGetErrorInfo(fpga_token token, uint32_t error_num,
			     struct fpga_error_info *error_info)
{
	opae_wrapped_token *wrapped_token = opae_validate_wrapped_token(token);

	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL(error_info);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaGetErrorInfo,
			       FPGA_NOT_SUPPORTED);

	return wrapped_token->adapter_table->fpgaGetErrorInfo(
		wrapped_token->opae_token, error_num, error_info);
}

const char *fpgaErrStr(fpga_result e)
{
	switch (e) {
	case FPGA_OK:
		return "success";
	case FPGA_INVALID_PARAM:
		return "invalid parameter";
	case FPGA_BUSY:
		return "resource busy";
	case FPGA_EXCEPTION:
		return "exception";
	case FPGA_NOT_FOUND:
		return "not found";
	case FPGA_NO_MEMORY:
		return "no memory";
	case FPGA_NOT_SUPPORTED:
		return "not supported";
	case FPGA_NO_DRIVER:
		return "no driver available";
	case FPGA_NO_DAEMON:
		return "no fpga daemon running";
	case FPGA_NO_ACCESS:
		return "insufficient privileges";
	case FPGA_RECONF_ERROR:
		return "reconfiguration error";
	default:
		return "unknown error";
	}
}

fpga_result fpgaCreateEventHandle(fpga_event_handle *event_handle)
{
	opae_wrapped_event_handle *wrapped_event_handle;

	ASSERT_NOT_NULL(event_handle);

	// We don't have an adapter table yet, so just create an empty object.
	wrapped_event_handle = opae_allocate_wrapped_event_handle(NULL, NULL);

	ASSERT_NOT_NULL_RESULT(wrapped_event_handle, FPGA_NO_MEMORY);

	*event_handle = wrapped_event_handle;

	return FPGA_OK;
}

fpga_result fpgaDestroyEventHandle(fpga_event_handle *event_handle)
{
	fpga_result res = FPGA_OK;
	opae_wrapped_event_handle *wrapped_event_handle;
	int ires;

	ASSERT_NOT_NULL(event_handle);

	wrapped_event_handle =
		opae_validate_wrapped_event_handle(*event_handle);

	ASSERT_NOT_NULL(wrapped_event_handle);

	opae_mutex_lock(ires, &wrapped_event_handle->lock);

	if (wrapped_event_handle->flags & OPAE_WRAPPED_EVENT_HANDLE_CREATED) {

		if (!wrapped_event_handle->adapter_table
			     ->fpgaDestroyEventHandle) {
			OPAE_ERR("NULL fpgaDestroyEventHandle() in adapter.");
			opae_mutex_unlock(ires, &wrapped_event_handle->lock);
			return FPGA_NOT_SUPPORTED;
		}

		if (!wrapped_event_handle->opae_event_handle) {
			OPAE_ERR("NULL fpga_event_handle in wrapper.");
			opae_mutex_unlock(ires, &wrapped_event_handle->lock);
			return FPGA_INVALID_PARAM;
		}

		res = wrapped_event_handle->adapter_table
			      ->fpgaDestroyEventHandle(
				      &wrapped_event_handle->opae_event_handle);
	}

	opae_mutex_unlock(ires, &wrapped_event_handle->lock);

	opae_destroy_wrapped_event_handle(wrapped_event_handle);

	return res;
}

fpga_result fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh, int *fd)
{
	fpga_result res;
	opae_wrapped_event_handle *wrapped_event_handle =
		opae_validate_wrapped_event_handle(eh);
	int ires;

	ASSERT_NOT_NULL(fd);
	ASSERT_NOT_NULL(wrapped_event_handle);

	opae_mutex_lock(ires, &wrapped_event_handle->lock);

	if (!(wrapped_event_handle->flags
	      & OPAE_WRAPPED_EVENT_HANDLE_CREATED)) {
		OPAE_ERR(
			"Attempting to query OS event object before event handle is registered.");
		opae_mutex_unlock(ires, &wrapped_event_handle->lock);
		return FPGA_INVALID_PARAM;
	}

	if (!wrapped_event_handle->opae_event_handle) {
		OPAE_ERR("NULL fpga_event_handle in wrapper.");
		opae_mutex_unlock(ires, &wrapped_event_handle->lock);
		return FPGA_INVALID_PARAM;
	}

	if (!wrapped_event_handle->adapter_table
		     ->fpgaGetOSObjectFromEventHandle) {
		OPAE_ERR("NULL fpgaGetOSObjectFromEventHandle in adapter.");
		opae_mutex_unlock(ires, &wrapped_event_handle->lock);
		return FPGA_NOT_SUPPORTED;
	}

	res = wrapped_event_handle->adapter_table
		      ->fpgaGetOSObjectFromEventHandle(
			      wrapped_event_handle->opae_event_handle, fd);

	opae_mutex_unlock(ires, &wrapped_event_handle->lock);

	return res;
}

fpga_result fpgaRegisterEvent(fpga_handle handle, fpga_event_type event_type,
			      fpga_event_handle event_handle, uint32_t flags)
{
	fpga_result res = FPGA_OK;
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);
	opae_wrapped_event_handle *wrapped_event_handle =
		opae_validate_wrapped_event_handle(event_handle);
	int ires;

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(wrapped_event_handle);

	opae_mutex_lock(ires, &wrapped_event_handle->lock);

	if (!(wrapped_event_handle->flags
	      & OPAE_WRAPPED_EVENT_HANDLE_CREATED)) {
		// Now that we have an adapter table, store the adapter in
		// the wrapped_event_handle, and create the event handle.

		if (!wrapped_handle->adapter_table->fpgaCreateEventHandle) {
			OPAE_ERR("NULL fpgaCreateEventHandle() in adapter.");
			opae_mutex_unlock(ires, &wrapped_event_handle->lock);
			return FPGA_NOT_SUPPORTED;
		}

		res = wrapped_handle->adapter_table->fpgaCreateEventHandle(
			&wrapped_event_handle->opae_event_handle);

		if (res != FPGA_OK) {
			opae_mutex_unlock(ires, &wrapped_event_handle->lock);
			return res;
		}

		// The event_handle is now created.
		wrapped_event_handle->adapter_table =
			wrapped_handle->adapter_table;
		wrapped_event_handle->flags |=
			OPAE_WRAPPED_EVENT_HANDLE_CREATED;
	}

	if (!wrapped_event_handle->opae_event_handle) {
		OPAE_ERR("NULL fpga_event_handle");
		opae_mutex_unlock(ires, &wrapped_event_handle->lock);
		return FPGA_INVALID_PARAM;
	}

	if (!wrapped_event_handle->adapter_table) {
		OPAE_ERR("NULL adapter table in wrapped event handle.");
		opae_mutex_unlock(ires, &wrapped_event_handle->lock);
		return FPGA_INVALID_PARAM;
	}

	if (!wrapped_event_handle->adapter_table->fpgaRegisterEvent) {
		OPAE_ERR("NULL fpgaRegisterEvent() in adapter.");
		opae_mutex_unlock(ires, &wrapped_event_handle->lock);
		return FPGA_NOT_SUPPORTED;
	}

	res = wrapped_event_handle->adapter_table->fpgaRegisterEvent(
		wrapped_handle->opae_handle, event_type,
		wrapped_event_handle->opae_event_handle, flags);

	opae_mutex_unlock(ires, &wrapped_event_handle->lock);

	return res;
}

fpga_result fpgaUnregisterEvent(fpga_handle handle, fpga_event_type event_type,
				fpga_event_handle event_handle)
{
	fpga_result res;
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);
	opae_wrapped_event_handle *wrapped_event_handle =
		opae_validate_wrapped_event_handle(event_handle);
	int ires;

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(wrapped_event_handle);

	opae_mutex_lock(ires, &wrapped_event_handle->lock);

	if (!(wrapped_event_handle->flags
	      & OPAE_WRAPPED_EVENT_HANDLE_CREATED)) {
		OPAE_ERR(
			"Attempting to unregister event object before registering it.");
		opae_mutex_unlock(ires, &wrapped_event_handle->lock);
		return FPGA_INVALID_PARAM;
	}

	if (!wrapped_event_handle->opae_event_handle) {
		OPAE_ERR("NULL fpga_event_handle in wrapper.");
		opae_mutex_unlock(ires, &wrapped_event_handle->lock);
		return FPGA_INVALID_PARAM;
	}

	if (!wrapped_event_handle->adapter_table->fpgaUnregisterEvent) {
		OPAE_ERR("NULL fpgaUnregisterEvent() in adapter.");
		opae_mutex_unlock(ires, &wrapped_event_handle->lock);
		return FPGA_NOT_SUPPORTED;
	}

	res = wrapped_event_handle->adapter_table->fpgaUnregisterEvent(
		wrapped_handle->opae_handle, event_type,
		wrapped_event_handle->opae_event_handle);

	opae_mutex_unlock(ires, &wrapped_event_handle->lock);

	return res;
}

fpga_result fpgaAssignPortToInterface(fpga_handle fpga, uint32_t interface_num,
				      uint32_t slot_num, int flags)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(fpga);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(
		wrapped_handle->adapter_table->fpgaAssignPortToInterface,
		FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaAssignPortToInterface(
		wrapped_handle->opae_handle, interface_num, slot_num, flags);
}

fpga_result fpgaAssignToInterface(fpga_handle fpga, fpga_token accelerator,
				  uint32_t host_interface, int flags)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(fpga);
	opae_wrapped_token *wrapped_token =
		opae_validate_wrapped_token(accelerator);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL_RESULT(
		wrapped_handle->adapter_table->fpgaAssignToInterface,
		FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaAssignToInterface(
		wrapped_handle->opae_handle, wrapped_token->opae_token,
		host_interface, flags);
}

fpga_result fpgaReleaseFromInterface(fpga_handle fpga, fpga_token accelerator)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(fpga);
	opae_wrapped_token *wrapped_token =
		opae_validate_wrapped_token(accelerator);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL_RESULT(
		wrapped_handle->adapter_table->fpgaReleaseFromInterface,
		FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaReleaseFromInterface(
		wrapped_handle->opae_handle, wrapped_token->opae_token);
}

fpga_result fpgaReconfigureSlot(fpga_handle fpga, uint32_t slot,
				const uint8_t *bitstream, size_t bitstream_len,
				int flags)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(fpga);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(bitstream);
	ASSERT_NOT_NULL_RESULT(
		wrapped_handle->adapter_table->fpgaReconfigureSlot,
		FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaReconfigureSlot(
		wrapped_handle->opae_handle, slot, bitstream, bitstream_len,
		flags);
}

fpga_result fpgaTokenGetObject(fpga_token token, const char *name,
			       fpga_object *object, int flags)
{
	fpga_result res;
	fpga_result dres = FPGA_OK;
	fpga_object obj = NULL;
	opae_wrapped_object *wrapped_object;
	opae_wrapped_token *wrapped_token = opae_validate_wrapped_token(token);

	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL(name);
	ASSERT_NOT_NULL(object);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaTokenGetObject,
			       FPGA_NOT_SUPPORTED);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaDestroyObject,
			       FPGA_NOT_SUPPORTED);

	res = wrapped_token->adapter_table->fpgaTokenGetObject(
		wrapped_token->opae_token, name, &obj, flags);

	ASSERT_RESULT(res);

	wrapped_object =
		opae_allocate_wrapped_object(obj, wrapped_token->adapter_table);

	if (!wrapped_object) {
		OPAE_ERR("malloc failed");
		res = FPGA_NO_MEMORY;
		dres = wrapped_token->adapter_table->fpgaDestroyObject(&obj);
	}

	*object = wrapped_object;

	return res != FPGA_OK ? res : dres;
}

fpga_result fpgaHandleGetObject(fpga_handle handle, const char *name,
				fpga_object *object, int flags)
{
	fpga_result res;
	fpga_result dres = FPGA_OK;
	fpga_object obj = NULL;
	opae_wrapped_object *wrapped_object;
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(name);
	ASSERT_NOT_NULL(object);
	ASSERT_NOT_NULL_RESULT(
		wrapped_handle->adapter_table->fpgaHandleGetObject,
		FPGA_NOT_SUPPORTED);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaDestroyObject,
			       FPGA_NOT_SUPPORTED);

	res = wrapped_handle->adapter_table->fpgaHandleGetObject(
		wrapped_handle->opae_handle, name, &obj, flags);

	ASSERT_RESULT(res);

	wrapped_object = opae_allocate_wrapped_object(
		obj, wrapped_handle->adapter_table);

	if (!wrapped_object) {
		OPAE_ERR("malloc failed");
		res = FPGA_NO_MEMORY;
		dres = wrapped_handle->adapter_table->fpgaDestroyObject(&obj);
	}

	*object = wrapped_object;

	return res != FPGA_OK ? res : dres;
}

fpga_result fpgaObjectGetObject(fpga_object parent, const char *name,
				fpga_object *object, int flags)
{
	fpga_result res;
	fpga_result dres = FPGA_OK;
	fpga_object obj = NULL;
	opae_wrapped_object *wrapped_child_object;
	opae_wrapped_object *wrapped_object =
		opae_validate_wrapped_object(parent);

	ASSERT_NOT_NULL(wrapped_object);
	ASSERT_NOT_NULL(name);
	ASSERT_NOT_NULL(object);
	ASSERT_NOT_NULL_RESULT(
		wrapped_object->adapter_table->fpgaObjectGetObject,
		FPGA_NOT_SUPPORTED);
	ASSERT_NOT_NULL_RESULT(wrapped_object->adapter_table->fpgaDestroyObject,
			       FPGA_NOT_SUPPORTED);

	res = wrapped_object->adapter_table->fpgaObjectGetObject(
		wrapped_object->opae_object, name, &obj, flags);

	ASSERT_RESULT(res);

	wrapped_child_object = opae_allocate_wrapped_object(
		obj, wrapped_object->adapter_table);

	if (!wrapped_child_object) {
		OPAE_ERR("malloc failed");
		res = FPGA_NO_MEMORY;
		dres = wrapped_object->adapter_table->fpgaDestroyObject(&obj);
	}

	*object = wrapped_child_object;

	return res != FPGA_OK ? res : dres;
}

fpga_result fpgaDestroyObject(fpga_object *obj)
{
	fpga_result res;
	opae_wrapped_object *wrapped_object;

	ASSERT_NOT_NULL(obj);

	wrapped_object = opae_validate_wrapped_object(*obj);

	ASSERT_NOT_NULL(wrapped_object);
	ASSERT_NOT_NULL_RESULT(wrapped_object->adapter_table->fpgaDestroyObject,
			       FPGA_NOT_SUPPORTED);

	res = wrapped_object->adapter_table->fpgaDestroyObject(
		&wrapped_object->opae_object);

	opae_destroy_wrapped_object(wrapped_object);

	return res;
}

fpga_result fpgaObjectRead(fpga_object obj, uint8_t *buffer, size_t offset,
			   size_t len, int flags)
{
	opae_wrapped_object *wrapped_object = opae_validate_wrapped_object(obj);

	ASSERT_NOT_NULL(wrapped_object);
	ASSERT_NOT_NULL(buffer);
	ASSERT_NOT_NULL_RESULT(wrapped_object->adapter_table->fpgaObjectRead,
			       FPGA_NOT_SUPPORTED);

	return wrapped_object->adapter_table->fpgaObjectRead(
		wrapped_object->opae_object, buffer, offset, len, flags);
}

fpga_result fpgaObjectGetSize(fpga_object obj, uint64_t *value, int flags)
{
	opae_wrapped_object *wrapped_object = opae_validate_wrapped_object(obj);

	ASSERT_NOT_NULL(wrapped_object);
	ASSERT_NOT_NULL(value);
	ASSERT_NOT_NULL_RESULT(wrapped_object->adapter_table->fpgaObjectGetSize,
			       FPGA_NOT_SUPPORTED);

	return wrapped_object->adapter_table->fpgaObjectGetSize(
		wrapped_object->opae_object, value, flags);
}

fpga_result fpgaObjectRead64(fpga_object obj, uint64_t *value, int flags)
{
	opae_wrapped_object *wrapped_object = opae_validate_wrapped_object(obj);

	ASSERT_NOT_NULL(wrapped_object);
	ASSERT_NOT_NULL(value);
	ASSERT_NOT_NULL_RESULT(wrapped_object->adapter_table->fpgaObjectRead64,
			       FPGA_NOT_SUPPORTED);

	return wrapped_object->adapter_table->fpgaObjectRead64(
		wrapped_object->opae_object, value, flags);
}

fpga_result fpgaObjectWrite64(fpga_object obj, uint64_t value, int flags)
{
	opae_wrapped_object *wrapped_object = opae_validate_wrapped_object(obj);

	ASSERT_NOT_NULL(wrapped_object);
	ASSERT_NOT_NULL_RESULT(wrapped_object->adapter_table->fpgaObjectWrite64,
			       FPGA_NOT_SUPPORTED);

	return wrapped_object->adapter_table->fpgaObjectWrite64(
		wrapped_object->opae_object, value, flags);
}

fpga_result fpgaSetUserClock(fpga_handle handle, uint64_t high_clk,
			     uint64_t low_clk, int flags)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaSetUserClock,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaSetUserClock(
		wrapped_handle->opae_handle, high_clk, low_clk, flags);
}


fpga_result fpgaGetUserClock(fpga_handle handle, uint64_t *high_clk,
			     uint64_t *low_clk, int flags)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(low_clk);
	ASSERT_NOT_NULL(high_clk);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaGetUserClock,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaGetUserClock(
		wrapped_handle->opae_handle, high_clk, low_clk, flags);
}

fpga_result fpgaGetNumMetrics(fpga_handle handle, uint64_t *num_metrics)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(num_metrics);

	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaGetNumMetrics,
			     FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaGetNumMetrics(
		wrapped_handle->opae_handle, num_metrics);
}

fpga_result fpgaGetMetricsInfo(fpga_handle handle,
				fpga_metric_info *metric_info,
				uint64_t *num_metrics)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(metric_info);
	ASSERT_NOT_NULL(num_metrics);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaGetMetricsInfo,
			    FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaGetMetricsInfo(
		wrapped_handle->opae_handle, metric_info, num_metrics);
}

fpga_result fpgaGetMetricsByIndex(fpga_handle handle,
				uint64_t *metric_num,
				uint64_t num_metric_indexes,
				fpga_metric *metrics)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(num_metric_indexes);
	ASSERT_NOT_NULL(metrics);

	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaGetMetricsByIndex,
			   FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaGetMetricsByIndex(
		wrapped_handle->opae_handle, metric_num, num_metric_indexes, metrics);
}

fpga_result fpgaGetMetricsByName(fpga_handle handle,
				char **metrics_names,
				uint64_t num_metric_names,
				fpga_metric *metrics)
{
	opae_wrapped_handle *wrapped_handle =
		opae_validate_wrapped_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(metrics_names);
	ASSERT_NOT_NULL(metrics);

	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaGetMetricsByName,
			   FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaGetMetricsByName(
		wrapped_handle->opae_handle, metrics_names, num_metric_names, metrics);
}

fpga_result fpgaFeatureEnumerate(fpga_handle handle, fpga_feature_properties *prop, 
                      fpga_feature_token *tokens, uint32_t max_tokens,
                      uint32_t *num_matches)
{
	fpga_result res = FPGA_OK;
	uint32_t i;
	uint32_t num_tokens;
	fpga_feature_token * d_tokens = NULL;
	opae_wrapped_handle *wrapped_handle = opae_validate_wrapped_handle(handle);
  
	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL(prop);
	ASSERT_NOT_NULL(num_matches);

	if ((max_tokens > 0) && (NULL == tokens)) {
		FPGA_MSG("max_tokens > 0 with NULL tokens");
		return FPGA_INVALID_PARAM;
	}

	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaFeatureEnumerate,
		FPGA_NOT_SUPPORTED);
	
	if (tokens) {
		d_tokens = (fpga_feature_token *)calloc(max_tokens, sizeof(fpga_feature_token));
		if (!d_tokens) {
			OPAE_ERR("out of memory");
			return FPGA_NO_MEMORY;
		}
	}
	res = wrapped_handle->adapter_table->fpgaFeatureEnumerate(wrapped_handle->opae_handle,
							prop, d_tokens,	max_tokens, num_matches);
	if (res != FPGA_OK) {
		OPAE_ERR("fpgaFeatureEnumerate failed");
		goto out_free;
	}
	
	if (tokens == NULL)
		goto out_free;

	if (*num_matches > max_tokens)
		num_tokens = max_tokens;
	else
		num_tokens = *num_matches;

	for (i = 0; i < num_tokens; ++i) {
		opae_wrapped_feature_token *wt = opae_allocate_wrapped_feature_token(
			d_tokens[i], wrapped_handle->adapter_table);
		if (!wt) {
			OPAE_ERR("opae_allocate_wrapped_feature_token failed");
			res = FPGA_EXCEPTION;
			goto out_free;
		}
		tokens[i] = wt;
	}

	return res;

out_free:
	free(d_tokens);
	return res;
}

fpga_result fpgaDestroyFeatureToken(fpga_feature_token *token)
{
	fpga_result res = FPGA_OK;
	opae_wrapped_feature_token *wrapped_token;

	ASSERT_NOT_NULL(token);

	wrapped_token = opae_validate_wrapped_feature_token(*token);

	ASSERT_NOT_NULL(wrapped_token);

	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaDestroyFeatureToken,
		FPGA_NOT_SUPPORTED);

	res = wrapped_token->adapter_table->fpgaDestroyFeatureToken(wrapped_token->feature_token);

	opae_destroy_wrapped_feature_token(wrapped_token);

	return res;
}

fpga_result fpgaFeaturePropertiesGet(fpga_feature_token token,
	fpga_feature_properties *prop)
{
	opae_wrapped_feature_token *wrapped_token;

	wrapped_token = opae_validate_wrapped_feature_token(token);
	
	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL(prop);

	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaFeaturePropertiesGet,
		FPGA_NOT_SUPPORTED);
		
	return wrapped_token->adapter_table->fpgaFeaturePropertiesGet(wrapped_token->feature_token,
			prop);
}

fpga_result fpgaFeatureOpen(fpga_feature_token token, int flags,
                            void *priv_config, fpga_feature_handle *handle)
{
	fpga_result res;
	fpga_result cres = FPGA_OK;
	opae_wrapped_feature_token *wrapped_token;
	fpga_feature_handle feature_handle = NULL;
	opae_wrapped_feature_handle *wrapped_handle;

	wrapped_token = opae_validate_wrapped_feature_token(token);

	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL(handle);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaFeatureOpen,
		FPGA_NOT_SUPPORTED);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaFeatureClose,
		FPGA_NOT_SUPPORTED);

	res = wrapped_token->adapter_table->fpgaFeatureOpen(wrapped_token->feature_token,
						    flags, priv_config, &feature_handle);

	ASSERT_RESULT(res);

	wrapped_handle = opae_allocate_wrapped_feature_handle(
		wrapped_token, feature_handle, wrapped_token->adapter_table);

	if (!wrapped_handle) {
		OPAE_ERR("malloc failed");
		res = FPGA_NO_MEMORY;
		cres = wrapped_token->adapter_table->fpgaFeatureClose(feature_handle);
	}

	*handle = wrapped_handle;

	return res != FPGA_OK ? res : cres;
}

fpga_result fpgaFeatureClose(fpga_feature_handle handle)
{
	fpga_result res;
	opae_wrapped_feature_handle *wrapped_handle =
		opae_validate_wrapped_feature_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaFeatureClose,
			       FPGA_NOT_SUPPORTED);

	res = wrapped_handle->adapter_table->fpgaFeatureClose(
		wrapped_handle->feature_handle);

	opae_destroy_wrapped_feature_handle(wrapped_handle);

	return res;
}

fpga_result fpgaDMAPropertiesGet(fpga_feature_token token, fpga_dma_properties *prop,
									int max_ch)
{
	opae_wrapped_feature_token *wrapped_token =
				opae_validate_wrapped_feature_token(token);

	ASSERT_NOT_NULL(token);
	ASSERT_NOT_NULL(prop);
	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaDMAPropertiesGet,
			       FPGA_NOT_SUPPORTED);

	return wrapped_token->adapter_table->fpgaDMAPropertiesGet(wrapped_token->feature_token,
															prop, max_ch);
}

fpga_result fpgaDMATransferSync(fpga_feature_handle dma_h, dma_transfer_list *xfer_list)
{
	opae_wrapped_feature_handle *wrapped_handle =
		opae_validate_wrapped_feature_handle(dma_h);

	ASSERT_NOT_NULL(xfer_list);
	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaDMATransferSync,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaDMATransferSync(wrapped_handle->feature_handle,
																xfer_list);
}

fpga_result fpgaDMATransferAsync(fpga_feature_handle dma_h, dma_transfer_list *dma_xfer,
								fpga_dma_cb cb, void *context)
{
	opae_wrapped_feature_handle *wrapped_handle =
		opae_validate_wrapped_feature_handle(dma_h);

	ASSERT_NOT_NULL(dma_xfer);
	ASSERT_NOT_NULL(context);
	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaDMATransferAsync,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaDMATransferAsync(wrapped_handle->feature_handle,
																dma_xfer, cb, context);
}
