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
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>

int main(int argc, char* argv[])
{
	// This is a test executable meant to be compiled with mock.c
	// This currently does nothing but is used to test if mock.c can be
	// compiled into an executable
	(void)argc;
	(void)argv;
	int fd1 = -1, fd2 = -1;
	struct stat stat1, stat2;
	fd1 = open("/dev/intel-fpga-port.0", O_RDWR);
	if (fd1 == -1){
		fprintf(stderr, "Error opening device: /dev/intel-fpga-port.0\n");
		return -1;
	}
	fd2 = open("/tmp/intel-fpga-port.0", O_RDWR);
	if (fd2 == -1){
		fprintf(stderr, "Error opening device: /tmp/intel-fpga-port.0\n");
		return -1;
	}
	if (fstat(fd1, &stat1) == -1){
		fprintf(stderr, "Error stat on fd1\n");
		return -1;
	}
	if (fstat(fd2, &stat2) == -1){
		fprintf(stderr, "Error stat on fd1\n");
		return -1;
	}

	return (stat1.st_dev == stat2.st_dev && stat2.st_ino == stat2.st_ino) ? 0 : -1;

}
