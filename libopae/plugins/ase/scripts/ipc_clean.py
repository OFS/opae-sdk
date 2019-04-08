#!/usr/bin/env python
# Copyright(c) 2013-2017, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
#   may be used to  endorse or promote  products derived  from this  software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
# CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

from __future__ import print_function
import ase_functions
import os
import re
import sys
import signal
from subprocess import call

if sys.version_info < (2, 7):
    import commands
else:
    import subprocess


# Run command and get string output #
def commands_getoutput(cmd):
    if sys.version_info < (2, 7):
        return commands.getoutput(cmd)
    else:
        byte_out = subprocess.check_output(cmd.split())
        str_out = byte_out.decode("utf-8")
        return str_out


MQUEUE_MOUNT = "./work/"
SHM_MOUNT = "/dev/shm/"
GLOBAL_LIST = MQUEUE_MOUNT + "/.ase_ipc_global"
USER = os.getenv("USER")

print("############################################################")
print("#                                                          #")
print("#                ASE IPC Cleanup script                    #")
print("#                                                          #")
print("############################################################")

# Check if locations are mounted correctly #
print_admin = 0
if os.path.isdir(SHM_MOUNT) is False:
    print('* WARNING: ", SHM_MOUNT, " location is not mounted in this '
          'system. *')
    print_admin = 1


# Ask Admin to mount /dev/shm #
if print_admin == 1:
    print("Please contact your Linux administrator for help.")
    print('PLEASE NOTE: ASE will continue to function, but should IPC '
          'constructs misbehave, some of the constructs listed in ',
          GLOBAL_LIST, ' will need to be removed manually.')


# Straightforward delete operation #
if print_admin == 0:
    print('IPC mounts seem to be readable... will attempt cleaning up IPC '
          'constructs by user ', USER, "'")
    problem_ipc = commands_getoutput("find " + SHM_MOUNT + " -type f -user "
                                     + USER).split("\n")
    for ipc in problem_ipc:
        if os.path.isfile(ipc) is True:
            print("Removing ", ipc)
            os.unlink(ipc)


# Remove ready file #
print("Removing .ase_ready file ... ")
ready_list = commands_getoutput("find . -name .ase_ready.pid").split("\n")
for rfile in ready_list:
    rfile = rfile.replace("\n", "")
    if os.path.isfile(rfile) is True:
        print(rfile)
        os.unlink(rfile)


# Kill all ase_simv processes started by by $USER #
cleanme_input = raw_input("Type 'y' to clean up all zombie ase_simv "
                          "processes : ")
if cleanme_input == "y":
    print("Going ahead with cleaning up ASE processes opened by ", USER)
    os.system("pgrep ase_simv -u " + USER + " | xargs kill -9 ")
else:
    print("Skipping process removal")
