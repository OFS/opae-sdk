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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <iostream>
#include <string>
#include <algorithm>

#include "convert.hpp"
#include "grpc_client.hpp"

fpga_result OPAEClient::fpgaEnumerate(const std::vector<fpga_properties> &filters, uint32_t num_filters,
                                      uint32_t max_tokens, uint32_t &num_matches,
	                                    std::vector<fpga_token_header> &tokens)
{
  opaegrpc::EnumerateRequest request;

  for (auto f : filters) {
    _fpga_properties *p = reinterpret_cast<_fpga_properties *>(f);
    opaegrpc::fpga_properties *gprops = request.add_filters();
    to_grpc_fpga_properties(this, gprops, p);
  }
  request.set_num_filters(num_filters);
  request.set_max_tokens(max_tokens);

  opaegrpc::EnumerateReply reply;
  ClientContext context;

  Status status = stub_->fpgaEnumerate(&context, request, &reply);
	if (!status.ok()) {
    OPAE_ERR("gRPC failed: %s", status.error_message().c_str());
    OPAE_ERR("details: %s", status.error_details().c_str());
		return FPGA_EXCEPTION;
	}

std::cout << reply << std::endl;

  num_matches = reply.num_matches();
	const size_t toks = std::min(num_matches, reply.max_tokens());

  tokens.reserve(toks);
  tokens.resize(toks);
  std::transform(reply.tokens().cbegin(), reply.tokens().cbegin() + toks,
                 tokens.begin(), to_opae_token_header);

  return FPGA_OK;
}
