// Copyright(c) 2017-2018, Intel Corporation
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

#include <opae/access.h>
#include <opae/mmio.h>
#include <opae/properties.h>
#include <opae/types_enum.h>
#include <sys/mman.h>

#include "common_sys.h"
#include "common_utils.h"
#include "gtest/gtest.h"

#define FLAGS 0

using namespace common_utils;
using namespace std;

class StressLibopaecPRFCommonHW : public BaseFixture, public ::testing::Test {};

/**
  * @test       01
  *
  * @brief      In one application process, reconfigure a valid green
  *             NLB0 bitstream and run NLB0/fpgadiag to demonstrate it
  *             works. Reconfigure another valid green NLB0 bitstream
  *             and run NLB0/fpgadiag to demonstrate that one works as
  *             well.
  */

TEST_F(StressLibopaecPRFCommonHW, 01) {
  auto functor = [=]() -> void {

    sayHello(tokens[index]);

    fpga_result result = FPGA_OK;

    checkReturnCodes(
        result = loadBitstream(config_map[BITSTREAM_MODE0], tokens[index]),
        LINE(__LINE__));
    ASSERT_EQ(FPGA_OK, result);

    sayHello(tokens[index]);
  };

  // don't specify guid since we don't use it when calling nlb apps
  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}

/**
 * @test       02
 *
 * @brief      In one application process, reconfigure a valid green
 *             NLB bitstream and run NLB/fpgadiag to demonstrate it
 *             works.  In the first (or yet another) application
 *             process, reconfigure a different valid green NLB
 *             bitstream.  In a final application process, run
 *             NLB/fpgadiag to demonstrate that one works as well.
 */
TEST_F(StressLibopaecPRFCommonHW, 02) {
  auto functor = [=]() -> void {

    // in first application process, reconfigure to NLB0
    // run NLB0 to demonstrate it works.
    EXPECT_EQ(0, exerciseNLB0Function(tokens[index]));

    // in the first application process
    // reconfigure a different valid green NLB bitstream (bistream3)

    ASSERT_TRUE(checkReturnCodes(
        loadBitstream(config_map[BITSTREAM_MODE3], tokens[index]),
        LINE(__LINE__)));

    // in a final application process, run NLB/fpgadiag to
    // demonstrate that one works, too
    EXPECT_EQ(exerciseNLB3Function(tokens[index]), 0);
  };

  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}

/**
 * @test       06
 *
 * @brief      Validate Json metadata, reconfig the FPGA with GBS file
 *             without any issue.   (Use PR API to get metadata
 *             information from GBS file)
 */
TEST_F(StressLibopaecPRFCommonHW, 06) {
  auto functor = [=]() -> void {

    gbs_metadata mdata = {};
    uint8_t* bitbuffer = NULL;
    fpga_result result = FPGA_OK;

    fillBSBuffer(config_map[BITSTREAM_MODE0], &bitbuffer);
    assert(bitbuffer);

    fpga_handle h = NULL;

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      checkReturnCodes(result = read_gbs_metadata(bitbuffer, &mdata),
                       LINE(__LINE__));
      EXPECT_EQ(FPGA_OK, result);
      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      FAIL();
    }

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      checkReturnCodes(result = validate_bitstream_metadata(h, bitbuffer),
                       LINE(__LINE__));
      EXPECT_EQ(FPGA_OK, result);
      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      FAIL();
    }

    free(bitbuffer);
  };

  TestAllFPGA(FPGA_DEVICE,  // object type
              true,         // reconfig default NLB0
              functor);     // test code
}

/**
 * @test       07
 *
 * @brief      Use invalid afc-type-uuid in GBS, verify PR API to catch
 *             the Guid error, and no bitstream shall be loaded.
 */
TEST_F(StressLibopaecPRFCommonHW, 07) {
  auto functor = [=]() -> void {

    gbs_metadata mdata = {};
    fpga_result result = FPGA_OK;
    uint8_t* bitbuffer = NULL;

    fillBSBuffer(config_map[BITSTREAM_PR07], &bitbuffer);
    assert(bitbuffer);

    fpga_handle h = NULL;

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      checkReturnCodes(result = read_gbs_metadata(bitbuffer, &mdata),
                       LINE(__LINE__));
      EXPECT_EQ(FPGA_OK, result) << "read_gbs_metadata failed." << endl;
      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      ASSERT_EQ(FPGA_OK, fpgaDestroyToken(&tokens[index]));

      FAIL();
    }

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      checkReturnCodes(result = validate_bitstream_metadata(h, bitbuffer),
                       LINE(__LINE__));
      EXPECT_NE(FPGA_OK, result) << "Corruption in metadata not detected!!!"
                                 << endl;
      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
    }

    free(bitbuffer);
  };

  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}

/**
 * @test       08
 *
 * @brief      Set invalid metadata length in GBS header, verify PR API
 *             to catch the error, and no bitstream shall be loaded.
 */

TEST_F(StressLibopaecPRFCommonHW, 08) {
  auto functor = [=]() -> void {

    gbs_metadata mdata = {};
    uint8_t* bitbuffer = NULL;
    fpga_result result = FPGA_OK;

    fillBSBuffer(config_map[BITSTREAM_PR08], &bitbuffer);
    assert(bitbuffer);

    fpga_handle h = NULL;

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      checkReturnCodes(result = read_gbs_metadata(bitbuffer, &mdata),
                       LINE(__LINE__));
      EXPECT_EQ(FPGA_OK, result) << "read_gbs_metadata failed." << endl;
      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      ASSERT_EQ(FPGA_OK, fpgaDestroyToken(&tokens[index]));
      FAIL();
    }

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      checkReturnCodes(result = validate_bitstream_metadata(h, bitbuffer),
                       LINE(__LINE__));
      EXPECT_NE(FPGA_OK, result) << "Corruption in metadata not detected!!!"
                                 << endl;
      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      FAIL();
    }

    free(bitbuffer);
  };

  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}

/**
 * @test       09
 *
 * @brief      Set invalid Json metadata other fields (except Guid &
 *             metedata length), verify PR API to catch the error, and
 *             no bitstream shall be loaded.
 *
 *             @internal   Current implementation is an invalid magic
 *             number.
 */

TEST_F(StressLibopaecPRFCommonHW, 09) {
  auto functor = [=]() -> void {

    gbs_metadata mdata = {};
    uint8_t* bitbuffer = NULL;
    fpga_result result = FPGA_OK;

    fillBSBuffer(config_map[BITSTREAM_PR09], &bitbuffer);
    assert(bitbuffer);

    fpga_handle h = NULL;

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      checkReturnCodes(result = read_gbs_metadata(bitbuffer, &mdata),
                       LINE(__LINE__));
      EXPECT_EQ(FPGA_OK, result) << "read_gbs_metadata failed." << endl;
      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      FAIL();
    }

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      checkReturnCodes(result = validate_bitstream_metadata(h, bitbuffer),
                       LINE(__LINE__));
      EXPECT_NE(FPGA_OK, result) << "Corruption in metadata not detected!!!"
                                 << endl;
      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      FAIL();
    }

    free(bitbuffer);
  };

  TestAllFPGA(FPGA_DEVICE,  // object type
              true,         // reconfig default NLB0
              functor);     // test code
}

/**
 * @test       10
 *
 * @brief      Set invalid interface-uuid in GBS header, verify PR API
 *             to catch the error, and no bitstream shall be loaded.
 */
TEST_F(StressLibopaecPRFCommonHW, 10) {
  auto functor = [=]() -> void {

    gbs_metadata mdata = {};
    uint8_t* bitbuffer = NULL;
    fpga_result result = FPGA_OK;

    fillBSBuffer(config_map[BITSTREAM_PR10], &bitbuffer);
    assert(bitbuffer);

    fpga_handle h = NULL;

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      checkReturnCodes(result = read_gbs_metadata(bitbuffer, &mdata),
                       LINE(__LINE__));
      ASSERT_EQ(FPGA_OK, result) << "read_gbs_metadata failed." << endl;
      EXPECT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      FAIL();
    }

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      checkReturnCodes(result = validate_bitstream_metadata(h, bitbuffer),
                       LINE(__LINE__));
      EXPECT_NE(FPGA_OK, result) << "Corruption in metadata not detected!!!"
                                 << endl;
      ASSERT_TRUE(checkReturnCodes(fpgaClose(h), LINE(__LINE__)));
    }

    else {
      cout << "open failed\n";
      FAIL();
    }

    free(bitbuffer);
  };

  TestAllFPGA(FPGA_DEVICE,  // object type
              true,         // reconfig default NLB0
              functor);     // test code
}

/**
 * @test       11
 *
 * @brief      Set power threshold in Json configuration file, read
 *             from sysFS file. Verify 2 values are equal.
 */
TEST_F(StressLibopaecPRFCommonHW, 11) {
  auto functor = [=]() -> void {

    const uint64_t BBS_IDLE = 30;

    fpga_result result = FPGA_OK;

    uint64_t power = 50;
    uint64_t read_power = power + BBS_IDLE;

    // build path array
    char path[SYSFS_PATH_MAX] = {0};

    strcat_s(path, sizeof(path), ((_fpga_token*)tokens[index])->sysfspath);
    strcat_s(path, sizeof(path), "/");
    strcat_s(path, sizeof(path), FME_SYSFS_POWER_MGMT_THRESHOLD1);
    cout << path << endl;
    // end build path

    uint64_t value = 0;
    result = sysfs_read_64(path, &value);
    EXPECT_TRUE(result == 0);
    printf("\t FPGA threshold1: %ld \n", value);
    EXPECT_EQ(read_power, value);

    memset(path, 0, sizeof(path));

    strcat_s(path, sizeof(path), ((_fpga_token*)tokens[index])->sysfspath);
    strcat_s(path, sizeof(path), "/");
    strcat_s(path, sizeof(path), FME_SYSFS_POWER_MGMT_THRESHOLD2);
    cout << path << endl;

    value = 0;
    result = sysfs_read_64(path, &value);
    EXPECT_EQ(0, result);
    printf("\t FPGA threshold2: %ld \n", value);
    EXPECT_EQ(1, value);  // threshold2 is ignored by BBS
  };

  TestAllFPGA(FPGA_DEVICE,  // object type
              true,         // reconfig default NLB0
              functor);     // test code
}

/**
 * @test       16
 *
 * @brief      Use PR API to set MCP power threshold, use sysFS to
 *             verify the updated value.
 */
/*
TEST_F(StressLibopaecPRFCommonHW, 16) {
  auto functor = [=]() -> void {

    const uint64_t BBS_IDLE = 30;

    uint64_t power = 10;
    uint64_t read_power = power + BBS_IDLE;

    fpga_handle h = NULL;
    fpga_result result = FPGA_OK;

    if (checkReturnCodes(fpgaOpen(tokens[index], &h, 0), LINE(__LINE__))) {
      // set the power threshold
      checkReturnCodes(result = set_fpga_pwr_threshold(h, power),
                       LINE(__LINE__));
      EXPECT_EQ(FPGA_OK, result) << "Failed to set power!!!" << endl;
      ASSERT_EQ(FPGA_OK, fpgaClose(h));
    }

    else {
      cout << "open failed" << endl;
      FAIL();
    }

    // build path array
    char path[SYSFS_PATH_MAX] = {0};

    strcat_s(path, sizeof(path), ((_fpga_token*)tokens[index])->sysfspath);
    strcat_s(path, sizeof(path), "/");
    strcat_s(path, sizeof(path), FME_SYSFS_POWER_MGMT_THRESHOLD1);
    // end build path

    uint64_t value = 0;
    result = sysfs_read_64(path, &value);
    EXPECT_TRUE(result == 0);
    printf("\t FPGA threshold1: %ld \n", value);
    EXPECT_EQ(read_power, value);

    memset(path, 0, sizeof(path));

    strcat_s(path, sizeof(path), ((_fpga_token*)tokens[index])->sysfspath);
    strcat_s(path, sizeof(path), "/");
    strcat_s(path, sizeof(path), FME_SYSFS_POWER_MGMT_THRESHOLD2);

    value = 0;
    result = sysfs_read_64(path, &value);
    EXPECT_TRUE(result == 0);
    printf("\t FPGA threshold2: %ld \n", value);
    EXPECT_EQ(1, value);  // threshold 2 is ignored by BBS

    // set back to 50
    ASSERT_EQ(FPGA_OK, fpgaOpen(tokens[index], &h, 0));
    EXPECT_EQ(FPGA_OK, set_fpga_pwr_threshold(h, 50))
        << "Failed to set power!!!" << endl;
    ASSERT_EQ(FPGA_OK, fpgaClose(h));
  };

  TestAllFPGA(FPGA_DEVICE,  // object type
              true,         // reconfig default NLB0
              functor);     // test code
}
*/

/**
  * @test       17
  *
  * @brief      Set user clock, read back from sysFS to find the updated
  *             value.
  */
/*
TEST_F(StressLibopaecPRFCommonHW, 17) {
  auto functor = [=]() -> void {

    fpga_handle h = NULL;
    fpga_result result = FPGA_OK;

    if (checkReturnCodes(result = fpgaOpen(tokens[index], &h, 0),
                         LINE(__LINE__))) {
      uint64_t MIN = 100;  // frequency in hertz
      uint64_t MAX = 200;  // frequency in hertz

      checkReturnCodes(result = set_afu_userclock(h, MAX, MIN), LINE(__LINE__));
      ASSERT_EQ(FPGA_OK, result);

      result = fpgaClose(h);
      ASSERT_EQ(FPGA_OK, result);
    }

    else {
      cout << "open failed" << endl;
      FAIL();
    }
  };

  TestAllFPGA(FPGA_DEVICE,  // object type
              true,         // reconfig default NLB0
              functor);     // test code
}
*/
/**
  * @test       18
  *
  * @brief      Load a corrupt bitstream to cause a PR error through
  *             ioctl. Verify that the PR API returns
  *             FPGA_RECONF_ERROR.
  */
TEST_F(StressLibopaecPRFCommonHW, 18) {
  auto functor = [=]() -> void {

    fpga_result result = FPGA_OK;

    while (!checkReturnCodes(
        result = loadBitstream(config_map[BITSTREAM_MODE0], tokens[index]),
        LINE(__LINE__))) {
      if (FPGA_BUSY == result) {
        sleep(10);
      }

      else {
        break;
      }
    }

    ASSERT_FALSE(checkReturnCodes(
        result = loadBitstream(config_map[BITSTREAM_PR18], tokens[index]),
        LINE(__LINE__)));

    EXPECT_EQ(FPGA_RECONF_ERROR, result);

    // reset to valid bitstream
    while (!checkReturnCodes(
        result = loadBitstream(config_map[BITSTREAM_MODE0], tokens[index]),
        LINE(__LINE__))) {
      if (FPGA_BUSY == result) {
        sleep(10);
      }

      else {
        break;
      }
    }
  };

  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              false,             // don't load bitstream to avoid fpga_busy hang
              functor);
}

/**
  * @test       19
  *
  * @brief      Test NLB mode 0 function in one process, use another
  *             process to update bitstream.  PR session shall not
  *             fail.
  */
TEST_F(StressLibopaecPRFCommonHW, 19) {
  auto functor = [=]() -> void {

    // runs in a separate process
    EXPECT_EQ(0, exerciseNLB0Function(tokens[index]));

    // reconfigure a different valid green NLB bitstream (bistream0)
    // in another process
    ASSERT_TRUE(checkReturnCodes(
        loadBitstream(config_map[BITSTREAM_MODE0], tokens[index]),
        LINE(__LINE__)));
  };

  TestAllFPGA(FPGA_ACCELERATOR,  // object type
              true,              // reconfig default NLB0
              functor);          // test code
}

