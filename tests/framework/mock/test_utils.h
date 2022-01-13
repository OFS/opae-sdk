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
#include <json-c/json.h>
#include <regex.h>
#include <memory>
#include <string>

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
    if (regcomp(&m->regex_, pattern.c_str(), flags | REG_EXTENDED)) {
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
    } else {
      regerror(res, &regex_, err_, 128);
    }
    return m;
  }

  std::string error() { return std::string(err_); }

 private:
  regex() = delete;
  regex(const std::string &pattern) : pattern_(pattern) {}
  std::string pattern_;
  regex_t regex_;
  char err_[128];
  std::array<regmatch_t, _M> matches_;
};

class jobject {
 public:
  jobject() { obj_ = json_object_new_object(); }

  jobject(json_object *obj) { obj_ = obj; }

  jobject(int32_t v) : jobject(json_object_new_int(v)) {}
  jobject(int64_t v) : jobject(json_object_new_int64(v)) {}
  jobject(double v) : jobject(json_object_new_double(v)) {}
  jobject(const char *v) : jobject(json_object_new_string(v)) {}
  jobject(const std::string &v) : jobject(json_object_new_string(v.c_str())) {}
  jobject(const std::string &k, jobject o) : jobject() {
    json_object_object_add(obj_, k.c_str(), o.obj_);
  }
  jobject(std::initializer_list<jobject> arr) {
    obj_ = json_object_new_array();
    for (auto &o : arr) {
      json_object_array_add(obj_, o.obj_);
    }
  }

  jobject(const jobject &other) {
    if (&other != this) {
      obj_ = other.obj_;
    }
  }

  jobject &operator=(const jobject &other) {
    if (&other != this) {
      obj_ = other.obj_;
    }
    return *this;
  }

  virtual ~jobject() {}

  void put() { json_object_put(obj_); }

  void get() { json_object_get(obj_); }

  virtual jobject &operator()(const std::string &key, jobject j) {
    json_object_object_add(obj_, key.c_str(), j.obj_);
    return *this;
  }

  virtual const char *c_str() { return json_object_to_json_string(obj_); }

 protected:
  json_object *obj_;
};


}  // end of namespace testing
}  // end of namespace opae
