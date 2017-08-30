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

/// @file fpga_app.h
/// @brief fpga_app.h contains the fpga_app class
#pragma once

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <memory>
#include <vector>
#include "option_map.h"
#include "accelerator.h"
#include "log.h"

namespace intel
{
namespace fpga
{

class fpga_app
{
public:
    typedef std::shared_ptr<fpga_app> ptr_t;

    enum hwenv_t
    {
        hw = 0,
        ase = 1
    };


    fpga_app();
    ~fpga_app();

    /// @brief Starts the fpga_app by setting up any necessary resources
    void start(hwenv_t env = hw);

    /// @brief Close any open accelerator objects
    void shutdown();

    /// @brief Open an accelerator object using the given id
    ///
    /// @param accelerator_id The accelerator id (as a GUID string)
    /// @param options option map of options like:
    ///        bus, device, function
    ///
    /// @return A shared pointer to the accelerator object
    accelerator::ptr_t open(const std::string & accelerator_id, const intel::utils::option_map & options);

    static std::vector<fpga_resource> enumerate(intel::utils::option_map & filter);
protected:


private:
    intel::utils::logger log_;
    std::map<std::string, accelerator::ptr_t> open_accelerators_;
};

} // end of namepsace fpga
} // end of namespace intel
