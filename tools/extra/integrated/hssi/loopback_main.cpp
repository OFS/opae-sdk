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
#include <opae/cxx/core/token.h>
#include "e10.h"
#include "e40.h"
#include "e100.h"
#include "loopback_app.h"


using namespace intel::fpga;
using namespace intel::fpga::hssi;
using namespace intel::utils;

bool uuid_equal(const std::string uuid1, const std::string uuid2)
{
    uuid_t u1, u2;
    if (uuid_parse(uuid1.c_str(), u1)) return false;
    if (uuid_parse(uuid2.c_str(), u2)) return false;
    return uuid_compare(u1, u2) == 0;
}

std::map<std::string, loopback::ptr_t(*)(const std::string &)> app_factory =
{
    { "e10",   [](const std::string & name){ return loopback::ptr_t(new e10(name));}},
    { "e40",   [](const std::string & name){ return loopback::ptr_t(new e40(name));}},
    { "e100",  [](const std::string & name){ return loopback::ptr_t(new e100(name));}}
};


loopback::ptr_t find_app(const std::vector<opae::fpga::types::token::ptr_t> accelerator_list)
{
    loopback::ptr_t lpbk;
    for (auto & app : app_factory)
    {
        for ( auto & accelerator_ptr : accelerator_list)
        {
            lpbk = app.second(app.first);

            opae::fpga::types::properties::ptr_t prop_ptr =
		    opae::fpga::types::properties::get(accelerator_ptr);

            char guid_str[37] = { 0, };
            uuid_unparse(prop_ptr->guid, guid_str);

            if (uuid_equal(lpbk->afu_id(), guid_str))
            {
                opae::fpga::types::handle::ptr_t handle_ptr =
			opae::fpga::types::handle::open(accelerator_ptr, FPGA_OPEN_SHARED);

                if (handle_ptr)
                {
                    std::cout << "Found " << lpbk->name() << std::endl;
                    lpbk->assign(handle_ptr);
                    return lpbk;
                }
            }

            prop_ptr.reset();
            lpbk.reset();
        }
    }
    std::cerr << "Could not find suitable accelerator application" << std::endl;
    return lpbk;
}

int main(int argc, char* argv[])
{
    option_parser parser;

    logger log;
    log.set_level(logger::level::level_info);
    // run hssi loopback
    loopback_app app;
    auto & opts = app.get_options();
    if (!parser.parse_args(argc, argv, opts))
    {
       log.error("main") << "Error parsing command line arguments" << std::endl;
       return EXIT_FAILURE;
    };

    bool show_help = false;
    opts.get_value<bool>("help", show_help);
    if (show_help)
    {
        app.show_help();
        return 100;
    }

    option_map::ptr_t filter(new option_map(opts));
    option::ptr_t opt;
    opae::fpga::types::properties::ptr_t props =
	    opae::fpga::types::properties::get();

    uint8_t bus = 0;
    opt = filter->find("bus");
    if (opt && opt->is_set() && filter->get_value<uint8_t>("bus", bus)) {
        props->bus = bus;
    }
    uint8_t device = 0;
    opt = filter->find("device");
    if (opt && opt->is_set() && filter->get_value<uint8_t>("device", device)) {
        props->device = device;
    }
    uint8_t function = 0;
    opt = filter->find("function");
    if (opt && opt->is_set() && filter->get_value<uint8_t>("function", function)) {
        props->function = function;
    }
    uint8_t socket_id = 0;
    opt = filter->find("socket-id");
    if (opt && opt->is_set() && filter->get_value<uint8_t>("socket-id", socket_id)) {
        props->socket_id = socket_id;
    }

    loopback::ptr_t lpbk;
    std::string mode = "auto";
    opts.get_value<std::string>("mode", mode);
    if ( mode == "auto")
    {
        std::vector<opae::fpga::types::token::ptr_t> accelerator_list =
          opae::fpga::types::token::enumerate({props});
        if (accelerator_list.size() == 0)
        {
            log.error("main") << "Could not find any suitable accelerator on the system" << std::endl;
            return EXIT_FAILURE;
        }
        lpbk = find_app(accelerator_list);
    }
    else
    {
        auto it = app_factory.find(mode);
        if (it == app_factory.end())
        {
            log.error("main") << "Mode not supported: " << mode << "\n";
            return EXIT_FAILURE;
        }
        lpbk = it->second(mode);
        *(*filter)["guid"] = lpbk->afu_id();

        std::string guid_str;
        if (filter->get_value<std::string>("guid", guid_str)) {
            fpga_guid g;
	    if (!uuid_parse(guid_str.c_str(), g))
                props->guid = g;
	}

        std::vector<opae::fpga::types::token::ptr_t> accelerator_list =
		opae::fpga::types::token::enumerate({props});
        if (accelerator_list.size() == 0)
        {
            log.error("main") << "Could not find any suitable accelerator on the system" << std::endl;
            return EXIT_FAILURE;
        }

        opae::fpga::types::handle::ptr_t h =
		opae::fpga::types::handle::open(accelerator_list[0], FPGA_OPEN_SHARED);

        if (!h)
        {
            log.error("main") << "Could not open accelerator" << std::endl;
            return EXIT_FAILURE;
        }
        lpbk->assign(h);
    }
    if (!lpbk)
    {
        log.error("main") << "Could not find appropriate loopback software" << std::endl;
        return EXIT_FAILURE;
    }

    return app.run(lpbk, parser.leftover());
}
