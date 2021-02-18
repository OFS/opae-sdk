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
#pragma once
#include <memory>
#include "afu_test.h"

namespace hps {

enum {
  AFU_DFH = 0x0000,
  AFU_ID_L = 0x0008,
  AFU_ID_H = 0x0010,
  CSR_ADDR = 0x0018
};


union afu_dfh  {
  enum {
    offset = AFU_DFH
  };

  afu_dfh(uint64_t v) : value(v) {}
  uint64_t value;
  struct {
    uint64_t FeatureID : 12;
    uint64_t FeatureRev : 4;
    uint64_t NextDfhByteOffset : 24;
    uint64_t EOL : 1;
    uint64_t Reserved41 : 19;
    uint64_t FeatureType : 4;
  };
};

class hps : public opae::afu_test::afu {
public:
  hps()
  : opae::afu_test::afu("hps", nullptr)
  , feature_base_(0)
  {
  }

virtual int open_handle(const char *afu_id) override {
  int res = opae::afu_test::afu::open_handle(afu_id);
  if (!res) {
    auto value  = read64(CSR_ADDR);
    // chop off last bit
    feature_base_ = value & ~(1UL<<63);
  }
  return res;
}

template<class T>
volatile T* object64()
{
  return new(handle_->mmio_ptr(T::offset + feature_base_))T;
}

private:
  uint32_t feature_base_;

};

} // end of namespace hps

