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

#include <opae/fpga.h>
#include <linux/ioctl.h>

#include <array>
#include <cstdlib>
#include <cstdarg>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "mock/test_system.h"

using namespace opae::testing;

class object_c_p : public ::testing::TestWithParam<std::string> {
 protected:
  object_c_p()
  : tokens_accel_{{nullptr, nullptr}},
    tokens_device_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    filter_ = nullptr;
    ASSERT_EQ(fpgaInitialize(NULL), FPGA_OK);
    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetDeviceID(filter_, platform_.devices[0].device_id), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    num_matches_accel_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_accel_.data(), tokens_accel_.size(),
                            &num_matches_accel_), FPGA_OK);
    EXPECT_GT(num_matches_accel_, 0);

    accel_ = nullptr;
    ASSERT_EQ(fpgaOpen(tokens_accel_[0], &accel_, 0), FPGA_OK);

    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_DEVICE), FPGA_OK);
    num_matches_device_ = 0;
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_device_.data(), tokens_device_.size(),
                            &num_matches_device_), FPGA_OK);
    EXPECT_GT(num_matches_device_, 0);

    EXPECT_EQ(fpgaTokenGetObject(tokens_device_[0], "ports_num", &token_obj_, 0),
		    FPGA_OK);
    EXPECT_EQ(fpgaHandleGetObject(accel_, "afu_id", &handle_obj_, 0),
		    FPGA_OK);
    afu_guid_ = platform_.devices[0].afu_guid;
    system_->normalize_guid(afu_guid_, false);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyObject(&handle_obj_), FPGA_OK);
    EXPECT_EQ(fpgaDestroyObject(&token_obj_), FPGA_OK);
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (accel_) {
        EXPECT_EQ(fpgaClose(accel_), FPGA_OK);
        accel_ = nullptr;
    }
    for (auto &t : tokens_accel_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    for (auto &t : tokens_device_) {
      if (t) {
        EXPECT_EQ(fpgaDestroyToken(&t), FPGA_OK);
        t = nullptr;
      }
    }
    fpgaFinalize();
    system_->finalize();
  }

  std::array<fpga_token, 2> tokens_accel_;
  std::array<fpga_token, 2> tokens_device_;
  fpga_object token_obj_;
  fpga_object handle_obj_;
  fpga_properties filter_;
  fpga_handle accel_;
  uint32_t num_matches_accel_;
  uint32_t num_matches_device_;
  test_platform platform_;
  test_system *system_;
  std::string afu_guid_;
};

/**
 * @test       obj_read
 * @brief      Test: fpgaObjectRead
 * @details    When fpgaObjectRead is called with valid params,<br>
 *             the fn retrieves the value of the targeted object<br>
 *             and returns FPGA_OK.<br>
 */
TEST_P(object_c_p, obj_read) {
  char afu_id[33];
  EXPECT_EQ(fpgaObjectRead(handle_obj_, (uint8_t *) afu_id, 0,
                           32, 0), FPGA_OK);
  afu_id[32] = 0;
  EXPECT_STREQ(afu_id, afu_guid_.c_str());
}

/**
 * @test       obj_read64
 * @brief      Test: fpgaObjectRead64
 * @details    When fpgaObjectRead64 is called with valid params,<br>
 *             the fn retrieves the value of the targeted object<br>
 *             and returns FPGA_OK.<br>
 */
TEST_P(object_c_p, obj_read64) {
  uint64_t val = 0;
  EXPECT_EQ(fpgaObjectRead64(token_obj_, &val, 0), FPGA_OK);
  EXPECT_EQ(val, 1ul);
}

/**
 * @test       obj_write64
 * @brief      Test: fpgaObjectWrite64
 * @details    When fpgaObjectWrite64 is called with valid params,<br>
 *             the fn sets the value of the targeted object<br>
 *             and returns FPGA_OK.<br>
 */
TEST_P(object_c_p, obj_write64) {
  uint64_t errors = 0xbaddecaf;
  fpga_object obj = nullptr;

  // read the port errors
  ASSERT_EQ(fpgaHandleGetObject(accel_, "errors/errors", &obj, 0),
		    FPGA_OK);
  ASSERT_EQ(fpgaObjectRead64(obj, &errors, 0), FPGA_OK);
  EXPECT_EQ(fpgaDestroyObject(&obj), FPGA_OK);

  // clear the port errors
  ASSERT_EQ(fpgaHandleGetObject(accel_, "errors/errors", &obj, 0),
		    FPGA_OK);
  ASSERT_EQ(fpgaObjectWrite64(obj, errors, 0), FPGA_OK);
  EXPECT_EQ(fpgaDestroyObject(&obj), FPGA_OK);
}

/**
 * @test       obj_get_obj0
 * @brief      Test: fpgaObjectGetObject
 * @details    When fpgaObjectGetObject is called with valid parameters,<br>
 *             the fn opens the underlying object<br>
 *             and returns FPGA_OK.<br>
 */
TEST_P(object_c_p, obj_get_obj0) {
  fpga_object errors_obj = nullptr;
  fpga_object clear_obj = nullptr;

  ASSERT_EQ(fpgaHandleGetObject(accel_, "errors", &errors_obj, 0),
		    FPGA_OK);
  ASSERT_EQ(fpgaObjectGetObject(errors_obj, "errors",
                                &clear_obj, 0), FPGA_OK);
  ASSERT_EQ(fpgaObjectWrite64(clear_obj, 0, 0), FPGA_OK);
  EXPECT_EQ(fpgaDestroyObject(&clear_obj), FPGA_OK);
  EXPECT_EQ(fpgaDestroyObject(&errors_obj), FPGA_OK);
}

/**
 * @test       obj_get_obj1
 * @brief      Test: fpgaObjectGetObject
 * @details    When fpgaObjectGetObject is called with a name that has a null
 *             byte, the function returns FPGA_NOT_FOUND. <br>
 *             and returns FPGA_OK.<br>
 */
TEST_P(object_c_p, obj_get_obj1) {
  fpga_object errors_obj = nullptr;
  fpga_object obj = nullptr;
  const char *bad_name = "err\0rs";

  ASSERT_EQ(fpgaHandleGetObject(accel_, "errors", &errors_obj, 0), FPGA_OK);
  EXPECT_EQ(fpgaObjectGetObject(errors_obj, bad_name, &obj, 0),FPGA_NOT_FOUND);

  ASSERT_NE(fpgaDestroyObject(&obj), FPGA_OK);
  ASSERT_EQ(fpgaDestroyObject(&errors_obj), FPGA_OK);
}

/**
 * @test       handle_get_obj
 * @brief      Test: fpgaHandleGetObject
 * @details    When fpgaHandleGetObject is called with a name that has a null
 *             byte, the function returns FPGA_NOT_FOUND. <br>
 */
TEST_P(object_c_p, handle_get_obj) {
  fpga_object obj = nullptr;
  const char *bad_name = "err\0rs";

  EXPECT_EQ(fpgaHandleGetObject(accel_, bad_name, &obj, 0), FPGA_NOT_FOUND);
  ASSERT_NE(fpgaDestroyObject(&obj), FPGA_OK);
}

/**
 * @test       token_get_obj
 * @brief      Test: fpgaTokenGetObject
 * @details    When fpgaTokenGetObject is called with a name that has a null
 *             byte, the function returns FPGA_NOT_FOUND. <br>
 */
TEST_P(object_c_p, token_get_obj) {
  fpga_object obj = nullptr;
  const char *bad_name = "err\0rs";

  EXPECT_EQ(fpgaTokenGetObject(tokens_device_[0], bad_name, &obj, 0), 
                                FPGA_NOT_FOUND);
  ASSERT_NE(fpgaDestroyObject(&obj), FPGA_OK);
}

/**
 * @test       obj_get_size
 * @brief      Test: fpgaObjectGetSize
 * @details    Given an object created using name afu_id<br>
 *             When fpgaObjectGetSize is called with that object<br>
 *             Then the size retrieved equals the length of the afu_id
 *             string + one for the new line character<br>
 */
TEST_P(object_c_p, obj_get_size) {
  uint32_t value = 0;
  EXPECT_EQ(fpgaObjectGetSize(handle_obj_, &value, FPGA_OBJECT_SYNC), FPGA_OK);
  EXPECT_EQ(value, afu_guid_.size() + 1);
}

INSTANTIATE_TEST_CASE_P(object_c, object_c_p,
                        ::testing::ValuesIn(test_platform::platforms({ "dfl-n3000","dfl-d5005" })));

class object_c_mock_p : public object_c_p {
  protected:
    object_c_mock_p() {};
};

/**
 * @test       tok_get_err
 * @brief      Test: fpgaTokenGetObject
 * @details    When the call to opae_allocate_wrapped_object fails,<br>
 *             fpgaTokenGetObject destroys the underlying object<br>
 *             and returns FPGA_NO_MEMORY.<br>
 */
TEST_P(object_c_mock_p, tok_get_err) {
  fpga_object obj = nullptr;
  system_->invalidate_malloc(0, "opae_allocate_wrapped_object");
  EXPECT_EQ(fpgaTokenGetObject(tokens_device_[0], "ports_num",
                               &obj, 0), FPGA_NO_MEMORY);
}

/**
 * @test       handle_get_err
 * @brief      Test: fpgaHandleGetObject
 * @details    When the call to opae_allocate_wrapped_object fails,<br>
 *             fpgaHandleGetObject destroys the underlying object<br>
 *             and returns FPGA_NO_MEMORY.<br>
 */
TEST_P(object_c_mock_p, handle_get_err) {
  fpga_object obj = nullptr;
  system_->invalidate_malloc(0, "opae_allocate_wrapped_object");
  EXPECT_EQ(fpgaHandleGetObject(accel_, "id",
                               &obj, 0), FPGA_NO_MEMORY);
}

/**
 * @test       obj_get_obj_err
 * @brief      Test: fpgaObjectGetObject
 * @details    When opae_allocate_wrapped_object fails,<br>
 *             fpgaObjectGetObject frees the underlying object<br>
 *             and returns FPGA_NO_MEMORY.<br>
 */
TEST_P(object_c_mock_p, obj_get_obj_err) {
  fpga_object errors_obj = nullptr;
  fpga_object clear_obj = nullptr;

  ASSERT_EQ(fpgaHandleGetObject(accel_, "errors", &errors_obj, 0),
		    FPGA_OK);

  system_->invalidate_malloc(0, "opae_allocate_wrapped_object");
  ASSERT_EQ(fpgaObjectGetObject(errors_obj, "errors",
                                &clear_obj, 0), FPGA_NO_MEMORY);

  EXPECT_EQ(fpgaDestroyObject(&errors_obj), FPGA_OK);
}

INSTANTIATE_TEST_CASE_P(object_c, object_c_mock_p,
                        ::testing::ValuesIn(test_platform::mock_platforms({ "dfl-n3000","dfl-d5005" })));

