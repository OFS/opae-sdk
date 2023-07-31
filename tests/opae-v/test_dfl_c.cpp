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

#include "gtest/gtest.h"
#include "mock/opae_std.h"
#include "mock/test_system.h"
using namespace opae::testing;

extern "C" {
#include "dfl.h"

fpga_result legacy_port_reset(const vfio_pci_device_t *p,
			      volatile uint8_t *port_base);

extern uint32_t fme_ports[4];

}

/**
 * @test    port_reset_pass
 * @brief   Test: legacy_port_reset()
 * @details legacy_port_reset() accurately detects the<br>
 *          port_reset_ack bit in the control register.
 */
TEST(opae_v, port_reset_pass)
{
  port_control ctrl_reg;

  ctrl_reg.bits.port_reset = 1;
  ctrl_reg.bits.port_reset_ack = 0;

  uint8_t *port_base = (uint8_t *)&ctrl_reg - PORT_CONTROL;

  EXPECT_EQ(FPGA_OK, legacy_port_reset(NULL, port_base));
  EXPECT_EQ(0, ctrl_reg.bits.port_reset);
}

/**
 * @test    port_reset_fail
 * @brief   Test: legacy_port_reset()
 * @details legacy_port_reset() times out with FPGA_EXCEPTION<br>
 *          when the port_reset_ack bit in the control register<br>
 *          is not observed to be cleared by hardware.
 */
TEST(opae_v, port_reset_fail)
{
  port_control ctrl_reg;

  ctrl_reg.bits.port_reset = 1;
  ctrl_reg.bits.port_reset_ack = 1;

  uint8_t *port_base = (uint8_t *)&ctrl_reg - PORT_CONTROL;

  EXPECT_EQ(FPGA_EXCEPTION, legacy_port_reset(NULL, port_base));
}

/**
 * @test    walk_port_no_token
 * @brief   Test: walk_port()
 * @details walk_port() returns -1 when vfio_get_token() fails.
 */
TEST(opae_v, walk_port_no_token)
{
#ifndef OPAE_ENABLE_MOCK
  GTEST_SKIP() << "Invalidate test requires MOCK.";
#endif // OPAE_ENABLE_MOCK

  vfio_pci_device_t device;
  device.tokens = nullptr;
  device.next = nullptr;

  vfio_token parent;
  parent.device = &device;
  parent.parent = nullptr;
  parent.next = nullptr;

  test_system::instance()->invalidate_calloc(0, "vfio_get_token");
  EXPECT_EQ(-1, walk_port(&parent, 0, nullptr));
}

/**
 * @test    walk_port_ok
 * @brief   Test: walk_port()
 * @details walk_port() returns 0 on success.
 */
TEST(opae_v, walk_port_ok)
{
  uint8_t mmio[8192];
  memset(mmio, 0, sizeof(mmio));

  dfh *dfh_ptr = (dfh *)mmio;
  port_next_afu *next_afu_ptr = (port_next_afu *)&mmio[PORT_NEXT_AFU];
  port_capability *port_cap_ptr = (port_capability *)&mmio[PORT_CAPABILITY];
  port_control *port_ctrl_ptr = (port_control *)&mmio[PORT_CONTROL];
  dfh *stp_ptr = (dfh *)&mmio[2048];

  dfh_ptr->bits.eol = 0;
  dfh_ptr->bits.next = 2048;
  next_afu_ptr->bits.port_afu_dfh_offset = 4096;
  port_ctrl_ptr->bits.port_reset_ack = 1;
  port_cap_ptr->bits.mmio_size = 4096;
  stp_ptr->bits.id = PORT_STP_ID;
  stp_ptr->bits.eol = 1;

  uint64_t *guid_ptr = (uint64_t *)&mmio[4096];
  *guid_ptr = 0xdeadbeefc0cac01a;
  *(guid_ptr + 1) = 0xc0cac01adeadbeef;

  vfio_pci_device_t device;
  memset(&device, 0, sizeof(device));

  vfio_token parent;
  memset(&parent, 0, sizeof(parent));
  parent.device = &device;

  EXPECT_EQ(0, walk_port(&parent, 0, mmio));

  vfio_token *new_port = device.tokens;
  ASSERT_NE(nullptr, new_port);

  EXPECT_EQ(&parent, new_port->parent);
  EXPECT_EQ(4096, new_port->mmio_size);
  EXPECT_EQ(2, new_port->user_mmio_count);
  EXPECT_EQ(4096, new_port->user_mmio[0]);

  opae_free(new_port);
}

/**
 * @test    walk_fme_no_token
 * @brief   Test: walk_fme()
 * @details walk_fme() returns -1 when vfio_get_token() fails.
 */
TEST(opae_v, walk_fme_no_token)
{
#ifndef OPAE_ENABLE_MOCK
  GTEST_SKIP() << "Invalidate test requires MOCK.";
#endif // OPAE_ENABLE_MOCK

  vfio_pci_device_t device;
  device.tokens = nullptr;
  device.next = nullptr;

  test_system::instance()->invalidate_calloc(0, "vfio_get_token");
  EXPECT_EQ(-1, walk_fme(&device, nullptr, nullptr, 0));
}

/**
 * @test    walk_fme_vfio_error
 * @brief   Test: walk_fme()
 * @details walk_fme() skips the Port under<br>
 *          inspection when opae_vfio_region_get() fails.
 */
TEST(opae_v, walk_fme_vfio_error)
{
  uint8_t mmio[16384];
  memset(mmio, 0, sizeof(mmio));

  const int FME = 0;
  const int PORT = 8192;

  dfh *fme_dfh_ptr = (dfh *)&mmio[FME];
  uint64_t *bitstream_id = (uint64_t *)&mmio[FME + BITSTREAM_ID];
  uint64_t *bitstream_md = (uint64_t *)&mmio[FME + BITSTREAM_MD];
  fab_capability *cap_ptr = (fab_capability *)&mmio[FME + FAB_CAPABILITY];
  port_offset *port_offset_ptr = (port_offset *)&mmio[FME + fme_ports[0]];
  dfh *pr_ptr = (dfh *)&mmio[FME + 2048];

  fme_dfh_ptr->bits.eol = 0;
  fme_dfh_ptr->bits.next = 2048;
  port_offset_ptr->bits.implemented = 1;
  port_offset_ptr->bits.bar = 0;
  *bitstream_id = 0xdeadbeefc0cac01a;
  *bitstream_md = 0xc0cac01adeadbeef;
  cap_ptr->bits.num_ports = 1;
  pr_ptr->bits.id = PR_FEATURE_ID;
  pr_ptr->bits.eol = 1;

  dfh *port_dfh_ptr = (dfh *)&mmio[PORT];
  port_next_afu *port_next_afu_ptr = (port_next_afu *)&mmio[PORT + PORT_NEXT_AFU];
  port_capability *port_cap_ptr = (port_capability *)&mmio[PORT + PORT_CAPABILITY];
  port_control *port_ctrl_ptr = (port_control *)&mmio[PORT + PORT_CONTROL];
  dfh *stp_ptr = (dfh *)&mmio[PORT + 2048];

  port_dfh_ptr->bits.eol = 0;
  port_dfh_ptr->bits.next = 2048;
  port_next_afu_ptr->bits.port_afu_dfh_offset = 4096;
  port_ctrl_ptr->bits.port_reset_ack = 1;
  port_cap_ptr->bits.mmio_size = 4096;
  stp_ptr->bits.id = PORT_STP_ID;
  stp_ptr->bits.eol = 1;

  vfio_pci_device_t device;
  memset(&device, 0, sizeof(device));

  EXPECT_EQ(0, walk_fme(&device, nullptr, mmio, 0));

  vfio_token *fme_token = device.tokens;

  EXPECT_EQ(0xdeadbeefc0cac01a, fme_token->bitstream_id);
  EXPECT_EQ(0xc0cac01adeadbeef, fme_token->bitstream_mdata);
  EXPECT_EQ(1, fme_token->num_ports);

  opae_free(fme_token);
}

/**
 * @test    walk_fme_ok
 * @brief   Test: walk_fme()
 * @details walk_fme() returns 0 on success.
 */
TEST(opae_v, walk_fme_ok)
{
  uint8_t mmio[16384];
  memset(mmio, 0, sizeof(mmio));

  const int FME = 0;
  const int PORT = 8192;

  dfh *fme_dfh_ptr = (dfh *)&mmio[FME];
  uint64_t *bitstream_id = (uint64_t *)&mmio[FME + BITSTREAM_ID];
  uint64_t *bitstream_md = (uint64_t *)&mmio[FME + BITSTREAM_MD];
  fab_capability *cap_ptr = (fab_capability *)&mmio[FME + FAB_CAPABILITY];
  port_offset *port_offset_ptr = (port_offset *)&mmio[FME + fme_ports[0]];
  dfh *pr_ptr = (dfh *)&mmio[FME + 2048];

  fme_dfh_ptr->bits.eol = 0;
  fme_dfh_ptr->bits.next = 2048;
  port_offset_ptr->bits.implemented = 1;
  port_offset_ptr->bits.bar = 0;
  *bitstream_id = 0xdeadbeefc0cac01a;
  *bitstream_md = 0xc0cac01adeadbeef;
  cap_ptr->bits.num_ports = 1;
  pr_ptr->bits.id = PR_FEATURE_ID;
  pr_ptr->bits.eol = 1;

  dfh *port_dfh_ptr = (dfh *)&mmio[PORT];
  port_next_afu *port_next_afu_ptr = (port_next_afu *)&mmio[PORT + PORT_NEXT_AFU];
  port_capability *port_cap_ptr = (port_capability *)&mmio[PORT + PORT_CAPABILITY];
  port_control *port_ctrl_ptr = (port_control *)&mmio[PORT + PORT_CONTROL];
  dfh *stp_ptr = (dfh *)&mmio[PORT + 2048];

  port_dfh_ptr->bits.eol = 0;
  port_dfh_ptr->bits.next = 2048;
  port_next_afu_ptr->bits.port_afu_dfh_offset = 4096;
  port_ctrl_ptr->bits.port_reset_ack = 1;
  port_cap_ptr->bits.mmio_size = 4096;
  stp_ptr->bits.id = PORT_STP_ID;
  stp_ptr->bits.eol = 1;

  vfio_pci_device_t device;
  memset(&device, 0, sizeof(device));

  struct opae_vfio_device_region region;
  region.region_index = 0;
  region.region_ptr = &mmio[PORT];
  region.region_size = 8192;
  region.region_sparse = nullptr;
  region.next = nullptr;

  struct opae_vfio v;
  memset(&v, 0, sizeof(v));
  v.lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
  v.cont_device = (char *)"/dev/vfio/vfio";
  v.cont_pciaddr = (char *)"0000:00:00.0";
  v.cont_fd = -1;
  v.device.device_fd = -1;
  v.device.device_num_regions = 1;
  v.device.regions = &region;

  EXPECT_EQ(0, walk_fme(&device, &v, mmio, 0));

  vfio_token *port_token = device.tokens;
  ASSERT_NE(nullptr, port_token);

  vfio_token *fme_token = port_token->next;
  ASSERT_NE(nullptr, fme_token);

  EXPECT_EQ(fme_token, port_token->parent);
  EXPECT_EQ(4096, port_token->mmio_size);
  EXPECT_EQ(2, port_token->user_mmio_count);
  EXPECT_EQ(4096, port_token->user_mmio[0]);

  EXPECT_EQ(0xdeadbeefc0cac01a, fme_token->bitstream_id);
  EXPECT_EQ(0xc0cac01adeadbeef, fme_token->bitstream_mdata);
  EXPECT_EQ(1, fme_token->num_ports);

  opae_free(port_token);
  opae_free(fme_token);
}
