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
#pragma once
#include <map>
#include <string>
#include <memory>
#include "option.h"
#include "log.h"
#include <vector>

namespace intel
{
namespace utils
{

class option_map
{
public:
    typedef std::shared_ptr<option_map> ptr_t;

    option_map()
    : options_()
    , options_list_()
    {
    }

    option_map(const option_map & other)
    : options_(other.options_)
    , options_list_(other.options_list_)
    {
    }

    option_map & operator=(const option_map & other)
    {
        if (&other != this)
        {
            options_ = other.options_;
            options_list_ = other.options_list_;
        }
        return *this;
    }

    template<typename T>
    option::ptr_t add_option(const std::string & name, char short_opt, option::option_type has_arg, const std::string & help,  T default_value = T())
    {
        return add_option(option::create<T>(name, short_opt, has_arg, help, default_value));
    }

    template<typename T>
    option::ptr_t add_option(const std::string & name, option::option_type has_arg = option::option_type::with_argument, const std::string & help = "", T default_value = T())
    {
        return add_option(option::create<T>(name, 0, has_arg, help, default_value));
    }

    option::ptr_t add_option(option::ptr_t opt)
    {
        if (opt)
        {
            options_[opt->name()] = opt;
            options_list_.push_back(opt);
            if (opt->short_opt() > 0)
            {
                if (short_map_.find(opt->short_opt()) == short_map_.end())
                {
                    short_map_[opt->short_opt()] = opt;
                }
                else
                {
                    log_.error("option_map") << "duplicate short option - not inserting: " << (opt->short_opt()) << std::endl;
                }
            }
        }
        return opt;
    }

    static ptr_t create(const std::vector<option::ptr_t> & options);
    static ptr_t create(const std::initializer_list<option::ptr_t> & options);

    template<typename T>
    option::ptr_t add_option(const std::string & name, T default_value)
    {
        option::ptr_t opt(new typed_option<T>(name, default_value));
        options_[name] = opt;
        options_list_.push_back(opt);
        return opt;
    }

    bool remove_option(const std::string & name)
    {
        auto vec_it = std::find_if(options_list_.begin(),
                                   options_list_.end(),
                                   [&name](option::ptr_t o)
                                   {
                                       return o->name() == name;
                                   });
        auto map_it = options_.find(name);
        if (vec_it != options_list_.end() &&
            map_it != options_.end())
        {
            options_list_.erase(vec_it);
            options_.erase(map_it);
            return true;
        }
        return false;
    }

    template<typename T>
    T get_value(const std::string & name)
    {
        auto it = options_.find(name);
        if (it == options_.end())
        {
            return T();
        }
        return it->second->value<T>();
    }

    template<typename T>
    bool get_value(const std::string & name, T & value)
    {
        auto it = options_.find(name);
        if (it != options_.end())
        {
            value = it->second->value<T>();
            return true;
        }
        // TODO: Add debug log message here indicating no option found
        return false;

    }

    template<typename T>
    bool set_value(const std::string & name, const T & value)
    {
        auto it = options_.find(name);
        if (it != options_.end())
        {
            *(it->second) = value;
            return true;
        }
        // TODO: Add debug log message here indicating no option found
        return false;

    }

    option::ptr_t operator[](const std::string & name)
    {
        auto it = options_.find(name);
        if (it == options_.end())
        {
            return option::ptr_t();
        }
        return it->second;
    }

    option::ptr_t operator[](size_t index)
    {
        return options_list_[index];
    }

    std::vector<option::ptr_t>::iterator begin()
    {
        return options_list_.begin();
    }

    std::vector<option::ptr_t>::iterator end()
    {
        return options_list_.end();
    }

    option::ptr_t find(const std::string & key)
    {
        auto it = options_.find(key);
        if (it == options_.end())
        {
            return option::ptr_t();
        }
        return it->second;
    }

    option::ptr_t find(const std::string & key) const
    {
        auto it = options_.find(key);
        if (it == options_.end())
        {
            return option::ptr_t();
        }
        return it->second;
    }

    void show_help(const std::string & name, std::ostream &os)
    {
        os << "Usage: " << name << " [options]" << std::endl
           << std::endl;

        for (const auto & it : *this)
        {
            it->show_help(os);
        }
    }

    bool empty()
    {
        return options_.empty();
    }

private:
    logger log_;
    std::map<std::string, option::ptr_t> options_;
    std::map<char, option::ptr_t> short_map_;
    std::vector<option::ptr_t> options_list_;
};

} // end of namespace utils
} // end of namespace intel
