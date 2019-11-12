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
#include <stdio.h>
#include <json-c/json.h>
#include "adapter.h"
#include "opae_int.h"

#define DUMMY_HIDDEN __attribute__((visibility("hidden")))

static uint32_t _fake_tokens;

int DUMMY_HIDDEN dummy_plugin_initialize(void)
{
	return 0;
}

int DUMMY_HIDDEN dummy_plugin_finalize(void)
{
	return 0;
}

bool DUMMY_HIDDEN dummy_plugin_supports_device(const char *device_type)
{
	UNUSED_PARAM(device_type);
	return true;
}

bool DUMMY_HIDDEN dummy_plugin_supports_host(const char *hostname)
{
	UNUSED_PARAM(hostname);
	return true;
}

typedef struct _dummy_token {
	int number;
} dummy_token;

typedef struct _dummy_handle {
	int number;
	dummy_token *token;
} dummy_handle;

#define MIN(x,y) (x < y) ? x : y
fpga_result DUMMY_HIDDEN dummy_plugin_fpgaEnumerate(const fpga_properties *filters,
						    uint32_t num_filters, fpga_token *tokens,
						    uint32_t max_tokens, uint32_t *num_matches)
{
	uint32_t i = 0;
	uint32_t min = MIN(_fake_tokens, max_tokens);
	UNUSED_PARAM(filters);
	UNUSED_PARAM(num_filters);
	*num_matches = _fake_tokens;

	for ( ; i < min; ++i) {
		dummy_token *t = (dummy_token*)malloc(sizeof(dummy_token));
		if (!t) {
			goto err_enum;
		}
		t->number = i;
		tokens[i] = t;
	}
	return FPGA_OK;
err_enum:
	while (--i) {
		free(tokens[i]);
	}
	free(tokens[0]);
	return FPGA_NO_MEMORY;
}

fpga_result DUMMY_HIDDEN dummy_plugin_fpgaDestroyToken(fpga_token *t)
{
	dummy_token *dt = (dummy_token *)*t;
	free(dt);
	*t = NULL;
	return FPGA_OK;
}

fpga_result DUMMY_HIDDEN dummy_plugin_fpgaOpen(fpga_token t, fpga_handle *h, int flags)
{
	UNUSED_PARAM(h);
	dummy_token *dt = (dummy_token*)t;
	dummy_handle *dh = (dummy_handle*)malloc(sizeof(dummy_handle));
	//printf("dummy/fpgaOpen %d %d\n", dh->number, flags);
	dh->number = dt->number*flags*2;
	dh->token = dt;
	*h = (fpga_handle)dh;
	return FPGA_OK;
}

fpga_result DUMMY_HIDDEN dummy_plugin_fpgaClose(fpga_handle h)
{
	dummy_handle *dh = (dummy_handle *)h;
	//printf("dummy/fpgaClose %d\n", dh->number);
	free(dh);
	return FPGA_OK;
}

#define DUMMY_JSON_GET(_jobj, _key, _jval)                      \
  do {                                                          \
    if (!json_object_object_get_ex(_jobj, _key, _jval)) {       \
      fprintf(stderr, "error getting value for key: %s", _key); \
      return 1;                                                 \
    }                                                           \
  } while (0)
int __attribute__((visibility("default"))) opae_plugin_configure(opae_api_adapter_table *adapter,
				       const char *jsonConfig)
{
	json_object *root = NULL;
	json_object *j_hello = NULL;
	json_object *j_plugin = NULL;
	json_object *j_fake_tokens = NULL;
	enum json_tokener_error j_err = json_tokener_success;
	//printf("%s\n", jsonConfig);
	root = json_tokener_parse_verbose(jsonConfig, &j_err);
	if (j_err != json_tokener_success) {
		fprintf(stderr, "error parsing plugin config: %s\n",
				json_tokener_error_desc(j_err));
		return 1;
	}
	DUMMY_JSON_GET(root, "key1", &j_hello);
	DUMMY_JSON_GET(root, "key2", &j_plugin);
	DUMMY_JSON_GET(root, "fake_tokens", &j_fake_tokens);
        printf("%s %s!\n", json_object_get_string(j_hello),
               json_object_get_string(j_plugin));
	_fake_tokens = json_object_get_int(j_fake_tokens);

        adapter->initialize = dummy_plugin_initialize;
	adapter->finalize = NULL;
	adapter->supports_device = dummy_plugin_supports_device;
	adapter->supports_host = NULL;
	adapter->fpgaEnumerate = dummy_plugin_fpgaEnumerate;
	adapter->fpgaDestroyToken = dummy_plugin_fpgaDestroyToken;
	adapter->fpgaOpen = dummy_plugin_fpgaOpen;
	adapter->fpgaClose = dummy_plugin_fpgaClose;

	adapter->fpgaReset = NULL;
	adapter->fpgaGetPropertiesFromHandle = NULL;
	adapter->fpgaGetProperties = NULL;
	adapter->fpgaUpdateProperties = NULL;
	adapter->fpgaWriteMMIO64 = NULL;
	adapter->fpgaReadMMIO64 = NULL;
	adapter->fpgaWriteMMIO32 = NULL;
	adapter->fpgaReadMMIO32 = NULL;
	adapter->fpgaWriteMMIO512 = NULL;
	adapter->fpgaMapMMIO = NULL;
	adapter->fpgaUnmapMMIO = NULL;
	adapter->fpgaCloneToken = NULL;
	adapter->fpgaGetNumUmsg = NULL;
	adapter->fpgaSetUmsgAttributes = NULL;
	adapter->fpgaTriggerUmsg = NULL;
	adapter->fpgaGetUmsgPtr = NULL;
	adapter->fpgaPrepareBuffer = NULL;
	adapter->fpgaReleaseBuffer = NULL;
	adapter->fpgaGetIOAddress = NULL;
	/*
	**	adapter->fpgaGetOPAECVersion = NULL;
	**	adapter->fpgaGetOPAECVersionString = NULL;
	*adapter->fpgaGetOPAECBuildString = NULL;
	*/
	adapter->fpgaReadError = NULL;
	adapter->fpgaClearError = NULL;
	adapter->fpgaClearAllErrors = NULL;
	adapter->fpgaGetErrorInfo = NULL;
	adapter->fpgaCreateEventHandle = NULL;
	adapter->fpgaDestroyEventHandle = NULL;
	adapter->fpgaGetOSObjectFromEventHandle = NULL;
	adapter->fpgaRegisterEvent = NULL;
	adapter->fpgaUnregisterEvent = NULL;
	adapter->fpgaAssignPortToInterface = NULL;
	adapter->fpgaAssignToInterface = NULL;
	adapter->fpgaReleaseFromInterface = NULL;
	adapter->fpgaReconfigureSlot = NULL;
	adapter->fpgaTokenGetObject = NULL;
	adapter->fpgaHandleGetObject = NULL;
	adapter->fpgaObjectGetObject = NULL;
	adapter->fpgaDestroyObject = NULL;
	adapter->fpgaObjectRead = NULL;
	adapter->fpgaObjectRead64 = NULL;
	adapter->fpgaObjectGetSize = NULL;
	adapter->fpgaObjectWrite64 = NULL;
	adapter->fpgaSetUserClock = NULL;
	adapter->fpgaGetUserClock = NULL;
	adapter->fpgaGetNumMetrics = NULL;
	adapter->fpgaGetMetricsInfo = NULL;
	adapter->fpgaGetMetricsByIndex = NULL;
	adapter->fpgaGetMetricsByName = NULL;

	return 0;
}
