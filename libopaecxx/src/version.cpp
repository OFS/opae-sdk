// Copyright(c) 2018, Intel Corporation
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

#include <opae/cxx/core/except.h>
#include <opae/cxx/core/version.h>
#include <opae/types.h>
#include <opae/version.h>

namespace opae {
namespace fpga {
namespace types {

fpga_version version::as_struct() {
  fpga_version version_struct;
  ASSERT_FPGA_OK(fpgaGetOPAECVersion(&version_struct));
  return version_struct;
}

std::string version::as_string() {
  char ver_arr[32];
  ASSERT_FPGA_OK(fpgaGetOPAECVersionString(ver_arr, sizeof(ver_arr)));
  std::string ver_str(ver_arr);
  return ver_str;
}

std::string version::build() {
  char build_arr[32];
  ASSERT_FPGA_OK(fpgaGetOPAECBuildString(build_arr, sizeof(build_arr)));
  std::string build_str(build_arr);
  return build_str;
}

}  // end of namespace types
}  // end of namespace fpga
}  // end of namespace opae
