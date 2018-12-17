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

/**
 * @file feature.h
 * @brief APIs for feature resource enumeration and open
 *
 * These APIs are for discovery and open of fpga feature resources that are
 * present on the system. They allow selective enumeration (i.e. getting a
 * list of resources that match a given criteria) and open them.
 */

#ifndef __FPGA_FEATURE_H__
#define __FPGA_FEATURE_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Object for expressing FPGA feature resource properties
 *
 * Used for selective feature enumeration (discovery)
 *
 * For selective enumeration, set the relevant fields with the desired resource information.
 * Pass this structure to fpgaFeatureEnumerate.
 *
 * @note Initialize the fields in this structure to 0xFF to indicate value is not set.
 */
typedef struct {
	fpga_feature_type type;
	fpga_guid guid;
} fpga_feature_properties;

/**
 * Enumerate feature resources present in a FPGA device (discovery)
 *
 * fpgaFeatureEnumerate() will create a number of `fpga_feature_token`s to represent the
 * matching resources and populate the array `fpga_feature_tokens` with these tokens. The
 * `max_tokens` argument can be used to limit the number of tokens
 * allocated/returned by fpgaFeatuerEnumerate(); i.e., the number of tokens in the
 * returned `tokens` array will be either `max_tokens` or `num_matches`,
 *  whichever is smaller.
 *
 * @note fpgaFeatureEnumerate() will allocate memory for the created tokens returned
 * in `tokens`. It is the responsibility of the user application to free this
 * memory after use by calling fpgaFeatureDestroyToken() for each of the returned
 * tokens.
 * @note Use fpgaFeaturePropertiesGet() to get detailed information about any feature
 * token returned.
 *
 * @param[in]   fpga_h          Handle to previously opened accelerator resource
 * @param[in]   prop            The feature property that is being searched.
 * @param[out]  tokens          Pointer to an array of fpga_feature_token variables
 *                              to be populated.  If NULL is supplied,
 *                              fpgaFeatureEnumerate() will not create any tokens,
 *                              but it will return the number of possible matches
 *                              in `num_matches`.
 * @param[in]   max_tokens      Maximum number of tokens that fpgaFeatureEnumerate() shall
 *                              return (length of `tokens` array). There may be more
 *                              or fewer matches than this number; `num_matches` is
 *                              set to the number of actual matches.
 * @param[out]  num_matches     Number of feature resources
 *
 * @returns                FPGA_OK on success.
 *                         FPGA_INVALID_PARAM if invalid pointers or objects
 *                         are passed into the function.
 *                         FPGA_NO_MEMORY if there was not enough memory to
 *                         create tokens.
 *                         FPGA_NOT_SUPPORTED if the accelerator doesn't support
 *                         feature enumeration.
 *                         FPGA_EXCEPTION if some other errors occur.
 */
fpga_result
fpgaFeatureEnumerate(fpga_handle fpga_h, fpga_feature_properties *prop,
		fpga_feature_token *tokens, uint32_t max_tokens,
		uint32_t *num_matches);

/**
 * Destroy a feature Token
 *
 * This function destroys a feature token created by fpgaFeatureEnumerate() and frees the
 * associated memory.
 *
 * @param[in] feature_token     fpga_feature_token to destroy
 *
 * @returns                     FPGA_OK on success
 */
fpga_result
fpgaDestroyFeatureToken(fpga_feature_token *feature_token);

/**
 * Get feature properties from a feature token
 *
 * @param[in] token             fpga_feature_token to get its property
 *
 * @param[out] prop             fpga_feature_properties output
 * @returns                     FPGA_OK on success
 *                              FPGA_INVALID_PARAM if invalid pointers or objects
 *                              are passed into the function.
 *                              FPGA_NOT_SUPPORTED if the accelerator doesn't support
 *                              feature token destroy functionality.
 */
fpga_result
fpgaFeaturePropertiesGet(fpga_feature_token token,
		fpga_feature_properties *prop);

/**
 * Open a feature object
 *
 * Acquires ownership of the feature resource referred to by 'feature_token'.
 *
 * @param[in]   feature_token Pointer to a fpga_feature_token identifying resource to acquire
 *                            ownership of.
 * @param[in]   flags         One of the following flags:
 *                            FPGA_OPEN_SHARED allows the resource to be opened
 *                            multiple times.
 * @param[in]   priv_config   Private data for a specific implementation.
 * @param[out]  handle        Pointer to preallocated memory to place a feature handle in.
 *                            This handle will be used in subsequent API calls.
 *
 * @returns                   FPGA_OK on success.
 *                            FPGA_INVALID_PARAM if invalid pointers or objects
 *                            are passed into the function.
 *                            FPGA_NOT_SUPPORTED if the accelerator doesn't support feature
 *                            open functionality.
 */
fpga_result fpgaFeatureOpen(fpga_feature_token feature_token, int flags,
							void *priv_config, fpga_feature_handle *handle);

/**
 * Close a previously-opened feature object
 *
 * Relinquishes ownership of a previously fpgaFeatureOpen()ed resource. This enables
 * others to acquire ownership if the resource was opened exclusively.
 *
 * @param[in]   handle        Handle to previously-opened feature object
 *
 * @returns                   FPGA_OK on success.
 *                            FPGA_INVALID_PARAM if invalid pointers or objects
 *                            are passed into the function.
 *                            FPGA_NOT_SUPPORTED if the accelerator doesn't support feature
 *                            close functionality.
 */
fpga_result fpgaFeatureClose(fpga_feature_handle handle);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_FEATURE_H__

