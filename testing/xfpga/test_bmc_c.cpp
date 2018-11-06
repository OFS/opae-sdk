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
#include "types_int.h"
#include <fcntl.h>
#include <stdlib.h>
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
#include "xfpga.h"
#include "token_list_int.h"
#include <dlfcn.h>
#include "test_utils.h"
#include "safe_string/safe_string.h"
#include "metrics/bmc/bmc.h"
#include "metrics/bmc/bmcdata.h"
#include "metrics/bmc/bmcinfo.h"
#include "metrics/bmc/bmc_ioctl.h"
#include "sysfs_int.h"
using namespace opae::testing;


class bmc_c_p : public ::testing::TestWithParam<std::string> {
protected:
	bmc_c_p()
		: tokens_{ {nullptr, nullptr} },
		handle_(nullptr) {}

	virtual void SetUp() override {

		ASSERT_TRUE(test_platform::exists(GetParam()));
		platform_ = test_platform::get(GetParam());
		system_ = test_system::instance();
		system_->initialize();
		system_->prepare_syfs(platform_);

		ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
		ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
		ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
			&num_matches_),
			FPGA_OK);

		ASSERT_EQ(xfpga_fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);


	}

	virtual void TearDown() override {
		EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
		for (auto &t : tokens_) {
			if (t) {
				EXPECT_EQ(xfpga_fpgaDestroyToken(&t), FPGA_OK);
				t = nullptr;
			}
		}
		if (handle_ != nullptr) { EXPECT_EQ(xfpga_fpgaClose(handle_), FPGA_OK); }
		system_->finalize();
	}

	fpga_result write_sysfs_file(fpga_token token, const char* file, void *buf, size_t count);

	std::array<fpga_token, 2> tokens_;
	fpga_handle handle_;
	fpga_properties filter_;
	uint32_t num_matches_;
	test_platform platform_;
	test_system *system_;

};

fpga_result bmc_c_p ::write_sysfs_file(fpga_token token, const char* file, void *buf, size_t count)
{
	fpga_result res = FPGA_OK;
	char sysfspath[SYSFS_PATH_MAX];
	int fd = 0;

	struct _fpga_token *tok = (struct _fpga_token *)token;
	if (FPGA_TOKEN_MAGIC != tok->magic) {
		return FPGA_INVALID_PARAM;
	}

	snprintf_s_ss(sysfspath, sizeof(sysfspath), "%s/%s", tok->sysfspath, file);

	glob_t pglob;
	int gres = glob(sysfspath, GLOB_NOSORT, NULL, &pglob);
	if ((gres) || (1 != pglob.gl_pathc)) {
		globfree(&pglob);
		return FPGA_NOT_FOUND;
	}
	fd = open(pglob.gl_pathv[0], O_WRONLY);
	globfree(&pglob);
	if (fd < 0) {
		printf("open faild \n");
		return FPGA_NOT_FOUND;
	}

	ssize_t  total_written  = eintr_write(fd, buf, count);
	printf("count %ld \n", count);
	printf("total_written %ld \n", total_written);

	if (total_written == 0) {
		close(fd);
		printf("total_written faild \n");
		return FPGA_INVALID_PARAM;
	}

	close(fd);

	return  res;
}

/**
 * @test       bmc
 * @brief      Tests: bmcGetLastResetCause,
 *....................bmcGetFirmwareVersion
 *....................bmcGetLastPowerdownCause functions
 * @details    Validates bmc reset cause ,power down cause
 *.....................bmc version
 *
 *
 */
TEST_P(bmc_c_p, test_bmc_1) {

	uint32_t version    = 0;
	char *string        = NULL;

	// Get Reset & Power down cause

	EXPECT_EQ(bmcGetLastResetCause(tokens_[0], &string), FPGA_OK);
	if (string) {
		free(string);
		string = NULL;
	}
	EXPECT_NE(bmcGetLastResetCause(tokens_[0], NULL), FPGA_OK);

	EXPECT_EQ(bmcGetLastPowerdownCause(tokens_[0], &string), FPGA_OK);
	if (string) {
		free(string);
		string = NULL;
	}
	EXPECT_NE(bmcGetLastPowerdownCause(tokens_[0],NULL), FPGA_OK);

	// Get firmware version
	EXPECT_EQ(bmcGetFirmwareVersion(tokens_[0], &version), FPGA_OK);
	if (string) {
		free(string);
		string = NULL;
	}
	EXPECT_NE(bmcGetFirmwareVersion(tokens_[0], NULL), FPGA_OK);
}

/**
 * @test       bmc
 * @brief      Tests: bmcLoadSDRs,
 *....................bmcReadSensorValues,
 *....................bmcGetSDRDetails,
 *....................bmcGetSensorReading,
 *....................rawFromDouble,
 *....................bmcDestroySensorValues,
 *....................bmcDestroySDRs functions
 * @details    Validates bmc load SDR and Read sensor values
 *
 *
 *
 */
TEST_P(bmc_c_p, test_bmc_2) {

	bmc_sdr_handle sdrs            = NULL;
	bmc_values_handle values       = NULL;
	uint32_t num_sensors           = 0;
	uint32_t num_values            = 0;
	uint32_t i                     = 0;
	uint32_t is_valid              = 0;
	double tmp                     = 0;
	uint8_t raw                    = 0;
	sdr_details details           = { 0 };

	// Load SDR
	EXPECT_EQ(bmcLoadSDRs(tokens_[0], &sdrs, &num_sensors), FPGA_OK);
	EXPECT_EQ(bmcReadSensorValues(sdrs, &values, &num_values), FPGA_OK);
	
	// Read sensor details & value
	for (i = 0; i < num_sensors; i++) {

		EXPECT_EQ(bmcGetSDRDetails(values, i, &details), FPGA_OK);

		EXPECT_EQ(bmcGetSensorReading(values, i, &is_valid, &tmp), FPGA_OK);
		Values detail = { 0 };
		EXPECT_EQ(rawFromDouble(&detail, tmp, &raw), FPGA_OK);

		detail.result_exp = 2;
		EXPECT_EQ(rawFromDouble(&detail, tmp, &raw), FPGA_OK);

		detail.result_exp = -2;
		EXPECT_EQ(rawFromDouble(&detail, tmp, &raw), FPGA_OK);
	}

	// Destroy Sensor values & SDR
	EXPECT_EQ(bmcDestroySensorValues(&values), FPGA_OK);
	EXPECT_EQ(bmcDestroySDRs(&sdrs), FPGA_OK);
}

/**
 * @test       bmc
 * @brief      Tests: bmcThresholdsTripped,
 *....................bmcDestroyTripped functions
 * @details    Validates bmc threshold trip
 *
 *
 *
 */
TEST_P(bmc_c_p, test_bmc_3) {

	bmc_sdr_handle sdrs            = NULL;
	bmc_values_handle values       = NULL;
	uint32_t num_sensors           = 0;
	uint32_t num_values            = 0;
	uint32_t i                     = 0;
	uint32_t is_valid              = 0;
	tripped_thresholds *tripped    = NULL;
	uint32_t num_tripped           = 0;
	double tmp                     = 0;
	sdr_details details;

	// Load SDR
	EXPECT_EQ(bmcLoadSDRs(tokens_[0], &sdrs, &num_sensors), FPGA_OK);
	EXPECT_EQ(bmcReadSensorValues(sdrs, &values, &num_values),FPGA_OK);

	// Get threshold trip point
	EXPECT_EQ(bmcThresholdsTripped(values, &tripped, &num_tripped), FPGA_OK);
	printf("num_tripped = %d \n", num_tripped);

	struct _bmc_values *vals = (struct _bmc_values *)values;
	for (uint32_t i = 0; i < vals->num_records; i++) {
		vals->contents[i].threshold_events._value = 1;
	}

	// Get threshold trip point
	EXPECT_EQ(bmcThresholdsTripped(values, &tripped, &num_tripped), FPGA_OK);
	printf("num_tripped = %d \n", num_tripped);

	// Destroy Threshold
	EXPECT_EQ(bmcDestroyTripped(tripped), FPGA_OK);

	EXPECT_EQ(bmcDestroySensorValues(&values), FPGA_OK);
	EXPECT_EQ(bmcDestroySDRs(&sdrs), FPGA_OK);
}

/**
 * @test       bmc
 * @brief      Tests: bmcSetHWThresholds,
 *..................._bmcGetThreshold
 *..................._bmcGetThreshold
 *................. .fill_set_request fucntions
 * @details    Validates bmc set and get thresholds
 *
 *
 *
 */
TEST_P(bmc_c_p, test_bmc_4) {

	bmc_sdr_handle sdrs              = NULL;
	bmc_values_handle values         = NULL;
	uint32_t num_sensors             = 0;
	uint32_t num_values              = 0;
	uint32_t i                       = 0;
	uint32_t is_valid                = 0;
	tripped_thresholds *tripped      = NULL;
	uint32_t num_tripped             = 0;
	double tmp                       = 0;
	sdr_details details;

	// Load SDR
	EXPECT_EQ(bmcLoadSDRs(tokens_[0], &sdrs, &num_sensors), FPGA_OK);
	EXPECT_EQ(bmcReadSensorValues(sdrs, &values, &num_values), FPGA_OK);

	threshold_list thresh = { 0 };

	memset_s(&thresh, sizeof(thresh), 0);
	thresh.upper_nr_thresh.is_valid = 1;
	thresh.upper_nr_thresh.value = 20;

	EXPECT_NE(bmcSetHWThresholds(sdrs, 1, &thresh), FPGA_OK);

	// Destroy sensor value and SDR
	EXPECT_EQ(bmcDestroySensorValues(&values), FPGA_OK);
	EXPECT_EQ(bmcDestroySDRs(&sdrs), FPGA_OK);
	

	// Set & Get  threshold
	bmc_get_thresh_response  thres ;
	_bmcGetThreshold(1, 1, &thres);


	bmc_set_thresh_request req ;
	_bmcSetThreshold(1, 1, &req);

	Values vals = { 0 };

	fill_set_request(&vals, &thresh, &req);


	thresh.upper_nr_thresh.is_valid = true;
	thresh.upper_c_thresh.is_valid = true;
	thresh.upper_nc_thresh.is_valid = true;
	thresh.lower_nr_thresh.is_valid = true;
	thresh.lower_c_thresh.is_valid = true;
	thresh.lower_nc_thresh.is_valid = true;

	fill_set_request(&vals, &thresh, &req);

	thresh.upper_nr_thresh.is_valid = false;
	thresh.upper_c_thresh.is_valid = false;
	thresh.upper_nc_thresh.is_valid = false;
	thresh.lower_nr_thresh.is_valid = false;
	thresh.lower_c_thresh.is_valid = false;
	thresh.lower_nc_thresh.is_valid = false;

	fill_set_request(&vals, &thresh, &req);

	thresh.upper_nr_thresh.is_valid = false;
	fill_set_request(&vals, &thresh, &req);
}


/**
 * @test       bmc
 * @brief      Tests: bmcGetLastResetCause
 * @details    Validates reset cause
 *
 *
 *
 */
TEST_P(bmc_c_p, test_bmc_5) {

	uint32_t tot_bytes_ret     = 0 ;
	struct _sdr_content *tmp   = NULL;
	char *string               = NULL;
	void *buf = NULL;

	read_sysfs_file(tokens_[0], (const char*) "/sys/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/",
		(void **)&buf, &tot_bytes_ret);

	// write to reset cause
	reset_cause reset = { 0 };
	reset.completion_code = 1;
	write_sysfs_file(tokens_[0], SYSFS_RESET_FILE, (void*)(&reset), sizeof(reset_cause));
	EXPECT_NE(bmcGetLastResetCause(tokens_[0], &string), FPGA_OK);
	printf("string= %s", string);
	if (string) {
		free(string);
		string = NULL;
	}

	reset.completion_code = 0;
	reset.reset_cause = CHIP_RESET_CAUSE_EXTRST;
	write_sysfs_file(tokens_[0], SYSFS_RESET_FILE, (void*)(&reset), sizeof(reset_cause));
	EXPECT_EQ(bmcGetLastResetCause(tokens_[0], &string), FPGA_OK);
	printf("string= %s", string);
	if (string) {
		free(string);
		string = NULL;
	}

	reset.reset_cause = CHIP_RESET_CAUSE_BOD_IO;
	write_sysfs_file(tokens_[0], SYSFS_RESET_FILE, (void*)(&reset), sizeof(reset_cause));
	EXPECT_EQ(bmcGetLastResetCause(tokens_[0], &string), FPGA_OK);
	printf("string= %s", string);
	if (string) {
		free(string);
		string = NULL;
	}

	reset.reset_cause = CHIP_RESET_CAUSE_OCD;
	write_sysfs_file(tokens_[0], SYSFS_RESET_FILE, (void*)(&reset), sizeof(reset_cause));
	EXPECT_EQ(bmcGetLastResetCause(tokens_[0], &string), FPGA_OK);
	printf("string= %s", string);
	if (string) {
		free(string);
		string = NULL;
	}


	reset.reset_cause = CHIP_RESET_CAUSE_POR;
	write_sysfs_file(tokens_[0], SYSFS_RESET_FILE, (void*)(&reset), sizeof(reset_cause));
	EXPECT_EQ(bmcGetLastResetCause(tokens_[0], &string), FPGA_OK);
	printf("string= %s", string);
	if (string) {
		free(string);
		string = NULL;
	}

	reset.reset_cause = CHIP_RESET_CAUSE_SOFT;
	write_sysfs_file(tokens_[0], SYSFS_RESET_FILE, (void*)(&reset), sizeof(reset_cause));
	EXPECT_EQ(bmcGetLastResetCause(tokens_[0], &string), FPGA_OK);
	printf("string= %s", string);
	if (string) {
		free(string);
		string = NULL;
	}

	reset.reset_cause = CHIP_RESET_CAUSE_SPIKE;
	write_sysfs_file(tokens_[0], SYSFS_RESET_FILE, (void*)(&reset), sizeof(reset_cause));
	EXPECT_EQ(bmcGetLastResetCause(tokens_[0], &string), FPGA_OK);
	printf("string= %s", string);
	if (string) {
		free(string);
		string = NULL;
	}

	reset.reset_cause = CHIP_RESET_CAUSE_WDT;
	write_sysfs_file(tokens_[0], SYSFS_RESET_FILE, (void*)(&reset), sizeof(reset_cause));
	EXPECT_EQ(bmcGetLastResetCause(tokens_[0], &string), FPGA_OK);
	printf("string= %s", string);
	if (string) {
		free(string);
		string = NULL;
	}
}

/**
 * @test       bmc
 * @brief      Tests: bmcGetLastPowerdownCause
 *...................bmcGetFirmwareVersion functions
 * @details    Validates power down cause & FW version
 *
 *
 */
TEST_P(bmc_c_p, test_bmc_6) {

	powerdown_cause reset      = { 0 };
	reset.completion_code      = 1;
	char *string               = NULL;
	device_id dev_id           = { 0 };

	write_sysfs_file(tokens_[0], SYSFS_PWRDN_FILE, (void*)(&reset), sizeof(reset_cause));
	EXPECT_NE(bmcGetLastPowerdownCause(tokens_[0], &string), FPGA_OK);
	printf("string= %s", string);

	dev_id.completion_code = 1;
	uint32_t version;
	write_sysfs_file(tokens_[0], SYSFS_DEVID_FILE, (void*)(&dev_id), sizeof(device_id));
	EXPECT_NE(bmcGetFirmwareVersion(tokens_[0], &version), FPGA_OK);

}

/**
 * @test       bmc
 * @brief      Tests: bmc_build_values
 * @details    Validates build values
 *
 *
 */
TEST_P(bmc_c_p, test_bmc_7) {
	
	sensor_reading reading = { 0 };
	sdr_header header = { 0 };
	sdr_key key = { 0 };
	sdr_body body = { 0 };
	Values  *vals = NULL ;

	// build bmc values
	reading.sensor_validity.sensor_state.sensor_scanning_disabled = true;
	vals = bmc_build_values(&reading, &header, &key, &body);
	if (vals) {
		free(vals->name);
		free(vals);
		vals = NULL;

	}


	reading.sensor_validity.sensor_state.sensor_scanning_disabled = false;
	reading.sensor_validity.sensor_state.event_messages_disabled = true;
	vals = bmc_build_values(&reading, &header, &key, &body);
	if (vals) {
		free(vals->name);
		free(vals);
		vals = NULL;

	}


	reading.sensor_validity.sensor_state.sensor_scanning_disabled = false;
	reading.sensor_validity.sensor_state.event_messages_disabled = false;
	vals = bmc_build_values(&reading, &header, &key, &body);
	if (vals) {
		free(vals->name);
		free(vals);
		vals = NULL;

	}

	body.id_string_type_length_code.bits.format = ASCII_8;
	body.id_string_type_length_code.bits.len_in_characters = 0;
	vals = bmc_build_values(&reading, &header, &key, &body);
	if (vals) {
		free(vals->name);
		free(vals);
		vals = NULL;

	}

	body.sensor_units_1.bits.analog_data_format = 0x3;
	vals = bmc_build_values(&reading, &header, &key, &body);
	if (vals) {
		free(vals->name);
		free(vals);
		vals = NULL;

	}

	body.sensor_units_2 = 0xff;
	vals = bmc_build_values(&reading, &header, &key, &body);
	if (vals) {
		free(vals->name);
		free(vals);
		vals = NULL;

	}

}
INSTANTIATE_TEST_CASE_P(bmc_c, bmc_c_p, ::testing::ValuesIn(test_platform::mock_platforms({ "dcp-rc" })));