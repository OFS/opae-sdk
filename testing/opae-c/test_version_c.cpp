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

extern "C" {

#include <json-c/json.h>
#include <uuid/uuid.h>
#include "opae_int.h"

}

#include <config.h>
#include <opae/fpga.h>

#include <array>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

/**
 * @test       opaec
 * @brief      Test: fpgaGetOPAECVersion
 * @details    When fpgaGetOPAECVersion is called with a valid param,<br>
 *             then it retrieves the INTEL_FPGA_API_VER_* constants<br>
 *             from config.h.<br>
 */
TEST(version, opaec) {
  fpga_version v;
  EXPECT_EQ(fpgaGetOPAECVersion(&v), FPGA_OK);
  EXPECT_EQ(v.major, INTEL_FPGA_API_VER_MAJOR);
  EXPECT_EQ(v.minor, INTEL_FPGA_API_VER_MINOR);
  EXPECT_EQ(v.patch, INTEL_FPGA_API_VER_REV);
}

/**
 * @test       opaec_string
 * @brief      Test: fpgaGetOPAECVersionString
 * @details    When fpgaGetOPAECVersionString is called with valid params,<br>
 *             then it retrieves the INTEL_FPGA_API_VERSION string<br>
 *             from config.h.<br>
 */
TEST(version, opaec_string) {
  char ver_str[32];
  EXPECT_EQ(fpgaGetOPAECVersionString(ver_str, sizeof(ver_str)), FPGA_OK);
  EXPECT_STREQ(ver_str, INTEL_FPGA_API_VERSION);
}

/**
 * @test       opaec_string_err
 * @brief      Test: fpgaGetOPAECVersionString
 * @details    When fpgaGetOPAECVersionString is called with invalid params,<br>
 *             then it returns FPGA_EXCEPTION.<br>
 */
TEST(version, opaec_string_err) {
  char ver_str[4097];
  EXPECT_EQ(fpgaGetOPAECVersionString(ver_str, sizeof(ver_str)), FPGA_EXCEPTION);
}

/**
 * @test       opaec_build_string
 * @brief      Test: fpgaGetOPAECBuildString
 * @details    When fpgaGetOPAECBuildString is called with valid params,<br>
 *             then it retrieves the INTEL_FPGA_API_HASH string<br>
 *             from config.h.<br>
 */
TEST(version, opaec_build_string) {
  char b_str[32];
  EXPECT_EQ(fpgaGetOPAECBuildString(b_str, sizeof(b_str)), FPGA_OK);
  EXPECT_STREQ(b_str, INTEL_FPGA_API_HASH);
}

/**
 * @test       opaec_build_string_err
 * @brief      Test: fpgaGetOPAECBuildString
 * @details    When fpgaGetOPAECBuildString is called with invalid params,<br>
 *             then it returns FPGA_EXCEPTION.<br>
 */
TEST(version, opaec_build_string_err) {
  char b_str[4097];
  EXPECT_EQ(fpgaGetOPAECBuildString(b_str, sizeof(b_str)), FPGA_EXCEPTION);
}

/**
 * @test       str
 * @brief      Test: fpgaErrStr
 * @details    When fpgaErrStr is called with valid params,<br>
 *             then it returns the corresponding const char *.<br>
 */
TEST(err, str) {
  EXPECT_STREQ(fpgaErrStr(FPGA_OK),            "success");
  EXPECT_STREQ(fpgaErrStr(FPGA_INVALID_PARAM), "invalid parameter");
  EXPECT_STREQ(fpgaErrStr(FPGA_BUSY),          "resource busy");
  EXPECT_STREQ(fpgaErrStr(FPGA_EXCEPTION),     "exception");
  EXPECT_STREQ(fpgaErrStr(FPGA_NOT_FOUND),     "not found");
  EXPECT_STREQ(fpgaErrStr(FPGA_NO_MEMORY),     "no memory");
  EXPECT_STREQ(fpgaErrStr(FPGA_NOT_SUPPORTED), "not supported");
  EXPECT_STREQ(fpgaErrStr(FPGA_NO_DRIVER),     "no driver available");
  EXPECT_STREQ(fpgaErrStr(FPGA_NO_DAEMON),     "no fpga daemon running");
  EXPECT_STREQ(fpgaErrStr(FPGA_NO_ACCESS),     "insufficient privileges");
  EXPECT_STREQ(fpgaErrStr(FPGA_RECONF_ERROR),  "reconfiguration error");
  EXPECT_STREQ(fpgaErrStr((fpga_result)-1),    "unknown error");
}
