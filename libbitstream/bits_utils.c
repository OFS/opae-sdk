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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <string.h>
#include <opae/log.h>

#include "bits_utils.h"
#include "safe_string/safe_string.h"

fpga_result opae_bits_get_json_string(json_object *parent,
				      const char *name,
				      char **value)
{
	json_object *obj = NULL;
	const char *s;
	size_t len;
	errno_t err;

	if (!json_object_object_get_ex(parent,
				       name,
				       &obj)) {
		OPAE_ERR("metadata: failed to find \"%s\" key", name);
		return FPGA_EXCEPTION;
	}

	if (!json_object_is_type(obj, json_type_string)) {
		OPAE_ERR("metadata: \"%s\" key not string", name);
		return FPGA_EXCEPTION;
	}

	s = json_object_get_string(obj);

	len = strlen(s);

	*value = malloc(len + 1);
	if (!*value) {
		OPAE_ERR("malloc failed");
		return FPGA_NO_MEMORY;
	}

	err = strncpy_s(*value, len+1, s, len);
	if (err != EOK) {
		OPAE_ERR("strncpy_s failed");
		free(*value);
		*value = NULL;
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

fpga_result opae_bits_get_json_int(json_object *parent,
				   const char *name,
				   int *value)
{
	json_object *obj = NULL;

	if (!json_object_object_get_ex(parent,
				       name,
				       &obj)) {
		OPAE_ERR("metadata: failed to find \"%s\" key", name);
		return FPGA_EXCEPTION;
	}

	if (!json_object_is_type(obj, json_type_int)) {
		OPAE_ERR("metadata: \"%s\" key not int", name);
		return FPGA_EXCEPTION;
	}

	*value = json_object_get_int(obj);
	return FPGA_OK;
}
