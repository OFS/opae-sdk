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
#include <opae/fpga.h>

#ifdef __cplusplus

extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#include "gtest/gtest.h"
#include "test_system.h"
#include <opae/access.h>
#include <opae/mmio.h>
#include <sys/mman.h>
#include "types_int.h"


using namespace opae::testing;

int mmio_ioctl(mock_object * m, int request, va_list arg){
  struct fpga_port_region_info *rinfo = va_arg(arg, struct fpga_port_region_info *);
  if (!rinfo) {
    FPGA_MSG("rinfo is NULL");
    goto out_EINVAL;
  }
  if (rinfo->argsz != sizeof(*rinfo)) {
    FPGA_MSG("wrong structure size");
    goto out_EINVAL;
  }
  if (rinfo->index != 0) {
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
    return 0;
}

class mmio_c_p
    : public ::testing::TestWithParam<std::string> {
 protected:
  mmio_c_p() : tmpsysfs_("mocksys-XXXXXX"), handle_(nullptr) {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    tmpsysfs_ = system_->prepare_syfs(platform_);

    ASSERT_EQ(fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_),
              FPGA_OK);
    ASSERT_EQ(fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);
    system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO,mmio_ioctl);
  }

  virtual void TearDown() override {
    EXPECT_EQ(fpgaDestroyProperties(&filter_), FPGA_OK);
    if (handle_ != nullptr) EXPECT_EQ(fpgaClose(handle_), FPGA_OK);
    if (!tmpsysfs_.empty() && tmpsysfs_.size() > 1) {
      std::string cmd = "rm -rf " + tmpsysfs_;
      std::system(cmd.c_str());
    }
    system_->finalize();
  }

  std::string tmpsysfs_;
  fpga_properties filter_;
  std::array<fpga_token, 2> tokens_;
  fpga_handle handle_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
};

TEST_P (mmio_c_p, test_mmio) {
  uint64_t * mmio_ptr = NULL;
  EXPECT_TRUE(((struct _fpga_handle*)handle_)->mmio_root == NULL);

  ASSERT_EQ(FPGA_OK, fpgaMapMMIO(handle_, 0, &mmio_ptr));
  EXPECT_FALSE(mmio_ptr == NULL);
  EXPECT_FALSE(((struct _fpga_handle *)handle_)->mmio_root == NULL);
}


//TEST_P (mmio_c_p, test_read_write_32) {
//
//}
INSTANTIATE_TEST_CASE_P(mmio_c, mmio_c_p, ::testing::ValuesIn(test_platform::keys(true)));
