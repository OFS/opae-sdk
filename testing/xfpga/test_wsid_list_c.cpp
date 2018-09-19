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

#include <chrono>
#include <thread>
#include "gtest/gtest.h"

// define some operators to alter index consistently
constexpr uint64_t index_to_wsid(uint64_t i) { return i * 6; }
constexpr uint64_t index_to_addr(uint64_t i) { return i * 5; }
constexpr uint64_t index_to_phys(uint64_t i) { return i * 4; }
constexpr uint64_t index_to_len(uint64_t i) { return i * 3; }
constexpr uint64_t index_to_offset(uint64_t i) { return i * 2; }
constexpr uint64_t index_to_index(uint64_t i) { return i * 1; }
constexpr uint64_t index_to_flags(uint64_t i) { return i * i; }

static uint64_t stress_count = 0;

void cleanup_cb(wsid_map *ws) { stress_count--; }

class wsid_list_f : public ::testing::Test {
 protected:
  wsid_list_f() : wsid_root_(nullptr) {}
  virtual void SetUp() override {
    count_ = 100;
    distribution_ = std::uniform_int_distribution<int>(0, count_);
    for (uint64_t i = 0; i < count_; ++i) {
      EXPECT_TRUE(wsid_add(&wsid_root_, index_to_wsid(i), index_to_addr(i),
                           index_to_phys(i), index_to_len(i),
                           index_to_offset(i), index_to_index(i),
                           index_to_flags(i)));
    }
  }

  virtual void TearDown() override {
    if (wsid_root_) {
      wsid_cleanup(&wsid_root_, nullptr);
      EXPECT_EQ(wsid_root_, nullptr);
    }
  }

  wsid_map *wsid_root_;
  uint32_t count_;
  std::default_random_engine generator_;
  std::uniform_int_distribution<int> distribution_;
};

TEST_F(wsid_list_f, wsid_add) {
  // the setup adds, now we just confirm that it added the right data

  wsid_map *it = wsid_root_;
  int i = count_;
  while (--i >= 0 && it != nullptr) {
    EXPECT_EQ(it->wsid, index_to_wsid(i));
    EXPECT_EQ(it->addr, index_to_addr(i));
    EXPECT_EQ(it->phys, index_to_phys(i));
    EXPECT_EQ(it->len, index_to_len(i));
    EXPECT_EQ(it->offset, index_to_offset(i));
    EXPECT_EQ(it->index, index_to_index(i));
    ASSERT_EQ(it->flags, index_to_flags(i));
    it = it->next;
  }
}

TEST_F(wsid_list_f, wsid_del) {
  uint64_t wsid = index_to_wsid(distribution_(generator_));
  EXPECT_TRUE(wsid_del(&wsid_root_, wsid));
  wsid_map *it = wsid_root_;
  // now look for the wsid in the list
  while (it != nullptr) {
    if (it->wsid == wsid) {
      break;
    }
    it = it->next;
  }
  // it is null when we've looked at whole list without finding wsid
  EXPECT_EQ(it, nullptr);
  // it isn't there so we shouldn't be able to delete it again
  EXPECT_FALSE(wsid_del(&wsid_root_, wsid));
}

TEST_F(wsid_list_f, wsid_cleanup) {
  auto cleanup = [](wsid_map *w) -> void {
    EXPECT_EQ(w->wsid, index_to_wsid(w->index));
  };
  wsid_cleanup(&wsid_root_, cleanup);
  EXPECT_EQ(wsid_root_, nullptr);
}

//
TEST_F(wsid_list_f, wsid_gen) {
  std::vector<uint64_t> wsids;
  for (uint64_t i = 0; i < count_; ++i) {
    auto wsid = wsid_gen();
    auto it = std::find(wsids.begin(), wsids.end(), wsid);
    ASSERT_TRUE(it == wsids.end()) << "duplicate wsid after " << wsids.size();
    wsids.push_back(wsid);
    // FIXME: wsid_gen uses gettimeofday in its calculation of an id Because of
    // that, it may generate duplicate ids if the time in between calls is less
    // than one usec. wsid_gen should be fixed
    std::this_thread::sleep_for(std::chrono::microseconds(1));
  }
}

TEST_F(wsid_list_f, wsid_find) {
  uint32_t index = distribution_(generator_);
  wsid_map *ws = wsid_find_by_index(wsid_root_, index);
  ASSERT_NE(ws, nullptr);
  EXPECT_EQ(ws->wsid, index_to_wsid(index));
}

TEST_F(wsid_list_f, wsid_find_by_index) {
  uint32_t index = distribution_(generator_);
  wsid_map *ws = wsid_find(wsid_root_, index_to_wsid(index));
  ASSERT_NE(ws, nullptr);
  EXPECT_EQ(ws->index, index);
}

TEST_F(wsid_list_f, stress) {
  int count = count_;
  // FIXME: wsid_add can result in process being killed (out of memory) if it's
  // called too many times.
  uint64_t count_max = 1024;
  for (count = count_; count < count_max; ++count) {
    EXPECT_TRUE(wsid_add(&wsid_root_, index_to_wsid(count),
                         index_to_addr(count), index_to_phys(count),
                         index_to_len(count), index_to_offset(count),
                         index_to_index(count), index_to_flags(count)));
  }
  stress_count = count;
  wsid_cleanup(&wsid_root_, cleanup_cb);
  EXPECT_EQ(stress_count, 0);
  EXPECT_EQ(wsid_root_, nullptr);
}
