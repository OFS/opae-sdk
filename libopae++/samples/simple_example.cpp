// Copyright(c) 2017, Intel Corporation
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
#include <opae/cxx/core/handle.h>
#include <opae/cxx/core/properties.h>
#include <opae/cxx/core/token.h>
#include <opae/mmio.h>
#include <uuid/uuid.h>
#include <iostream>

using namespace opae::fpga::types;

const char* NLB0 = "D8424DC4-A4A3-C413-F89E-433683F9040B";
int main(int argc, char* argv[]) {
  if (argc == 0) {
    std::cerr << argv[0] << "\n";
  }

  properties props_filter;

  props_filter.socket_id = 1;
  props_filter.type = FPGA_ACCELERATOR;
  uuid_t uuid;
  if (uuid_parse(NLB0, uuid) == 0) {
    props_filter.guid = uuid;
  }
  props_filter.bbs_id = 0;  // This is invalid - libopae-c prints out a warning
  auto tokens = token::enumerate({props_filter});
  if (tokens.size() > 0) {
    auto tok = tokens[0];
    auto props = properties::read(tok);
    std::cout << "guid prop read: " << props->guid << "\n";

    std::cout << "bus: 0x" << std::hex << props->bus << "\n";
    handle::ptr_t h = handle::open(tok, FPGA_OPEN_SHARED);
    uint64_t value1 = 0xdeadbeef, value2 = 0;
    h->write_csr64(0x100, value1);
    value2 = h->read_csr64(0x100);
    std::cout << "mmio @0x100: 0x" << std::hex << value2 << "\n";
    std::cout << "mmio @0x100: 0x" << std::hex
              << *reinterpret_cast<uint64_t*>(h->mmio_ptr(0x100)) << "\n";
  }

  return 0;
}
