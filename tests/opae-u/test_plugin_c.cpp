// Copyright(c) 2023, Intel Corporation
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

#include <dlfcn.h>

#include "gtest/gtest.h"
#include "mock/opae_std.h"
#include "mock/test_system.h"

#include "adapter.h"
#include "cfg-file.h"

using namespace opae::testing;

extern "C" {
#include "opae_uio.h"

int uio_plugin_initialize(void);
int uio_plugin_finalize(void);
int opae_plugin_configure(opae_api_adapter_table *adapter,
                          const char *jsonConfig);

fpga_result uio_fpgaOpen(fpga_token token, fpga_handle *handle, int flags);
fpga_result uio_fpgaClose(fpga_handle handle);
fpga_result uio_fpgaReset(fpga_handle handle);
fpga_result uio_fpgaUpdateProperties(fpga_token token, fpga_properties prop);
fpga_result uio_fpgaGetProperties(fpga_token token, fpga_properties *prop);
fpga_result uio_fpgaGetPropertiesFromHandle(fpga_handle handle, fpga_properties *prop);
fpga_result uio_fpgaWriteMMIO64(fpga_handle handle, uint32_t mmio_num,
                                uint64_t offset, uint64_t value);
fpga_result uio_fpgaReadMMIO64(fpga_handle handle, uint32_t mmio_num,
                               uint64_t offset, uint64_t *value);
fpga_result uio_fpgaWriteMMIO32(fpga_handle handle, uint32_t mmio_num,
                                uint64_t offset, uint32_t value);
fpga_result uio_fpgaReadMMIO32(fpga_handle handle, uint32_t mmio_num,
                               uint64_t offset, uint32_t *value);
fpga_result uio_fpgaWriteMMIO512(fpga_handle handle, uint32_t mmio_num,
                                 uint64_t offset, const void *value);
fpga_result uio_fpgaMapMMIO(fpga_handle handle, uint32_t mmio_num,
                            uint64_t **mmio_ptr);
fpga_result uio_fpgaUnmapMMIO(fpga_handle handle, uint32_t mmio_num);
fpga_result uio_fpgaEnumerate(const fpga_properties *filters,
                              uint32_t num_filters, fpga_token *tokens,
                              uint32_t max_tokens, uint32_t *num_matches);
fpga_result uio_fpgaCloneToken(fpga_token src, fpga_token *dst);
fpga_result uio_fpgaDestroyToken(fpga_token *token);
fpga_result uio_fpgaCreateEventHandle(fpga_event_handle *event_handle);
fpga_result uio_fpgaDestroyEventHandle(fpga_event_handle *event_handle);
fpga_result uio_fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh,
                                               int *fd);
fpga_result uio_fpgaRegisterEvent(fpga_handle handle,
                                  fpga_event_type event_type,
                                  fpga_event_handle event_handle,
                                  uint32_t flags);
fpga_result uio_fpgaUnregisterEvent(fpga_handle handle,
                                    fpga_event_type event_type,
                                    fpga_event_handle event_handle);

extern libopae_config_data *opae_u_supported_devices;
extern uio_pci_device_t *_pci_devices;
}

/**
 * @test    uio_plugin_init
 * @brief   Test: uio_plugin_initialize()
 * @details The function initializes the<br>
 *          opae_u_supported_devices table.
 */
TEST(opae_u, uio_plugin_init)
{
  EXPECT_EQ(0, uio_plugin_initialize());
  EXPECT_NE(nullptr, opae_u_supported_devices);

  uio_free_device_list();
  opae_free_libopae_config(opae_u_supported_devices);
  opae_u_supported_devices = nullptr;
}

/**
 * @test    uio_plugin_final
 * @brief   Test: uio_plugin_finalize()
 * @details The function releases the<br>
 *          _pci_devices list and the<br>
 *          opae_u_supported_devices table,<br>
 *          then returns 0.
 */
TEST(opae_u, uio_plugin_final)
{
  _pci_devices = (uio_pci_device_t *)opae_calloc(1, sizeof(uio_pci_device_t));
  opae_u_supported_devices = (libopae_config_data *)opae_calloc(1, sizeof(libopae_config_data));

  ASSERT_NE(nullptr, _pci_devices);
  ASSERT_NE(nullptr, opae_u_supported_devices);

  EXPECT_EQ(0, uio_plugin_finalize());
  EXPECT_EQ(nullptr, _pci_devices);
  EXPECT_EQ(nullptr, opae_u_supported_devices);
}

/**
 * @test    uio_plugin_config
 * @brief   Test: opae_plugin_configure()
 * @details The function properly initializes the
 *          correct functions in the adapter and
 *          returns 0.<br>
 */
TEST(opae_u, uio_plugin_config)
{
  opae_api_adapter_table adapter;
  memset(&adapter, 0, sizeof(adapter));

  adapter.plugin.dl_handle = dlopen(nullptr, RTLD_LAZY | RTLD_LOCAL);
  ASSERT_NE(nullptr, adapter.plugin.dl_handle);

  EXPECT_EQ(0, opae_plugin_configure(&adapter, nullptr));

  EXPECT_EQ(uio_fpgaOpen, adapter.fpgaOpen);
  EXPECT_EQ(uio_fpgaClose, adapter.fpgaClose);
  EXPECT_EQ(uio_fpgaReset, adapter.fpgaReset);
  EXPECT_EQ(uio_fpgaGetPropertiesFromHandle, adapter.fpgaGetPropertiesFromHandle);
  EXPECT_EQ(uio_fpgaGetProperties, adapter.fpgaGetProperties);
  EXPECT_EQ(uio_fpgaUpdateProperties, adapter.fpgaUpdateProperties);
  EXPECT_EQ(uio_fpgaWriteMMIO64, adapter.fpgaWriteMMIO64);
  EXPECT_EQ(uio_fpgaReadMMIO64, adapter.fpgaReadMMIO64);
  EXPECT_EQ(uio_fpgaWriteMMIO32, adapter.fpgaWriteMMIO32);
  EXPECT_EQ(uio_fpgaReadMMIO32, adapter.fpgaReadMMIO32);
  EXPECT_EQ(uio_fpgaWriteMMIO512, adapter.fpgaWriteMMIO512);
  EXPECT_EQ(uio_fpgaMapMMIO, adapter.fpgaMapMMIO);
  EXPECT_EQ(uio_fpgaUnmapMMIO, adapter.fpgaUnmapMMIO);
  EXPECT_EQ(uio_fpgaEnumerate, adapter.fpgaEnumerate);
  EXPECT_EQ(uio_fpgaCloneToken, adapter.fpgaCloneToken);
  EXPECT_EQ(uio_fpgaDestroyToken, adapter.fpgaDestroyToken);
  EXPECT_EQ(uio_fpgaCreateEventHandle, adapter.fpgaCreateEventHandle);
  EXPECT_EQ(uio_fpgaDestroyEventHandle, adapter.fpgaDestroyEventHandle);
  EXPECT_EQ(uio_fpgaGetOSObjectFromEventHandle, adapter.fpgaGetOSObjectFromEventHandle);
  EXPECT_EQ(uio_fpgaRegisterEvent, adapter.fpgaRegisterEvent);
  EXPECT_EQ(uio_fpgaUnregisterEvent, adapter.fpgaUnregisterEvent);
  EXPECT_EQ(uio_plugin_initialize, adapter.initialize);
  EXPECT_EQ(uio_plugin_finalize, adapter.finalize);

  dlclose(adapter.plugin.dl_handle);
}
