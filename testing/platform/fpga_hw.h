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
#pragma once
#include <opae/fpga.h>

#include <map>
#include <vector>

#define UUID_LENGTH 37

namespace opae {
namespace testing {

struct test_device {
  char fme_guid[UUID_LENGTH];
  char afu_guid[UUID_LENGTH];
  uint16_t segment;
  uint8_t bus;
  uint8_t device;
  uint8_t function;
  uint8_t socket_id;
  uint32_t num_slots;
  uint64_t bbs_id;
  fpga_version bbs_version;
  fpga_accelerator_state state;
  uint32_t num_mmio;
  uint32_t num_interrupts;
  uint64_t fme_object_id;
  uint64_t port_object_id;
  uint16_t vendor_id;
  uint32_t device_id;
  uint32_t fme_num_errors;
  uint32_t port_num_errors;
  const char *gbs_guid;
  const char *mdata;
  static test_device unknown();
};


struct test_platform;

typedef std::map<std::string, test_platform> platform_db;
class fpga_db {
public:
  static fpga_db *instance();
  test_platform get(const std::string &key);
  bool exists(const std::string &key);
  std::vector<std::string> keys(bool sorted = false);
private:
  fpga_db();
  static fpga_db *instance_;
  void discover_hw();
  platform_db platforms_;
};

#define COUNT_DEVICES(P, F, V) \
  P.count_devices([V](const test_device &td) { return td.F == V;})

#define COUNT_DEVICES_STR(P, F, V) \
  P.count_devices([V](const test_device &td) { return strcmp(td.F, V) == 0;})

struct test_platform {
  const char *mock_sysfs;
  std::vector<test_device> devices;
  static test_platform get(const std::string &key);
  static bool exists(const std::string &key);
  static std::vector<std::string> keys(bool sorted = false);
  static std::vector<std::string> platforms(std::initializer_list<std::string> names = {});
  static std::vector<std::string> mock_platforms(std::initializer_list<std::string> names = {});
  static std::vector<std::string> hw_platforms(std::initializer_list<std::string> names = {});
  template<class P>
  int count_devices(P op) {
    int count = 0;
    auto b = devices.cbegin();
    auto e = devices.cend();
    while (b != e) {
      if (op(*b)) {
        count++;
      }
      b++;
    }
    return count;
  }
};


}  // end of namespace testing
}  // end of namespace opae
