// Original work Copyright(c) 2019-2020, Intel Corporation
// Modifications Copyright(c) 2021, Silicom Denmark A/S
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

#include <opae/fpga.h>

extern "C" {

#include <dirent.h>
#include <linux/perf_event.h>
#include <glob.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define EVENT_FILTER  {(char *)"clock", (char *)"fab_port_mmio_read", (char *)"fab_port_mmio_write", (char *)"fab_port_pcie0_read", (char *)"fab_port_pcie0_write"}

#define DFL_PERF_STR_MAX        256

#define DFL_BUFSIZ_MAX  512

/*initilaize the perf_counter structureand get the dfl_fme device*/
fpga_result fpgaperfcounterinit(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function);

/* Dynamically enumerate sysfs path and get the device type, cpumask, format and generic events
Reset the counter to 0 and enable the counters to get workload instructions */
fpga_result fpgaperfcounterstart(void);

/* Stops performance counter and get the counters values */
fpga_result fpgaperfcounterstop(void);

/* parse the each format and get the shift val */
fpga_result prepare_format(DIR *dir, char *dir_name);

/* parse the evnts for the perticular device directory */
fpga_result prepare_event_mask(DIR *dir, char **filter, int filter_size, char *dir_name);

/* read the performance counter values */
fpga_result read_fab_counters(int val);

}

#include "intel-fpga.h"
#include <linux/ioctl.h>

#include <cstdlib>
#include <string>
#include "gtest/gtest.h"
#include "mock/test_system.h"


using namespace opae::testing;

class fpgaperf_counter_c_p : public ::testing::TestWithParam<std::string> {
protected:
        fpgaperf_counter_c_p() {}

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
* @test       fpgaperf_0
* @brief      Tests: fpgaperfcounterinit
* @details    Validates the dfl-fme device path based on pci address  <br>
*/
TEST_P(fpgaperf_counter_c_p, fpgaperf_0) {

	uint16_t segment=0;
	uint8_t bus=0;
	uint8_t device=0;
	uint8_t function=0;
	
	segment = platform_.devices[0].segment;
	bus = platform_.devices[0].bus;
	device = platform_.devices[0].device;
	function = platform_.devices[0].function;
	
	EXPECT_EQ(fpgaperfcounterinit(segment, bus, device, function), FPGA_OK);
}

/**
* @test       fpgaperf_1
* @brief      Tests: fpgaperfcounterstart
* @details    Validates counter start  <br>
*/
TEST_P(fpgaperf_counter_c_p, fpgaperf_1) {
	
	EXPECT_EQ(fpgaperfcounterstart(), FPGA_OK);
}

/**
* @test       fpgaperf_2
* @brief      Tests: fpgaperfcounterstop
* @details    Validates fpga perf counter stop  <br>
*/
TEST_P(fpgaperf_counter_c_p, fpgaperf_2) {

	EXPECT_EQ(fpgaperfcounterstop(), FPGA_OK);
}
/**
* @test       fpgaperf_3
* @brief      Tests: read_fab_counters
* @details    Validates fpga perf counter reads  <br>
*/
TEST_P(fpgaperf_counter_c_p, fpgaperf_3) {

	EXPECT_EQ(read_fab_counters(1), FPGA_OK);
}

/**
* @test       fpgaperf_4
* @brief      Tests: prepare_format
* @details    Validates format parse and prepare <br>
*/
TEST_P(fpgaperf_counter_c_p, fpgaperf_4) {
        DIR *format_dir = NULL;

        char dir_name[DFL_PERF_STR_MAX] = "/sys/bus/event_source/devices/dfl_fme0";
	char format_name[DFL_BUFSIZ_MAX] = "";

	snprintf(format_name, sizeof(format_name), "%s/format", dir_name);
        format_dir = opendir(format_name);


	EXPECT_EQ(prepare_format(format_dir, dir_name), FPGA_OK);

	EXPECT_EQ(prepare_format(NULL, dir_name), FPGA_INVALID_PARAM);
	
	EXPECT_EQ(prepare_format(format_dir, NULL), FPGA_INVALID_PARAM);

}

/**
* @test       fpgaperf_5
* @brief      Tests: prepare_event_mask
* @details    Validates events  <br>
*/
TEST_P(fpgaperf_counter_c_p, fpgaperf_5) {	
        DIR *event_dir = NULL;

        char dir_name[DFL_PERF_STR_MAX] = "/sys/bus/event_source/devices/dfl_fme0";
        char event_name[DFL_BUFSIZ_MAX] = "";
	char *event_filter[] =  EVENT_FILTER;
	int size = 0;
	
	size = sizeof(event_filter) / sizeof(event_filter[0]);

        snprintf(event_name, sizeof(event_name), "%s/events", dir_name);
        event_dir = opendir(event_name);

	EXPECT_EQ(prepare_event_mask(event_dir, event_filter, size, dir_name), FPGA_OK);
	EXPECT_EQ(prepare_event_mask(NULL, event_filter, size, dir_name), FPGA_INVALID_PARAM);
	EXPECT_EQ(prepare_event_mask(event_dir, event_filter, size, NULL), FPGA_INVALID_PARAM);

}

INSTANTIATE_TEST_CASE_P(fpgaperf_counter_c, fpgaperf_counter_c_p,
	::testing::ValuesIn(test_platform::hw_platforms({ "dfl-n3000","dfl-d5005" })));

//test invalid attributes
class fpgaperf_counter_invalid_c_p : public fpgaperf_counter_c_p { };

/**
 * * @test       fpgaperf_6
 * * @brief      Tests: fpgaperfcounterinit,
 * * @details    Validates function with invalid attributes <br>
 * */
TEST_P(fpgaperf_counter_invalid_c_p, fpgaperf_6) {

	uint16_t segment=0;
	uint8_t bus=0;
	uint8_t device=0;
	uint8_t function=0;

	segment = platform_.devices[0].segment;
	device = platform_.devices[0].device;
	function = platform_.devices[0].function;

	EXPECT_EQ(fpgaperfcounterinit(segment, bus, device, function), FPGA_INVALID_PARAM);
}

INSTANTIATE_TEST_CASE_P(fpgaperf_counter_invalid_c, fpgaperf_counter_invalid_c_p,
	::testing::ValuesIn(test_platform::hw_platforms({ "dfl-n3000","dfl-d5005" })));
