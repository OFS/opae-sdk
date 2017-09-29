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

#include <iostream>
#include <sstream>
#include "mtnlb7.h"
#include "log.h"
#include "utils.h"
#include "option.h"
#include "option_parser.h"
#include "accelerator.h"

using namespace intel::fpga;
using namespace intel::fpga::diag;
using namespace intel::utils;

int main(int argc, char* argv[])
{
    mtnlb7 nlb;
    option_parser parser;
    option_map & opts = nlb.get_options();

    parser.parse_args(argc, argv, opts);

    bool show_help = false;
    opts.get_value<bool>("help", show_help);
    if (show_help)
    {
        opts.show_help("mtnlb7", std::cout);
        return 100;
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

    std::vector<accelerator::ptr_t> accelerator_list = accelerator::enumerate({ filter });
    if (accelerator_list.size() >= 1)
    {
        accelerator::ptr_t accelerator_obj = accelerator_list[0];
        if (accelerator_obj->open(shared))
        {
            nlb.assign(accelerator_obj);
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
            std::cerr << "Error: couldn't open device." << std::endl;
        }
    }
    else
    {
        std::cerr << "Error: device enumeration failed." << std::endl;
    }

    return 102;
}

