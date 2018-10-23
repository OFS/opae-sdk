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
#include "metrics/vector.h"
#include "metrics/metrics_int.h"


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

using namespace opae::testing;

#if 0

/**
 * @test       opaec
 * @brief      Tests: fpga_vector_init
 * @details    When fpgaGetOPAECVersion is called with a valid param,<br>
 *             then it retrieves the INTEL_FPGA_API_VER_* constants<br>
 *             from config.h.<br>
 */
TEST(metrics_utils, test_metric_utils_01) {

	char group_sysfs[FPGA_METRICS_STR_SIZE] = { 0 };

	EXPECT_NE(FPGA_OK, sysfs_path_is_dir(NULL));

	EXPECT_NE(FPGA_OK, sysfs_path_is_dir(group_sysfs));

	EXPECT_NE(FPGA_OK, sysfs_path_is_dir((const char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/bitstream_id"));

	EXPECT_NE(FPGA_OK, sysfs_path_is_dir((const char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/bitstream_id1"));

	EXPECT_EQ(FPGA_OK, sysfs_path_is_dir((const char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/"));

}

/**
 * @test       opaec
 * @brief      Tests: fpga_vector_init
 * @details    When fpgaGetOPAECVersion is called with a valid param,<br>
 *             then it retrieves the INTEL_FPGA_API_VER_* constants<br>
 *             from config.h.<br>
 */
TEST(metrics_utils, test_metric_utils_02) {

	char group_name[FPGA_METRICS_STR_SIZE] = { "power_mgmt" };
	char group_sysfs[FPGA_METRICS_STR_SIZE] = { "tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/" };

	char metrics_name[FPGA_METRICS_STR_SIZE] = { "consumed" };
	char metrics_sysfs[FPGA_METRICS_STR_SIZE] = { "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/" };

	char qualifier_name[FPGA_METRICS_STR_SIZE] = { "power_mgmt" };
	char metric_units[FPGA_METRICS_STR_SIZE] = { "watts" };

	fpga_metric_vector metric_vector;

	EXPECT_NE(FPGA_OK, add_metric_vector(NULL, 0, qualifier_name, group_name, group_sysfs, metrics_name,
		metrics_sysfs, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

	EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, NULL, group_name, group_sysfs, metrics_name,
		metrics_sysfs, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

	EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, qualifier_name, NULL, group_sysfs, metrics_name,
		metrics_sysfs, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

	EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, qualifier_name, group_name, NULL, metrics_name,
		metrics_sysfs, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));


	EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, qualifier_name, group_name, group_sysfs, NULL,
		metrics_sysfs, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

	EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, qualifier_name, group_name, group_sysfs, metrics_name,
		NULL, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));

	EXPECT_NE(FPGA_OK, add_metric_vector(&metric_vector, 0, qualifier_name, group_name, group_sysfs, metrics_name,
		metrics_sysfs, NULL, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP, 0));
	

	EXPECT_EQ(FPGA_OK, fpga_vector_init(&metric_vector));

	EXPECT_EQ(FPGA_OK, add_metric_vector(&metric_vector, 0, qualifier_name, group_name, group_sysfs, metrics_name,
		metrics_sysfs, metric_units, FPGA_METRIC_DATATYPE_INT, FPGA_METRIC_TYPE_POWER, FPGA_HW_MCP,0));



	EXPECT_EQ(FPGA_OK, fpga_vector_free(&metric_vector));
	


}


/**
 * @test       opaec
 * @brief      Tests: fpga_vector_init
 * @details    When fpgaGetOPAECVersion is called with a valid param,<br>
 *             then it retrieves the INTEL_FPGA_API_VER_* constants<br>
 *             from config.h.<br>
 */
TEST(metrics_utils, test_metric_utils_03) {

	char group_sysfs[FPGA_METRICS_STR_SIZE] = { "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0" };
	char group_sysfs_invalid[FPGA_METRICS_STR_SIZE] = { 0 };

	fpga_metric_vector vector;
	uint64_t metric_id = 0;

	EXPECT_NE(FPGA_OK, enum_thermalmgmt_metrics(NULL, &metric_id, group_sysfs, FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_thermalmgmt_metrics(&vector, &metric_id, NULL, FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_thermalmgmt_metrics(&vector, NULL, group_sysfs, FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_thermalmgmt_metrics(&vector, &metric_id, group_sysfs_invalid, FPGA_HW_MCP));

	EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));

	EXPECT_EQ(FPGA_OK, enum_thermalmgmt_metrics(&vector, &metric_id, group_sysfs, FPGA_HW_MCP));

	EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));


}


/**
 * @test       opaec
 * @brief      Tests: fpga_vector_init
 * @details    When fpgaGetOPAECVersion is called with a valid param,<br>
 *             then it retrieves the INTEL_FPGA_API_VER_* constants<br>
 *             from config.h.<br>
 */
TEST(metrics_utils, test_metric_utils_04) {

	char group_sysfs[FPGA_METRICS_STR_SIZE] = { "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0" };
	char group_sysfs_invalid[FPGA_METRICS_STR_SIZE] = { 0 };
	uint64_t metric_id = 0;
	fpga_metric_vector vector;


	EXPECT_NE(FPGA_OK, enum_powermgmt_metrics(NULL, &metric_id, group_sysfs, FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_powermgmt_metrics(&vector, &metric_id, NULL, FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_powermgmt_metrics(&vector, NULL, group_sysfs, FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_powermgmt_metrics(&vector, &metric_id, group_sysfs_invalid, FPGA_HW_MCP));

	EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));

	EXPECT_EQ(FPGA_OK, enum_powermgmt_metrics(&vector, &metric_id, group_sysfs, FPGA_HW_MCP));

	EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));



}

/**
 * @test       opaec
 * @brief      Tests: fpga_vector_init
 * @details    When fpgaGetOPAECVersion is called with a valid param,<br>
 *             then it retrieves the INTEL_FPGA_API_VER_* constants<br>
 *             from config.h.<br>
 */
TEST(metrics_utils, test_metric_utils_05) {

	char qualifier_name[FPGA_METRICS_STR_SIZE] = { 0 };
	char group_sysfs[FPGA_METRICS_STR_SIZE] = { 0 };
	char sysfs_name[FPGA_METRICS_STR_SIZE] = { 0 };
	uint64_t metric_id = 0;
	fpga_metric_vector vector;


	EXPECT_NE(FPGA_OK, enum_perf_counter_items(NULL, &metric_id, qualifier_name, group_sysfs, sysfs_name, FPGA_METRIC_TYPE_PERF_FABRIC, FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, NULL, qualifier_name, group_sysfs, sysfs_name, FPGA_METRIC_TYPE_PERF_FABRIC, FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, NULL, group_sysfs, sysfs_name, FPGA_METRIC_TYPE_PERF_FABRIC, FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, qualifier_name, NULL, sysfs_name, FPGA_METRIC_TYPE_PERF_FABRIC, FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, qualifier_name, group_sysfs, NULL, FPGA_METRIC_TYPE_PERF_FABRIC, FPGA_HW_MCP));


	EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, qualifier_name, group_sysfs, NULL, FPGA_METRIC_TYPE_PERF_FABRIC, FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, qualifier_name, (char*)"/tmp/class/fpga/intel-fpga-dev.1", sysfs_name, FPGA_METRIC_TYPE_PERF_FABRIC, FPGA_HW_MCP));




	EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));

	EXPECT_EQ(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, qualifier_name, (char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/iperf/", (char*)"fabric", FPGA_METRIC_TYPE_PERF_FABRIC, FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_perf_counter_items(&vector, &metric_id, qualifier_name, (char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/iperf/perf", (char*)"port0", FPGA_METRIC_TYPE_PERF_FABRIC, FPGA_HW_MCP));

	EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));


}


/**
 * @test       opaec
 * @brief      Tests: fpga_vector_init
 * @details    When fpgaGetOPAECVersion is called with a valid param,<br>
 *             then it retrieves the INTEL_FPGA_API_VER_* constants<br>
 *             from config.h.<br>
 */
TEST(metrics_utils, test_metric_utils_06) {

	char group_sysfs[FPGA_METRICS_STR_SIZE] = { 0 };
	fpga_metric_vector vector;
	uint64_t metric_id = 0;


	EXPECT_NE(FPGA_OK, enum_perf_counter_metrics(NULL, &metric_id, group_sysfs, FPGA_HW_MCP));
	EXPECT_NE(FPGA_OK, enum_perf_counter_metrics(&vector, NULL, group_sysfs, FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_perf_counter_metrics(&vector, &metric_id, NULL, FPGA_HW_MCP));

	EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));

	EXPECT_EQ(FPGA_OK, enum_perf_counter_metrics(&vector, &metric_id, (char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0", FPGA_HW_MCP));

	EXPECT_NE(FPGA_OK, enum_perf_counter_metrics(&vector, &metric_id, (char*)"/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.1", FPGA_HW_MCP));

	EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));

}


/**
 * @test       opaec
 * @brief      Tests: fpga_vector_init
 * @details    When fpgaGetOPAECVersion is called with a valid param,<br>
 *             then it retrieves the INTEL_FPGA_API_VER_* constants<br>
 *             from config.h.<br>
 */
TEST(metrics_utils, test_metric_utils_07) {


	struct _fpga_enum_metric _enum_metric = {
		 "power_mgmt",
		 "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/power_mgmt",
		 "consumed",
		 "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-fme.0/power_mgmt/consumed",
		"power_mgmt:consumed",
		 "watts",
		1,
		FPGA_METRIC_DATATYPE_INT,
		 FPGA_METRIC_TYPE_POWER,
		 FPGA_HW_MCP
	};


	struct fpga_metric_t  fpga_metric;

	EXPECT_NE(FPGA_OK, add_metric_info(NULL, &fpga_metric));
	EXPECT_NE(FPGA_OK, add_metric_info(&_enum_metric, NULL));

	EXPECT_EQ(FPGA_OK, add_metric_info(&_enum_metric, &fpga_metric));

}


class metics_uitls_c_p : public ::testing::TestWithParam<std::string>{
 protected:
	 metics_uitls_c_p()
  : tokens_{{nullptr, nullptr}},
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

  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;

};

/**
* @test    set_afu_userclock
* @brief   Tests: set_afu_userclock
* @details set_afu_userclock sets afu user clock
*          Returns FPGA_OK if parameters are valid. Returns
*          error code if invalid user clock or handle.
*/
TEST_P(metics_uitls_c_p, test_metric_utils_08) {
	fpga_result result;
	struct _fpga_handle _handle_invlaid;

	struct _fpga_handle *_handle = (struct _fpga_handle *) handle_;

	EXPECT_EQ(FPGA_OK, enum_fpga_metrics(_handle));

	EXPECT_EQ(FPGA_OK, enum_fpga_metrics(_handle));

	EXPECT_NE(FPGA_OK, enum_fpga_metrics(NULL));

	//EXPECT_NE(FPGA_OK, enum_fpga_metrics(&_handle_invlaid));

	

}

/**
* @test    set_afu_userclock
* @brief   Tests: set_afu_userclock
* @details set_afu_userclock sets afu user clock
*          Returns FPGA_OK if parameters are valid. Returns
*          error code if invalid user clock or handle.
*/
TEST_P(metics_uitls_c_p, test_metric_utils_09) {
	fpga_result result;


	struct _fpga_handle *_handle = (struct _fpga_handle *) handle_;

	EXPECT_EQ(FPGA_OK, enum_fpga_metrics(_handle));

	struct fpga_metric_t  fpga_metric;

	EXPECT_EQ(FPGA_OK, get_metric_value_byid(_handle,&(_handle->fpga_enum_metric_vector), 1, &fpga_metric));

	EXPECT_EQ(FPGA_OK, get_metric_value_byid(_handle, &(_handle->fpga_enum_metric_vector), 5, &fpga_metric));

	EXPECT_EQ(FPGA_OK, get_metric_value_byid(_handle, &(_handle->fpga_enum_metric_vector), 10, &fpga_metric));


	EXPECT_EQ(FPGA_OK, get_metric_value_byid(_handle, &(_handle->fpga_enum_metric_vector), 15, &fpga_metric));


	EXPECT_EQ(FPGA_OK, get_metric_value_byid(_handle, &(_handle->fpga_enum_metric_vector), 20, &fpga_metric));

	EXPECT_EQ(FPGA_OK, get_metric_value_byid(_handle, &(_handle->fpga_enum_metric_vector), 25, &fpga_metric));

	EXPECT_EQ(FPGA_OK, get_metric_value_byid(_handle, &(_handle->fpga_enum_metric_vector), 30, &fpga_metric));

	EXPECT_EQ(FPGA_OK, get_metric_value_byid(_handle, &(_handle->fpga_enum_metric_vector), 34, &fpga_metric));

	EXPECT_EQ(FPGA_OK, get_metric_value_byid(_handle, &(_handle->fpga_enum_metric_vector), 31, &fpga_metric));

	EXPECT_EQ(FPGA_OK, get_metric_value_byid(_handle, &(_handle->fpga_enum_metric_vector), 32, &fpga_metric));

	EXPECT_NE(FPGA_OK, get_metric_value_byid(_handle, NULL, 1, &fpga_metric));

	EXPECT_NE(FPGA_OK, get_metric_value_byid(_handle, &(_handle->fpga_enum_metric_vector), 1, NULL));

}

/**
* @test    set_afu_userclock
* @brief   Tests: set_afu_userclock
* @details set_afu_userclock sets afu user clock
*          Returns FPGA_OK if parameters are valid. Returns
*          error code if invalid user clock or handle.
*/
TEST_P(metics_uitls_c_p, test_metric_utils_10) {
	fpga_result result;


	struct _fpga_handle *_handle = (struct _fpga_handle *) handle_;

	EXPECT_EQ(FPGA_OK, enum_fpga_metrics(_handle));

	char serach_string[] = { "power_mgmt:consumed" };
	uint64_t metric_id;
	EXPECT_EQ(FPGA_OK, get_metricid_from_serach_string((const char*)serach_string, &(_handle->fpga_enum_metric_vector), &metric_id));


	EXPECT_NE(FPGA_OK, get_metricid_from_serach_string(NULL, &(_handle->fpga_enum_metric_vector), &metric_id));

	EXPECT_NE(FPGA_OK, get_metricid_from_serach_string((const char*)serach_string, &(_handle->fpga_enum_metric_vector), NULL));

	EXPECT_NE(FPGA_OK, get_metricid_from_serach_string((const char*)serach_string, NULL, &metric_id));

	EXPECT_NE(FPGA_OK, get_metricid_from_serach_string((const char*) "power_mgmt consumed", &(_handle->fpga_enum_metric_vector), &metric_id));

}


/**
* @test    set_afu_userclock
* @brief   Tests: set_afu_userclock
* @details set_afu_userclock sets afu user clock
*          Returns FPGA_OK if parameters are valid. Returns
*          error code if invalid user clock or handle.
*/
TEST_P(metics_uitls_c_p, test_metric_utils_11) {
	fpga_result result;


	struct _fpga_handle *_handle = (struct _fpga_handle *) handle_;

	EXPECT_EQ(FPGA_OK, enum_fpga_metrics(_handle));

	EXPECT_EQ(FPGA_OK, delete_fpga_enum_metrics_vector(_handle));

	EXPECT_NE(FPGA_OK, delete_fpga_enum_metrics_vector(NULL));

	struct _fpga_handle _handle_invalid;

	EXPECT_NE(FPGA_OK, delete_fpga_enum_metrics_vector(&_handle_invalid));

}

INSTANTIATE_TEST_CASE_P(metics_uitls_c, metics_uitls_c_p, ::testing::ValuesIn(test_platform::keys(true)));


class metics_uitls_dcp_c_p : public ::testing::TestWithParam<std::string> {
protected:
	metics_uitls_dcp_c_p()
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

	std::array<fpga_token, 2> tokens_;
	fpga_handle handle_;
	fpga_properties filter_;
	uint32_t num_matches_;
	test_platform platform_;
	test_system *system_;

};

TEST_P(metics_uitls_dcp_c_p, test_metric_utils_12) {
	fpga_result result;
	uint64_t metric_id;

	struct _fpga_handle *_handle = (struct _fpga_handle *) handle_;
	fpga_metric_vector vector;
	_handle->dl_handle = dlopen("libbmc.so", RTLD_LAZY | RTLD_LOCAL);

	if (!_handle->dl_handle) {
		OPAE_ERR("--------------------------failed to load ");

	}

	EXPECT_EQ(FPGA_OK, fpga_vector_init(&vector));

	

	enum_bmc_metrics_info(_handle, _handle->token,  &vector, &metric_id, FPGA_HW_DCP_RC);

	//EXPECT_NE(FPGA_OK, enum_fpga_metrics(&_handle_invlaid));


	EXPECT_EQ(FPGA_OK, fpga_vector_free(&vector));

	dlclose(_handle->dl_handle);

}

TEST_P(metics_uitls_dcp_c_p, test_metric_utils_13) {
	fpga_result result;


	struct _fpga_handle *_handle = (struct _fpga_handle *) handle_;

	EXPECT_EQ(FPGA_OK, enum_fpga_metrics(_handle));



}

INSTANTIATE_TEST_CASE_P(metics_uitls_c, metics_uitls_dcp_c_p, ::testing::Values(std::string("dcp-rc")));

#endif