// Copyright(c) 2018-2020, Intel Corporation
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

#include "bmcdata.h"
#include <opae/fpga.h>
#ifndef _WIN32
#include <unistd.h>
#include <uuid/uuid.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <wchar.h>

double getvalue(Values *val, uint8_t raw)
{
	int i;

	double res = val->M * raw + val->B;
	for (i = 0; i < abs(val->result_exp); i++) {
		if (val->result_exp >= 0) {
			res *= 10.0;
		} else {
			res /= 10.0;
		}
	}

	return res;
}

void calc_params(sdr_body *body, Values *val)
{
	int32_t i;
	int32_t M_val = 0;
	int32_t B_val = 0;
	uint32_t A_val = 0;
	uint32_t T_val = 0;
	uint32_t A_exp = 0;
	int32_t R_exp = 0;
	int32_t B_exp = 0;

#define SIGN_EXT(val, bitpos) (((val) ^ (1 << (bitpos))) - (1 << (bitpos)))

	B_val = (body->B_accuracy.bits.B_2_msb << 8) | body->B_8_lsb;
	B_val = SIGN_EXT(B_val, 9);

	M_val = (body->M_tolerance.bits.M_2_msb << 8) | body->M_8_lsb;
	M_val = SIGN_EXT(M_val, 9);

	A_val = (body->accuracy_accexp_sensor_direction.bits.accuracy_4_msb
		 << 6)
		| body->B_accuracy.bits.accuracy_6_lsb;

	T_val = body->M_tolerance.bits.tolerance;

	A_exp = body->accuracy_accexp_sensor_direction.bits.accuracy_exp;
	R_exp = body->R_exp_B_exp.bits.R_exp;
	R_exp = SIGN_EXT(R_exp, 3);
	B_exp = body->R_exp_B_exp.bits.B_exp;
	B_exp = SIGN_EXT(B_exp, 3);

	val->M = (double)M_val;
	val->tolerance = T_val;
	val->B = (double)B_val;
	val->result_exp = R_exp;
	val->A_exp = A_exp;
	for (i = 0; i < abs(B_exp); i++) {
		if (B_exp >= 0) {
			val->B *= 10.0;
		} else {
			val->B /= 10.0;
		}
	}

	val->accuracy = (double)A_val;
	for (i = 0; i < (int32_t)A_exp; i++) {
		val->accuracy *= 10.0;
	}
}
