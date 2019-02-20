// Copyright(c) 2019, Intel Corporation
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
#include <opae/types.h>
#include "types_int.h"
}

#include <opae/fpga.h>

#include <array>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "gtest/gtest.h"
#include "test_system.h"

using namespace opae::testing;

// ASE ID
#define ASE_TOKEN_MAGIC    0x46504741544f4b40
static const fpga_guid ASE_GUID = {
			0xd8, 0x42, 0x4d, 0xc4, 0xa4,  0xa3, 0xc4, 0x13, 0xf8,0x9e,
						0x43, 0x36, 0x83, 0xf9, 0x04, 0x0b
};

inline void token_for_afu0(struct _fpga_token* _tok)
{
    memcpy(_tok->accelerator_id,ASE_GUID, sizeof(fpga_guid));
    _tok->magic = ASE_TOKEN_MAGIC;
    _tok->ase_objtype=FPGA_ACCELERATOR;
}

class open_c_ase_p : public testing::Test {
  protected:
    open_c_ase_p() : tok(nullptr) {}

    virtual void SetUp() override {
        system_ = test_system::instance();
        system_->initialize();

        tok = &_tok;
        token_for_afu0(&_tok);
        accel_ = nullptr;
    }

    virtual void TearDown() override {
        system_->finalize();
    }

    struct _fpga_token _tok;
    fpga_token tok;
    fpga_handle accel_;
    test_system *system_;
};

/**
 * @test       open_01
 *
 * @brief      When the fpga_handle * parameter to fpgaOpen is nullptr, the
 *             function returns FPGA_INVALID_PARAM.
 */
TEST_F(open_c_ase_p, ase_open_01) {
    EXPECT_EQ(FPGA_INVALID_PARAM, fpgaOpen(tok, nullptr, 0));
}

/**
 * @test       open_02
 *
 * @brief      When the fpga_token parameter to fpgaOpen is nullptr, the
 *             function returns FPGA_INVALID_PARAM.
 */
TEST_F(open_c_ase_p, ase_open_02) {
    EXPECT_EQ(FPGA_INVALID_PARAM, fpgaOpen(nullptr, &accel_, 0));
}

/**
 * @test       open_03
 *
 * @brief      When the flags parameter to fpgaOpen is invalid, the
 *             function returns FPGA_INVALID_PARAM.
 *
 */
TEST_F(open_c_ase_p, ase_open_03) {
    EXPECT_EQ(FPGA_INVALID_PARAM, fpgaOpen(tok, &accel_, 42));
}

/**
 * @test       mallocfails
 *
 * @brief      When the malloc function called by fpgaOpen() failed, the function returns
 *             FPGA_INVALID_PARAM.
 */
TEST_F(open_c_ase_p, mallocfails) {
    // Invalidate the allocation of the wrapped handle.
    system_->invalidate_malloc(0, "ase_malloc");
    ASSERT_EQ(fpgaOpen(tok, &accel_, 0), FPGA_NO_MEMORY);
    EXPECT_EQ(accel_, nullptr);
}

