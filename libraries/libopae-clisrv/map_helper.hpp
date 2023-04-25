// Copyright(c) 2023, Intel Corporation
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
#pragma once

#include <map>
#include <mutex>

template <typename K, typename V, typename L = std::mutex>
class opae_map_helper {
 public:
  opae_map_helper(V none) : none_(none) {}

  V find(const K &key) const {
    std::lock_guard<std::mutex> g(lock_);
    typename std::map<K, V>::const_iterator it = map_.find(key);
    return (it == map_.cend()) ? none_ : it->second;
  }

  bool add(const K &key, V value) {
    std::lock_guard<std::mutex> g(lock_);
    typename std::pair<typename std::map<K, V>::iterator, bool> res =
        map_.insert(std::make_pair(key, value));
    return res.second;
  }

  bool add_unique(const K &key, V value) {
    std::lock_guard<std::mutex> g(lock_);
    typename std::map<K, V>::const_iterator it = map_.find(key);
    if (it == map_.cend()) {
      typename std::pair<typename std::map<K, V>::iterator, bool> res =
          map_.insert(std::make_pair(key, value));
      return res.second;
    }
    return false;
  }

  bool remove(const K &key) {
    std::lock_guard<std::mutex> g(lock_);
    typename std::map<K, V>::iterator it = map_.find(key);
    if (it == map_.end()) return false;
    map_.erase(it);
    return true;
  }

  typename std::map<K, V>::iterator begin() { return map_.begin(); }

  typename std::map<K, V>::iterator end() { return map_.end(); }

  void clear() { map_.clear(); }

 private:
  V none_;
  std::map<K, V> map_;
  mutable L lock_;
};
