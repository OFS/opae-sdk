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

#include "gtest/gtest.h"
#include "ofs_primitives.h"

using hrc = std::chrono::high_resolution_clock;

TEST(libofs, timespec_usec_macro)
{
  double t = 1.1;
  long t_usec = static_cast<long>(t * 1E6);
  OFS_TIMESPEC_USEC(ts, t_usec);
  double converted = static_cast<double>(ts.tv_sec + ts.tv_nsec*1E-9);
  ASSERT_EQ(t, converted);
}

TEST(libofs, diff_timespec)
{
  struct timespec lhs, rhs, result;

  lhs.tv_sec = 1;
  lhs.tv_nsec = 1000;

  rhs.tv_sec = 1;
  rhs.tv_nsec = 900;

  EXPECT_EQ(ofs_diff_timespec(&result, &lhs, &rhs), 0);
  EXPECT_EQ(result.tv_sec, 0);
  EXPECT_EQ(result.tv_nsec, 100);

  EXPECT_EQ(ofs_diff_timespec(&result, &rhs, &lhs), 1);
  EXPECT_EQ(result.tv_sec, -1);
  EXPECT_EQ(result.tv_nsec, 999999900);

}

template<typename T>
uint64_t wait_test(int(*wait_fn)(T*, T, uint64_t, uint32_t), bool modify, uint64_t modify_usec, uint64_t timeout_usec)
{
  T bit = 0;
  auto modify_fn =
    [&bit](int sleep_usec, bool do_modify) {
      struct timespec ts = {
        .tv_sec = 0,
        .tv_nsec = sleep_usec * 1000
      };
      nanosleep(&ts, nullptr);
      if (do_modify) bit = 0b111;
    };

  bit = 0b101;
  std::future<void> f = std::async(std::launch::async, modify_fn, modify_usec, modify);
  auto begin = hrc::now();
  auto status = wait_fn(&bit, 0b111, timeout_usec, 10);
  auto end = hrc::now();
  f.wait();
  if (modify) {
    EXPECT_EQ(bit, 0b111);
    EXPECT_EQ(status, 0);
  } else {
    EXPECT_EQ(bit, 0b101);
    EXPECT_EQ(status, 1);
  }
  auto delta_usec = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
  return delta_usec;
}

TEST(libofs, wait_for_eq)
{
  uint64_t modify_usec = 1000;
  uint64_t timeout_usec = 1500;
  uint64_t delta_usec = 0;

  delta_usec = wait_test<uint32_t>(ofs_wait_for_eq32, true, modify_usec, timeout_usec);
  EXPECT_GE(delta_usec, modify_usec);
  EXPECT_LT(delta_usec, timeout_usec);

  delta_usec = wait_test<uint64_t>(ofs_wait_for_eq64, true, modify_usec, timeout_usec);
  EXPECT_GE(delta_usec, modify_usec);
  EXPECT_LT(delta_usec, timeout_usec);
}

TEST(libofs, wait_for_eq_timeout)
{
  uint64_t modify_usec = 1000;
  uint64_t timeout_usec = 1500;
  uint64_t delta_usec = 0;

  delta_usec = wait_test<uint32_t>(ofs_wait_for_eq32, false, modify_usec, timeout_usec);
  EXPECT_GE(delta_usec, timeout_usec);

  delta_usec = wait_test<uint64_t>(ofs_wait_for_eq64, false, modify_usec, timeout_usec);
  EXPECT_GE(delta_usec, timeout_usec);
}
