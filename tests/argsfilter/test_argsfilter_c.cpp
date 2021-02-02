// Copyright(c) 2019, Intel Corporation
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

#include <opae/fpga.h>

extern "C" {
int set_properties_from_args(fpga_properties filter, fpga_result *result,
                             int *argc, char *argv[]);
}

#include "gtest/gtest.h"
#include "mock/test_system.h"

using namespace opae::testing;

class argsfilter_c_p : public ::testing::TestWithParam<std::string> {
    protected:
        argsfilter_c_p() {}

        virtual void SetUp() override 
        {
            std::string platform_key = GetParam();
            ASSERT_TRUE(test_platform::exists(platform_key));
            platform_ = test_platform::get(platform_key);
            system_ = test_system::instance();
            system_->initialize();
            system_->prepare_syfs(platform_);

            EXPECT_EQ(fpgaInitialize(nullptr), FPGA_OK);

            optind = 0;
        }

        virtual void TearDown() override {
            fpgaFinalize();
            system_->finalize();
        }

        test_platform platform_;
        test_system *system_;
};

/**
 * @test       bdf
 * @brief      Test: set_properties_from_args
 * @details    When passed with valid arguments for bdf, the function <br>
 *             returns 0. <br>
 */
TEST_P(argsfilter_c_p, bdf) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char four[20];
    char five[20];
    char six[20];
    char seven[20];
    char eight[20];
    char nine[20];
    char *argv[] = { zero, one, two, three, four, five,
                     six, seven, eight, nine };
    int argc = 10;
    fpga_result result;
    char bus[20];
    char device[20];
    char function[20];
    char segment[20];

    sprintf(bus, "0x%x", platform_.devices[0].bus);
    sprintf(device, "0x%x", platform_.devices[0].device);
    sprintf(function, "0x%x", platform_.devices[0].function);
    sprintf(segment, "0x%x", platform_.devices[0].segment);

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    strcpy(two, "-B");
    strcpy(three, bus);
    strcpy(four, "-D");
    strcpy(five, device);
    strcpy(six, "-F");
    strcpy(seven, function);
    strcpy(eight, "-S");
    strcpy(nine, segment);
    EXPECT_EQ(set_properties_from_args(filter, &result, &argc, argv), 0);
    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
    EXPECT_EQ(result, 0);
}

/**
 * @test       bus
 * @brief      Test: set_properties_from_args
 * @details    When passed with valid argument for the bus, <br>
 *             the function returns 0. <br>
 */
TEST_P(argsfilter_c_p, bus) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char *argv[] = { zero, one, two, three };
    int argc = 4;
    fpga_result result;
    char bus[10];

    sprintf(bus, "0x%x", platform_.devices[0].bus);

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    strcpy(two, "-B");
    strcpy(three, bus);
    EXPECT_EQ(set_properties_from_args(filter, &result, &argc, argv), 0);
    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
    EXPECT_EQ(result, 0);
}

/**
 * @test       bus_neg
 * @brief      Test: set_properties_from_args
 * @details    When passed with invalid argument for the bus, <br>
 *             the function returns an error. <br>
 */
TEST_P(argsfilter_c_p, bus_neg) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char *argv[] = { zero, one, two, three };
    int argc = 4;
    fpga_result result;

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    strcpy(two, "-B");
    strcpy(three, "zxyw");
    EXPECT_NE(set_properties_from_args(filter, &result, &argc, argv), 0);
    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       device
 * @brief      Test: set_properties_from_args
 * @details    When passed with valid argument for the device, <br>
 *             the function returns 0. <br>
 */
TEST_P(argsfilter_c_p, device) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char *argv[] = { zero, one, two, three };
    int argc = 4;
    fpga_result result;
    char device[10];

    sprintf(device, "0x%x", platform_.devices[0].device);

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    strcpy(two, "-D");
    strcpy(three, device);
    EXPECT_EQ(set_properties_from_args(filter, &result, &argc, argv), 0);
    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
    EXPECT_EQ(result, 0);
}

/**
 * @test       device_neg
 * @brief      Test: set_properties_from_args
 * @details    When passed with invalid argument for the device, <br>
 *             the function returns an error. <br>
 */
TEST_P(argsfilter_c_p, device_neg) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char *argv[] = { zero, one, two, three };
    int argc = 4;
    fpga_result result;

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    strcpy(two, "-D");
    strcpy(three, "zxyw");
    EXPECT_NE(set_properties_from_args(filter, &result, &argc, argv), 0);
    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       function
 * @brief      Test: set_properties_from_args
 * @details    When passed with valid argument for the function, <br>
 *             the function returns 0. <br>
 */
TEST_P(argsfilter_c_p, function) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char *argv[] = { zero, one, two, three };
    int argc = 4;
    fpga_result result;
    char function[10];

    sprintf(function, "0x%x", platform_.devices[0].function);

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    strcpy(two, "-F");
    strcpy(three, function);
    EXPECT_EQ(set_properties_from_args(filter, &result, &argc, argv), 0);
    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
    EXPECT_EQ(result, 0);
}

/**
 * @test       function_neg
 * @brief      Test: set_properties_from_args
 * @details    When passed with invalid argument for the function, <br>
 *             the function returns an error. <br>
 */
TEST_P(argsfilter_c_p, function_neg) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char *argv[] = { zero, one, two, three };
    int argc = 4;
    fpga_result result;

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    strcpy(two, "-F");
    strcpy(three, "zxyw");
    EXPECT_NE(set_properties_from_args(filter, &result, &argc, argv), 0);
    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       segment
 * @brief      Test: set_properties_from_args
 * @details    When passed with valid argument for the segment, <br>
 *             the function returns 0. <br>
 */
TEST_P(argsfilter_c_p, segment) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char *argv[] = { zero, one, two, three };
    int argc = 4;
    fpga_result result;
    char segment[20];

    sprintf(segment, "0x%x", platform_.devices[0].segment);

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    strcpy(two, "--segment");
    strcpy(three, segment);
    EXPECT_EQ(set_properties_from_args(filter, &result, &argc, argv), 0);
    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
    EXPECT_EQ(result, 0);
}

/**
 * @test       segment_neg
 * @brief      Test: set_properties_from_args
 * @details    When passed an invalid argument for the segment, <br>
 *             the function returns non-zero. <br>
 */
TEST_P(argsfilter_c_p, segment_neg) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char *argv[] = { zero, one, two, three };
    int argc = 4;
    fpga_result result;

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    strcpy(two, "--segment");
    strcpy(three, "zxyw");
    EXPECT_NE(set_properties_from_args(filter, &result, &argc, argv), 0);
    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       argsfilter_neg
 * @brief      Test: set_properties_from_args
 * @details    When missing an argument for the bus, <br>
 *             the function returns an error. <br>
 */
TEST_P(argsfilter_c_p, argsfilter_neg) {
    char zero[20];
    char one[20];
    char two[20];
    char *argv[] = { zero, one, two };
    int argc = 3;
    fpga_result result;

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    strcpy(two, "-B");
    EXPECT_NE(set_properties_from_args(filter, &result, &argc, argv), 0);
    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
}

/**
 * @test       addr_sbdf
 * @brief      Test: set_properties_from_args
 * @details    When the arguments to the function contain<br>
 *             a properly formatted ssss:bb:dd.f, then the<br>
 *             properties are set appropriately.
 */
TEST_P(argsfilter_c_p, addr_sbdf) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char four[20];
    char five[20];
    char six[20];
    char seven[20];
    char eight[20];
    char nine[20];
    char ten[20];
    char *argv[] = { zero, one, two, three,
                     four, five, six, seven,
                     eight, nine, ten };
    int argc = 11;
    fpga_result result;

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    strcpy(two, "-S");
    strcpy(three, "0xff");
    strcpy(four, "-B");
    strcpy(five, "0xff");
    strcpy(six, "-D");
    strcpy(seven, "0xff");
    strcpy(eight, "-F");
    strcpy(nine, "7");
    strcpy(ten, "1234:56:78.7");

    EXPECT_EQ(set_properties_from_args(filter, &result, &argc, argv), 0);

    uint16_t segment = 0;
    uint8_t bus = 0;
    uint8_t device = 0;
    uint8_t function = 0;

    EXPECT_EQ(fpgaPropertiesGetSegment(filter, &segment), FPGA_OK);
    EXPECT_EQ(segment, 0x1234);

    EXPECT_EQ(fpgaPropertiesGetBus(filter, &bus), FPGA_OK);
    EXPECT_EQ(bus, 0x56);

    EXPECT_EQ(fpgaPropertiesGetDevice(filter, &device), FPGA_OK);
    EXPECT_EQ(device, 0x78);

    EXPECT_EQ(fpgaPropertiesGetFunction(filter, &function), FPGA_OK);
    EXPECT_EQ(function, 7);

    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
    EXPECT_EQ(result, 0);
}

/**
 * @test       addr_bdf
 * @brief      Test: set_properties_from_args
 * @details    When the arguments to the function contain<br>
 *             a properly formatted bb:dd.f, then the<br>
 *             properties are set appropriately. The segment<br>
 *             field is set to 0.
 */
TEST_P(argsfilter_c_p, addr_bdf) {
    char zero[20];
    char one[20];
    char two[20];
    char three[20];
    char four[20];
    char five[20];
    char six[20];
    char seven[20];
    char eight[20];
    char nine[20];
    char ten[20];
    char *argv[] = { zero, one, two, three,
                     four, five, six, seven,
                     eight, nine, ten };
    int argc = 11;
    fpga_result result;

    fpga_properties filter = NULL;
    ASSERT_EQ(fpgaGetProperties(NULL, &filter), FPGA_OK);

    strcpy(zero, "fpgainfo");
    strcpy(one, "fme");
    strcpy(two, "-S");
    strcpy(three, "0xff");
    strcpy(four, "-B");
    strcpy(five, "0xff");
    strcpy(six, "-D");
    strcpy(seven, "0xff");
    strcpy(eight, "-F");
    strcpy(nine, "7");
    strcpy(ten, "56:78.7");

    EXPECT_EQ(set_properties_from_args(filter, &result, &argc, argv), 0);

    uint16_t segment = 0xff;
    uint8_t bus = 0;
    uint8_t device = 0;
    uint8_t function = 0;

    EXPECT_EQ(fpgaPropertiesGetSegment(filter, &segment), FPGA_OK);
    EXPECT_EQ(segment, 0);

    EXPECT_EQ(fpgaPropertiesGetBus(filter, &bus), FPGA_OK);
    EXPECT_EQ(bus, 0x56);

    EXPECT_EQ(fpgaPropertiesGetDevice(filter, &device), FPGA_OK);
    EXPECT_EQ(device, 0x78);

    EXPECT_EQ(fpgaPropertiesGetFunction(filter, &function), FPGA_OK);
    EXPECT_EQ(function, 7);

    EXPECT_EQ(fpgaDestroyProperties(&filter), FPGA_OK);
    EXPECT_EQ(result, 0);
}

INSTANTIATE_TEST_CASE_P(argsfilter_c, argsfilter_c_p,
        ::testing::ValuesIn(test_platform::platforms()));
