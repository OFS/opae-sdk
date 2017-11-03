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
#include "process.h"
#include <algorithm>
#include <future>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>

using namespace std;
using namespace std::chrono;

namespace intel
{
namespace utils
{
process::process(int pid) : pid_(pid)
{

}

process::~process()
{

}

int process::wait(int timeout_msec)
{
    // make a task to call the waitid call
    packaged_task<int()> task([this]()
                {
                    siginfo_t info;
                    int options = WEXITED | WSTOPPED;
                    waitid(P_PID, this->pid_, &info, options);
                    if (info.si_code == CLD_KILLED || info.si_code == CLD_DUMPED)
                    {
                        cerr << "Warning: child exited abnormally with code: "
                             << info.si_status << std::endl;
                    }
                    return info.si_status;
                });

    // get the task's future value
    future<int> fv = task.get_future();
    // run the task in a detached thread (since we will wait on the future value)
    thread(move(task)).detach();
    if ( timeout_msec > -1 && fv.wait_for(milliseconds(timeout_msec)) == future_status::timeout)
    {
        return -1;
    }
    // wait on the value
    fv.wait();
    // return the value returned by the task (lambda)
    return fv.get();
}

void process::terminate()
{
    kill(pid_, SIGINT);
}

void process::terminate(int signal)
{
    kill(pid_, signal);
}

process process::start(const string &file, const vector<string> &args)
{
    // new args are first the program name,
    // followed by the arguments
    // terminated with a null char
    char** cargs = new char*[args.size()+2];

    // set the program name
    cargs[0] = const_cast<char*>(file.c_str());

    // copy the args vector to char**
    // starting at index 1
    for (int i = 0; i < args.size(); ++i)
    {
        cargs[i+1] = const_cast<char*>(args[i].c_str());
    }

    // last element is null char
    cargs[args.size()+1] = 0;

    pid_t child = fork();

   // if we got a good pid, return that to caller
   if (child != 0)
    {
        delete []cargs;
        return process(child);
    }
    // else (in addition), invoke external application in the child process
    else
    {
        execve(file.c_str(), cargs, { nullptr });
        // shouldn't be here
        cerr << "We shouldn't be here after exec" << endl;
        abort();
    }
}

void process::set_thread_maxpriority(const std::thread & t)
{
    struct sched_param sched;
    auto native_handle = const_cast<std::thread&>(t).native_handle();
    int policy = SCHED_FIFO;
    auto result = pthread_getschedparam(native_handle, &policy, &sched);
    if (result == 0)
    {
        sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
        result = pthread_setschedparam(native_handle, SCHED_FIFO, &sched);
        if (result != 0)
        {
            std::cerr << "Could not set sched parameters: " << strerror(result) << "\n";
        }
    }
}

void process::set_process_maxpriority(pid_t p)
{
    auto max_priority = sched_get_priority_max(SCHED_FIFO);
    struct sched_param sched = { .sched_priority = max_priority };
    auto result = sched_setscheduler(p, SCHED_FIFO, &sched);
    if (result != 0)
    {
        std::cerr << "Could not set sched parameters: " << strerror(result) << "\n";
    }
}

} // end of namespace utils
} // end of namespace intel
