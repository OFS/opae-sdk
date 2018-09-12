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
/*
 * test_utils.h
 */
#pragma once
#include <memory>
#include <regex.h>

namespace opae {
namespace testing {

class match_t {
 public:
  typedef std::shared_ptr<match_t> ptr_t;
  match_t(const std::string &str, const std::vector<std::string> &matches)
      : string_(str), groups_(matches) {}

  std::vector<std::string> &groups() { return groups_; }

  std::string group(uint32_t idx) { return groups_[idx]; }

  std::string str() { return string_; }

 private:
  std::string string_;
  std::vector<std::string> groups_;
};

template <uint64_t _M = 32>
class regex {
 public:
  typedef std::shared_ptr<regex> ptr_t;

  static regex::ptr_t create(const std::string &pattern, int flags = 0) {
    regex::ptr_t m(new regex(pattern));
    if (regcomp(&m->regex_, pattern.c_str(), flags)) {
      m.reset();
    }
    return m;
  }

  ~regex() { regfree(&regex_); }

  match_t::ptr_t match(const std::string str, int flags = 0) {
    std::vector<std::string> matches;
    match_t::ptr_t m;
    auto res =
        regexec(&regex_, str.c_str(), matches_.size(), matches_.data(), flags);
    if (!res) {
      for (const auto &m : matches_) {
        if (m.rm_so >= 0) {
          matches.push_back(str.substr(m.rm_so, m.rm_eo - m.rm_so));
        } else {
          break;
        }
      }
      m.reset(new match_t(str, matches));
    }
    return m;
  }

 private:
  regex() = delete;
  regex(const std::string &pattern) : pattern_(pattern) {}
  std::string pattern_;
  regex_t regex_;
  std::array<regmatch_t, _M> matches_;
};

}  // end of namespace testing
}  // end of namespace opae
