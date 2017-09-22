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

#include <memory>
#include <map>
#include <typeindex>
#include <chrono>
#include "nlb0.h"
#include "nlb3.h"
#include "nlb7.h"
#include "mtnlb7.h"
#include "mtnlb8.h"
#include "nlb_stats.h"
#include "fpga_app/accelerator_mux.h"
#include "log.h"
#include "utils.h"
#include "option.h"
#include "option_parser.h"
#include "fpga_app/fpga_common.h"
#include <json-c/json.h>


using namespace intel::fpga;
using namespace intel::fpga::diag;
using namespace intel::utils;


std::map<std::string, accelerator_app::ptr_t(*)(const std::string &)> app_factory =
{
    { "nlb0",   [](const std::string & name){ return accelerator_app::ptr_t(new nlb0());   }},
    { "nlb3",   [](const std::string & name){ return accelerator_app::ptr_t(new nlb3());   }},
    { "nlb7",   [](const std::string & name){ return accelerator_app::ptr_t(new nlb7());   }},
    { "mtnlb7", [](const std::string & name){ return accelerator_app::ptr_t(new mtnlb7()); }},
    { "mtnlb8", [](const std::string & name){ return accelerator_app::ptr_t(new mtnlb8()); }}
};

enum test_result
{
    incomplete = 0,
    pass,
    fail,
    error
};

bool make_apps(const std::string & muxfile, std::vector<accelerator_app::ptr_t> & apps)
{
    std::ifstream inp;
    inp.open(muxfile.c_str());
    if (!inp.good())
    {
        return false;
    }
    inp.seekg(0, inp.end);
    size_t length = inp.tellg();
    inp.seekg(0, inp.beg);
    char jbuff[length];
    inp.read(jbuff, length);

    struct json_object * root = json_tokener_parse(jbuff);

    if (root == nullptr)
    {
        return false;
    }

    auto count = json_object_array_length(root);
    option_parser parser;
    for (int i = 0; i < count; ++i)
    {
        auto instance = json_object_array_get_idx(root, i);
        json_object *name_obj;
        json_object *config_obj;
        json_object *app_obj;
        if (json_object_object_get_ex(instance, "config", &config_obj) &&
            json_object_object_get_ex(instance, "name",   &name_obj) &&
            json_object_object_get_ex(instance, "app",    &app_obj))
        {
            std::string name = json_object_get_string(name_obj);
            std::string appname = json_object_get_string(app_obj);
            auto it = app_factory.find(appname);
            if (it != app_factory.end())
            {
                auto app = it->second(name);
                parser.parse_json(json_object_to_json_string(config_obj), app->get_options());
                apps.push_back(app);
            }
        }
        else
        {
            return false;
        }
    }
    return (apps.size() > 0);
}

int main(int argc, char* argv[])
{
    option_parser parser;
    option_map opts;

    opts.add_option<uint8_t>("socket-id",     'S', option::with_argument, "Socket id encoded in BBS");
    opts.add_option<uint8_t>("bus-number",    'B', option::with_argument, "Bus number of PCIe device");
    opts.add_option<uint8_t>("device",        'D', option::with_argument, "Device number of PCIe device");
    opts.add_option<uint8_t>("function",      'F', option::with_argument, "Function number of PCIe device");
    opts.add_option<std::string>("target",    't', option::with_argument, "Target platform. fpga or ase - default is fpga", "fpga");
    opts.add_option<std::string>("guid",      'G', option::with_argument, "GUID of accelerator to open");
    opts.add_option<std::string>("muxfile",   'm', option::with_argument, "Path to JSON file containing mux sw apps");

    logger log;
    log.set_level(logger::level::level_debug);
    if (!parser.parse_args(argc, argv, opts))
    {
       log.error("main") << "Error parsing command line arguments" << std::endl;
       return EXIT_FAILURE;
    };

    auto muxopt = opts.find("muxfile");
    if (!muxopt || !muxopt->is_set() || !path_exists(muxopt->value<std::string>()))
    {
        log.error("main") << "Invalid or no muxfile specified" << std::endl;
        return EXIT_FAILURE;
    }

    std::string muxfile = muxopt->value<std::string>();
    std::vector<accelerator_app::ptr_t> apps;

    if (!make_apps(muxfile, apps))
    {
        return EXIT_FAILURE;
    }

    auto guid_opt = opts.find("guid");
    if (guid_opt)
    {
        if (!guid_opt->is_set())
        {
            log.info("main") << "Setting guid from first app in mux configuration: " << apps[0]->afu_id() << "\n";
            *guid_opt = std::string(apps[0]->afu_id());
        }
        else
        {
            log.info("main") << "Using guid from options: " << guid_opt->value<std::string>() << "\n";
        }
    }

    std::vector<accelerator::ptr_t> acceleratorlist = accelerator::enumerate({ std::make_shared<option_map>(opts) });
    if (acceleratorlist.size() == 0)
    {
        log.error("accelerator::enumerate") << "Could not find AFC with given filter\n";
        return EXIT_FAILURE;
    }

    std::string target = "fpga";
    opts.get_value("target", target);
    bool shared = target == "fpga";
    std::map<std::string, std::future<bool>> futures;
    std::map<std::string, test_result>       results;
    size_t instance = 0;
    auto accelerator_ptr = acceleratorlist[0];
    if (!accelerator_ptr->open(shared))
    {
        log.error("accelerator::open") << "Error opening accelerator" << std::endl;
        return EXIT_FAILURE;
    }
    accelerator_ptr->reset();
    auto buffer = accelerator_ptr->allocate_buffer(GB(1));
    buffer_pool::ptr_t pool(new buffer_pool(buffer));
    for (auto app : apps)
    {
        accelerator::ptr_t muxed(new accelerator_mux(acceleratorlist[0], apps.size(), instance++, pool));
        app->assign(muxed);
        if (app->setup())
        {
            results[app->name()] = test_result::incomplete;
            futures[app->name()] = app->run_async();
        }
    }

    int complete = 0;
    while(complete < results.size())
    {
        complete = 0;
        for(auto & kv : results)
        {
            if (kv.second == test_result::incomplete &&
                futures[kv.first].valid() &&
                futures[kv.first].wait_for(std::chrono::milliseconds(10)) == std::future_status::ready)
            {
                results[kv.first] = futures[kv.first].get() ? test_result::pass : test_result::fail;
                complete++;
            }
            else if(kv.second != test_result::incomplete)
            {
                complete++;
            }
        }
    }


    for(auto & kv : results)
    {
        if (kv.second != test_result::pass)
        {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
