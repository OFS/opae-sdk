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
#include <string>
#include <map>
#include <uuid/uuid.h>
#include "log.h"
#include "utils.h"
#include "option.h"
#include "option_parser.h"
#include "config_app.h"


using namespace intel::fpga;
using namespace intel::fpga::hssi;
using namespace intel::utils;

int main(int argc, char* argv[])
{
    option_parser parser;

    logger log;
    // run hssi config
    config_app config;
    auto & opts = config.get_options();
    if (!parser.parse_args(argc, argv, opts))
    {
        log.error("main") << "Error parsing command line arguments" << std::endl;
        return EXIT_FAILURE;
    }

    bool show_help = false;
    opts.get_value<bool>("help", show_help);
    if (show_help)
    {
        config.show_help();
        return 100;
    }

    if (!config.setup())
    {
        log.error("main") << "Error in config setup" << std::endl;
        return EXIT_FAILURE;
    }
    return config.run(parser.leftover());
}
