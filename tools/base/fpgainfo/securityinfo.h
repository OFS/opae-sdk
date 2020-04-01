// Copyright(c) 2020, Intel Corporation
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
/*
 * @file securityinfo.h
 *
 * @brief
 */
#ifndef SECURITYINFO_H
#define SECURITYINFO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <opae/fpga.h>

#define SYSFS_SPI0_GLOB "spi-altera.*/"
#define SYSFS_SPI1_GLOB "spi*/"
#define SYSFS_SEC_GLOB "ifpga_sec_mgr/*"
#define SYSFS_SEC_BIP_VER "security/bip_version"
#define SYSFS_SEC_BMC_CANCEL "security/bmc_canceled_csks"
#define SYSFS_SEC_SMBUS_COUNT "security/smbus_flash_count"
#define SYSFS_SEC_BMC_FWVERS "security/bmcfw_version"
#define SYSFS_SEC_BMC_ROOT "security/bmc_root_hash"
#define SYSFS_SEC_CRYPTO_VER "security/crypto_version"
#define SYSFS_SEC_PR_CANCEL "security/pr_canceled_csks"
#define SYSFS_SEC_PR_ROOT "security/pr_root_hash"
#define SYSFS_SEC_QSPI_COUNT "security/qspi_flash_count"
#define SYSFS_SEC_SR_CANCEL "security/sr_canceled_csks"
#define SYSFS_SEC_SR_ROOT "security/sr_root_hash"
#define SYSFS_SEC_FW_VER "security/tcmfw_version"

#define REH_D5005_v201 \
  "0x16609930bf6e65ee0d929a87884c37826a731bb317a11f4feb47b3cb328b9b0c"
#define REH_A10GX_v121 \
  "0xdbdcd1755fbe285489481f909cbe68600fa544c050b14f2d777858521a18786a"
#define REH_N3000_v11 \
  "0x757f524c2f45db58ac2a6c93e72b9167149979b795195d09d5e2efad82f2b031"
#define REH_N3000_v12 \
  "0x77698ea203e459f6cb0e65b54a1dd4ab47a6a6600e7988f723ad89f5b7f3673a"
#define REH_N3000_v13 \
  "0xec0f42d3af138e3eca7141107f7fed5f7c13846fadbba884e51ad26bf36a3d21"

fpga_result security_filter(fpga_properties *filter, int argc, char *argv[]);
fpga_result security_command(fpga_token *tokens, int num_tokens, int argc,
			     char *argv[]);
void security_help(void);

#ifdef __cplusplus
}
#endif

#endif /* !SECURITYINFO_H */
