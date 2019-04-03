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
#include <sys/mman.h>
#include <sys/stat.h>

#include "common_utils.h"
#include "gtest/gtest.h"
#include "common_sys.h"

#define FME_SYSFS_PERF "/iperf"
#define PERF_LIST_SIZE 10
#define SYSFS_PATH_MAX 256

#define FME_SYSFS_FME_PERF_CACHE "/iperf/cache"
#define FME_SYSFS_FME_PERF_CACHE_FREEZE "/iperf/cache/freeze"
#define FME_SYSFS_FME_PERF_CACHE_READHIT "/iperf/cache/read_hit"
#define FME_SYSFS_FME_PERF_CACHE_WRITEHIT "/iperf/cache/write_hit"
#define FME_SYSFS_FME_PERF_CACHE_READMISS "/iperf/cache/read_miss"
#define FME_SYSFS_FME_PERF_CACHE_WRITEMISS "/iperf/cache/write_miss"
#define FME_SYSFS_FME_PERF_CACHE_HOLDREQ "/iperf/cache/hold_request"
#define FME_SYSFS_FME_PERF_CACHE_RXEVICTION "/iperf/cache/rx_eviction"
#define FME_SYSFS_FME_PERF_CACHE_RXREQSTALL "/iperf/cache/rx_req_stall"
#define FME_SYSFS_FME_PERF_CACHE_TAGWRITE \
  "/iperf/cache/tag_write_port_contention"
#define FME_SYSFS_FME_PERF_CACHE_TXREQSTALL "/iperf/cache/tx_req_stall"
#define FME_SYSFS_FME_PERF_CACHE_DATAWRITE \
  "/iperf/cache/data_write_port_contention"

#define FME_SYSFS_FME_PERF_FABRIC "/iperf/fabric"
#define FME_SYSFS_FME_PERF_FABRIC_FREEZE "/iperf/fabric/freeze"
#define FME_SYSFS_FME_PERF_FABRIC_ENABLE "/iperf/fabric/enable"
#define FME_SYSFS_FME_PERF_FABRIC_PCIE0READ "/iperf/fabric/pcie0_read"
#define FME_SYSFS_FME_PERF_FABRIC_PCIE0WRITE "/iperf/fabric/pcie0_write"
#define FME_SYSFS_FME_PERF_FABRIC_PCIE1READ "/iperf/fabric/pcie1_read"
#define FME_SYSFS_FME_PERF_FABRIC_PCIE1WRITE "/iperf/fabric/pcie1_write"
#define FME_SYSFS_FME_PERF_FABRIC_UPIREAD "/iperf/fabric/upi_read"
#define FME_SYSFS_FME_PERF_FABRIC_UPIWRITE "/iperf/fabric/upi_write"
#define FME_SYSFS_FME_PERF_FABRIC_PORT0_ENABLE "/iperf/fabric/port0/enable"
#define FME_SYSFS_FME_PERF_FABRIC_PORT0_PCIE0READ \
  "/iperf/fabric/port0/pcie0_read"
#define FME_SYSFS_FME_PERF_FABRIC_PORT0_PCIE0WRITE \
  "/iperf/fabric/port0/pcie0_write"
#define FME_SYSFS_FME_PERF_FABRIC_PORT0_PCIE1READ \
  "/iperf/fabric/port0/pcie1_read"
#define FME_SYSFS_FME_PERF_FABRIC_PORT0_PCIE1WRITE \
  "/iperf/fabric/port0/pcie1_write"
#define FME_SYSFS_FME_PERF_FABRIC_PORT0_UPIREAD "/iperf/fabric/port0/upi_read"
#define FME_SYSFS_FME_PERF_FABRIC_PORT0_UPIWRITE "/iperf/fabric/port0/upi_write"
#define FME_SYSFS_FME_PERF_FABRIC_PORT1_ENABLE "/iperf/fabric/port1/enable"
#define FME_SYSFS_FME_PERF_FABRIC_PORT1_PCIE0READ \
  "/iperf/fabric/port1/pcie0_read"
#define FME_SYSFS_FME_PERF_FABRIC_PORT1_PCIE0WRITE \
  "/iperf/fabric/port1/pcie0_write"
#define FME_SYSFS_FME_PERF_FABRIC_PORT1_PCIE1READ \
  "/iperf/fabric/port1/pcie1_read"
#define FME_SYSFS_FME_PERF_FABRIC_PORT1_PCIE1WRITE \
  "/iperf/fabric/port1/pcie1_write"
#define FME_SYSFS_FME_PERF_FABRIC_PORT1_UPIREAD "/iperf/fabric/port1/upi_read"
#define FME_SYSFS_FME_PERF_FABRIC_PORT1_UPIWRITE "/iperf/fabric/port1/upi_write"

using namespace std;
using namespace common_utils;

class StressLibopaecPerfFCommonHW : public BaseFixture, public ::testing::Test {
 public:
  enum Perf_Counter_Type {
    READHIT = 0,
    WRITEHIT,
    READMISS,
    WRITEMISS,
    PCIE0READ,
    PCIE0WRITE,
    PCIE1READ,
    PCIE1WRITE,
    UPIREAD,
    UPIWRITE
  };

  struct perf_counter {
    Perf_Counter_Type key;
    unsigned long value;
  };

  uint8_t m_socketNumber;

  fpga_result get_perf_counter_value(struct perf_counter* const counterList) {
    unsigned long value = 0;
    struct perf_counter* listPtr = NULL;
    fpga_result result = FPGA_OK;

    listPtr = counterList;

    // Verify PERF attributes are supported in SYSFS
    EXPECT_TRUE(feature_is_supported(FME_SYSFS_PERF, tokens[index]));

    // Freeze
    result = write_sysfs_value(FME_SYSFS_FME_PERF_CACHE_FREEZE, 1,
                               tokens[index], DEC);
    EXPECT_EQ(FPGA_OK, result);

    // Freeze
    result = write_sysfs_value(FME_SYSFS_FME_PERF_FABRIC_FREEZE, 1,
                               tokens[index], DEC);
    EXPECT_EQ(FPGA_OK, result);

    result = read_sysfs_value(FME_SYSFS_FME_PERF_CACHE_READHIT, &value,
                              tokens[index]);
    EXPECT_EQ(FPGA_OK, result);
    printf("\t FPGA read_hit: %lu \n", value);
    listPtr->key = READHIT;
    listPtr->value = value;

    value = 0;
    listPtr++;
    result = read_sysfs_value(FME_SYSFS_FME_PERF_CACHE_WRITEHIT, &value,
                              tokens[index]);

    EXPECT_EQ(FPGA_OK, result);
    listPtr->key = WRITEHIT;
    listPtr->value = value;
    printf("\t FPGA write_hit: %lu \n", value);

    value = 0;
    listPtr++;
    result = read_sysfs_value(FME_SYSFS_FME_PERF_CACHE_READMISS, &value,
                              tokens[index]);

    EXPECT_EQ(FPGA_OK, result);
    listPtr->key = READMISS;
    listPtr->value = value;
    printf("\t FPGA read_miss: %lu \n", value);

    value = 0;
    listPtr++;
    result = read_sysfs_value(FME_SYSFS_FME_PERF_CACHE_WRITEMISS, &value,
                              tokens[index]);
    EXPECT_EQ(FPGA_OK, result);
    listPtr->key = WRITEMISS;
    listPtr->value = value;
    printf("\t FPGA write_miss: %lu \n", value);

    // Verify Fabric Perf attributes are supported in SYSFS
    EXPECT_TRUE(feature_is_supported(FME_SYSFS_FME_PERF_FABRIC, tokens[index]));

    // Enable
    EXPECT_EQ(FPGA_OK,
              result = write_sysfs_value(FME_SYSFS_FME_PERF_FABRIC_ENABLE, 1,
                                         tokens[index], DEC));

    value = 0;
    listPtr++;
    result = read_sysfs_value(FME_SYSFS_FME_PERF_FABRIC_PCIE0READ, &value,
                              tokens[index]);

    listPtr->key = PCIE0READ;
    listPtr->value = value;
    EXPECT_EQ(FPGA_OK, result);
    printf("\t FPGA PCIE0READ: %lu \n", value);

    value = 0;
    listPtr++;
    result = read_sysfs_value(FME_SYSFS_FME_PERF_FABRIC_PCIE0WRITE, &value,
                              tokens[index]);

    listPtr->key = PCIE0WRITE;
    listPtr->value = value;
    EXPECT_EQ(FPGA_OK, result);
    printf("\t FPGA PCIE0WRITE: %lu \n", value);

    value = 0;
    listPtr++;
    result = read_sysfs_value(FME_SYSFS_FME_PERF_FABRIC_PCIE1READ, &value,
                              tokens[index]);

    EXPECT_EQ(FPGA_OK, result);
    listPtr->key = PCIE1READ;
    listPtr->value = value;
    printf("\t FPGA pcie1_read: %lu \n", value);

    value = 0;
    listPtr++;
    result = read_sysfs_value(FME_SYSFS_FME_PERF_FABRIC_PCIE1WRITE, &value,
                              tokens[index]);

    EXPECT_EQ(FPGA_OK, result);
    listPtr->key = PCIE1READ;
    listPtr->value = value;
    printf("\t FPGA pcie1_write: %lu \n", value);

    value = 0;
    listPtr++;
    result = read_sysfs_value(FME_SYSFS_FME_PERF_FABRIC_UPIREAD, &value,
                              tokens[index]);

    EXPECT_EQ(FPGA_OK, result);
    listPtr->key = UPIREAD;
    listPtr->value = value;
    printf("\t FPGA upi_read: %lu \n", value);

    value = 0;
    listPtr++;
    result = read_sysfs_value(FME_SYSFS_FME_PERF_FABRIC_UPIWRITE, &value,
                              tokens[index]);

    EXPECT_EQ(FPGA_OK, result);
    listPtr->key = UPIWRITE;
    listPtr->value = value;
    printf("\t FPGA upi_write: %lu \n", value);

    // unFreeze
    EXPECT_EQ(FPGA_OK,
              result = write_sysfs_value(FME_SYSFS_FME_PERF_FABRIC_FREEZE, 0,
                                         tokens[index], DEC));

    // unfreeze
    EXPECT_EQ(FPGA_OK,
              result = write_sysfs_value(FME_SYSFS_FME_PERF_CACHE_FREEZE, 0,
                                         tokens[index], DEC));

    return result;
  }

  // return true if the values have changed
  static bool compare_perf_counter(struct perf_counter* const counterList0,
                                   struct perf_counter* const counterList1) {
    bool ret = false;
    struct perf_counter* listPrt0;
    struct perf_counter* listPrt1;

    listPrt0 = counterList0;
    listPrt1 = counterList1;

    for (int i = 0; i < PERF_LIST_SIZE; i++) {
      if ((listPrt0->key == listPrt1->key) &&
          (listPrt0->value != listPrt1->value)) {
        ret = true;
        break;
      }

      listPrt0++;
      listPrt1++;
    }

    return ret;
  }

  void perfc01() {
    uint8_t size = PERF_LIST_SIZE;

    struct StressLibopaecPerfFCommonHW::perf_counter counterList0[size];
    memset(&counterList0, 0,
           sizeof(StressLibopaecPerfFCommonHW::perf_counter) * size);

    struct StressLibopaecPerfFCommonHW::perf_counter counterList1[size];
    memset(&counterList1, 0,
           sizeof(StressLibopaecPerfFCommonHW::perf_counter) * size);

    EXPECT_EQ(FPGA_OK, get_perf_counter_value(&counterList0[0]));
  }

  void perfc02() {
    uint8_t size = PERF_LIST_SIZE;

    struct StressLibopaecPerfFCommonHW::perf_counter counterList0[size];
    memset(&counterList0, 0,
           sizeof(StressLibopaecPerfFCommonHW::perf_counter) * size);

    struct StressLibopaecPerfFCommonHW::perf_counter counterList1[size];
    memset(&counterList1, 0,
           sizeof(StressLibopaecPerfFCommonHW::perf_counter) * size);

    EXPECT_EQ(FPGA_OK, get_perf_counter_value(&counterList0[0]));

    EXPECT_EQ(0, exerciseNLB0Function(tokens[index]));

    EXPECT_EQ(FPGA_OK, get_perf_counter_value(&counterList1[0]));

    EXPECT_TRUE(StressLibopaecPerfFCommonHW::compare_perf_counter(
        &counterList0[0], &counterList1[0]));
  }
};

/*
 * @test       01
 *
 * @brief      Null test:  Get performance counters.  Do nothing. Get
 *             them again.  They should not change.
 *
 */
TEST_F(StressLibopaecPerfFCommonHW, 01) {
  SCOPED_TRACE("StressLibopaecPerfFCommonHW 01");

  auto functor = [=]() -> void { perfc01(); };

  TestAllFPGA(FPGA_DEVICE,  // object type
              true,         // reconfig default NLB0
              functor);     // test code
}

/**
 * @test       02
 *
 * @brief      Get performance counters.  Run any NLB test. Get the
 *             performance counters again.  If they changed, then the
 *             SW is accessing the HW. Test Passes.
 */

TEST_F(StressLibopaecPerfFCommonHW, 02) {
  SCOPED_TRACE("StressLibopaecPerfFCommonHW 02");

  auto functor = [=]() -> void { perfc02(); };

  // pass test code to enumerator
  TestAllFPGA(FPGA_DEVICE,  // object type
              true,         // reconfig default NLB0
              functor);     // test code
}

/**
 * @test       03
 *
 * @brief      Randomly select between test 01 and test 02. Repeat this
 *             sequence of random executions 1,000 times. What
 *             key/values do we receive?
 */
TEST_F(StressLibopaecPerfFCommonHW, 03) {
  SCOPED_TRACE("StressLibopaecPerfFCommonHW 03");
  // randomly select and call test 01/02 N (default to 1000) times

  auto functor = [=]() -> void {

    common_utils::Random<1, 2> rand;

    for (int i = 0; i < 1000; ++i) {
      if (rand() % 2) {
        printf("calling perfc01 \n");
        perfc01();
      }

      else {
        perfc02();
        printf("calling perfc02 \n");
      }
    }
  };

  // pass test code to enumerator
  TestAllFPGA(FPGA_DEVICE,  // object type
              true,         // reconfig default NLB0
              functor);     // test code
}
