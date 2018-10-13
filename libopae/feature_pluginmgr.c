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
#include <uuid/uuid.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>
#include <linux/limits.h>
#define __USE_GNU
#include <pthread.h>

#include "safe_string/safe_string.h"

#include "feature_pluginmgr.h"
#include "opae_int.h"

#define DMA_PLUGIN_CONFIGURE "dma_plugin_configure"
typedef int (*dma_plugin_configure_t)(opae_dma_adapter_table *, const char *);

typedef struct _feature_data {
  char *feature_id;
	const char *feature_plugin;
	uint32_t flags;
#define OPAE_FEATURE_DATA_DETECTED 0x00000001
#define OPAE_FEATURE_DATA_LOADED   0x00000002
} feature_data;

static feature_data feature_data_table[] = {
	{ DMA_ID1, "libintel-dma.so", 0 },
	{ NULL, NULL, 0 },
};

static int initialized;

STATIC opae_dma_adapter_table *dma_adapter_list = (void *)0;
static pthread_mutex_t dma_adapter_list_lock =
	PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

STATIC opae_dma_adapter_table *dma_plugin_mgr_alloc_adapter(const char *lib_path, fpga_guid guid)
{
	void *dl_handle;
	opae_dma_adapter_table *adapter;
  errno_t e;

	dl_handle = dlopen(lib_path, RTLD_LAZY | RTLD_LOCAL);

	if (!dl_handle) {
		char *err = dlerror();
		OPAE_ERR("failed to load \"%s\" %s", lib_path, err ? err : "");
		return NULL;
	}

	adapter = (opae_dma_adapter_table *)calloc(
		1, sizeof(opae_dma_adapter_table));

	if (!adapter) {
		dlclose(dl_handle);
		OPAE_ERR("out of memory");
		return NULL;
	}

	e = memcpy_s(adapter->guid, sizeof(fpga_guid), guid, sizeof(fpga_guid));
	if (EOK != e) {
		OPAE_ERR("memcpy_s failed");
		return NULL;
	}
	adapter->plugin.path = (char *)lib_path;
	adapter->plugin.dl_handle = dl_handle;

	return adapter;
}

STATIC int dma_plugin_mgr_free_adapter(opae_dma_adapter_table *adapter)
{
  UNUSED_PARAM(adapter);
  return 0;
  /*
	int res;
	char *err;

	res = dlclose(adapter->plugin.dl_handle);

	if (res) {
		err = dlerror();
		OPAE_ERR("dlclose failed with %d %s", res, err ? err : "");
	}

	free(adapter);

	return res; */
}

STATIC int dma_plugin_mgr_configure_plugin(opae_dma_adapter_table *adapter,
					    const char *config)
{
  UNUSED_PARAM(adapter);
  UNUSED_PARAM(config);
  return 0;
/*	dma_plugin_configure_t cfg;

	cfg = (dma_plugin_configure_t)dlsym(adapter->plugin.dl_handle,
					     DMA_PLUGIN_CONFIGURE);

	if (!cfg) {
		OPAE_ERR("failed to find %s in \"%s\"", DMA_PLUGIN_CONFIGURE,
			 adapter->plugin.path);
		return 1;
	}

	return cfg(adapter, config);
*/
}

STATIC int dma_plugin_mgr_initialize_all(void)
{
	int res;
	opae_dma_adapter_table *aptr;
	int errors = 0;

	for (aptr = dma_adapter_list; aptr; aptr = aptr->next) {

		if (aptr->initialize) {
			res = aptr->initialize();
			if (res) {
				OPAE_MSG("\"%s\" initialize() routine failed",
					 aptr->plugin.path);
				++errors;
			}
		}
	}

	return errors;
}

int dma_plugin_mgr_finalize_all(void)
{
	int res;
	opae_dma_adapter_table *aptr;
	int errors = 0;

	opae_mutex_lock(res, &dma_adapter_list_lock);

	for (aptr = dma_adapter_list; aptr;) {
		opae_dma_adapter_table *trash;

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

		if (dma_plugin_mgr_free_adapter(trash))
			++errors;
	}

	dma_adapter_list = NULL;
	initialized = 0;

	opae_mutex_unlock(res, &dma_adapter_list_lock);

	return errors;
}


STATIC int dma_plugin_mgr_register_adapter(opae_dma_adapter_table *adapter)
{
	opae_dma_adapter_table *aptr;

	adapter->next = NULL;

	if (!dma_adapter_list) {
		dma_adapter_list = adapter;
		return 0;
	}

	// new entries go to the end of the list.
	for (aptr = dma_adapter_list; aptr->next; aptr = aptr->next)
		/* find the last entry */;

	aptr->next = adapter;

	return 0;
}

static int opae_plugin_mgr_detect_feature(fpga_guid guid)
{
	int i;
  int res, errors;
  char feature_id[40];
  uuid_unparse_upper(guid, feature_id);
  errors = 0;  

	for (i = 0 ; feature_data_table[i].feature_plugin ; ++i) {

		if (EOK != strcmp_s(feature_data_table[i].feature_id, 
                  strnlen_s(feature_data_table[i].feature_id, 256), feature_id, &res)) {
			OPAE_ERR("strcmp_s failed");
			++errors;
			goto out_close;
		}
                  
    if (0 == res) {
			OPAE_DBG("platform detected: feature_id=%s -> %s", feature_id
					feature_data_table[i].feature_plugin);

			feature_data_table[i].flags |= OPAE_FEATURE_DATA_DETECTED;
		}
	}
 
out_close:
  return errors;
}

void get_guid(uint64_t uuid_lo, uint64_t uuid_hi, fpga_guid *guid)
{
  char id_lo[18];
  char id_hi[18];
  
  snprintf_s_l(id_lo, 18, "%lx", uuid_lo);
  snprintf_s_l(id_hi, 18, "%lx", uuid_hi);
  
  char *p = NULL;
  int i = 0;
  uint32_t tmp;
  for (i = 14; i>=0; i-=2){
    p = id_lo + i;
    sscanf_s_u(p,"%2x", &tmp);
    (*guid)[7-i/2 ] = tmp;
    p[0] = '\0';
  }
  
  for (i = 14; i>=0; i-=2) {
    p = id_hi + i;
    sscanf_s_u(p,"%2x", &tmp);
    (*guid)[15-i/2 ] = tmp;
    p[0] = '\0';
  } 
}


static fpga_result opae_plugin_mgr_detect_features(fpga_handle handle)
{ 
  fpga_result res = FPGA_OK;
  // Discover feature BBB by traversing the device feature list 
  bool end_of_list = false; 
  uint64_t dfh = 0;
  uint32_t mmio_num = 0;
  uint64_t offset = 0;
  fpga_guid guid;
  
  do { 
    uint64_t feature_uuid_lo, feature_uuid_hi;
    
    // Read the current feature's UUID     
    res = fpgaReadMMIO64(handle, mmio_num, offset + 8, 
                        &feature_uuid_lo);
    if (res != FPGA_OK) {
      OPAE_ERR("fpgaReadMMIO64() failed");
      return res;
    }
 
    res = fpgaReadMMIO64(handle, mmio_num, offset + 16, 
            						&feature_uuid_hi); 
    if (res != FPGA_OK) {
      OPAE_ERR("fpgaReadMMIO64() failed");
      return res;
    }

    get_guid(feature_uuid_lo, feature_uuid_hi, &guid);
    
		// Detect feature for this guid.
		opae_plugin_mgr_detect_feature(guid);
 
    // End of the list? 
    end_of_list = _fpga_dfh_feature_eol(dfh); 
 
    // Move to the next feature header 
    offset = offset + _fpga_dfh_feature_next(dfh); 
  } while (!end_of_list);

	return res;
}

int dma_plugin_mgr_initialize(fpga_handle handle)
{
  fpga_result res = FPGA_OK;
	int ret;
	int errors = 0;
	int features_detected = 0;
	opae_dma_adapter_table *adapter;
	int i,j;
  fpga_guid guid;
 
	opae_mutex_lock(res, &dma_adapter_list_lock);

	if (initialized) { // prevent multiple init.
		opae_mutex_unlock(res, &dma_adapter_list_lock);
		return 0;
	}

	res = opae_plugin_mgr_detect_features(handle);
	if (res != FPGA_OK)
		goto out_unlock;

	// Load each of the native plugins that were detected.
	for (i = 0 ; feature_data_table[i].feature_plugin ; ++i) {
		const char *feature_plugin;
		int already_loaded;

		if (!(feature_data_table[i].flags & OPAE_FEATURE_DATA_DETECTED))
			continue; // This platform was not detected.

		feature_plugin = feature_data_table[i].feature_plugin;
		features_detected++;

		// Iterate over the table again to prevent multiple loads
		// of the same native plugin.
		already_loaded = 0;
		for (j = 0 ; feature_data_table[j].feature_plugin ; ++j) {

			if (EOK != strcmp_s(feature_plugin, strnlen_s(feature_plugin, 256),
						feature_data_table[j].feature_plugin, &ret)) {
				OPAE_ERR("strcmp_s failed");
				++errors;
				goto out_unlock;
			}

			if (!ret &&
			    (feature_data_table[j].flags & OPAE_FEATURE_DATA_LOADED)) {
				already_loaded = 1;
				break;
			}
		}

		if (already_loaded)
			continue;

    uuid_parse(feature_plugin, guid);
		adapter = dma_plugin_mgr_alloc_adapter(feature_plugin, guid);

		if (!adapter) {
			OPAE_ERR("malloc failed");
			++errors;
			goto out_unlock;
		}

		// TODO: pass serialized json for native plugin
		ret = dma_plugin_mgr_configure_plugin(adapter, "");
		if (ret) {
			dma_plugin_mgr_free_adapter(adapter);
			OPAE_ERR("failed to configure plugin \"%s\"",
				 feature_plugin);
			++errors;
			continue; // Keep going.
		}

		res = dma_plugin_mgr_register_adapter(adapter);
		if (res) {
			dma_plugin_mgr_free_adapter(adapter);
			OPAE_ERR("Failed to register \"%s\"", feature_plugin);
			++errors;
			continue; // Keep going.
		}

		feature_data_table[i].flags |= OPAE_FEATURE_DATA_LOADED;
	}

	// TODO: load non-native plugins described in config file.

	// Call each plugin's initialization routine.
	errors += dma_plugin_mgr_initialize_all();

	if (!errors && features_detected)
		initialized = 1;

out_unlock:
	opae_mutex_unlock(res, &dma_adapter_list_lock);

	return errors;
}

opae_dma_adapter_table *get_dma_plugin_adapter(fpga_guid guid)
{
	int res;
  opae_dma_adapter_table *adapter = NULL;
	opae_dma_adapter_table *aptr;

	opae_mutex_lock(res, &dma_adapter_list_lock);

	for (aptr = dma_adapter_list; aptr; aptr = aptr->next) {
		res = uuid_compare(aptr->guid, guid);
		if (!res) {
      adapter = aptr;
      break;
    }
  }
	opae_mutex_unlock(res, &dma_adapter_list_lock);

	return adapter;
}
