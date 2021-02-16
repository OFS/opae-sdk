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

#include <memory>
#include <map>
#include <typeindex>
#include <chrono>
#include "nlb0.h"
#include "nlb3.h"
#include "nlb7.h"
#include "nlb_stats.h"
#include "fpga_app/accelerator_mux.h"
#include "log.h"
#include "utils.h"
#include "option.h"
#include "option_parser.h"
#include "nlb_stats.h"
#include "fpga_app/fpga_common.h"
#include <json-c/json.h>


using namespace intel::fpga;
using namespace intel::fpga::diag;
using namespace intel::fpga::nlb;
using namespace intel::utils;
using namespace opae::fpga::types;


std::map<std::string, accelerator_app::ptr_t(*)(const std::string &)> app_factory =
{
    { "nlb0",   [](const std::string & name){ UNUSED_PARAM(name); return accelerator_app::ptr_t(new nlb0());   }},
    { "nlb3",   [](const std::string & name){ UNUSED_PARAM(name); return accelerator_app::ptr_t(new nlb3());   }},
    { "nlb7",   [](const std::string & name){ UNUSED_PARAM(name); return accelerator_app::ptr_t(new nlb7());   }},
};

enum test_result
{
    incomplete = 0,
    pass,
    fail,
    error
};

static std::map<json_tokener_error, std::string> json_parse_errors =
{
    { json_tokener_success, "json_tokener_success" },
    { json_tokener_continue, "json_tokener_continue" },
    { json_tokener_error_depth, "json_tokener_error_depth" },
    { json_tokener_error_parse_eof, "json_tokener_error_parse_eof" },
    { json_tokener_error_parse_unexpected, "json_tokener_error_parse_unexpected" },
    { json_tokener_error_parse_null, "json_tokener_error_parse_null" },
    { json_tokener_error_parse_boolean, "json_tokener_error_parse_boolean" },
    { json_tokener_error_parse_number, "json_tokener_error_parse_number" },
    { json_tokener_error_parse_array, "json_tokener_error_parse_array" },
    { json_tokener_error_parse_object_key_name, "json_tokener_error_parse_object_key_name" },
    { json_tokener_error_parse_object_key_sep, "json_tokener_error_parse_object_key_sep" },
    { json_tokener_error_parse_object_value_sep, "json_tokener_error_parse_object_value_sep" },
    { json_tokener_error_parse_string, "json_tokener_error_parse_string" },
    { json_tokener_error_parse_comment, "json_tokener_error_parse_comment" },
    { json_tokener_error_size, "json_tokener_error_size" }
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
    enum json_tokener_error err = json_tokener_success;
    struct json_object * root = json_tokener_parse_verbose(jbuff, &err);

    if (root == nullptr)
    {
        auto it = json_parse_errors.find(err);
        auto msg = it == json_parse_errors.end() ? "Unknown" : it->second;
        std::cerr << "ERROR parsing muxfile: " << msg << "\n";
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
                json_object *disabled_obj;
                auto app = it->second(name);
                if (json_object_object_get_ex(instance, "disabled", &disabled_obj))
                {
                    app->disabled(json_object_get_boolean(disabled_obj));
                }
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
    uint64_t freq = 300000000;

    opts.add_option<bool>("help",             'h', option::no_argument,   "Show help", false);
    opts.add_option<uint8_t>("socket-id",     's', option::with_argument, "Socket id encoded in BBS");
    opts.add_option<uint8_t>("bus-number",    'B', option::with_argument, "Bus number of PCIe device");
    opts.add_option<uint8_t>("device",        'D', option::with_argument, "Device number of PCIe device");
    opts.add_option<uint8_t>("function",      'F', option::with_argument, "Function number of PCIe device");
    opts.add_option<std::string>("target",    't', option::with_argument, "Target platform. fpga or ase - default is fpga", "fpga");
    opts.add_option<std::string>("guid",      'G', option::with_argument, "GUID of accelerator to open");
    opts.add_option<std::string>("config",    'c', option::with_argument, "Path to JSON file containing mux sw apps");
    opts.add_option<bool>("suppress-header",  'H', option::no_argument, "Suppress header when showing results", false);
    opts.add_option<bool>("csv",              'V', option::no_argument, "Show results in CSV format", false);
    opts.add_option<uint64_t>("frequency",    'T', option::with_argument, "Clock frequency (used for bw measurements)", freq);
    logger log;
    log.set_level(logger::level::level_debug);
    if (!parser.parse_args(argc, argv, opts))
    {
        log.error("main") << "Error parsing command line arguments" << std::endl;
        opts.show_help("fpgamux", std::cout);
        return EXIT_FAILURE;
    };

    bool help = false;
    if (opts.get_value<bool>("help", help) && help){
        opts.show_help("fpgamux", std::cout);
        return EXIT_SUCCESS;
    }

    auto muxopt = opts.find("config");
    if (!muxopt || !muxopt->is_set() || !path_exists(muxopt->value<std::string>()))
    {
        log.error("main") << "Invalid or no muxfile specified" << std::endl;
        return EXIT_FAILURE;
    }
    opts.get_value<uint64_t>("frequency", freq);

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
        log.error("accelerator::enumerate") << "Could not find accelerator with given filter\n";
        return EXIT_FAILURE;
    }

    std::string target = "fpga";
    opts.get_value("target", target);
    bool shared = target == "fpga";
    std::map<std::string, std::future<bool>> futures;
    std::map<std::string, test_result>       results;
    std::map<std::string, shared_buffer::ptr_t> dsm_list;
    size_t instance = 0;
    auto accelerator_ptr = acceleratorlist[0];
    if (!accelerator_ptr->open(shared))
    {
        log.error("accelerator::open") << "Error opening accelerator" << std::endl;
        return EXIT_FAILURE;
    }
    accelerator_ptr->reset();
    auto buffer = accelerator_ptr->allocate_buffer(GB(1));
    if (!buffer) {
        log.error("main") << "failed to allocate workspace and input/output buffers." << std::endl;
        return EXIT_FAILURE;
    }
    buffer_pool::ptr_t pool(new buffer_pool(buffer));
    auto dsm = pool->allocate_buffer(MB(2));
   // Read perf counters.
    fpga_cache_counters    start_cache_ctrs  = accelerator_ptr->cache_counters();
    fpga_fabric_counters   start_fabric_ctrs = accelerator_ptr->fabric_counters();
    for (auto app : apps)
    {
        accelerator::ptr_t muxed(new accelerator_mux(acceleratorlist[0], apps.size(), instance++, pool));
        app->assign(muxed->handle());
        if (!app->disabled() && app->setup())
        {
            results[app->name()] = test_result::incomplete;
            futures[app->name()] = app->run_async();
            dsm_list[app->name()] = app->dsm();
        }
    }

    std::map<std::string, test_result>::size_type complete = 0;
    dsm_tuple tpl;
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
                tpl += dsm_tuple(dsm_list[kv.first]);
                complete++;
            }
            else if(kv.second != test_result::incomplete)
            {
                complete++;
            }
        }
    }

    uint64_t cachelines = 0;
    for (auto app : apps)
    {
        if (!app->disabled())  cachelines += app->cachelines();
    }
    bool suppress_header = false;
    bool csv_format = false;
    opts.get_value<bool>("suppress-header", suppress_header);
    opts.get_value<bool>("csv-format", csv_format);
    // Read Perf Counters
    fpga_cache_counters  end_cache_ctrs  = accelerator_ptr->cache_counters();
    fpga_fabric_counters end_fabric_ctrs = accelerator_ptr->fabric_counters();
    tpl.put(dsm);
    std::cout << intel::fpga::nlb::nlb_stats(dsm,
                                             cachelines,
                                             end_cache_ctrs - start_cache_ctrs,
                                             end_fabric_ctrs - start_fabric_ctrs,
                                             freq,
                                             false,
                                             suppress_header,
                                             csv_format);

    for(auto & kv : results)
    {
        if (kv.second != test_result::pass)
        {
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
