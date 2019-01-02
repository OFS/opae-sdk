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
#include <random>
#include <chrono>
#include <thread>
#include "gtest/gtest.h"

#ifndef BUILD_ASE
 /*
 * On hardware, the mmio map is a hash table.
 */
static bool mmio_map_is_empty(struct wsid_tracker *root) {
  if (!root || (root->n_hash_buckets == 0))
    { return true; }
  else{
    uint64_t i;
    for (i = 0; i < root->n_hash_buckets; ++i) {
      if (root->table[i])
        { return false; }
    }
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

// define some operators to alter index consistently
constexpr uint64_t index_to_wsid(uint64_t i) { return i * 6; }
constexpr uint64_t index_to_addr(uint64_t i) { return i * 5; }
constexpr uint64_t index_to_phys(uint64_t i) { return i * 4; }
constexpr uint64_t index_to_len(uint64_t i) { return i * 3; }
constexpr uint64_t index_to_offset(uint64_t i) { return i * 2; }
constexpr uint64_t index_to_index(uint64_t i) { return i * 1; }
constexpr uint64_t index_to_flags(uint64_t i) { return i * i; }

static uint64_t stress_count = 0;

void cleanup_cb(wsid_map *ws) { (void) ws; stress_count--; }
   
class wsid_list_f : public ::testing::Test {
 protected:
  wsid_list_f() 
       : wsid_root_(nullptr) {}

  virtual void SetUp() override {
    wsid_root_ = wsid_tracker_init(1000);
    count_ = 100;
    distribution_ = std::uniform_int_distribution<int>(0, count_);
    uint64_t i;
    for (i = 0; i < count_; ++i) {
      EXPECT_TRUE(wsid_add(wsid_root_, index_to_wsid(i), index_to_addr(i),
                           index_to_phys(i), index_to_len(i),
                           index_to_offset(i), index_to_index(i),
                           index_to_flags(i)));
    }
  }

  virtual void TearDown() override {
      auto cleanup = [](struct wsid_map *w) -> void {
           EXPECT_EQ(w->wsid, index_to_wsid(w->index));};
 
      bool empty = mmio_map_is_empty(wsid_root_);
      if ( !empty ) {
        wsid_tracker_cleanup(wsid_root_, cleanup);
        wsid_root_ = nullptr;
      }
  }

  struct wsid_tracker *wsid_root_;
  uint64_t count_;
  std::default_random_engine generator_;
  std::uniform_int_distribution<int> distribution_;
};

/*
 * @test    wsid_init_neg
 *
 * @details When wsid_tracker_init()'s n_hash_buckets parameter
 *          is greater then the max, the function returns NULL.
 */
TEST_F(wsid_list_f, wsid_init_neg) {
  EXPECT_EQ(wsid_tracker_init(123456789), nullptr);
}

TEST_F(wsid_list_f, wsid_add) {
  // the setup adds, now we just confirm that it added the right data
  wsid_map *it = nullptr;
  int i = count_;
  while (i-- >= 0) {
    it = wsid_find_by_index(wsid_root_, i);
    if (it) {
      EXPECT_EQ(it->wsid, index_to_wsid(i));
      EXPECT_EQ(it->addr, index_to_addr(i));
      EXPECT_EQ(it->phys, index_to_phys(i));
      EXPECT_EQ(it->len, index_to_len(i));
      EXPECT_EQ(it->offset, index_to_offset(i));
      EXPECT_EQ(it->index, index_to_index(i));
      ASSERT_EQ(it->flags, index_to_flags(i));
      it = nullptr;
    }
  }
  it = nullptr;
}

TEST_F(wsid_list_f, wsid_del) {
  uint32_t wsid = index_to_wsid(distribution_(generator_));
  EXPECT_TRUE(wsid_del(wsid_root_, wsid));
  wsid_map *it = wsid_find(wsid_root_, wsid);
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
  EXPECT_FALSE(wsid_del(wsid_root_, wsid));
}

TEST_F(wsid_list_f, wsid_find) {
  uint32_t index = distribution_(generator_);
  wsid_map *ws = wsid_find_by_index(wsid_root_, index);
  ASSERT_NE(ws, nullptr);
  EXPECT_EQ(ws->wsid, index_to_wsid(index));
}

TEST_F(wsid_list_f, wsid_find_by_index) {
  uint64_t index = distribution_(generator_);
  wsid_map *ws = wsid_find(wsid_root_, index_to_wsid(index));
  ASSERT_NE(ws, nullptr);
  EXPECT_EQ(ws->index, index);
}

TEST_F(wsid_list_f, stress) {
  uint64_t count = count_;
  // FIXME: wsid_add can result in process being killed (out of memory) if it's
  // called too many times.
  uint64_t count_max = 1024;
  for (count = count_; count < count_max; ++count) {
    EXPECT_TRUE(wsid_add(wsid_root_, index_to_wsid(count),
                         index_to_addr(count), index_to_phys(count),
                         index_to_len(count), index_to_offset(count),
                         index_to_index(count), index_to_flags(count)));
  }
  stress_count = count;
  wsid_tracker_cleanup(wsid_root_, cleanup_cb);
  EXPECT_EQ(stress_count, 0);
  wsid_root_ = nullptr;
}
