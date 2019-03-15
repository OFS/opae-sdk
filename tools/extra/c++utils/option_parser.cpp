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
#include "option_parser.h"
#include <map>
#include <functional>
#include <typeindex>
#include <type_traits>
#include <functional>
#include <getopt.h>
#include "option_map.h"
#include "utils.h"
#include <iostream>
#include <json-c/json.h>
#include "any_value.h"

typedef struct option option_struct;

namespace intel
{
namespace utils
{

template<typename T>
any_value cast_string(const std::string & v)
{
    if (std::is_integral<T>::value)
    {
        T val;
        if (parse_int(v, val))
        {
            return val;
        }
    }
    return v;
}

template<>
any_value cast_string<bool>(const std::string & v)
{
    if (v == "true") return true;
    if (v == "false") return false;

    return v;
}

template<>
any_value cast_string<float>(const std::string & v)
{
    try{
        return stof(v, nullptr);
    }catch(const std::invalid_argument & inv){
        std::cerr << "Command line option could not be parsed as a 'float': \"" << v << "\"\n";
        throw inv;
    }catch(const std::out_of_range & our){
        std::cerr << "Value out of range for a 'float': \"" << v << "\"\n";
        throw our;
    }
}

template<>
any_value cast_string<double>(const std::string & v)
{
    try{
        return stod(v, nullptr);
    }catch(const std::invalid_argument & inv){
        std::cerr << "Command line option could not be parsed as a 'double': \"" << v << "\"\n";
        throw inv;
    }catch(const std::out_of_range & our){
        std::cerr << "Value out of range for a 'double': \"" << v << "\"\n";
        throw our;
    }
}


template<>
any_value cast_string<std::string>(const std::string & v)
{
    return v;
}

static std::map<std::type_index, any_value(*)(const std::string&)> type_cast_map =
{
    { std::type_index(typeid(uint8_t) ), cast_string<uint8_t>},
    { std::type_index(typeid(int8_t) ), cast_string<int8_t>},
    { std::type_index(typeid(uint16_t)), cast_string<uint16_t>},
    { std::type_index(typeid(int16_t)), cast_string<int16_t>},
    { std::type_index(typeid(uint32_t)), cast_string<uint32_t>},
    { std::type_index(typeid(int32_t)), cast_string<int32_t>},
    { std::type_index(typeid(uint64_t)), cast_string<uint64_t>},
    { std::type_index(typeid(int64_t)), cast_string<int64_t>},
    { std::type_index(typeid(float)), cast_string<float>},
    { std::type_index(typeid(double)), cast_string<double>},
    { std::type_index(typeid(bool)), cast_string<bool>},
    { std::type_index(typeid(std::string)), cast_string<std::string>}
};

static std::map<std::type_index, any_value(*)(struct json_object * v )> json_to_any =
{
    { std::type_index(typeid(uint8_t) ), [](struct json_object * v) -> any_value { return static_cast<uint8_t>(json_object_get_int(v));}},
    { std::type_index(typeid(int8_t) ), [](struct json_object * v) -> any_value { return static_cast<int8_t>(json_object_get_int(v)); }},
    { std::type_index(typeid(uint16_t)), [](struct json_object * v) -> any_value { return static_cast<uint16_t>(json_object_get_int(v)); }},
    { std::type_index(typeid(int16_t)), [](struct json_object * v) -> any_value { return static_cast<int16_t>(json_object_get_int(v)); }},
    { std::type_index(typeid(uint32_t)), [](struct json_object * v) -> any_value { return static_cast<uint32_t>(json_object_get_int(v)); }},
    { std::type_index(typeid(int32_t)), [](struct json_object * v) -> any_value {  return static_cast<int32_t>(json_object_get_int(v)); }},
    { std::type_index(typeid(uint64_t)), [](struct json_object * v) -> any_value { return static_cast<uint64_t>(json_object_get_int64(v)); }},
    { std::type_index(typeid(int64_t)), [](struct json_object * v) -> any_value { return static_cast<int64_t>(json_object_get_int64(v)); }},
    { std::type_index(typeid(float)), [](struct json_object * v) -> any_value { return static_cast<float>(json_object_get_double(v)); }},
    { std::type_index(typeid(double)), [](struct json_object * v) -> any_value { return json_object_get_double(v); }},
    { std::type_index(typeid(bool)), [](struct json_object * v) -> any_value { return json_object_get_boolean(v) ? true:false; }},
    { std::type_index(typeid(std::string)), [](struct json_object * v) -> any_value { return std::string(json_object_get_string(v)); }}
};
option_parser::option_parser()
{

}

bool option_parser::parse_args(int argc, char* argv[], option_map & known_options)
{
    std::vector<option_struct> optarray;
    int offset = 4096;
    int optindex = 0;
    int flag = 0;
    std::string argstr;
    std::map<char, option::ptr_t> short_map;

    for (const auto & opt_ptr : known_options)
    {
        char short_opt = opt_ptr->short_opt();
        int has_arg = static_cast<int>(opt_ptr->has_arg());
        if (short_opt)
        {
            short_map[short_opt] = opt_ptr;
            argstr += short_opt;
            switch(has_arg)
            {
                case required_argument: argstr += ':'; break;
                case optional_argument: argstr += "::"; break;
            }
        }

        optarray.push_back({opt_ptr->name().c_str(),
                            has_arg,
                            short_opt ? nullptr : &flag,
                            short_opt ? short_opt : offset + optindex});
        optindex++;

    }

    optarray.push_back({nullptr,0,nullptr,0});

    int longidx;
    int c;
    while(true)
    {
        c = getopt_long(argc, argv, argstr.c_str(), optarray.data(), &longidx);
        if (c == '?')
        {
            return false;
        }

        if (c == -1)
        {
            break;
        }

        option::ptr_t opt_ptr;
        if (c != 0 && c < offset)
        {
            if (short_map.find(c) != short_map.end())
            {
                opt_ptr = short_map[c];
            }
        }
        else
        {
            opt_ptr = known_options[optarray[flag-offset].name];
        }

        if (!opt_ptr)
        {
            return false;
        }

        if (opt_ptr->has_arg() == 0)
        {
            *opt_ptr = true;
        }
        else
        {
            auto it = type_cast_map.find(opt_ptr->type());
            if (it != type_cast_map.end())
            {
                std::string optstr(optarg);
                if ('=' == optstr.front())
                {
                    optstr.erase(0, 1);
                }
                *opt_ptr = it->second(optstr);
            }
        }
    }

    if (optind < argc)
    {
        leftover_.clear();
        leftover_.resize(argc-optind);
        std::copy(argv+optind, argv+argc, leftover_.begin());
    }

    return true;
}

bool option_parser::parse_json(const std::string & value, option_map & options)
{
    struct json_object * root = json_tokener_parse(value.c_str());

    if (root == nullptr)
    {
        return false;
    }

    for(option::ptr_t opt : options)
    {
        struct json_object *item;
        if (json_object_object_get_ex(root, opt->name().c_str(), &item))
        {
            if (json_to_any.find(opt->type()) != json_to_any.end())
            {
                auto converter = json_to_any[opt->type()];
                auto value = converter(item);
                *opt = value;
            }
            else
            {
                json_object_put(root);
                return false;
            }
        }
    }
    json_object_put(root);
    return true;
}


} // end of namespace utils
} // end of namespace intel
