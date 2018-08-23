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
#include <opae/utils.h>
#include "wsid_list_int.h"

#ifdef __cplusplus
}
#endif

#include "gtest/gtest.h"


TEST(utils_h, fpgaErrStr)
{
	fpga_result e = FPGA_OK;
	auto res = fpgaErrStr(e);
	(void)res;
}


TEST(wsid_list_int_h, wsid_add)
{
	struct wsid_map **root = 0;
	uint64_t wsid = 0;
	uint64_t addr = 0;
	uint64_t phys = 0;
	uint64_t len = 0;
	uint64_t offset = 0;
	uint64_t index = 0;
	int flags = 0;
	auto res = wsid_add(root, wsid, addr, phys, len, offset, index, flags);
	EXPECT_EQ(res, 0);
}


TEST(wsid_list_int_h, wsid_del)
{
	struct wsid_map **root = 0;
	uint64_t wsid = 0;
	auto res = wsid_del(root, wsid);
	EXPECT_EQ(res, 0);
}


TEST(wsid_list_int_h, wsid_cleanup)
{
	struct wsid_map **root = 0;
	wsid_cleanup(root, NULL);
}


TEST(wsid_list_int_h, wsid_gen)
{
	auto res = wsid_gen();
	(void)res;
}


TEST(wsid_list_int_h, wsid_find)
{
	struct wsid_map *root = 0;
	uint64_t wsid = 0;
	auto res = wsid_find(root, wsid);
	(void)res;
}


TEST(wsid_list_int_h, wsid_find_by_index)
{
	struct wsid_map *root = 0;
	uint32_t index = 0;
	auto res = wsid_find_by_index(root, index);
	(void)res;
}
