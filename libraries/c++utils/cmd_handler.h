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
#include <vector>
#include <functional>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdint>

namespace intel
{
namespace utils
{

class cmd_handler
{
public:
    typedef std::vector<std::string> cmd_vector_t;
    typedef std::function<bool(const cmd_vector_t &)> cmd_func_t;
    enum history
    {
         direction_none = 0,
         direction_back,
         direction_forward
    };

    cmd_handler();
    ~cmd_handler();

    /// @brief Register a command function with a command verb
    ///
    /// @param[in] verb The command verb
    /// @param[in] func The command function that processes the verb
    /// @param[in] arg_count The minimum number of arguments required for the command
    /// @param[in] help Help string to print out if help is requested
    void register_handler(const std::string & verb,
                          cmd_func_t func,
                          uint16_t arg_count,
                          const std::string & help);

    /// @brief Call the command function associated with a command
    ///
    /// @param[in] cmd A vector of strings. The first word is the command verb
    /// @param[out] help Help string associated with verb. This is set when the cmd
    ///                  vector isn't long enough.
    ///
    /// @return true if able to successfully run the command, false otherwise
    bool do_cmd(const cmd_vector_t & cmd, std::string & help);

    /// @brief Check if command has a registered handler
    ///
    /// @param cmd The command verb to check for
    ///
    /// @return true if a handler is registered for the verb, falase otherwise
    bool have_cmd(const std::string & cmd)
    {
        return cmd.size() > 0 && handlers_.find(cmd) != handlers_.end();
    }

    bool do_help(const cmd_vector_t & cmd);
    void show_help(std::ostream & str, bool include_help = false);
    std::string readline(const std::string & prompt);

    template<typename ...Types>
    void writeline(Types ... args)
    {
        writeline_(std::forward<Types>(args)...);
    }

    template<typename Front, typename ... Tail>
    void writeline_(Front f, Tail ... tail)
    {
        std::cout << f;
        if (log_.is_open())
        {
            log_ << f;
        }

        writeline(std::forward<Tail>(tail)...);
    }

    void writeline()
    {
        std::cout << std::endl;
        if (log_.is_open())
        {
            log_ << std::endl;
        }
    }
    void log(const std::string & filename)
    {
        if (log_.is_open())
        {
            log_.close();
        }

        log_.open(filename, std::fstream::out);
        if (!log_.is_open())
        {
            std::cerr << "ERROR opening file " << filename << std::endl;
        }
    }

    void run_command_loop(const std::string & prompt);
    void add_history(const std::string & line);
private:
    bool go_history(history direction, std::string & line);
    typedef std::map<std::string, cmd_func_t> handler_map_t;
    typedef std::map<std::string, std::pair<uint16_t, std::string>> cmd_help_map_t;
    std::fstream log_;
    handler_map_t handlers_;
    cmd_help_map_t help_;
    std::vector<std::string> verbs_;
    std::vector<std::string> history_;
    std::vector<std::string>::iterator history_iter_;
};

} // end of namespace utils
} // end of namespace intel
