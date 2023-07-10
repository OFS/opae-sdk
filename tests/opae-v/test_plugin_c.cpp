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
#include "opae_vfio.h"

int vfio_plugin_initialize(void);
int vfio_plugin_finalize(void);
int opae_plugin_configure(opae_api_adapter_table *adapter,
                          const char *jsonConfig);

fpga_result vfio_fpgaOpen(fpga_token token, fpga_handle *handle, int flags);
fpga_result vfio_fpgaClose(fpga_handle handle);
fpga_result vfio_fpgaReset(fpga_handle handle);
fpga_result vfio_fpgaUpdateProperties(fpga_token token, fpga_properties prop);
fpga_result vfio_fpgaGetProperties(fpga_token token, fpga_properties *prop);
fpga_result vfio_fpgaGetPropertiesFromHandle(fpga_handle handle, fpga_properties *prop);
fpga_result vfio_fpgaWriteMMIO64(fpga_handle handle, uint32_t mmio_num,
                                 uint64_t offset, uint64_t value);
fpga_result vfio_fpgaReadMMIO64(fpga_handle handle, uint32_t mmio_num,
                                uint64_t offset, uint64_t *value);
fpga_result vfio_fpgaWriteMMIO32(fpga_handle handle, uint32_t mmio_num,
                                 uint64_t offset, uint32_t value);
fpga_result vfio_fpgaReadMMIO32(fpga_handle handle, uint32_t mmio_num,
                                uint64_t offset, uint32_t *value);
fpga_result vfio_fpgaWriteMMIO512(fpga_handle handle, uint32_t mmio_num,
                                  uint64_t offset, const void *value);
fpga_result vfio_fpgaMapMMIO(fpga_handle handle, uint32_t mmio_num,
                             uint64_t **mmio_ptr);
fpga_result vfio_fpgaUnmapMMIO(fpga_handle handle, uint32_t mmio_num);
fpga_result vfio_fpgaEnumerate(const fpga_properties *filters,
                               uint32_t num_filters, fpga_token *tokens,
                               uint32_t max_tokens, uint32_t *num_matches);
fpga_result vfio_fpgaCloneToken(fpga_token src, fpga_token *dst);
fpga_result vfio_fpgaDestroyToken(fpga_token *token);
fpga_result vfio_fpgaPrepareBuffer(fpga_handle handle,
                                   uint64_t len,
                                   void **buf_addr,
                                   uint64_t *wsid,
                                   int flags);
fpga_result vfio_fpgaReleaseBuffer(fpga_handle handle, uint64_t wsid);
fpga_result vfio_fpgaGetIOAddress(fpga_handle handle,
                                  uint64_t wsid,
                                  uint64_t *ioaddr);
fpga_result vfio_fpgaCreateEventHandle(fpga_event_handle *event_handle);
fpga_result vfio_fpgaDestroyEventHandle(fpga_event_handle *event_handle);
fpga_result vfio_fpgaGetOSObjectFromEventHandle(const fpga_event_handle eh,
                                                int *fd);
fpga_result vfio_fpgaRegisterEvent(fpga_handle handle,
                                   fpga_event_type event_type,
                                   fpga_event_handle event_handle,
                                   uint32_t flags);
fpga_result vfio_fpgaUnregisterEvent(fpga_handle handle,
                                     fpga_event_type event_type,
                                     fpga_event_handle event_handle);

extern libopae_config_data *opae_v_supported_devices;
extern vfio_pci_device_t *_pci_devices;
}

/**
 * @test    vfio_plugin_init
 * @brief   Test: vfio_plugin_initialize()
 * @details The function initializes the<br>
 *          opae_v_supported_devices table.
 */
TEST(opae_v, vfio_plugin_init)
{
  EXPECT_EQ(0, vfio_plugin_initialize());
  EXPECT_NE(nullptr, opae_v_supported_devices);

  vfio_free_device_list();
  opae_free_libopae_config(opae_v_supported_devices);
  opae_v_supported_devices = nullptr;
}

/**
 * @test    vfio_plugin_final
 * @brief   Test: vfio_plugin_finalize()
 * @details The function releases the<br>
 *          _pci_devices list and the<br>
 *          opae_v_supported_devices table,<br>
 *          then returns 0.
 */
TEST(opae_v, vfio_plugin_final)
{
  _pci_devices = (vfio_pci_device_t *)opae_calloc(1, sizeof(vfio_pci_device_t));
  opae_v_supported_devices = (libopae_config_data *)opae_calloc(1, sizeof(libopae_config_data));

  ASSERT_NE(nullptr, _pci_devices);
  ASSERT_NE(nullptr, opae_v_supported_devices);

  EXPECT_EQ(0, vfio_plugin_finalize());
  EXPECT_EQ(nullptr, _pci_devices);
  EXPECT_EQ(nullptr, opae_v_supported_devices);
}

/**
 * @test    vfio_plugin_config
 * @brief   Test: opae_plugin_configure()
 * @details The function properly initializes the
 *          correct functions in the adapter and
 *          returns 0.<br>
 */
TEST(opae_v, vfio_plugin_config)
{
  opae_api_adapter_table adapter;
  memset(&adapter, 0, sizeof(adapter));

  adapter.plugin.dl_handle = dlopen(nullptr, RTLD_LAZY | RTLD_LOCAL);
  ASSERT_NE(nullptr, adapter.plugin.dl_handle);

  EXPECT_EQ(0, opae_plugin_configure(&adapter, nullptr));

  EXPECT_EQ(vfio_fpgaOpen, adapter.fpgaOpen);
  EXPECT_EQ(vfio_fpgaClose, adapter.fpgaClose);
  EXPECT_EQ(vfio_fpgaReset, adapter.fpgaReset);
  EXPECT_EQ(vfio_fpgaGetPropertiesFromHandle, adapter.fpgaGetPropertiesFromHandle);
  EXPECT_EQ(vfio_fpgaGetProperties, adapter.fpgaGetProperties);
  EXPECT_EQ(vfio_fpgaUpdateProperties, adapter.fpgaUpdateProperties);
  EXPECT_EQ(vfio_fpgaWriteMMIO64, adapter.fpgaWriteMMIO64);
  EXPECT_EQ(vfio_fpgaReadMMIO64, adapter.fpgaReadMMIO64);
  EXPECT_EQ(vfio_fpgaWriteMMIO32, adapter.fpgaWriteMMIO32);
  EXPECT_EQ(vfio_fpgaReadMMIO32, adapter.fpgaReadMMIO32);
  EXPECT_EQ(vfio_fpgaWriteMMIO512, adapter.fpgaWriteMMIO512);
  EXPECT_EQ(vfio_fpgaMapMMIO, adapter.fpgaMapMMIO);
  EXPECT_EQ(vfio_fpgaUnmapMMIO, adapter.fpgaUnmapMMIO);
  EXPECT_EQ(vfio_fpgaEnumerate, adapter.fpgaEnumerate);
  EXPECT_EQ(vfio_fpgaCloneToken, adapter.fpgaCloneToken);
  EXPECT_EQ(vfio_fpgaDestroyToken, adapter.fpgaDestroyToken);
  EXPECT_EQ(vfio_fpgaPrepareBuffer, adapter.fpgaPrepareBuffer);
  EXPECT_EQ(vfio_fpgaReleaseBuffer, adapter.fpgaReleaseBuffer);
  EXPECT_EQ(vfio_fpgaGetIOAddress, adapter.fpgaGetIOAddress);
  EXPECT_EQ(vfio_fpgaCreateEventHandle, adapter.fpgaCreateEventHandle);
  EXPECT_EQ(vfio_fpgaDestroyEventHandle, adapter.fpgaDestroyEventHandle);
  EXPECT_EQ(vfio_fpgaGetOSObjectFromEventHandle, adapter.fpgaGetOSObjectFromEventHandle);
  EXPECT_EQ(vfio_fpgaRegisterEvent, adapter.fpgaRegisterEvent);
  EXPECT_EQ(vfio_fpgaUnregisterEvent, adapter.fpgaUnregisterEvent);
  EXPECT_EQ(vfio_plugin_initialize, adapter.initialize);
  EXPECT_EQ(vfio_plugin_finalize, adapter.finalize);

  dlclose(adapter.plugin.dl_handle);
}
