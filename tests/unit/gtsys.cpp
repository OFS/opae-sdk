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

#ifdef __cplusplus

extern "C" {
#endif
#include <opae/sysobject.h>

#ifdef __cplusplus
}
#endif

#include <string>
#include <vector>
#include <fstream>
#include "types_int.h"
#include "gtest/gtest.h"
#include "common_test.h"

using std::ofstream;
using std::ifstream;

const std::string scratch_text = R"test(
Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum
)test";

class sysobject_f1 : public ::testing::Test
{
      protected:
	sysobject_f1()
	    : buffer_(0), scratch_(scratch_text), sysfspath_(""), matches_(0)
	{
	}

	virtual void SetUp() override
	{
		ASSERT_EQ(fpgaGetProperties(NULL, &filter_), FPGA_OK);
		ASSERT_EQ(
			fpgaPropertiesSetObjectType(filter_, FPGA_ACCELERATOR),
			FPGA_OK);
		ASSERT_EQ(fpgaEnumerate(&filter_, 1, &token_, 1, &matches_),
			  FPGA_OK);
		ASSERT_EQ(matches_, 1);
		ASSERT_EQ(fpgaOpen(token_, &handle_, 0), FPGA_OK);
		struct _fpga_token *tok =
			reinterpret_cast<struct _fpga_token *>(token_);
		sysfspath_ = std::string(tok->sysfspath).replace(0, 4, "/tmp");
		ofstream f;
		f.open(sysfspath_ + "/id", ofstream::out | ofstream::trunc);
		ASSERT_TRUE(f.is_open());
		f << "0x0\n";
		f.close();
		f.open(sysfspath_ + "/scratch",
		       ofstream::out | ofstream::trunc);
		f << scratch_text;
		f.close();
	}

	virtual void TearDown() override
	{
		EXPECT_EQ(fpgaClose(handle_), FPGA_OK);
	}
	std::vector<uint8_t> buffer_;
	std::string scratch_;
	std::string sysfspath_;
	uint32_t matches_;
	fpga_properties filter_;
	fpga_token token_;
	fpga_handle handle_;
};

TEST_F(sysobject_f1, read32)
{
	uint32_t value = 9999;
	EXPECT_EQ(fpgaReadObject32(token_, "id", &value), FPGA_OK);
	EXPECT_EQ(value, 0);
}

TEST_F(sysobject_f1, rdwr64)
{
	uint64_t value = 9999;
	EXPECT_EQ(fpgaReadObject64(token_, "id", &value), FPGA_OK);
	EXPECT_EQ(value, 0);
	EXPECT_EQ(fpgaWriteObject64(handle_, "id", 0xbeef), FPGA_OK);
	EXPECT_EQ(fpgaReadObject64(token_, "id", &value), FPGA_OK);
	EXPECT_EQ(value, 0xbeef);
}

TEST_F(sysobject_f1, rdwr_bytes)
{
	//size_t len = 0;
  // get the object size
	//EXPECT_EQ(fpgaReadObjectBytes(token_, "scratch", nullptr, 0, &len),
	//	  FPGA_OK);
	//EXPECT_EQ(len, scratch_text.size());
  //// resize our destination buffer
	//buffer_.resize(len);
  //// get the buffer contents
	//EXPECT_EQ(
	//	fpgaReadObjectBytes(token_, "scratch", buffer_.data(), 0, &len),
	//	FPGA_OK);
	//EXPECT_STREQ(scratch_text.c_str(),
	//	     reinterpret_cast<const char *>(buffer_.data()));
	//std::string dummy = "the quick brown fox jumped over the lazy dog";
	//uint8_t *ptr = const_cast<uint8_t *>(
	//	reinterpret_cast<const uint8_t *>(dummy.c_str()));
	//EXPECT_EQ(fpgaWriteObjectBytes(handle_, "scratch", ptr, 16, dummy.size()),
	//	  FPGA_OK);
  //ifstream f(sysfspath_ + "/scratch");
  //std::vector<char> buf(scratch_.size());
  //f.read(buf.data(), scratch_.size());
  //f.close();
  //EXPECT_EQ(memcmp(buf.data()+16, dummy.c_str(), dummy.size()), 0);
  //buffer_.clear();
  //buffer_.resize(dummy.size());
  //len = dummy.size();
	//EXPECT_EQ(
	//	fpgaReadObjectBytes(token_, "scratch", buffer_.data(), 16, &len),
	//	FPGA_OK);
  //EXPECT_EQ(memcmp(buffer_.data(), dummy.c_str(), dummy.size()), 0);
}
