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
#include <iostream>
#include <string>
#include <memory>
#include <type_traits>
#include <sstream>
#include "any_value.h"

namespace intel
{
namespace utils
{



/// @brief option is a base class to hold any value with an expected value type
///  It is intended to be used for command line parsing as well as JSON parsing
class option
{
public:
    typedef std::shared_ptr<option> ptr_t;
    enum option_type
    {
        no_argument   = 0,
        with_argument,
        optional_argument
    };

    option(const std::string & name, char short_opt, option::option_type has_arg, const std::string & help, any_value default_value = nullptr)
    : is_set_(false)
    , name_(name)
    , short_opt_(short_opt)
    , option_type_(has_arg)
    , help_(help)
    , default_(default_value)
    {

    }

    option(const option & other)
    : is_set_(other.is_set_)
    , name_(other.name_)
    , short_opt_(other.short_opt_)
    , option_type_(other.option_type_)
    , help_(other.help_)
    , value_(other.value_)
    , default_(other.default_)
    {

    }
    virtual ~option() {};

    option & operator=(const option & other)
    {
        if (&other != this)
        {
            is_set_ = other.is_set_;
            name_ = other.name_;
            short_opt_ = other.short_opt_;
            option_type_ = other.option_type_;
            help_ = other.help_;
            value_ = other.value_;
            default_ = other.default_;
        }
        return *this;
    }

    template<typename T>
    static ptr_t create(const std::string & name, char short_opt, option::option_type has_arg, const std::string & help, T default_value = T());

    template<typename T>
    static ptr_t create(const std::string & name, option::option_type has_arg, const std::string & help = "", T default_value = T());

    virtual const std::string & name() const
    {
        return name_;
    }

    const std::string & help()
    {
        return help_;
    }

    bool is_set()
    {
        return is_set_;
    }

    virtual void show_help(std::ostream & os) = 0;
#if DEBUG
    virtual void show(std::ostream & os) = 0;
#endif

    virtual const std::type_info & type() = 0;

    template<typename T>
    T value()
    {
        return is_set_ ? value_.value<T>() : default_.value<T>();
    }

    template<typename T>
    void set_default(T d)
    {
        default_ = d;
    }

    template<typename T>
    option & operator=(T v)
    {
        is_set_ = true;
        value_ = v;
        return *this;
    }

    option_type has_arg()
    {
        return option_type_;
    }

    char short_opt()
    {
        return short_opt_;
    }

    virtual std::string to_string() = 0;

    virtual any_value any() { return value_; }

protected:
    option()
    : is_set_(false)
    , short_opt_(0)
    , option_type_(no_argument)
    {
    }

private:
    bool        is_set_;
    std::string name_;
    char        short_opt_;
    option_type option_type_;
    std::string help_;
    any_value   value_;
    any_value   default_;
};

template<typename T>
class typed_option : public option
{
public:
    typedef std::shared_ptr<typed_option<T>> ptr_t;

    typed_option()
    {
    }

    typed_option(const std::string & name, any_value default_value = 0)
    : option(name, 0, option::with_argument, "", default_value)
    {

    }

    typed_option(const std::string & name, char short_opt, option::option_type has_arg, const std::string & help, any_value default_value = 0)
    : option(name, short_opt, has_arg, help, default_value)
    {
    }

    virtual void show_help(std::ostream & os)
    {
        char sopt = short_opt();
        T    dval = value<T>();

        os << "    --" << name();

        if (sopt)
        {
            os << ", -" << sopt << ".";
        }

        os << " " << help();

        if (name() != "help" && dval != T())
        {
           os << " Default=" << dval;
        }

        os << std::endl;
    }

#if DEBUG
    virtual void show(std::ostream & os)
    {
        os << name() << ": \t" << "is-set=" << is_set() << "\tvalue=" << value<T>() << "\n";
    }
#endif

    virtual const std::type_info & type()
    {
        return typeid(T);
    }

    virtual std::string to_string()
    {
        std::stringstream ss;
        ss << value<T>();
        auto str = ss.str();
        if (!std::is_arithmetic<T>())
        {
            str = "\"" + str + "\"";
        }
        return str;
    }

};

template<typename T>
option::ptr_t option::create(const std::string & name, char short_opt, option::option_type has_arg, const std::string & help, T default_value)
{
    return option::ptr_t(new typed_option<T>(name, short_opt, has_arg, help, default_value));
}

template<typename T>
option::ptr_t option::create(const std::string & name, option::option_type has_arg, const std::string & help, T default_value)
{
    return option::ptr_t(new typed_option<T>(name, 0, has_arg, help, default_value));
}

} // end of namespace utils
} // end of namespace intel
