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

#ifdef __cplusplus
extern "C" {
#endif

#include <json-c/json.h>
#include <uuid/uuid.h>
#include <opae/fpga.h>
#include "intel-fpga.h"
#include "safe_string/safe_string.h"
#include "xfpga.h"
#include "feature_int.h"
#include "feature_pluginmgr.h"

int feature_plugin_mgr_initialize_all(void);
feature_adapter_table *feature_plugin_mgr_alloc_adapter(const char *lib_path, fpga_guid guid);
int feature_plugin_mgr_free_adapter(feature_adapter_table *adapter);
int feature_plugin_mgr_register_adapter(feature_adapter_table *adapter);
feature_adapter_table *get_feature_plugin_adapter(fpga_guid guid);
int feature_plugin_mgr_configure_plugin(feature_adapter_table *adapter,
				     const char *config);
int feature_plugin_mgr_finalize_all(void);

extern feature_adapter_table *feature_adapter_list;
#ifdef __cplusplus
}
#endif
#include <config.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "test_system.h"

#include <opae/access.h>
#include <opae/mmio.h>
#include <linux/ioctl.h>

using namespace opae::testing;

/**
 * @test       alloc_adapter01
 * @brief      Test: feature_plugin_mgr_alloc_adapter
 * @details    When the given library name is not found,<br>
 *             feature_plugin_mgr_alloc_adapter returns NULL.<br>
 */
TEST(feature_pluginmgr, alloc_adapter01) {
	fpga_guid guid = {0xE7, 0xE3, 0xE9, 0x58, 0xF2, 0xE8, 0x73, 0x9D, 
					0xE0, 0x4C, 0x48, 0xC1, 0x58, 0x69, 0x81, 0x87 };
	EXPECT_EQ(NULL, feature_plugin_mgr_alloc_adapter("libthatdoesntexist.so", guid));
}

/**
 * @test       alloc_adapter02
 * @brief      Test: feature_plugin_mgr_alloc_adapter
 * @details    When calloc fails,<br>
 *             feature_plugin_mgr_alloc_adapter returns NULL.<br>
 */
TEST(feature_pluginmgr, alloc_adapter02) {
	fpga_guid guid = {0xE7, 0xE3, 0xE9, 0x58, 0xF2, 0xE8, 0x73, 0x9D, 
					0xE0, 0x4C, 0x48, 0xC1, 0x58, 0x69, 0x81, 0x87 };
	test_system::instance()->invalidate_calloc(0, "feature_plugin_mgr_alloc_adapter");
	EXPECT_EQ(NULL, feature_plugin_mgr_alloc_adapter("libintel-dma.so", guid));
}

/**
 * @test       free_adapter01
 * @brief      Test: feature_plugin_mgr_free_adapter
 * @details    feature_plugin_mgr_free_adapter frees the given adapter table<br>
 *             and returns 0 on success.<br>
 */
TEST(feature_pluginmgr, free_adapter) {
	fpga_guid guid = {0xE7, 0xE3, 0xE9, 0x58, 0xF2, 0xE8, 0x73, 0x9D, 
					0xE0, 0x4C, 0x48, 0xC1, 0x58, 0x69, 0x81, 0x87 };
	feature_adapter_table *at;
	at = feature_plugin_mgr_alloc_adapter("libintel-dma.so", guid);
	ASSERT_NE(nullptr, at);
	EXPECT_EQ(0, feature_plugin_mgr_free_adapter(at));
}

/**
 * @test       config_err
 * @brief      Test: feature_plugin_mgr_configure_plugin
 * @details    When feature_plugin_mgr_configure_plugin is called on a load library<br>
 *             that has no FEATURE_PLUGIN_CONFIGURE symbol,<br>
 *             then the fn returns non-zero.<br>
 */
TEST(feature_pluginmgr, config_err) {
	fpga_guid guid = {0xE7, 0xE3, 0xE9, 0x58, 0xF2, 0xE8, 0x73, 0x9D, 
					0xE0, 0x4C, 0x48, 0xC1, 0x58, 0x69, 0x81, 0x87 };
	feature_adapter_table *at;
	at = feature_plugin_mgr_alloc_adapter("libopae-c.so", guid);  // TODO: checking
	ASSERT_NE(nullptr, at);
	EXPECT_NE(0, feature_plugin_mgr_configure_plugin(at, ""));
	EXPECT_EQ(0, feature_plugin_mgr_free_adapter(at));
}

extern "C" {

	static int test_feature_plugin_initialize_called;
	static int test_feature_plugin_initialize(void)
	{
		++test_feature_plugin_initialize_called;
		return 0;
	}

	static int test_feature_plugin_bad_initialize(void)
	{
		++test_feature_plugin_initialize_called;
		return 1;
	}

	static int test_feature_plugin_finalize_called;
	static int test_feature_plugin_finalize(void)
	{
		++test_feature_plugin_finalize_called;
		return 0;
	}

	static int test_feature_plugin_bad_finalize(void)
	{
		++test_feature_plugin_finalize_called;
		return 1;
	}
}

int mmio_ioctl(mock_object * m, int request, va_list argp) {
	int retval = -1;
	errno = EINVAL;
	UNUSED_PARAM(m);
	UNUSED_PARAM(request);
	struct fpga_port_region_info *rinfo = va_arg(argp, struct fpga_port_region_info *);
	if (!rinfo) {
		FPGA_MSG("rinfo is NULL");
		goto out_EINVAL;
	}
	if (rinfo->argsz != sizeof(*rinfo)) {
		FPGA_MSG("wrong structure size");
		goto out_EINVAL;
	}
	if (rinfo->index > 1) {
		FPGA_MSG("unsupported MMIO index");
		goto out_EINVAL;
	}
	if (rinfo->padding != 0) {
		FPGA_MSG("unsupported padding");
		goto out_EINVAL;
	}
	rinfo->flags = FPGA_REGION_READ | FPGA_REGION_WRITE | FPGA_REGION_MMAP;
	rinfo->size = 0x40000;
	rinfo->offset = 0;
	retval = 0;
	errno = 0;

	return retval;

out_EINVAL:
	retval = -1;
	errno = EINVAL;
	return retval;
}

class feature_pluginmgr_c_p : public ::testing::TestWithParam<std::string> {
	protected:
	feature_pluginmgr_c_p() : tokens_{{nullptr, nullptr}} {}

	virtual void SetUp() override {
		ASSERT_TRUE(test_platform::exists(GetParam()));
		platform_ = test_platform::get(GetParam());
		system_ = test_system::instance();
		system_->initialize();
		system_->prepare_syfs(platform_);
		invalid_device_ = test_device::unknown();

		ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
		ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
		num_matches_ = 0;
		ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
								&num_matches_), FPGA_OK);
		EXPECT_EQ(num_matches_, platform_.devices.size());
		accel_ = nullptr;
		ASSERT_EQ(xfpga_fpgaOpen(tokens_[0], &accel_, 0), FPGA_OK);
		system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
		which_mmio_ = 0;
		num_matches_ = 0;
		feature_filter_.type = FPGA_DMA_FEATURE;
		fpga_guid guid = {0xE7, 0xE3, 0xE9, 0x58, 0xF2, 0xE8, 0x73, 0x9D, 
						0xE0, 0x4C, 0x48, 0xC1, 0x58, 0x69, 0x81, 0x87};
		memcpy_s(feature_filter_.guid, sizeof(fpga_guid), guid, sizeof(fpga_guid));

		// save the global adapter list.
		feature_adapter_list_ = feature_adapter_list;
		feature_adapter_list = nullptr;

		test_feature_plugin_initialize_called = 0;
		test_feature_plugin_finalize_called = 0;

		faux_adapter0_ = feature_plugin_mgr_alloc_adapter("libintel-dma.so", guid);
		ASSERT_NE(nullptr, faux_adapter0_);

		faux_adapter0_->initialize = test_feature_plugin_initialize;
		faux_adapter0_->finalize = test_feature_plugin_finalize;
		EXPECT_EQ(0, feature_plugin_mgr_register_adapter(faux_adapter0_));

		faux_adapter1_ = feature_plugin_mgr_alloc_adapter("libintel-dma.so", guid);
		ASSERT_NE(nullptr, faux_adapter1_);

		faux_adapter1_->initialize = test_feature_plugin_initialize;
		faux_adapter1_->finalize = test_feature_plugin_finalize;
		EXPECT_EQ(0, feature_plugin_mgr_register_adapter(faux_adapter1_));
	} 

	virtual void TearDown() override {

		if (filter_ != nullptr) {
			EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
		}

		EXPECT_EQ(feature_plugin_mgr_finalize_all(), 0);

		// Restore the global adapter list.
		feature_adapter_list = feature_adapter_list_;

		if (accel_) {
			EXPECT_EQ(xfpga_fpgaClose(accel_), FPGA_OK);
			accel_ = nullptr;
		}
		for (auto &t : tokens_) {
			if (t) {
				EXPECT_EQ(xfpga_fpgaDestroyToken(&t), FPGA_OK);
				t = nullptr;
			}
		}
		system_->finalize();
	}

	std::array<fpga_token, 2> tokens_;
	uint32_t which_mmio_;
	std::array<fpga_feature_token, 2> ftokens_;
	fpga_properties filter_;  
	fpga_handle accel_;
	fpga_feature_properties feature_filter_;
	feature_adapter_table *feature_adapter_list_;
	feature_adapter_table *faux_adapter0_;
	feature_adapter_table *faux_adapter1_;
	uint32_t num_matches_;
	test_platform platform_;
	test_device invalid_device_;
	test_system *system_;
};

/**
 * @test       get_feature_plugin_adapter
 * @brief      Test: get_feature_plugin_adapte()
 * @details    When a bad guid of a feature is given to get_feature_plugin_adapter(),<br>
 *             it should return nullptr <br>
 */
TEST_P(feature_pluginmgr_c_p, get_feature_plugin_adapter) {

	struct DFH dfh ;
	dfh.id = 0x1;
	dfh.revision = 0;
	dfh.next_header_offset = 0x100;
	dfh.eol = 1;
	dfh.reserved = 0;
	dfh.type = 0x1;

	// Write to AFU's CSR and GUID registers
	EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, which_mmio_, 0x0, dfh.csr));
	EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, which_mmio_, 0x8, 0xf89e433683f9040b));
	EXPECT_EQ(FPGA_OK,xfpga_fpgaWriteMMIO64(accel_, which_mmio_, 0x10, 0xd8424dc4a4a3c413));

	struct DFH dfh_bbb = { 0 };

	dfh_bbb.type = 0x2;
	dfh_bbb.id = 0x2;
	dfh_bbb.revision = 0;
	dfh_bbb.next_header_offset = 0x000;
	dfh_bbb.eol = 1;
	dfh_bbb.reserved = 0;

	// Write to DMA BBB's CSR and GUID registers
	EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, which_mmio_, 0x100, dfh_bbb.csr));
	EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, which_mmio_, 0x108, 0x9D73E8F258E9E3E7));
	EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, which_mmio_, 0x110, 0x87816958C1484CE0));


	EXPECT_EQ(xfpga_fpgaFeatureEnumerate(accel_, &feature_filter_, ftokens_.data(),
		ftokens_.size(), &num_matches_), FPGA_OK);
	EXPECT_EQ(num_matches_, 1);

	fpga_guid bad_guid = {0xFF, 0xE3, 0xE9, 0x58, 0xF2, 0xE8, 0x73, 0x9D, 
					0xE0, 0x4C, 0x48, 0xC1, 0x58, 0x69, 0x81, 0x87};
	EXPECT_EQ(nullptr, get_feature_plugin_adapter(bad_guid));

	for (auto &ft : ftokens_) {
		if (ft && &ft) {
			xfpga_fpgaDestroyFeatureToken(&ft);
			ft = nullptr;
		}
	}
} 

/**
 * @test       bad_init_all
 * @brief      Test: feature_plugin_mgr_initialize_all, feature_plugin_mgr_finalize_all
 * @details    When any of the registered adapters' initialize fn returns non-zero,<br>
 *             then feature_plugin_mgr_initialize_all returns non-zero.<br>
 *             When any of the registered adapters' finalize fn returns non-zero,<br>
 *             then feature_plugin_mgr_finalize_all returns non-zero.<br>
 */
TEST_P(feature_pluginmgr_c_p, bad_init_all) {

	struct DFH dfh ;
	dfh.id = 0x1;
	dfh.revision = 0;
	dfh.next_header_offset = 0x100;
	dfh.eol = 1;
	dfh.reserved = 0;
	dfh.type = 0x1;

	EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, which_mmio_, 0x0, dfh.csr));

	EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, which_mmio_, 0x8, 0xf89e433683f9040b));
	EXPECT_EQ(FPGA_OK,xfpga_fpgaWriteMMIO64(accel_, which_mmio_, 0x10, 0xd8424dc4a4a3c413));

	struct DFH dfh_bbb = { 0 };

	dfh_bbb.type = 0x2;
	dfh_bbb.id = 0x2;
	dfh_bbb.revision = 0;
	dfh_bbb.next_header_offset = 0x000;
	dfh_bbb.eol = 1;
	dfh_bbb.reserved = 0;

	EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, which_mmio_, 0x100, dfh_bbb.csr));

	EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, which_mmio_, 0x108, 0x9D73E8F258E9E3E7));
	EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(accel_, which_mmio_, 0x110, 0x87816958C1484CE0));

	faux_adapter1_->initialize = test_feature_plugin_bad_initialize;
	test_feature_plugin_initialize_called = 0;
	EXPECT_NE(0, feature_plugin_mgr_initialize_all());
	EXPECT_EQ(2, test_feature_plugin_initialize_called);

	faux_adapter1_->finalize = test_feature_plugin_bad_finalize;
	EXPECT_NE(0, feature_plugin_mgr_finalize_all());  
	EXPECT_EQ(nullptr, feature_adapter_list);
	EXPECT_EQ(2, test_feature_plugin_finalize_called);
 }

INSTANTIATE_TEST_CASE_P(feature_pluginmgr_c, feature_pluginmgr_c_p, ::testing::ValuesIn(test_platform::keys(true)));
