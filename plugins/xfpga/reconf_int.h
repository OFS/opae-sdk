// Copyright(c) 2017-2018, Intel Corporation
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

#ifndef __RECONF_INT_H__
#define __RECONF_INT_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H


#include <stdint.h>

#include <opae/access.h>
#include <opae/utils.h>
#include <opae/manage.h>
#include <opae/properties.h>
#include "common_int.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
* @brief set afu user clock
*
* @param handle
* @param usrlclock_high user clock low frequency
* @param usrlclock_low user clock high frequency
*
* @return error code
*/
fpga_result set_afu_userclock(fpga_handle handle,
				uint64_t usrlclock_high,
				uint64_t usrlclock_low);

/**
* @brief Sets FPGA power threshold values
*
* @param fpga handle
* @param gbs_power gbs power value
*
* @return error code
*/
fpga_result set_fpga_pwr_threshold(fpga_handle handle,
				uint64_t gbs_power);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // __RECONF_INT_H__
