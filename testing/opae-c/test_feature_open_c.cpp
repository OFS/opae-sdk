// Copyright(c) 2018-2019, Intel Corporation
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
#include <opae/fpga.h>
#include <uuid/uuid.h>
#include "opae_int.h"
#include "safe_string/safe_string.h"

#include "types_int.h"
#include "xfpga.h"
#include "intel-fpga.h"
#include "feature_pluginmgr.h"
#ifdef __cplusplus
}
#endif

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

class opae_feature_open_c_p : public ::testing::TestWithParam<std::string> {
	protected:
	opae_feature_open_c_p() : filter_(nullptr), tokens_{{nullptr, nullptr}} {}

	virtual void SetUp() override {
		ASSERT_TRUE(test_platform::exists(GetParam()));
		platform_ = test_platform::get(GetParam());
		system_ = test_system::instance();
		system_->initialize();
		system_->prepare_syfs(platform_);
		invalid_device_ = test_device::unknown();

		ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
		ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
		ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
		num_matches_ = 0;
		ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
			  &num_matches_),
			  FPGA_OK);
		EXPECT_EQ(num_matches_, platform_.devices.size());
		accel_ = nullptr;
		ASSERT_EQ(fpgaOpen(tokens_[0], &accel_, 0), FPGA_OK);
		system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
		which_mmio_ = 0;

		feature_filter_.type = FPGA_DMA_FEATURE;
		fpga_guid guid = {0xE7, 0xE3, 0xE9, 0x58, 0xF2, 0xE8, 0x73, 0x9D, 
						0xE0, 0x4C, 0x48, 0xC1, 0x58, 0x69, 0x81, 0x87 };
		memcpy_s(feature_filter_.guid, sizeof(fpga_guid), guid, sizeof(fpga_guid));
	}

	void DestroyTokens() {
		for (auto &t : tokens_) {
			if (t) {
				EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
				t = nullptr;
			}
		}
		num_matches_ = 0;
	}

	virtual void TearDown() override {
		DestroyTokens();
		if (filter_ != nullptr) {
			EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
		}
		system_->finalize();
	}

	uint32_t which_mmio_;
	std::array<fpga_feature_token, 2> ftokens_;
	fpga_handle accel_;
	fpga_feature_properties feature_filter_;
	fpga_feature_handle feature_h;

	fpga_properties filter_;
	std::array<fpga_token, 2> tokens_;
	uint32_t num_matches_;
	test_platform platform_;
	test_device invalid_device_;
	test_system *system_;
};

/**
 * @test       test_feature_functions
 * @brief      Tests: fpgaFeatureOpen, fpgaFeaturePropertiesGet 
 *                    fpgaDMAPropertiesGet, fpgaDMATransferSync
 *                    fpgaFeatureClose, fpgaDestroyFeatureToken
 * @details    When fpgaFeatureOpen() is called with a valid param,<br>
 *             then it creates the accelerateor feature handle for the feature<br>
 *             token given in the parameter.<br>
 *             When fpgaFeatureClose() is called with a valid feature<br>
 *             handle, it will close the feature handle.<br>
 */
TEST_P(opae_feature_open_c_p, test_feature_functions) {

	struct DFH dfh ;
	dfh.id = 0x1;
	dfh.revision = 0;
	dfh.next_header_offset = 0x100;
	dfh.eol = 1;
	dfh.reserved = 0;
	dfh.type = 0x1;

	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x0, dfh.csr));

	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x8, 0xf89e433683f9040b));
	EXPECT_EQ(FPGA_OK,fpgaWriteMMIO64(accel_, which_mmio_, 0x10, 0xd8424dc4a4a3c413));

	struct DFH dfh_bbb = { 0 };

	dfh_bbb.type = 0x2;
	dfh_bbb.id = 0x2;
	dfh_bbb.revision = 0;
	dfh_bbb.next_header_offset = 0x000;
	dfh_bbb.eol = 1;
	dfh_bbb.reserved = 0;

	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x100, dfh_bbb.csr));

	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x108, 0x9D73E8F258E9E3E7));
	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x110, 0x87816958C1484CE0));

	EXPECT_EQ(fpgaFeatureEnumerate(accel_, &feature_filter_, ftokens_.data(),
		ftokens_.size(), &num_matches_), FPGA_OK);
	
	fpga_feature_properties feature_prop;
	EXPECT_EQ(fpgaFeaturePropertiesGet(ftokens_[0], &feature_prop), FPGA_OK);

	EXPECT_EQ(fpgaFeatureOpen(ftokens_[0], 0, nullptr, &feature_h), FPGA_OK);
	
	fpga_dma_properties dma_prop;
	EXPECT_EQ(fpgaDMAPropertiesGet(ftokens_[0], &dma_prop), FPGA_OK);
	
	dma_transfer_list t_list;
	EXPECT_EQ(fpgaDMATransferSync(feature_h, &t_list), FPGA_OK);
	
	EXPECT_EQ(fpgaFeatureClose(feature_h), FPGA_OK);
	EXPECT_EQ(fpgaDestroyFeatureToken(&(ftokens_[0])), FPGA_OK);
}

/**
 * @test       nulltokens
 * @brief      Tests: fpgaFeatureOpen
 * @details    When fpgaFeatureOpen() is called with nullptr for<br>
 *             feature token, it will return FPGA_INVALID_PARAM.<br>
 */
TEST_P(opae_feature_open_c_p, nulltokens) {
	EXPECT_EQ(fpgaFeatureOpen(nullptr, 0, nullptr, &feature_h),
	  FPGA_INVALID_PARAM);
 }

 /**
 * @test       nullhandle
 * @brief      Tests: fpgaFeatureOpen
 * @details    When fpgaFeatureOpen() is called with nullptr for<br>
 *             the feature handle, it will return FPGA_INVALID_PARAM.<br>
 */
TEST_P(opae_feature_open_c_p, nullhandle) {
	EXPECT_EQ(fpgaFeatureOpen(ftokens_[0], 0, nullptr, nullptr),
	  FPGA_INVALID_PARAM);
}

 /**
 * @test       mallocfail
 * @brief      Tests: fpgaFeatureOpen
 * @details    When the system is running out of memory, fpgaFeatureOpen()<br>
 *             will return FPGA_NO_MEMORY.<br>
 */
TEST_P(opae_feature_open_c_p, mallocfail) {
	system_->invalidate_malloc(0, "allocate_wrapped_feature_handle");

	struct DFH dfh ;
	dfh.id = 0x1;
	dfh.revision = 0;
	dfh.next_header_offset = 0x100;
	dfh.eol = 1;
	dfh.reserved = 0;
	dfh.type = 0x1;

	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x0, dfh.csr));

	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x8, 0xf89e433683f9040b));
	EXPECT_EQ(FPGA_OK,fpgaWriteMMIO64(accel_, which_mmio_, 0x10, 0xd8424dc4a4a3c413));

	struct DFH dfh_bbb = { 0 };

	dfh_bbb.type = 0x2;
	dfh_bbb.id = 0x2;
	dfh_bbb.revision = 0;
	dfh_bbb.next_header_offset = 0x000;
	dfh_bbb.eol = 1;
	dfh_bbb.reserved = 0;

	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x100, dfh_bbb.csr));

	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x108, 0x9D73E8F258E9E3E7));
	EXPECT_EQ(FPGA_OK, fpgaWriteMMIO64(accel_, which_mmio_, 0x110, 0x87816958C1484CE0));

	EXPECT_EQ(fpgaFeatureEnumerate(accel_, &feature_filter_, ftokens_.data(),
		ftokens_.size(), &num_matches_), FPGA_OK);
	
	ASSERT_EQ(fpgaFeatureOpen(ftokens_[0], 0, nullptr, &feature_h),
	  FPGA_NO_MEMORY);
	EXPECT_EQ(fpgaDestroyFeatureToken(&(ftokens_[0])), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(opae_feature_open_c, opae_feature_open_c_p,
                        ::testing::ValuesIn(test_platform::keys(true)));
