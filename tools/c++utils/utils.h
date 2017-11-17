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
/// @file utils.h
/// @brief utility functions and classes

#pragma once
#include <string>
#include <vector>
#include <typeinfo>
#include <random>
#include <map>
#include <iostream>
#include <iomanip>

/// @brief utility functions grouped under the utils namespace
namespace intel
{
namespace utils
{
    /// @brief Use RTTI to get a type name
    /// @param[in] tinfo the typeid of a type
    /// @returns the string type name (includes namespace)
    std::string get_typename(const std::type_info &tinfo);

    /// @brief Template version of get_typename function
    /// @tparam T the type to get the type of
    /// @returns the string type name of T
    template<typename T>
    std::string get_typename()
    {
        return get_typename(typeid(T));
    }

    /// @brief Checks if a path exists in the filesystem
    //
    /// @param[in] path to a filesystem object
    /// @returns true if the object exists, false otherwise
    bool path_exists(const std::string &path);

    std::string rtrim(std::string str);
    std::string ltrim(std::string str);

    /// @brief Splits an input string using a string delimeter
    //
    /// @param[in] inp input string to split
    /// @param[in] delim delimeter string to split the string on
    /// @returns a vector of substrings of the input string
    // separated by the delimeter
    template<typename T = std::string, typename U = char>
    std::vector<T> split(const T &inp, const U &delim)
    {
        std::vector<T> out;
        std::size_t pos = 0;
        while(pos < inp.size())
        {
            auto next = inp.find(delim, pos);
            if (next == T::npos && pos < inp.size())
            {
                out.push_back(rtrim(inp.substr(pos, inp.size() - pos)));
                break;
            }
            else if (next > pos)
            {
                out.push_back(rtrim(inp.substr(pos, next-pos)));
                pos = next + 1;
            }
            else if (next == pos)
            {
                pos++;
            }
        }

        return out;
    }

    void csv_parse(const std::string & filename,
                   std::map<std::string, std::vector<std::string>> & data);

    void csv_parse(std::istream & stream,
                   std::map<std::string, std::vector<std::string>> & data);

    void csv_parse(std::istream & stream,
                   std::vector<std::vector<std::string>> & data, bool by_row = true);

    /// @brief Helper function to parse a string
    ///        Hex numbers must be prefixed with 0x or 0X
    ///        as this function has logic to use radix of 16 if string begins with 0x or 0X
    ///
    /// @param[in] str
    /// @param[out] value
    ///
    /// @return true if able to parse the string
    template<typename T>
    bool parse_int(const std::string & str, T & value)
    {
        auto sub = str.substr(0,2);
        size_t index = sub == "0x" || sub == "0X" ? 2 : 0;
        int radix = index == 0 ? 10 : 16;
        try
        {
            value = std::stoi(str.c_str(), &index, radix);
            return true;
        }
        catch(std::invalid_argument &e)
        {
            std::cerr << "Exception caught: " << e.what() << " - could not convert " << str << " to a number" << std::endl;
        }
        return false;
    }


    /// @brief random integer generator
    /// @details
    /// Used to generate random integers between
    /// a given range.
    /// Usage random r(1,100); int number = r();
    /// @tparam lo_ the low end of the range
    /// @tparam hi_ the high end of the range
    /// @tparam IntTYpe the type of integer to generage
    /// short, int, long, unsigned, ...
    template<typename IntType = int>
    class random
    {
        public:
            /// @brief random<> constructor
            random(IntType lo, IntType hi):
                gen_(rdev_()),
                dist_(lo, hi)
            {
            }

            /// @brief generates a random integer
            /// @returns a random number between range
            /// lo_ and hi_
            IntType operator()()
            {
                return dist_(gen_);
            }

        private:
            std::random_device rdev_;
            std::mt19937 gen_;
            std::uniform_int_distribution<IntType> dist_;
    };

    template<typename T>
    struct print_hex
    {
        template<typename U>
        print_hex(U value)
            : value_(static_cast<T>(value))
            , width_(sizeof(T)*2)
        {
        }
        friend std::ostream& operator<<(std::ostream& os, const print_hex<T> & v)
        {
            os << "0x" << std::hex << std::setw(v.width_) << std::setfill('0') << +(v.value_);
            os.copyfmt(std::ios(0));
            return os;
        }

    private:
        T value_;
        size_t width_;

    };

#define UNUSED_PARAM(x) (void)x

}
}
