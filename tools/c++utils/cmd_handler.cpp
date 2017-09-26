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
#include "cmd_handler.h"
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include "utils.h"
#include <termios.h>
#include <unistd.h>
#include <string.h>

using namespace std::placeholders;

namespace intel
{
namespace utils
{

bool cmd_handler::go_history(history direction, std::string & line)
{
    if (direction == direction_none)
    {
        return false;
    }

    if (direction == direction_back)
    {
        if (history_iter_ == history_.begin())
        {
            return false;
        }
        history_iter_--;
        line = *history_iter_;
        return true;
    }
    else
    {
        if (history_iter_ < history_.end())
        {
            history_iter_++;
            if (history_iter_ < history_.end())
            {
                line = *history_iter_;
                return true;
            }
        }
    }
    return false;
}

std::string cmd_handler::readline(const std::string & prompt)
{
    char c;
    char arrow[2];

    const char del[] = "\b \b";
    auto clear_line = [del](int count)
                      {
                          for (int i = 0; i < count; i++)
                          {
                               if (write(STDOUT_FILENO, &del, 3) < 0)
                               {
                                   std::cerr << "write: " << strerror(errno) << std::endl;
                               }
                          }
                      };
    history_iter_ = history_.end();
    std::string line, word;
    struct termios tty, tty_change;
    tcgetattr(STDIN_FILENO, &tty);
    tty_change = tty;
    tty_change.c_lflag &= ~ICANON;
    tty_change.c_lflag &= ~ECHO;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tty_change);
    if (write(STDOUT_FILENO, prompt.data(), prompt.size()) < 0)
    {
        std::cerr << "write: " << strerror(errno) << std::endl;
    }
    auto dir = direction_none;
    size_t current_size = 0;
    std::string value;
    while(true)
    {
        if (read(STDIN_FILENO, &c, 1) < 0)
        {
            std::cerr << "read: " << strerror(errno) << std::endl;
        }
        if (std::iscntrl(c))
        {
            switch(c)
            {
                case '\033':
                    current_size = line.size();
                    // eat the rest of the input
                    if (read(STDIN_FILENO, &arrow, 2) < 0)
                    {
                        std::cerr << "read: " << strerror(errno) << std::endl;
                    }
                    if (arrow[1] == 'A')
                    {
                        dir = direction_back;
                    }
                    else if (arrow[1] == 'B')
                    {
                        dir = direction_forward;
                    }
                    if (dir != direction_none && go_history(dir, line))
                    {
                        clear_line(current_size);
                        if (write(STDOUT_FILENO, line.data(), line.size()) < 0)
                        {
                            std::cerr << "write: " << strerror(errno) << std::endl;
                        }
                        dir = direction_none;
                        current_size = 0;
                    }
                    break;
                case '\n':
                    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
                    std::cout << std::endl;
                    if (line.empty() && !history_.empty())
                    {
                        value = history_.back();
                    }
                    else
                    {
                        value = rtrim(ltrim(line));
                    }

                    if (log_.is_open())
                    {
                        log_ << prompt << value << std::endl;
                    }
                    return value;
                case '\b':
                case 127:
                    if (!line.empty())
                    {
                        line.erase(line.end()-1, line.end());
                        if (write(STDOUT_FILENO, &del, 3) < 0)
                        {
                            std::cerr << "write: " << strerror(errno) << std::endl;
                        }
                    }
                    break;
                case '\t':
                    // TODO : tab complete
                    break;
            }
        }
        else
        {
            if (write(STDOUT_FILENO, &c, 1) < 0)
            {
                std::cerr << "write: " << strerror(errno) << std::endl;
            }
            if (std::iswspace(c))
            {
                if (!word.empty())
                {
                    word.clear();
                }
            }
            else
            {
                word.push_back(c);
            }
            line.push_back(c);
        }
    }
}


cmd_handler::cmd_handler()
{
    register_handler("help",
                      std::bind(&cmd_handler::do_help, this, _1),
                      0,
                      "show help message");
}

cmd_handler::~cmd_handler()
{
}

bool cmd_handler::do_help(const cmd_vector_t & cmd)
{
    if (cmd.size() == 0)
    {
        for(const auto & v : verbs_)
        {
            std::cerr << std::setw(16) << v << " - " << help_[v].second << std::endl;
        }
    }
    else
    {
        auto it = help_.find(cmd[0]);
        if (cmd.size() == 1 && it != help_.end())
        {
            std::cerr << std::setw(16) << it->first << " - " << it->second.second << std::endl;
        }
        else
        {
            std::cerr << "Unrecognized command: " << cmd[0] << std::endl;
        }
    }
    return true;
}

void cmd_handler::register_handler( const std::string & verb,
                                    cmd_func_t func,
                                    uint16_t arg_count,
                                    const std::string & help)
{
    handlers_[verb] = func;
    help_[verb] = std::make_pair(arg_count, help);
    if (std::find(verbs_.begin(), verbs_.end(), verb) == verbs_.end())
    {
        verbs_.push_back(verb);
    }
}

bool cmd_handler::do_cmd(const cmd_vector_t & cmd, std::string & help)
{
    if (cmd.size() == 0)
    {
        return false;
    }

    const std::string & verb = cmd[0];
    auto it1 = handlers_.find(verb);
    auto it2 = help_.find(verb);
    if (it1 != handlers_.end() && it2 != help_.end())
    {
        help = it2->second.second;

        if (cmd.size() < it2->second.first)
        {
            return false;
        }
        else
        {
            return it1->second(cmd_vector_t(cmd.begin()+1, cmd.end()));
        }
    }
    return false;
}

void cmd_handler::run_command_loop(const std::string & prompt)
{
    bool run = true;
    register_handler("quit",
            [&run](const cmd_vector_t & cmd)
            {
                run = false;
                return true;
            }, 0, "quit this program");

    std::string line;

    while (run)
    {
        line = readline("\n>");
        if (line.empty())
        {
            continue;
        }
        auto splits = split<std::string>(line, ' ');
        if (splits.size() > 0)
        {
            const std::string & verb = splits[0];
            if (have_cmd(verb))
            {
                std::string help;
                if (!do_cmd(splits, help))
                {
                    std::cerr << help << std::endl;
                }
                else
                {
                    add_history(line);
                }
            }
            else
            {
                std::cerr << "Unrecognized command: " << verb << std::endl;
            }
        }
    }
}

void cmd_handler::add_history(const std::string & line)
{
    history_.push_back(line);
}

} // end of namespace utils
} // end of namespace intel
