#!/bin/bash
## Copyright(c) 2016-2017, Intel Corporation
##
## Redistribution  and  use  in source  and  binary  forms,  with  or  without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of  source code  must retain the  above copyright notice,
##   this list of conditions and the following disclaimer.
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
## * Neither the name  of Intel Corporation  nor the names of its contributors
##   may be used to  endorse or promote  products derived  from this  software
##   without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
## IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
## LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
## CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
## SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
## INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
## CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.

os=$(uname -s | tr '\[A-Z\]' '\[a-z\]')
kernel_rel=$(uname -r)
arch=$(uname -p)
dist_id=$(lsb_release -i -s | tr '\[A-Z\]' '\[a-z\]')
dist_ver=$(lsb_release -r -s | tr '\[A-Z\]' '\[a-z\]')
dist_code=$(lsb_release -c -s | tr '\[A-Z\]' '\[a-z\]')
shm_testfile="/dev/shm/$USER.ase_envcheck"

## Version greater than tester function
function version_check()
{
    test "$(echo "$@" | tr " " "\n" | sort -V | head -n 1)" != "$1";
}

## Print header, and basic info
echo "#################################################################"
echo "#                                                               #"
echo "#             Intel(R) Xeon(R) + FPGA OPAE Library              #"
echo "#               AFU Simulation Environment (ASE)                #"
echo "#                                                               #"
echo "#################################################################"
echo "  Checking Machine... "
echo "  Operating System = ${os} "
echo "  Kernel Release   = ${kernel_rel}"
echo "  Machine          = ${arch}"
echo "  Distro ID        = ${dist_id}"
echo "  Distro Version   = ${dist_ver}"
echo "  Distro Code      = ${dist_code}"
echo "-----------------------------------------------------------"

## If Machine is not 64-bit, flash message
if [ "$os" == "linux" ]; then
    if [ "$arch" == "x86_64" ]; then
	echo "  [INFO] 64-bit Linux found"
    else
	echo "  [WARN] 32-bit Linux found --- ASE works best on 64-bit Linux !"
    fi
    # Check distro
    if   [ "$dist_id" == "ubuntu" ] ; then
	if version_check "$dist_ver" "12.04"; then
    	    echo "  [INFO] Ubuntu $dist_ver found"
	else
	    echo "  [WARN] ASE behavior on Ubuntu $dist_ver is unknown !"
	fi
    elif [ "$dist_id" == "suse linux" ] ; then
    	echo "  [INFO] SLES found"
	if version_check "$dist_ver" "10"; then
	    echo "  [INFO] SLES version seems to be OK"
	else
	    echo "  [WARN] ASE behaviour on SLES < 11 is unknown !"
	fi
    else
    	echo "  [WARN] Machine is running an unknown Distro --- ASE compatibility unknown !"
    fi
else
    echo "  [WARN] Non-Linux distro found --- ASE is not supported on non-Linux platforms !"
fi


echo "-----------------------------------------------------------"

## Check shell environment
shell=$(basename "$SHELL")
echo "  [INFO] SHELL identified as ${shell} (located \"$SHELL\")"
if   [ "$shell" == "bash" ] ; then
    echo "  [INFO] SHELL ${shell} version : \"$BASH_VERSION\""
elif [ "$shell" == "zsh" ] ; then
    echo "  [INFO] SHELL ${shell} version : $(zsh --version)"
elif [ "$shell" == "tcsh" ] ; then
    echo "  [INFO] SHELL ${shell} version : $(tcsh --version)"
elif [ "$shell" == "csh" ] ; then
    echo "  [INFO] SHELL ${shell} version : $(csh --version)"
else
    echo "  [WARN] SHELL ${shell} is unknown !"
fi
echo "-----------------------------------------------------------"

## Check if /dev/shm is mounted, try writing then deleting a file for access check
if [ -d /dev/shm/ ]; then
    echo "  [INFO] /dev/shm is accessible ... testing further"
    echo "  [INFO] Testing with file \"$shm_testfile\""
    touch "$shm_testfile"
    echo "$USER" >> "$shm_testfile"
    readback_shmfile=$(cat "$shm_testfile")
    if [ "$readback_shmfile" == "$USER" ] ; then
	echo "  [INFO] SHM self-check completed successfully."
    else
	echo "  [WARN] SHM self-check failed !"
    fi
    rm "$shm_testfile"
else
    echo "  [WARN] /dev/shm seems to be inaccessible ! "
    echo "  [WARN] ASE uses this location for data sharing between SW and simulator"
    echo "  [WARN] Please mount this location before proceeding...  see 'man shm_overview'"
fi

echo "-----------------------------------------------------------"

## GCC version check
# GCCVERSION=$(gcc --version | grep ^gcc | sed 's/^.* //g')
GCCVERSION=$(gcc -dumpversion)
echo "  [INFO] GCC version found : $GCCVERSION"
if version_check "$GCCVERSION" "4.4"; then
    echo "  [INFO] GCC version seems to be OK"
else
    echo "  [WARN] Possible incompatible GCC found in path"
    echo "  [INFO] ASE recommends using GCC version > 4.4"
fi
echo "-----------------------------------------------------------"

## Python version check
PYTHONVER=$(python -c 'import sys; print(".".join(map(str, sys.version_info[:3])))')
echo "  [INFO] Python version found : $PYTHONVER"
if version_check "$PYTHONVER" "2.7"; then
    echo "  [INFO] Python version seems to be OK"
else
    echo "  [WARN] Possible incompatible Python found in path"
    echo "  [INFO] ASE recommends using Python version > 2.7"
fi
echo "-----------------------------------------------------------"

## RTL tool check
if [ "$VCS_HOME" ] ; then
    echo "  [INFO] env(VCS_HOME) is set."
    if [ -x "$(command -v vcs)" ] && [ -x "$(command -v vlogan)" ] && [ -x "$(command -v vhdlan)" ] ; then
	echo "  [INFO] $(type vhdlan)"
	echo "  [INFO] $(type vlogan)"
	echo "  [INFO] $(type vcs)"
    else
	echo "  [WARN] VCS commands (vcs, vlogan, vhdlan) was not found !"
	echo "  [WARN] Check VCS settings !"
    fi
elif [ "$QUESTA_HOME" ] ; then
    echo "  [INFO] env(QUESTA_HOME) is set."
    if [ -x "$(command -v vlog)" ] && [ -x "$(command -v vlib)" ] && [ -x "$(command -v vsim)" ] ; then
	echo "  [INFO] $(type vlib)"
	echo "  [INFO] $(type vlog)"
	echo "  [INFO] $(type vsim)"
    else
	echo "  [WARN] VCS commands (vcs, vlogan, vhdlan) was not found !"
	echo "  [WARN] Check VCS settings !"
    fi
else
    echo "  [WARN] No Compatible RTL tool seems to be available !"
fi

echo "-----------------------------------------------------------"
## Quartus version not available
if [ "$QUARTUS_HOME" ] ; then
    echo "  [INFO] env(QUARTUS_HOME) is set."
    if [ -x "$(command -v quartus)" ] ; then
	echo "  [INFO] $(type quartus)"
    else
	echo "  [WARN] quartus command not found !"
	echo "  [WARN] Check Quartus settings !"
    fi
else
    echo "  [WARN] Quartus not found, ASE won't run Altera eda_lib library simulation !"
    echo "  [INFO] Alternately, if you have a non-standard Quartus install, the Makefile may need editing"
fi
echo "-----------------------------------------------------------"
