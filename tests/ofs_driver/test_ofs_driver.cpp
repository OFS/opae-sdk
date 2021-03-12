// Copyright(c) 2021, Intel Corporation
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
#include <chrono>
#include <future>
#include <thread>
#include <uuid/uuid.h>
#include <ofs/ofs.h>
#include "gtest/gtest.h"
#include "ofs_test.h"

union uuid_bytes {
  struct {
    uint64_t lo;
    uint64_t hi;
  };
  uuid_t u;
};

int parse_reverse(const char *guid_str, uuid_t reversed)
{
  uuid_t u;
  auto r = uuid_parse(guid_str, u);
  if (r) return r;
  for (int i = 0; i < 16; ++i) {
    reversed[i] = u[15-i];
  }
  return r;
}

/**
 * @test    ofs_test_init
 * @brief   Tests: ofs_test_init
 * @details ofs_test is a sample ofs user mode driver defined in ofs_test.yml
 *          The build system should generate the ofs_test.h file and use that
 *          for making an ofs_test library. ofs_test_init is the initializer
 *          function used to connect data structures in ofs_test to correct
 *          offsets according to ofs_test.yml. This test calls ofs_test_init
 *          without a valid fpga_handle so it is expected to return non-zero
 * */
TEST(ofs_driver, ofs_test_init)
{
  ofs_test otest;
  fpga_handle h = nullptr;
  EXPECT_NE(0, ofs_test_init(&otest, h));
}


/**
 * @test    ofs_test_read_guid
 * @brief   Tests: ofs_test_read_guid
 * @details The user-mode driver Python code in ofs_test.yml for function
 *          read_guid reads in the guid registers id_lo and id_hi in reverse
 *          order. This test gets a uuid string, parses it and reverses it into
 *          a member of uuid_bytes union. Because we haven't initialized the
 *          ofs_test data structure, our register variables are invalid. Point
 *          them to the corresponding data structures in our uuid_bytes
 *          variable. Call ofs_test_read_guid and use uuid_unparse to get the
 *          uuid string. This should be equal to the original string we used to
 *          parse/reverse.
 * */
TEST(ofs_driver, ofs_test_read_guid)
{
  ofs_test otest;
  uuid_bytes u;
  const char * guid_str = "4bd15660-dec9-4d32-bd3d-e46e87d7d292";
  ASSERT_EQ(parse_reverse(guid_str, u.u), 0);

  otest.r_id_hi = reinterpret_cast<volatile _id_hi*>(&u.hi);
  otest.r_id_lo = reinterpret_cast<volatile _id_lo*>(&u.lo);
  uuid_t u2;
  ofs_test_read_guid(&otest, u2);
  char unparsed[56];
  uuid_unparse(u2, unparsed);
  EXPECT_STREQ(guid_str, unparsed);
}
