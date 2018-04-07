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

/*
 * daemonize.c : routine to become a system daemon process.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

int daemonize(void (*hndlr)(int, siginfo_t *, void *), mode_t mask, const char *dir)
{
	pid_t pid;
	pid_t sid;
	int res;
	int fd;
	struct sigaction sa;

	pid = fork();
	if (pid < 0) // fork() failed.
		return errno;

	// 1) Orphan the child process so that it runs in the background.
	if (pid > 0)
		exit(0);

	// 2) Become leader of a new session and process group leader of new process
	// group. The process is now detached from its controlling terminal.
	sid = setsid();
	if (sid < 0)
		return errno;

	// 3) Establish signal handler.
	memset(&sa, 0, sizeof(sa));
	sa.sa_flags     = SA_SIGINFO | SA_RESETHAND;
	sa.sa_sigaction = hndlr;

	res = sigaction(SIGINT, &sa, NULL);
	if (res < 0)
		return errno;

	// TODO handle other signals

	// 4) Orphan the child again - the session leading process terminates.
	// (only session leaders can request TTY).
	pid = fork();
	if (pid < 0) // fork() failed.
		return errno;

	if (pid > 0)
		exit(0);

	// 5) Set new file mode mask.
	umask(mask);

	// 6) change directory
	res = chdir(dir);
	if (res < 0)
		return errno;

	// 7) Close all open file descriptors
	fd = sysconf(_SC_OPEN_MAX);
	while (fd >= 0)
		close(fd--);

	return 0;
}

