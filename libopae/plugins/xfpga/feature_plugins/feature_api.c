// Copyright(c) 2018-2019, Intel Corporation
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
#include "xfpga.h"
#include "common_int.h"
#include "props.h"

#include "feature_pluginmgr.h"
#include "feature_int.h"
#include "feature_token_list_int.h"

extern pthread_mutex_t global_lock;

wrapped_feature_token *
allocate_wrapped_feature_token(fpga_feature_token token,
			    const feature_adapter_table *adapter)
{
	wrapped_feature_token *wtok =
		(wrapped_feature_token *)malloc(sizeof(wrapped_feature_token));

	if (wtok) {
		wtok->magic = FPGA_WRAPPED_FEATURE_TOKEN_MAGIC;
		wtok->feature_token = token;
		wtok->adapter_table = (feature_adapter_table *)adapter;
	}

	return wtok;
}

wrapped_feature_handle *
allocate_wrapped_feature_handle(wrapped_feature_token *wt, fpga_feature_handle feature_handle,
			     feature_adapter_table *adapter)
{
	wrapped_feature_handle *whan =
		(wrapped_feature_handle *)malloc(sizeof(wrapped_feature_handle));

	if (whan) {
		whan->magic = FPGA_WRAPPED_FEATURE_HANDLE_MAGIC;
		whan->wrapped_feature_token = wt;
		whan->feature_handle = feature_handle;
		whan->adapter_table = adapter;
	}

	return whan;
}

fpga_result
xfpga_fpgaCloneFeatureToken(fpga_feature_token src, fpga_feature_token *dst)
{
	struct _fpga_feature_token *_src = (struct _fpga_feature_token *)src;
	struct _fpga_feature_token *_dst;
	fpga_result res;
	errno_t e;

	if (NULL == src || NULL == dst) {
		FPGA_MSG("src or dst in NULL");
		return FPGA_INVALID_PARAM;
	}

	if (_src->magic != FPGA_FEATURE_TOKEN_MAGIC) {
		FPGA_MSG("Invalid src");
		return FPGA_INVALID_PARAM;
	}

	_dst = malloc(sizeof(struct _fpga_feature_token));
	if (NULL == _dst) {
		FPGA_MSG("Failed to allocate memory for token");
		return FPGA_NO_MEMORY;
	}

	_dst->magic = FPGA_FEATURE_TOKEN_MAGIC;
	_dst->feature_type = _src->feature_type;
	_dst->handle = _src->handle;
	e = memcpy_s(_dst->feature_guid, sizeof(fpga_guid), _src->feature_guid,
		sizeof(fpga_guid));
	if (EOK != e) {
		FPGA_ERR("memcpy_s failed");
		res = FPGA_EXCEPTION;
		goto out_free;
	}

	*dst = _dst;
	return FPGA_OK;

out_free:
	free(_dst);
	return res;
}

fpga_result __FPGA_API__
xfpga_fpgaFeatureEnumerate(fpga_handle handle, fpga_feature_properties *prop, 
                      fpga_feature_token *tokens, uint32_t max_tokens,
                      uint32_t *num_matches)
{
	fpga_result result = FPGA_OK;
	uint32_t mmio_num = 0;
	uint64_t offset = 0;
	fpga_guid guid;
	struct _fpga_feature_token *_ftoken;
	struct DFH dfh;
	int errors;
	
	ASSERT_NOT_NULL(prop);
	ASSERT_NOT_NULL(tokens);
	ASSERT_NOT_NULL(num_matches);	
  
	*num_matches = 0;
	
	errors = feature_plugin_mgr_initialize(handle);
	if (errors) {
		FPGA_ERR("Feature token initialize errors");
		result = FPGA_EXCEPTION;
	}	
	
	mmio_num = 0;  // TODO : check how to get the mmio_num
	result = xfpga_fpgaReadMMIO64(handle, mmio_num, 0x0, &(dfh.csr));
	if (result != FPGA_OK) {
		FPGA_ERR("fpgaReadMMIO64() failed");
		return result;
	}
	
	// Discover feature BBB by traversing the device feature list 
	offset = dfh.next_header_offset;
	
	do {
		uint64_t feature_uuid_lo, feature_uuid_hi;
		uint32_t feature_type;
		
		result = xfpga_fpgaReadMMIO64(handle, mmio_num, offset + 0, &(dfh.csr));
		if (result != FPGA_OK) {
			FPGA_ERR("fpgaReadMMIO64() failed");
			return result;
		}
		feature_type = dfh.type;   // TODO: check bit

		// Read the current feature's UUID
		result = xfpga_fpgaReadMMIO64(handle, mmio_num, offset + 8,
				     &feature_uuid_lo);
		if (result != FPGA_OK) {
			FPGA_ERR("fpgaReadMMIO64() failed");
			return result;
		}

		result = xfpga_fpgaReadMMIO64(handle, mmio_num, offset + 16,
				     &feature_uuid_hi);
		if (result != FPGA_OK) {
			FPGA_ERR("fpgaReadMMIO64() failed");
			return result;
		}

		get_guid(feature_uuid_lo, feature_uuid_hi, &guid);

		_ftoken = feature_token_add(feature_type, mmio_num, guid, offset, handle);

		if (_ftoken->feature_type == prop->type) {
			if ((!uuid_is_null(prop->guid) && (uuid_compare(prop->guid, guid) == 0))
				|| (uuid_is_null(prop->guid))) { 
				if (*num_matches < max_tokens) {
					fpga_feature_token tmp = 0;
					feature_adapter_table * adapter;
					
					result = xfpga_fpgaCloneFeatureToken((fpga_feature_token)_ftoken, &tmp);
					if (result	!= FPGA_OK) {
						FPGA_MSG("Error cloning token");
						return result;
					}
					adapter = get_feature_plugin_adapter(guid);
					wrapped_feature_token *wt = 
					     allocate_wrapped_feature_token(tmp, adapter);
					tokens[*num_matches] = wt;
				}
				++(*num_matches);
			}
		}

		// Move to the next feature header 
		offset = offset + dfh.next_header_offset;
	} while (!dfh.eol);

	return result;
}

fpga_result __FPGA_API__ xfpga_fpgaDestroyFeatureToken(fpga_feature_token *token)
{
	int err = 0;
	wrapped_feature_token *wrapped_token;

	ASSERT_NOT_NULL(token);

	wrapped_token = validate_wrapped_feature_token(*token);

	ASSERT_NOT_NULL(wrapped_token);

	if (pthread_mutex_lock(&global_lock)) {
		FPGA_MSG("Failed to lock global mutex");
		return FPGA_EXCEPTION;
	}

	free(wrapped_token->feature_token);
	wrapped_token->feature_token = NULL;

	destroy_wrapped_feature_token(wrapped_token);

	err = pthread_mutex_unlock(&global_lock);
	if (err) {
		FPGA_ERR("pthread_mutex_unlock() failed: %S", strerror(err));
	}
	return FPGA_OK;
}

fpga_result __FPGA_API__ xfpga_fpgaFeaturePropertiesGet(fpga_feature_token token,
	fpga_feature_properties *prop)
{
	fpga_result res = FPGA_OK;
	wrapped_feature_token *wrapped_token;
	struct _fpga_feature_token *_ftoken;
	errno_t e;

	wrapped_token = validate_wrapped_feature_token(token);
	_ftoken = (struct _fpga_feature_token *)wrapped_token->feature_token;
	
	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL(prop);
	ASSERT_NOT_NULL(_ftoken);

	if (_ftoken->magic != FPGA_FEATURE_TOKEN_MAGIC) {
		FPGA_ERR("Invalid feature token");
		return FPGA_INVALID_PARAM;
	}

	prop->type = (fpga_feature_type )_ftoken->feature_type;
	e = memcpy_s(prop->guid, sizeof(fpga_guid), _ftoken->feature_guid,
		sizeof(fpga_guid));

	if (EOK != e) {
		FPGA_ERR("memcpy_s failed");
		res = FPGA_EXCEPTION;
	}

	return res;
}

fpga_result __FPGA_API__ xfpga_fpgaFeatureOpen(fpga_feature_token token, int flags,
                            void *priv_config, fpga_feature_handle *handle)
{
	fpga_result res;
	fpga_result cres = FPGA_OK;
	wrapped_feature_token *wrapped_token;
	fpga_feature_handle feature_handle = NULL;
	wrapped_feature_handle *wrapped_handle;

	wrapped_token = validate_wrapped_feature_token(token);

	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL(handle);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaFeatureOpen,
			       FPGA_NOT_SUPPORTED);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaFeatureClose,
			       FPGA_NOT_SUPPORTED);

	res = wrapped_token->adapter_table->fpgaFeatureOpen(wrapped_token->feature_token,
						    flags, priv_config, &feature_handle);

	ASSERT_RESULT(res);

	wrapped_handle = allocate_wrapped_feature_handle(
		wrapped_token, feature_handle, wrapped_token->adapter_table);

	if (!wrapped_handle) {
		FPGA_ERR("malloc failed");
		res = FPGA_NO_MEMORY;
		cres = wrapped_token->adapter_table->fpgaFeatureClose(feature_handle);
	}

	*handle = wrapped_handle;

	return res != FPGA_OK ? res : cres;
}

fpga_result __FPGA_API__ xfpga_fpgaFeatureClose(fpga_feature_handle handle)
{
	fpga_result res;
	wrapped_feature_handle *wrapped_handle =
		validate_wrapped_feature_handle(handle);

	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaFeatureClose,
			       FPGA_NOT_SUPPORTED);

	res = wrapped_handle->adapter_table->fpgaFeatureClose(
		wrapped_handle->feature_handle);

	destroy_wrapped_feature_handle(wrapped_handle);

	return res;
}	
	
fpga_result __FPGA_API__
xfpga_fpgaDMAPropertiesGet(fpga_feature_token token, fpga_dma_properties *prop,
									int max_ch)
{
	wrapped_feature_token *wrapped_token =
				validate_wrapped_feature_token(token);

	ASSERT_NOT_NULL(token);
	ASSERT_NOT_NULL(prop);
	ASSERT_NOT_NULL(wrapped_token);
	ASSERT_NOT_NULL_RESULT(wrapped_token->adapter_table->fpgaDMAPropertiesGet,
			       FPGA_NOT_SUPPORTED);

	return wrapped_token->adapter_table->fpgaDMAPropertiesGet(wrapped_token->feature_token,
															prop, max_ch);
}

fpga_result __FPGA_API__
xfpga_fpgaDMATransferSync(fpga_feature_handle dma_h, dma_transfer_list *xfer_list)
{
	//fpga_result res;
	wrapped_feature_handle *wrapped_handle =
		validate_wrapped_feature_handle(dma_h);

	ASSERT_NOT_NULL(xfer_list);
	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaDMATransferSync,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaDMATransferSync(wrapped_handle->feature_handle,
																xfer_list);

}

fpga_result __FPGA_API__
xfpga_fpgaDMATransferAsync(fpga_feature_handle dma_h, dma_transfer_list *dma_xfer,
								fpga_dma_cb cb, void *context)
{
	//fpga_result res;
	wrapped_feature_handle *wrapped_handle =
		validate_wrapped_feature_handle(dma_h);

	ASSERT_NOT_NULL(dma_xfer);
	ASSERT_NOT_NULL(context);
	ASSERT_NOT_NULL(wrapped_handle);
	ASSERT_NOT_NULL_RESULT(wrapped_handle->adapter_table->fpgaDMATransferAsync,
			       FPGA_NOT_SUPPORTED);

	return wrapped_handle->adapter_table->fpgaDMATransferAsync(wrapped_handle->feature_handle,
																dma_xfer, cb, context);

}


