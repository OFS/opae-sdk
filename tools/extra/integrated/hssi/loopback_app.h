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
#include "loopback.h"
#include "option_map.h"
#include "cmd_handler.h"
#include "log.h"

namespace intel
{
namespace fpga
{
namespace hssi
{

class loopback_app
{
public:
    loopback_app();

    virtual ~loopback_app();

    virtual intel::utils::option_map & get_options()
    {
        return options_;
    }
    void show_help();
    uint32_t run(loopback::ptr_t lpbk, const std::vector<std::string> & args);
private:
    double timeout_;
    double delay_;
    std::vector<loopback::pair_t> ports_;
    std::vector<mac_report>       generators_;
    std::vector<mac_report>       monitors_;
    intel::utils::logger log_;
    loopback::ptr_t lpbk_;
    intel::utils::option_map options_;
    intel::utils::cmd_handler console_;
    bool wait_for_loopback(bool external);
    bool do_readmacs(const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_loopback(const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_stop(const intel::utils::cmd_handler::cmd_vector_t & cmd);
    bool do_status(const intel::utils::cmd_handler::cmd_vector_t & cmd);

};

} // end of namespace hssi
} // end of namespace fpga
} // end of namespace intel


