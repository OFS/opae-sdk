// Copyright(c) 2017-2019, Intel Corporation
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
#include <sstream>
#include "nlb0.h"
#include "log.h"
#include "utils.h"
#include "option.h"
#include "option_parser.h"
#include "diag_utils.h"
#include <opae/cxx/core/token.h>
#include <opae/cxx/core/handle.h>

using namespace opae::fpga::types;

using namespace intel::fpga;
using namespace intel::fpga::diag;
using namespace intel::utils;


int main(int argc, char* argv[])
{
    nlb0 nlb;
    option_parser parser;
    option_map & opts = nlb.get_options();

    parser.parse_args(argc, argv, opts);

    bool show_help = false;
    opts.get_value<bool>("help", show_help);
    if (show_help)
    {
        nlb.show_help(std::cout);
        return 100;
    }

    bool show_version = false;
    opts.get_value<bool>("version", show_version);
    if (show_version)
    {
        std::cout << "nlb0 " << INTEL_FPGA_API_VERSION
                  << " " << INTEL_FPGA_API_HASH;
        if (INTEL_FPGA_TREE_DIRTY)
            std::cout << "*";
        std::cout << std::endl;
        return 103;
    }

    logger log;
    std::string config;
    if (opts.get_value<std::string>("config", config) && path_exists(config))
    {
        std::ifstream f(config);
        std::stringstream buf;

        buf << f.rdbuf();

        if (!parser.parse_json(buf.str(), opts))
        {
            std::cerr << "Error: json parse (" << config << ") failed." << std::endl;
            return 101;
        }
        log.info() << "Using config file: " << config << std::endl;
    }

    option_map::ptr_t filter(new option_map(opts));
    std::string target = "fpga";
    opts.get_value("target", target);
    bool shared = target == "fpga";
    auto props = get_properties(filter, FPGA_ACCELERATOR);
    auto accelerator_list = token::enumerate({ props });
    if (accelerator_list.size() >= 1)
    {
        token::ptr_t accelerator_tok = accelerator_list[0];
        auto h = handle::open(accelerator_tok, (shared ? FPGA_OPEN_SHARED: 0));
        nlb.assign(h);
        if (nlb.setup())
        {
            return nlb.run() ? 0 : 3;
        }
        else
        {
            std::cerr << "Error: configuration failed." << std::endl;
        }
    }
    else
    {
        std::cerr << "Error: device enumeration failed." << std::endl;
        std::cerr << "Please make sure that the driver is loaded and that a bitstream for" << std::endl
                  << "AFU id: " << nlb.afu_id() << " is programmed." << std::endl;
    }

    return 102;
}

