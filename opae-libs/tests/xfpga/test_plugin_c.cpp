// Copyright(c) 2017-2020, Intel Corporation
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

extern "C" {
#include <opae/utils.h>
#include "sysfs_int.h"
#include "types_int.h"
#include "adapter.h"

bool xfpga_plugin_supports_device(const char *device_type);
int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
bool xfpga_plugin_supports_host(const char *hostname);
int opae_plugin_configure(opae_api_adapter_table *adapter,
	const char *jsonConfig);
}

#include <opae/enum.h>
#include <opae/fpga.h>
#include <dlfcn.h>
#include "xfpga.h"
#include <fcntl.h>
#include "gtest/gtest.h"
#include "mock/test_system.h"
#include "adapter.h"

using namespace opae::testing;

class xfpga_plugin_c_p : public ::testing::TestWithParam<std::string> {
protected:
	xfpga_plugin_c_p()
		: tokens_{ {nullptr, nullptr} },
		handle_(nullptr) {}

	virtual void SetUp() override {
		ASSERT_TRUE(test_platform::exists(GetParam()));
		platform_ = test_platform::get(GetParam());
		system_ = test_system::instance();
		system_->initialize();
		system_->prepare_syfs(platform_);
		ASSERT_EQ(xfpga_plugin_initialize(), FPGA_OK);

		ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
		ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
		ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
			&num_matches_), FPGA_OK);
		ASSERT_GT(num_matches_, 0);
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
		if (handle_ != nullptr) {
			EXPECT_EQ(xfpga_fpgaClose(handle_), FPGA_OK);
			handle_ = nullptr;
		}
		xfpga_plugin_finalize();
		system_->finalize();
	}

	std::array<fpga_token, 2> tokens_;
	fpga_handle handle_;
	fpga_properties filter_;
	uint32_t num_matches_;
	test_platform platform_;
	test_system *system_;

	opae_api_adapter_table *opae_plugin_mgr_alloc_adapter_test(const char *lib_path)
	{
		void *dl_handle;
		opae_api_adapter_table *adapter;
		dl_handle = dlopen(lib_path, RTLD_LAZY | RTLD_LOCAL);
		if (!dl_handle) {
			char *err = dlerror();
			OPAE_ERR("failed to load \"%s\" %s", lib_path, err ? err : "");
			return NULL;
		}

		adapter = (opae_api_adapter_table *)calloc(
			1, sizeof(opae_api_adapter_table));

		if (!adapter) {
			dlclose(dl_handle);
			OPAE_ERR("out of memory");
			return NULL;
		}
		adapter->plugin.path = (char *)lib_path;
		adapter->plugin.dl_handle = dl_handle;

		return adapter;
	}

	int opae_plugin_mgr_free_adapter_test(opae_api_adapter_table *adapter)
	{
		int res;
		char *err;

		res = dlclose(adapter->plugin.dl_handle);

		if (res) {
			err = dlerror();
			OPAE_ERR("dlclose failed with %d %s", res, err ? err : "");
		}

		free(adapter);

		return res;
	}
};

/*
* @test       plugin
* @brief      Tests: xfpga_plugin_supports_device
*                    xfpga_plugin_supports_host
 @details    When passed with valid argument,the fn returns true <br>
*            When passed with invalid argument,the fn returns false <br>
*/
TEST_P(xfpga_plugin_c_p, test_plugin_1) {
	EXPECT_EQ(xfpga_plugin_supports_device(NULL), true);
	EXPECT_EQ(xfpga_plugin_supports_host(NULL), true);
}

/*
* @test       plugin
* @brief      Tests:xfpga_plugin_initialize
* @details    When passed with NULL argument,the fn returns -1 <br>
*             When passed with valid argument,the fn returns 0 <br>
*/
TEST_P(xfpga_plugin_c_p, test_plugin_2) {

	opae_api_adapter_table *adapter_table = opae_plugin_mgr_alloc_adapter_test("libxfpga.so");

	EXPECT_EQ(opae_plugin_configure(adapter_table, NULL), 0);

	if (adapter_table)
		opae_plugin_mgr_free_adapter_test(adapter_table);

}
INSTANTIATE_TEST_CASE_P(xfpga_plugin_c, xfpga_plugin_c_p,
	::testing::ValuesIn(test_platform::mock_platforms({"skx-p","dcp-rc"})));

class xfpga_plugin_mock_c_p : public ::testing::TestWithParam<std::string> {
protected:
    xfpga_plugin_mock_c_p() {};
};

/*
* @test       test_plugin_neg
* @brief      Tests:xfpga_plugin_initialize
*                   xfpga_plugin_finalize
* @details    When no driver is present, returns FPGA_NO_DRIVER.
*/
TEST_P(xfpga_plugin_mock_c_p, test_plugin_neg) {
	EXPECT_EQ(xfpga_plugin_initialize(), FPGA_NO_DRIVER);
	EXPECT_EQ(xfpga_plugin_finalize(), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(xfpga_plugin_mock_c, xfpga_plugin_mock_c_p,
     ::testing::ValuesIn(test_platform::mock_platforms()));

