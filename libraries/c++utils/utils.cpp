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
#include "utils.h"
#include <sys/stat.h>
#ifdef __linux
#include <cxxabi.h>
#endif

#include <algorithm>
#include <cwctype>
#include <fstream>

namespace intel
{
namespace utils
{

    std::string get_typename(const std::type_info& tinfo)
    {
#ifdef __linux
        int status;
        std::string demangled_name = abi::__cxa_demangle(tinfo.name(), 0, 0, &status);
        return demangled_name;
#else
        return tinfo.name();
#endif
    }

    bool path_exists(const std::string &path)
    {
        std::fstream fs(path);
        return fs.good();
    }

    std::string rtrim(std::string str)
    {
        auto it = str.end()-1;
        while(std::iswspace(*it))
        {
            it--;
        }
        if (it < str.end())
        {
            it++;
            str.erase(it, str.end());
        }

        return str;
    }

    std::string ltrim(std::string str)
    {
        auto it = str.begin();
        while(std::iswspace(*it))
        {
            it++;
        }

        if (it > str.begin())
        {
            str.erase(str.begin(), it);
        }

        return str;
    }

void csv_parse(const std::string & filename,
               std::map<std::string, std::vector<std::string>> & data)
{
    std::ifstream filestream(filename, std::ifstream::in);
    csv_parse(dynamic_cast<std::istream&>(filestream), data);
}

void csv_parse(std::istream & stream,
               std::map<std::string, std::vector<std::string>> & data)
{
    std::vector<std::vector<std::string>> raw_data;
    csv_parse(stream, raw_data, false);
    for(auto it : raw_data)
    {
        std::string key = it.front();
        it.erase(it.begin());
        data[key] = it;
    }
}

void csv_parse(std::istream & stream,
               std::vector<std::vector<std::string>> & data, bool by_row)
{
    std::vector<std::string> lines;
    std::string line;
    std::getline(stream, line);
    auto pos = stream.tellg();
    size_t column_count = std::count(line.begin(), line.end(), ',') + 1;
    size_t line_count = std::count(std::istreambuf_iterator<char>(stream),
                                   std::istreambuf_iterator<char>(), '\n') + 1;
    stream.seekg(pos);
    if (by_row)
    {
        data.reserve(line_count);
    }
    else
    {
        data.resize(column_count);
    }

    while (true)
    {
        if (line.size() > 0 && line[0] != '#')
        {
            auto splits = split(line, ",");
            if (by_row)
            {
                data.push_back(splits);
            }
            else
            {
                if (splits.size() == column_count)
                {
                    for (size_t i = 0; i < column_count; ++i)
                    {
                        data[i].push_back(splits[i]);
                    }
                }
            }
        }

        if (!std::getline(stream,  line))
        {
            break;
        }
    }
}


}
}
