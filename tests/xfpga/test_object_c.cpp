// Copyright(c) 2018-2022, Intel Corporation
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

#define NO_OPAE_C
#include "mock/opae_fixtures.h"
KEEP_XFPGA_SYMBOLS

extern "C" {
#include "types_int.h"
#include "xfpga.h"
#include "sysfs_int.h"

int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}

using namespace opae::testing;

const std::string DATA =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

class sysobject_p : public opae_device_p<xfpga_> {
 protected:

  virtual void OPAEInitialize() override {
    ASSERT_EQ(xfpga_plugin_initialize(), 0);
  }

  virtual void OPAEFinalize() override {
    ASSERT_EQ(xfpga_plugin_finalize(), 0);
  }

  virtual fpga_properties device_filter() const override {
    fpga_properties filter = nullptr;

    if (GetProperties(nullptr, &filter) != FPGA_OK)
      return nullptr;

    fpga_guid fme_guid;
    if (uuid_parse(platform_.devices[0].fme_guid, fme_guid) != 0) {
      fpgaDestroyProperties(&filter);
      return nullptr;
    }

    fpgaPropertiesSetGUID(filter, fme_guid);

    return filter;
  }

};

TEST_P(sysobject_p, xfpga_fpgaTokenGetObject) {
  const char *name = "bitstream_id";
  fpga_object object;
  int flags = 0;

  EXPECT_EQ(xfpga_fpgaTokenGetObject(device_token_, name, &object, flags),
            FPGA_OK);
  uint64_t bitstream_id = 0;
  EXPECT_EQ(xfpga_fpgaObjectRead64(object, &bitstream_id, 0),
            FPGA_OK);
  EXPECT_EQ(bitstream_id, platform_.devices[0].bbs_id);
  EXPECT_EQ(xfpga_fpgaTokenGetObject(device_token_, "invalid_name", &object, 0),
            FPGA_NOT_FOUND);
  EXPECT_EQ(xfpga_fpgaTokenGetObject(device_token_, "../../../fpga", &object, 0),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(xfpga_fpgaDestroyObject(&object), FPGA_OK);
}

TEST_P(sysobject_p, xfpga_fpgaHandleGetObject) {
  const char *name = "bitstream_id";
  fpga_object object;
  int flags = 0;

  ASSERT_EQ(xfpga_fpgaHandleGetObject(device_, name, &object, flags), FPGA_OK);
  uint64_t bitstream_id = 0;
  EXPECT_EQ(xfpga_fpgaObjectRead64(object, &bitstream_id, 0),
            FPGA_OK);
  EXPECT_EQ(bitstream_id, platform_.devices[0].bbs_id);
  EXPECT_EQ(xfpga_fpgaHandleGetObject(device_, "invalid_name", &object, 0),
            FPGA_NOT_FOUND);
  EXPECT_EQ(xfpga_fpgaHandleGetObject(device_, "../../../fpga", &object, 0),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(xfpga_fpgaHandleGetObject(device_, "errors/../../../../fpga", &object, 0),
            FPGA_INVALID_PARAM);

  EXPECT_EQ(xfpga_fpgaDestroyObject(&object), FPGA_OK);
}

TEST_P(sysobject_p, xfpga_fpgaObjectGetObject) {
  fpga_object err_object, object, non_object;
  int flags = 0;
  const char *name = "errors";

  EXPECT_EQ(xfpga_fpgaTokenGetObject(device_token_, name, &err_object, flags),
            FPGA_OK);
  ASSERT_EQ(xfpga_fpgaObjectGetObject(err_object, "fme_errors", &object,
                                      flags),
            FPGA_OK);
  EXPECT_EQ(xfpga_fpgaHandleGetObject(err_object, "../../../fpga", &non_object, 0),
            FPGA_INVALID_PARAM);
  uint64_t bbs_errors = 0;
  EXPECT_EQ(xfpga_fpgaObjectRead64(object, &bbs_errors, 0),
            FPGA_OK);
  EXPECT_EQ(xfpga_fpgaDestroyObject(&object), FPGA_OK);
  EXPECT_EQ(xfpga_fpgaDestroyObject(&err_object), FPGA_OK);
}

TEST_P(sysobject_p, xfpga_fpgaDestroyObject) {
  EXPECT_EQ(xfpga_fpgaDestroyObject(NULL), FPGA_INVALID_PARAM);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(sysobject_p);
INSTANTIATE_TEST_SUITE_P(sysobject_c, sysobject_p,
                         ::testing::ValuesIn(test_platform::platforms({
                                                                        "dfl-d5005",
                                                                        "dfl-n3000",
                                                                        "dfl-n6000"
                                                                      })));

class sysobject_mock_p : public sysobject_p {};

TEST_P(sysobject_mock_p, xfpga_fpgaObjectRead) {
  _fpga_token *tk = static_cast<_fpga_token *>(device_token_);
  std::string syspath(tk->sysfspath);
  syspath += "/testdata";
  auto fp = system_->register_file(syspath);
  ASSERT_NE(fp, nullptr) << strerror(errno);
  fwrite(DATA.c_str(), DATA.size(), 1, fp);
  fflush(fp);
  fpga_object object;
  int flags = 0;
  ASSERT_EQ(xfpga_fpgaTokenGetObject(device_token_, "testdata", &object, flags),
            FPGA_OK);
  std::vector<uint8_t> buffer(DATA.size());
  EXPECT_EQ(xfpga_fpgaObjectRead(object, buffer.data(), 0, DATA.size() + 1, 0),
            FPGA_INVALID_PARAM);
  EXPECT_EQ(xfpga_fpgaObjectRead(object, buffer.data(), 0, 10, FPGA_OBJECT_SYNC),
            FPGA_OK);
  buffer[10] = '\0';
  EXPECT_STREQ(reinterpret_cast<const char *>(buffer.data()),
               DATA.substr(0, 10).c_str());
  rewind(fp);
  std::string c0c0str = "0xc0c0cafe\n";
  uint64_t value = 0;
  fwrite(c0c0str.c_str(), c0c0str.size(), 1, fp);
  fflush(fp);
  fclose(fp);
  EXPECT_EQ(xfpga_fpgaObjectRead64(object, &value, FPGA_OBJECT_SYNC), FPGA_OK);
  EXPECT_EQ(value, 0xc0c0cafe);
  EXPECT_EQ(xfpga_fpgaDestroyObject(&object), FPGA_OK);
}

TEST_P(sysobject_mock_p, xfpga_fpgaObjectWrite64) {
  _fpga_handle *h = static_cast<_fpga_handle *>(device_);
  _fpga_token *tok = static_cast<_fpga_token *>(h->token);
  std::string syspath(tok->sysfspath);
  syspath += "/testdata";
  auto fp = system_->register_file(syspath);
  fpga_object object;
  ASSERT_EQ(xfpga_fpgaHandleGetObject(device_, "testdata", &object, 0),
            FPGA_OK);
  EXPECT_EQ(xfpga_fpgaObjectWrite64(object, 0xc0c0cafe, 0), FPGA_OK);
  EXPECT_EQ(xfpga_fpgaObjectWrite64(object, 0xc0c0cafe, 0),
            FPGA_OK);

  _fpga_object *obj = static_cast<_fpga_object *>(object);
  char *path = obj->path;
  std::string invalid_path = "test";

  char *inv_path = new char[invalid_path.length() + 1];
  strcpy(inv_path, invalid_path.c_str());

  obj->path = inv_path;

  EXPECT_EQ(xfpga_fpgaObjectWrite64(object, 0xc0c0cafe, 0), FPGA_EXCEPTION);

  obj->path = path;
  delete[] inv_path;

  rewind(fp);
  std::vector<char> buffer(256);
  auto result = fread(buffer.data(), buffer.size(), 1, fp);
  (void) result;
  fclose(fp);
  EXPECT_EQ(xfpga_fpgaDestroyObject(&object), FPGA_OK);
}

TEST_P(sysobject_mock_p, xfpga_fpgaGetSize) {
  _fpga_token *tk = static_cast<_fpga_token *>(device_token_);
  std::string syspath(tk->sysfspath);
  syspath += "/testdata";
  auto fp = system_->register_file(syspath);
  ASSERT_NE(fp, nullptr) << strerror(errno);
  fwrite(DATA.c_str(), DATA.size(), 1, fp);
  fflush(fp);
  fclose(fp);
  fpga_object object;
  int flags = 0;
  ASSERT_EQ(xfpga_fpgaTokenGetObject(device_token_, "testdata", &object, flags),
            FPGA_OK);
  uint32_t value = 0;
  EXPECT_EQ(xfpga_fpgaObjectGetSize(object, &value, 0), FPGA_OK);
  EXPECT_EQ(value, DATA.size());
  EXPECT_EQ(xfpga_fpgaDestroyObject(&object), FPGA_OK);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(sysobject_mock_p);
INSTANTIATE_TEST_SUITE_P(sysobject_c, sysobject_mock_p,
                         ::testing::ValuesIn(test_platform::mock_platforms({
                                                                             "dfl-d5005",
                                                                             "dfl-n3000",
                                                                             "dfl-n6000"
                                                                           })));
