#!/bin/bash
## Copyright(c) 2014-2017, Intel Corporation
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

set -e

##################################
#                                #
# Usage notes:                   #
# $ ./create_ase_simbuild_env.sh #
#                                #
##################################

## Pseudocode
## ----------------------------------------------------------------------------------
## * Check if 'ase' directory exists in current directory
##   * FALSE:
##     * Create references
##       * $ASE_SRCDIR - Find running directory
##       * Check if $ASE_SRCDIR/rtl/platform.vh and $ASE_SRCDIR/sw/ase_common.h exist
##         * TRUE
##           * Create 'ase' directory
##           * Copy ase/Makefile script, then ase_regress.sh, and ase.cfg
##           * grep/sed ASE_SRCDIR to point to retrieved source directory
##         * FALSE
##           * Error exit here
##     * Copy 'ase' directory from
##   * TRUE:
##     * Error exit here
##

echo "#################################################################"
echo "#                                                               #"
echo "#             OPAE Intel(R) Xeon(R) + FPGA Library              #"
echo "#               AFU Simulation Environment (ASE)                #"
echo "#                                                               #"
echo "#################################################################"

## Check if 'ase' directory exists, else EXIT
if [ -d ${PWD}/ase ] ;
then
    echo "ERROR: 'ase' already exists in this location.. Script will EXIT here.\n"
    echo "       Please run the script in another location.\n"
    exit 1
fi

## Check for ASE script origin location
run_from_dir=$(dirname `readlink -e $0`)/
echo "Script running from: $run_from_dir"

## Check ASE source paths
check_rtlpath=${run_from_dir}/../rtl/platform.vh
check_swpath=${run_from_dir}/../sw/ase_common.h

## Check SW and RTL paths
if [ -f ${check_rtlpath} ] && [ -f ${check_swpath} ] ;
then
    echo "ASE Sources found... will proceed to create directory"
    ase_srcdir=$(dirname $run_from_dir/)
    ## Print ASE location
    echo "ASE Source directory: $ase_srcdir"
else
    echo "ERROR: ASE Sources could not be found\n"
    echo "       Check the Package Distribution documentation, and that the script hasn't been modified"
    exit 1
fi


## Create ASE Directory
mkdir ase
cp $ase_srcdir/Makefile ./ase/
cp $ase_srcdir/ase.cfg ./ase/
cp $ase_srcdir/ase_regress.sh ./ase/

## Change permission of 'ase' directory
chmod 644 ase/Makefile ase/ase.cfg
chmod 744 ase/ase_regress.sh

## Modify ASE_SRCDIR location
## grep/sed and replace only the first instance of ASE_SRCDIR
sed -i 's#^ASE_SRCDIR.*#ASE_SRCDIR = '$ase_srcdir'#g' ase/Makefile

## Print information about ase_sources.mk
echo ""
echo "ASE simulator build environment created"
echo "Next steps: This simulation environment must be configured for an AFU"
echo "            See ASE Documentation on usage steps"
echo ""
