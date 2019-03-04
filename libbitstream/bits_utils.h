// Copyright(c) 2019, Intel Corporation
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
 * @file bits_utils.h
 * @brief Utility functions for GBS metadata parsing.
 *
 * These functions extract basic types (strings, integers)
 * from the given JSON object.
 *
 */

#ifndef __OPAE_BITS_UTILS_H__
#define __OPAE_BITS_UTILS_H__

#include <json-c/json.h>
#include <opae/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Allocate and populate string value from JSON object.
 *
 * @param[in] parent The JSON object in which to search for `name`.
 * @param[in] name The search key.
 * @param[out] value Receives the allocated buffer.
 *
 * @returns FPGA_OK on success. FPGA_EXCEPTION if `name`
 * is not found or if `name` is not a key of type string.
 * FPGA_NO_MEMORY if memory allocation fails.
 *
 * @note Allocates memory. The buffer saved at `*value`
 * on success must be tracked and freed when no longer
 * needed.
 */
fpga_result opae_bitstream_get_json_string(json_object *parent,
					   const char *name,
					   char **value);

/**
 * Populate integer value from JSON object.
 *
 * @param[in] parent The JSON object in which to search for `name`.
 * @param[in] name The search key.
 * @param[out] value Receives the integer value.
 *
 * @returns FPGA_OK on success. FPGA_EXCEPTION if `name`
 * is not found or if `name` is not a key of type integer.
 */ 
fpga_result opae_bitstream_get_json_int(json_object *parent,
					const char *name,
					int *value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __OPAE_BITS_UTILS_H__ */
