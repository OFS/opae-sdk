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

fpga_result legacy_port_reset(const uio_pci_device_t *p,
			      volatile uint8_t *port_base);

static inline dfh_ptr next_dfh(dfh_ptr h)
{
        if (h && h->next && !h->eol)
                return (dfh_ptr)(((volatile uint8_t *)h) + h->next);
        return NULL;
}

extern uint32_t fme_ports[4];

}

/**
 * @test    port_reset_pass
 * @brief   Test: legacy_port_reset()
 * @details legacy_port_reset() accurately detects the<br>
 *          port_reset_ack bit in the control register.
 */
TEST(opae_u, port_reset_pass)
{
  port_control ctrl_reg;

  ctrl_reg.port_reset = 0;
  ctrl_reg.port_reset_ack = 1;

  uint8_t *port_base = (uint8_t *)&ctrl_reg - PORT_CONTROL;

  EXPECT_EQ(FPGA_OK, legacy_port_reset(NULL, port_base));
  EXPECT_EQ(0, ctrl_reg.port_reset);
}

/**
 * @test    port_reset_fail
 * @brief   Test: legacy_port_reset()
 * @details legacy_port_reset() times out with FPGA_EXCEPTION<br>
 *          when the port_reset_ack bit in the control register<br>
 *          is not observed.
 */
TEST(opae_u, port_reset_fail)
{
  port_control ctrl_reg;

  ctrl_reg.port_reset = 0;
  ctrl_reg.port_reset_ack = 0;

  uint8_t *port_base = (uint8_t *)&ctrl_reg - PORT_CONTROL;

  EXPECT_EQ(FPGA_EXCEPTION, legacy_port_reset(NULL, port_base));
}

/**
 * @test    next_dfh_eol
 * @brief   Test: next_dfh()
 * @details next_dfh() returns NULL when the DFH's eol bit is set.
 */
TEST(opae_u, next_dfh_eol)
{
  dfh d;

  d.id = 3;
  d.major_rev = 2;
  d.next = 13;
  d.eol = 1;
  d.minor_rev = 2;
  d.version = 16;
  d.type = 3;

  EXPECT_EQ(nullptr, next_dfh(&d));
}

/**
 * @test    next_dfh_ok
 * @brief   Test: next_dfh()
 * @details next_dfh() returns an appropriate pointer<br>
 *          when the next field is non-zero and when<br>
 *          the eol bit is clear.
 */
TEST(opae_u, next_dfh_ok)
{
  dfh d;

  d.id = 3;
  d.major_rev = 2;
  d.next = 4 * sizeof(uint64_t);
  d.eol = 0;
  d.minor_rev = 2;
  d.version = 16;
  d.type = 3;

  const uint64_t *next = (uint64_t *)(((uint8_t *)&d) + 4 * sizeof(uint64_t));

  EXPECT_EQ((volatile _dfh *)next, next_dfh(&d));
}

/**
 * @test    walk_port_no_token
 * @brief   Test: walk_port()
 * @details walk_port() returns -1 when uio_get_token() fails.
 */
TEST(opae_u, walk_port_no_token)
{
  uio_pci_device_t device;
  device.tokens = nullptr;
  device.next = nullptr;

  uio_token parent;
  parent.device = &device;
  parent.parent = nullptr;
  parent.next = nullptr;

  test_system::instance()->invalidate_calloc(0, "uio_get_token");
  EXPECT_EQ(-1, walk_port(&parent, 0, nullptr));
}

/**
 * @test    walk_port_ok
 * @brief   Test: walk_port()
 * @details walk_port() returns 0 on success.
 */
TEST(opae_u, walk_port_ok)
{
  uint8_t mmio[8192];
  memset(mmio, 0, sizeof(mmio));

  dfh_ptr dfh = (dfh_ptr)mmio;
  port_next_afu_ptr next_afu = (port_next_afu_ptr)&mmio[PORT_NEXT_AFU];
  port_capability_ptr port_cap = (port_capability_ptr)&mmio[PORT_CAPABILITY];
  port_control_ptr port_ctrl = (port_control_ptr)&mmio[PORT_CONTROL];
  dfh_ptr stp = (dfh_ptr)&mmio[2048];

  dfh->eol = 0;
  dfh->next = 2048;
  next_afu->port_afu_dfh_offset = 4096;
  port_ctrl->port_reset_ack = 1;
  port_cap->mmio_size = 4096;
  stp->id = PORT_STP_ID;
  stp->eol = 1;

  uint64_t *guid_ptr = (uint64_t *)&mmio[4096];
  *guid_ptr = 0xdeadbeefc0cac01a;
  *(guid_ptr + 1) = 0xc0cac01adeadbeef;

  uio_pci_device_t device;
  memset(&device, 0, sizeof(device));

  uio_token parent;
  memset(&parent, 0, sizeof(parent));
  parent.device = &device;

  EXPECT_EQ(0, walk_port(&parent, 0, mmio));

  uio_token *new_port = device.tokens;
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
 * @details walk_fme() returns -1 when uio_get_token() fails.
 */
TEST(opae_u, walk_fme_no_token)
{
  uio_pci_device_t device;
  device.tokens = nullptr;
  device.next = nullptr;

  test_system::instance()->invalidate_calloc(0, "uio_get_token");
  EXPECT_EQ(-1, walk_fme(&device, nullptr, nullptr, 0));
}

/**
 * @test    walk_fme_ok
 * @brief   Test: walk_fme()
 * @details walk_fme() returns 0 on success.
 */
TEST(opae_u, walk_fme_ok)
{
  uint8_t mmio[16384];
  memset(mmio, 0, sizeof(mmio));

  const int FME = 0;
  const int PORT = 8192;

  dfh_ptr fme_dfh = (dfh_ptr)&mmio[FME];
  uint64_t *bitstream_id = (uint64_t *)&mmio[FME + BITSTREAM_ID];
  uint64_t *bitstream_md = (uint64_t *)&mmio[FME + BITSTREAM_MD];
  fab_capability_ptr cap = (fab_capability_ptr)&mmio[FME + FAB_CAPABILITY];
  port_offset_ptr port_offset = (port_offset_ptr)&mmio[FME + fme_ports[0]];
  dfh_ptr pr = (dfh_ptr)&mmio[FME + 2048];

  fme_dfh->eol = 0;
  fme_dfh->next = 2048;
  port_offset->implemented = 1;
  port_offset->bar = 0;
  *bitstream_id = 0xdeadbeefc0cac01a;
  *bitstream_md = 0xc0cac01adeadbeef;
  cap->num_ports = 1;
  pr->id = PR_FEATURE_ID;
  pr->eol = 1;

  dfh_ptr port_dfh = (dfh_ptr)&mmio[PORT];
  port_next_afu_ptr port_next_afu = (port_next_afu_ptr)&mmio[PORT + PORT_NEXT_AFU];
  port_capability_ptr port_cap = (port_capability_ptr)&mmio[PORT + PORT_CAPABILITY];
  port_control_ptr port_ctrl = (port_control_ptr)&mmio[PORT + PORT_CONTROL];
  dfh_ptr stp = (dfh_ptr)&mmio[PORT + 2048];

  port_dfh->eol = 0;
  port_dfh->next = 2048;
  port_next_afu->port_afu_dfh_offset = 4096;
  port_ctrl->port_reset_ack = 1;
  port_cap->mmio_size = 4096;
  stp->id = PORT_STP_ID;
  stp->eol = 1;

  uio_pci_device_t device;
  memset(&device, 0, sizeof(device));

  struct opae_uio_device_region region;
  region.region_index = 0;
  region.region_ptr = &mmio[PORT];
  region.region_page_offset = 0;
  region.region_size = 8192;
  region.next = nullptr;

  struct opae_uio u;
  strcpy(u.device_path, "/dev/uio0");
  u.device_fd = 0;
  u.regions = &region;

  EXPECT_EQ(0, walk_fme(&device, &u, mmio, 0));

  uio_token *port_token = device.tokens;
  ASSERT_NE(nullptr, port_token);

  uio_token *fme_token = port_token->next;
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
