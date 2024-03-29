// Copyright(c) 2017-2022, Intel Corporation
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

#define NO_OPAE_C
#include "mock/opae_fixtures.h"
KEEP_XFPGA_SYMBOLS

extern "C" {
#include "sysfs_int.h"
#include "types_int.h"
#include "adapter.h"
#include "xfpga.h"

int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
int opae_plugin_configure(opae_api_adapter_table *adapter,
                          const char *jsonConfig);
}

using namespace opae::testing;

class xfpga_plugin_c_p : public opae_base_p<xfpga_> {
 protected:

  virtual void OPAEInitialize() override {
    ASSERT_EQ(xfpga_plugin_initialize(), 0);
  }

  virtual void OPAEFinalize() override {
    ASSERT_EQ(xfpga_plugin_finalize(), 0);
  }

  opae_api_adapter_table *opae_plugin_mgr_alloc_adapter_test(const char *lib_path);
  int opae_plugin_mgr_free_adapter_test(opae_api_adapter_table *adapter);
};

opae_api_adapter_table * xfpga_plugin_c_p::opae_plugin_mgr_alloc_adapter_test(const char *lib_path)
{
  void *dl_handle;
  opae_api_adapter_table *adapter;
  dl_handle = dlopen(lib_path, RTLD_LAZY | RTLD_LOCAL);
  if (!dl_handle) {
    char *err = dlerror();
    OPAE_ERR("failed to load \"%s\" %s", lib_path, err ? err : "");
    return NULL;
  }

  adapter = (opae_api_adapter_table *)opae_calloc(
             1, sizeof(opae_api_adapter_table));

  if (!adapter) {
    dlclose(dl_handle);
    OPAE_ERR("out of memory");
    return NULL;
  }
  adapter->plugin.path = (char *)lib_path;
  adapter->plugin.dl_handle = dl_handle;

  return adapter;
}

int xfpga_plugin_c_p::opae_plugin_mgr_free_adapter_test(opae_api_adapter_table *adapter)
{
  int res;
  char *err;

  res = dlclose(adapter->plugin.dl_handle);

  if (res) {
    err = dlerror();
    OPAE_ERR("dlclose failed with %d %s", res, err ? err : "");
  }

  opae_free(adapter);

  return res;
}

/*
* @test       plugin
* @brief      Tests:xfpga_plugin_initialize
* @details    When passed with NULL argument,the fn returns -1 <br>
*             When passed with valid argument,the fn returns 0 <br>
*/
TEST_P(xfpga_plugin_c_p, test_plugin_2) {

  opae_api_adapter_table *adapter_table = opae_plugin_mgr_alloc_adapter_test("libxfpga.so");

  EXPECT_EQ(opae_plugin_configure(adapter_table, NULL), 0);

  if (adapter_table)
    opae_plugin_mgr_free_adapter_test(adapter_table);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(xfpga_plugin_c_p);
INSTANTIATE_TEST_SUITE_P(xfpga_plugin_c, xfpga_plugin_c_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({
                                                                             "dfl-d5005",
                                                                             "dfl-n3000",
                                                                             "dfl-n6000-sku0",
                                                                             "dfl-n6000-sku1",
                                                                             "dfl-c6100"
                                                                           })));
