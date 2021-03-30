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

/**
 * @file error.h
 * @brief Functions for reading and clearing errors in resources
 *
 * Many FPGA resources have the ability to track the occurrence of errors.
 * This file provides functions to retrieve information about errors within
 * resources.
 */

#ifndef __FPGA_ERROR_H__
#define __FPGA_ERROR_H__

#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Read error value
 *
 * This function will read the value of error register `error_num` of
 * the resource referenced by `token` into the memory location pointed to
 * by `value`.
 *
 * @param[in]  token     Token to accelerator resource to query
 * @param[in]  error_num Number of error register to read
 * @param[out] value     Pointer to memory to store error value into (64 bit)
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to access the token.
 */
fpga_result fpgaReadError(fpga_token token, uint32_t error_num, uint64_t *value);

/**
 * Clear error register
 *
 * This function will clear the error register `error_num` of the resource
 * referenced by `token`.
 *
 * @param[in]  token     Token to accelerator resource to query
 * @param[in]  error_num Number of error register to clear
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to access the token, and FPGA_BUSY if error could not be
 * cleared.
 */
fpga_result fpgaClearError(fpga_token token, uint32_t error_num);

/**
 * Clear all error registers of a particular resource
 *
 * This function will clear all error registers of the resource referenced by
 * `token`, observing the necessary order of clearing errors, if any.
 *
 * @param[in]  token     Token to accelerator resource to query
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to access the token, and FPGA_BUSY if error could not be
 * cleared.
 */
fpga_result fpgaClearAllErrors(fpga_token token);

/**
 * Get information about a particular error register
 *
 * This function will populate a `fpga_error_info` struct with information
 * about error number `error_num` of the resource referenced by `token`.
 *
 * @param[in]  token      Token to accelerator resource to query
 * @param[in]  error_num  Error register to retrieve information about
 * @param[out] error_info Pointer to memory to store information into
 * @returns FPGA_OK on success. FPGA_INVALID_PARAM if any of the supplied
 * parameters is invalid. FPGA_EXCEPTION if an internal exception occurred
 * while trying to access the token.
 */
fpga_result fpgaGetErrorInfo(fpga_token token,
			     uint32_t error_num,
			     struct fpga_error_info *error_info);


#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __FPGA_ERROR_H__
