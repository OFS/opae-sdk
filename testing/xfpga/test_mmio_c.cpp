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


#include "intel-fpga.h"
#include "fpga-dfl.h"
#include "gtest/gtest.h"
#include "test_system.h"
#include <opae/access.h>
#include <opae/mmio.h>
#include <sys/mman.h>
#include <cstdarg>
#include <linux/ioctl.h>

#include "xfpga.h"
#include "types_int.h"
#include "sysfs_int.h"

#undef FPGA_MSG
#define FPGA_MSG(fmt, ...) \
	printf("MOCK " fmt "\n", ## __VA_ARGS__)

extern "C" {
int xfpga_plugin_initialize(void);
int xfpga_plugin_finalize(void);
}

using namespace opae::testing;

#ifndef BUILD_ASE

/*
 * On hardware, the mmio map is a hash table.
 */
static bool mmio_map_is_empty(struct wsid_tracker *root) {
  if (!root || (root->n_hash_buckets == 0))
    return true;

  for (uint32_t i = 0; i < root->n_hash_buckets; i += 1) {
    if (root->table[i])
      return false;
  }

  return true;
}

#else

/*
 * In ASE, the mmio map is a list.
 */
static bool mmio_map_is_empty(struct wsid_map *root) {
  return !root;
}

#endif



int mmio_ioctl(mock_object * m, int request, va_list argp){
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
    if (rinfo->index > 1 ) {
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
out:
    return retval;

out_EINVAL:
    retval = -1;
    errno = EINVAL;
    goto out;
}

class mmio_c_p
    : public ::testing::TestWithParam<std::string> {
 protected:
  mmio_c_p()
  : handle_(nullptr),
    tokens_{{nullptr, nullptr}} {}

  virtual void SetUp() override {
    ASSERT_TRUE(test_platform::exists(GetParam()));
    platform_ = test_platform::get(GetParam());
    system_ = test_system::instance();
    system_->initialize();
    system_->prepare_syfs(platform_);

    ASSERT_EQ(xfpga_plugin_initialize(), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaGetProperties(nullptr, &filter_), FPGA_OK);
    ASSERT_EQ(fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR), FPGA_OK);
    ASSERT_EQ(xfpga_fpgaEnumerate(&filter_, 1, tokens_.data(), tokens_.size(),
                            &num_matches_),
              FPGA_OK);
    ASSERT_EQ(xfpga_fpgaOpen(tokens_[0], &handle_, 0), FPGA_OK);
    system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
    system_->register_ioctl_handler(DFL_FPGA_PORT_GET_REGION_INFO, mmio_ioctl);
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
    xfpga_plugin_finalize();
    system_->finalize();
  }

  fpga_handle handle_;
  std::array<fpga_token, 2> tokens_;
  fpga_properties filter_;
  uint32_t num_matches_;
  test_platform platform_;
  test_system *system_;
  const uint64_t CSR_SCRATCHPAD0 = 0x100;
  const uint64_t MMIO_OUT_REGION_ADDRESS = 1024 * 1024 * 256;;
};




/**
* @test       mmio_c_p
* @brief      Tests: test_pos_map_mmio
* @details    When the parameters are valid and the drivers are loaded:
*             xfpga_fpgaMapMMIOPtr initializes handle->mmio_root != NULL
*
*/
TEST_P (mmio_c_p, test_pos_map_mmio) {
  uint64_t * mmio_ptr = NULL;
  EXPECT_TRUE(mmio_map_is_empty(((struct _fpga_handle*)handle_)->mmio_root));

  // Open  port device
#ifndef BUILD_ASE
  ASSERT_EQ(FPGA_OK, xfpga_fpgaMapMMIO(handle_, 0, &mmio_ptr));
  EXPECT_NE(mmio_ptr,nullptr);
  EXPECT_FALSE(mmio_map_is_empty(((struct _fpga_handle*)handle_)->mmio_root));
  EXPECT_EQ(FPGA_OK, xfpga_fpgaUnmapMMIO(handle_, 0));
#else
  // ASE
  ASSERT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaMapMMIO(handle_, 0, &mmio_ptr));
  EXPECT_TRUE(mmio_map_is_empty(((struct _fpga_handle*)handle_)->mmio_root));
  EXPECT_NE(mmio_ptr,nullptr);
#endif 
}


/**
* @test       mmio_c_p
* @brief      Tests: test_neg_map_mmio
* @details    When the parameters are valid and the drivers are loaded:
*             xfpga_fpgaMapMMIOPtr must fail for non-existent MMIO area,
*             xfpga_fpgaUnmapMMIOPtr must fail for non-existent MMIO area.
*/
TEST_P (mmio_c_p, test_neg_map_mmio) {
  uint64_t * mmio_ptr = NULL;
  EXPECT_TRUE(mmio_map_is_empty(((struct _fpga_handle*)handle_)->mmio_root));

  // Open  port device
#ifndef BUILD_ASE
  EXPECT_NE(FPGA_OK, xfpga_fpgaMapMMIO(handle_, -1, &mmio_ptr));
#else
  EXPECT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaMapMMIO(handle_, -1, &mmio_ptr));
#endif

  // Do not modify mmio_ptr and mmio_root
  EXPECT_TRUE(mmio_map_is_empty(((struct _fpga_handle*)handle_)->mmio_root));

// Unmap memory range otherwise, will not accept open from same process
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaMapMMIO(NULL, 0, NULL));
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaUnmapMMIO(NULL, 0));
  EXPECT_NE(FPGA_OK, xfpga_fpgaUnmapMMIO(handle_, 0));
#endif
}


/**
* @test       mmio_c_p
* @brief      Tests: test_port_region_err
* @details    When the parameters are invalid and the drivers are loaded:
*             FPGA_NO_ACCESS is return when calling xfpga_fpgaMapMMIO
*/
TEST_P (mmio_c_p, test_port_map_region_err) {
  uint64_t * mmio_ptr = NULL;

  system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, dummy_ioctl<-1,EINVAL>);
  EXPECT_EQ(FPGA_NO_ACCESS, xfpga_fpgaMapMMIO(handle_,-1,&mmio_ptr));

  system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, dummy_ioctl<-1,EFAULT>);
  EXPECT_EQ(FPGA_NO_ACCESS, xfpga_fpgaMapMMIO(handle_,-1,&mmio_ptr));

  system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, dummy_ioctl<-1,ENOTSUP>);
  EXPECT_EQ(FPGA_NO_ACCESS, xfpga_fpgaMapMMIO(handle_,-1,&mmio_ptr));

}


TEST_P (mmio_c_p, test_port_unmap_region_err) {
  uint64_t * mmio_ptr = NULL;
  EXPECT_NE(FPGA_OK, xfpga_fpgaMapMMIO(handle_,-1,&mmio_ptr));

  system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, dummy_ioctl<-1,EINVAL>);
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaUnmapMMIO(handle_, 0));

  system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, dummy_ioctl<-1,EFAULT>);
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaUnmapMMIO(handle_, 0));

  system_->register_ioctl_handler(FPGA_PORT_GET_REGION_INFO, dummy_ioctl<-1,ENOTSUP>);
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaUnmapMMIO(handle_, 0));
}




/**
* @test       mmio_c_p
* @brief      Test: test_pos_read_write_32
* @details    When the parameters are valid and the drivers are loaded:
*             xfpga_fpgaWriteMMIO32 must write correct value at given MMIO
*             offset.  xfpga_fpgaReadMMIO32 must read correct value at given
*             MMIO offset.
*/
TEST_P (mmio_c_p, test_pos_read_write_32) {
  uint64_t* mmio_ptr = NULL;
  uint32_t value = 0;
  uint32_t read_value = 0;

  // Open  port device
#ifndef BUILD_ASE
  ASSERT_EQ(FPGA_OK, xfpga_fpgaMapMMIO(handle_, 0, &mmio_ptr));
  EXPECT_NE(mmio_ptr,nullptr);
#else
  ASSERT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaMapMMIO(handle_, 0, &mmio_ptr));
  EXPECT_EQ(mmio_ptr,nullptr);
#endif

  // Write value and check correctness
  for (value = 0; value < 100; value += 10) {
    EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO32(handle_, 0, CSR_SCRATCHPAD0, value));
    EXPECT_EQ(FPGA_OK, xfpga_fpgaReadMMIO32(handle_, 0, CSR_SCRATCHPAD0, &read_value));
    EXPECT_EQ(read_value, value);
  }

// Unmap memory range otherwise, will not accept open from same process
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, xfpga_fpgaUnmapMMIO(handle_, 0));
#endif
}


/**
* @test       mmio_c_p
* @brief      Test: test_neg_read_write_32
* @details    When the parameters are valid and the drivers are loaded:
*             xfpga_fpgaWriteMMIO32 must write correct value at given MMIO
*             offset.  xfpga_fpgaReadMMIO32 must read correct value at given
*             MMIO offset.
*/
TEST_P (mmio_c_p, test_neg_read_write_32) {
  uint64_t* mmio_ptr = NULL;
  uint32_t value = 0;
  uint32_t read_value = 0;

  // Open  port device
#ifndef BUILD_ASE
  ASSERT_EQ(FPGA_OK, xfpga_fpgaMapMMIO(handle_, 0, &mmio_ptr));
  EXPECT_NE(mmio_ptr,nullptr);
#else
  ASSERT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaMapMMIO(handle_, 0, &mmio_ptr));
  EXPECT_EQ(mmio_ptr,nullptr);
#endif

  // Check errors for misaligned or out of boundary memory accesses
  EXPECT_NE(FPGA_OK, xfpga_fpgaWriteMMIO32(handle_, 0, CSR_SCRATCHPAD0 + 1, value));
  EXPECT_NE(FPGA_OK, xfpga_fpgaReadMMIO32(handle_, 0, CSR_SCRATCHPAD0 + 1, &read_value));
  EXPECT_NE(FPGA_OK, xfpga_fpgaWriteMMIO32(handle_, 0, MMIO_OUT_REGION_ADDRESS, value));
  EXPECT_NE(FPGA_OK, xfpga_fpgaReadMMIO32(handle_, 0, MMIO_OUT_REGION_ADDRESS, &read_value));

// Unmap memory range otherwise, will not accept open from same process
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaReadMMIO32(NULL, 0, CSR_SCRATCHPAD0, &read_value));
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaWriteMMIO32(NULL, 0, MMIO_OUT_REGION_ADDRESS, value));
  EXPECT_EQ(FPGA_OK, xfpga_fpgaUnmapMMIO(handle_, 0));
#endif
}


/**
*  @test      mmio_c_p
*  @brief     Test: test_pos_read_write_64
*  @details   When the parameters are valid and the drivers are loaded:
*             xfpga_fpgaWriteMMIO64 must write correct value at given MMIO
*             offset.  xfpga_fpgaReadMMIO64 must read correct value at given
*             MMIO offset.
*
*/
TEST_P (mmio_c_p, test_mmio_read_write_64) {
  uint64_t* mmio_ptr = NULL;
  uint64_t value = 0;
  uint64_t read_value = 0;

#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, xfpga_fpgaMapMMIO(handle_, 0, &mmio_ptr));
  EXPECT_NE(mmio_ptr,nullptr);
#else
  ASSERT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaMapMMIO(handle_, 0, &mmio_ptr));
  EXPECT_EQ(mmio_ptr,nullptr);
  mmio_ptr = 0;
#endif

  // Write value and check correctness
  for (value = 0; value < 100; value += 10) {
    EXPECT_EQ(FPGA_OK, xfpga_fpgaWriteMMIO64(handle_, 0, CSR_SCRATCHPAD0, value));
#ifndef BUILD_ASE
    EXPECT_EQ(value,*((volatile uint64_t*)(mmio_ptr + CSR_SCRATCHPAD0 / sizeof(uint64_t))));
#endif
    EXPECT_EQ(FPGA_OK, xfpga_fpgaReadMMIO64(handle_, 0, CSR_SCRATCHPAD0, &read_value));
    EXPECT_EQ(read_value, value);
  }

// Unmap memory range otherwise, will not accept open from same process
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, xfpga_fpgaUnmapMMIO(handle_, 0));
#endif
}

/**
* @test       mmio_c_p
* @brief      Test: test_neg_read_write_64
* @details    When the parameters are valid and the drivers are loaded:
*             xfpga_fpgaWriteMMIO64 must fail for misaligned offset.
*             xfpga_fpgaReadMMIO64 must fail for misaligned offset.
*             xfpga_fpgaWriteMMIO64 must fail for out-of-region offset.
*             xfpga_fpgaReadMMIO64 must fail for out-of-region offset.
*
*/

TEST_P (mmio_c_p, test_neg_read_write_64) {
  uint64_t* mmio_ptr = NULL;
  uint64_t value = 0;
  uint64_t read_value = 0;

#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_OK, xfpga_fpgaMapMMIO(handle_, 0, &mmio_ptr));
  EXPECT_NE(mmio_ptr,nullptr);
#else
  EXPECT_EQ(FPGA_NOT_SUPPORTED, xfpga_fpgaMapMMIO(handle_, 0, &mmio_ptr));
  EXPECT_EQ(mmio_ptr,nullptr);
#endif

  // Check errors for misalinged or out of boundary memory accesses
  EXPECT_NE(FPGA_OK, xfpga_fpgaWriteMMIO64(handle_, 0, CSR_SCRATCHPAD0 + 1, value));
  EXPECT_NE(FPGA_OK, xfpga_fpgaReadMMIO64(handle_, 0, CSR_SCRATCHPAD0 + 1, &read_value));
  EXPECT_NE(FPGA_OK, xfpga_fpgaWriteMMIO64(handle_, 0, MMIO_OUT_REGION_ADDRESS, value));
  EXPECT_NE(FPGA_OK, xfpga_fpgaReadMMIO64(handle_, 0, MMIO_OUT_REGION_ADDRESS, &read_value));

// Unmap memory range otherwise, will not accept open from same process
#ifndef BUILD_ASE
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaReadMMIO64(NULL, 0, CSR_SCRATCHPAD0, &read_value));
  EXPECT_EQ(FPGA_INVALID_PARAM, xfpga_fpgaWriteMMIO64(NULL, 0, CSR_SCRATCHPAD0, value));
  EXPECT_EQ(FPGA_OK, xfpga_fpgaUnmapMMIO(handle_, 0));
#endif
}


INSTANTIATE_TEST_CASE_P(mmio_c, mmio_c_p, ::testing::ValuesIn(test_platform::keys(true)));
