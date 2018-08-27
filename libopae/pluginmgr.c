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
#endif /* HAVE_CONFIG_H */

#include <stdlib.h>
#include <dlfcn.h>
#define __USE_GNU
#include <pthread.h>

#include "pluginmgr.h"
#include "opae_int.h"

#define OPAE_PLUGIN_CONFIGURE "opae_plugin_configure"
typedef int (*opae_plugin_configure_t)(opae_api_adapter_table * , const char * );

static const char *native_plugins[] = {
	"libxfpga.so",
	NULL
};

static opae_api_adapter_table *adapter_list = NULL;
static pthread_mutex_t adapter_list_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

opae_api_adapter_table * opae_plugin_mgr_alloc_adapter(const char *lib_path)
{
	void *dl_handle;
	opae_api_adapter_table *adapter;

	dl_handle = dlopen(lib_path, RTLD_LAZY|RTLD_LOCAL);

	if (!dl_handle) {
		char *err = dlerror();
		OPAE_ERR("failed to load \"%s\" %s", lib_path, err ? err:"");
		return NULL;
	}

	adapter = (opae_api_adapter_table *) calloc(1, sizeof(opae_api_adapter_table));

	if (!adapter) {
		dlclose(dl_handle);
		OPAE_ERR("out of memory");
		return NULL;
	}

	adapter->plugin.path = (char *) lib_path;
	adapter->plugin.dl_handle = dl_handle;

	return adapter;
}

int opae_plugin_mgr_free_adapter(opae_api_adapter_table *adapter)
{
	int res;
	char *err;

	res = dlclose(adapter->plugin.dl_handle);

	if (res) {
		err = dlerror();
		OPAE_ERR("dlclose failed with %d %s", res, err ? err:"");
	}

	free(adapter);

	return res;
}

int opae_plugin_mgr_configure_plugin(opae_api_adapter_table *adapter,
				     const char *config)
{
	opae_plugin_configure_t cfg;

	cfg = (opae_plugin_configure_t) dlsym(adapter->plugin.dl_handle,
						OPAE_PLUGIN_CONFIGURE);

	if (!cfg) {
		OPAE_ERR("failed to find %s in \"%s\"",
				OPAE_PLUGIN_CONFIGURE,
				adapter->plugin.path);
		return 1;
	}

	return cfg(adapter, config);
}

int opae_plugin_mgr_initialize_all(void)
{
	int res;
	opae_api_adapter_table *aptr;
	int errors = 0;

	opae_mutex_lock(res, &adapter_list_lock);

	for (aptr = adapter_list ; aptr ; aptr = aptr->next) {

		if (aptr->initialize) {
			res = aptr->initialize();
			if (res) {
				OPAE_MSG("\"%s\" initialize() routine failed",
						aptr->plugin.path);
				++errors;
			}
		}

	}

	opae_mutex_unlock(res, &adapter_list_lock);

	return errors;
}

int opae_plugin_mgr_finalize_all(void)
{
	int res;
	opae_api_adapter_table *aptr;
	int errors = 0;

	opae_mutex_lock(res, &adapter_list_lock);

	for (aptr = adapter_list ; aptr ; ) {
		opae_api_adapter_table *trash;

		if (aptr->finalize) {
			res = aptr->finalize();
			if (res) {
				OPAE_MSG("\"%s\" finalize() routine failed",
						aptr->plugin.path);
				++errors;
			}
		}

		trash = aptr;
		aptr = aptr->next;

		if (opae_plugin_mgr_free_adapter(trash))
			++errors;
	}

	opae_mutex_unlock(res, &adapter_list_lock);

	return errors;


}

int opae_plugin_mgr_initialize(const char *cfg_file)
{
	int i;
	int res;
	opae_api_adapter_table *adapter;

	// TODO: parse config file
	opae_plugin_mgr_parse_config();


	// load each of the native plugins

	for (i = 0 ; native_plugins[i] ; ++i) {

		adapter = opae_plugin_mgr_alloc_adapter(native_plugins[i]);

		if (!adapter)
			return 1;

		// TODO: pass serialized json for native plugin
		res = opae_plugin_mgr_configure_plugin(adapter, "");
		if (res) {
			opae_plugin_mgr_free_adapter(adapter);
			OPAE_ERR("failed to configure plugin \"%s\"",
					native_plugins[i]);
			continue;
		}

		res = opae_plugin_mgr_register_adapter(adapter);
		if (res) {
			opae_plugin_mgr_free_adapter(adapter);
			OPAE_ERR("registering \"%s\"", native_plugins[i]);
		}
	}

	// TODO: load non-native plugins described in config file.


	return opae_plugin_mgr_initialize_all();
}

int opae_plugin_mgr_parse_config(/* json_object *jobj */)
{

	return 1;
}

int opae_plugin_mgr_register_adapter(opae_api_adapter_table *adapter)
{
	int res;
	opae_api_adapter_table *aptr;

	adapter->next = NULL;

	opae_mutex_lock(res, &adapter_list_lock);

	if (!adapter_list) {
		adapter_list = adapter;
		goto out_unlock;
	}

	// new entries go to the end of the list.
	for (aptr = adapter_list ; aptr->next ; aptr = aptr->next)
		/* find the last entry */;

	aptr->next = adapter;

out_unlock:
	opae_mutex_unlock(res, &adapter_list_lock);

	return 0;
}

int opae_plugin_mgr_for_each_adapter(
		int (*callback)(const opae_api_adapter_table * , void * ),
		void *context)
{
	int res;
	int cb_res = OPAE_ENUM_CONTINUE;
	opae_api_adapter_table *aptr;

	if (!callback) {
		OPAE_ERR("NULL callback passed to %s()", __func__);
		return OPAE_ENUM_STOP;
	}

	opae_mutex_lock(res, &adapter_list_lock);

	for (aptr = adapter_list ; aptr ; aptr = aptr->next) {
		cb_res = callback(aptr, context);
		if (cb_res)
			break;
	}

	opae_mutex_unlock(res, &adapter_list_lock);

	return cb_res;
}
