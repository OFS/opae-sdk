#!/bin/bash -e
#
# Copyright(c) 2015-2016, Intel Corporation
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
#****************************************************************************
# @file test-helloalivtpnlb-ase.sh
# @brief Test script for running Hello_ALI_VTP_NLB
# @verbatim
# Intel(R) QuickAssist Technology Accelerator Abstraction Layer
# Memory Property Factory / Virtual to Physical
#
# Test script to build and run the following components for ASE simulation:
#
#    - simulation model for NLB including MPF with VTP
#    - VTP AAL service
#    - Hello_ALI_VTP_NLB sample application
#
# AUTHORS: Enno Luebbers, Intel Corporation
#
# HISTORY:
# WHEN:          WHO:     WHAT:
# 02/24/2016     EL       Initial version@endverbatim
#****************************************************************************

# =============================================================================
# INITIAL SETUP
# - no need to edit
# =============================================================================

# save current directory
OLDDIR=`pwd`

# derive relatve paths
cd `dirname $0`
SCRIPT_DIR=`pwd`
CCI_MPF=$SCRIPT_DIR/..

# =============================================================================
# CONFIGURE TEST RUN
# - edit this
# - set paths to external components as necessary
# - modify HDL package elaboration order
# =============================================================================

# the AFU RTL to simulate
if [ -z "$NLB_MPF_SRC" ]; then
   NLB_MPF_SRC=/home/eluebber/work/bdx_fpga_piu/design/afu/nlb_400
fi

# path to the installed AALSDK (prefix)
AALSDK_INST_PREFIX=$HOME/test

# path to ASE (inside aalsdk source tree)
if [ -z "$ASE_DIR" ]; then
   ASE_DIR=$HOME/work/aalsdk/aaluser/ase
fi

# path to AAL sample application
SAMPLE_DIR=$CCI_MPF/sample/Hello_ALI_VTP_NLB/SW
SAMPLE_BIN=helloALIVTPnlb

# the HDL package files _in the order they need to be elaborated_ (for vcs)
VLOG_ORDER=(
   "ccis_if_pkg.sv"
   "ccis_if_funcs_pkg.sv"
   "ccip_if_funcs_pkg.sv"
   "cci_mpf_csrs_pkg.sv"
   "cci_mpf_if_pkg.sv"
   "cci_csr_if_pkg.sv"
   "ccip_feature_list_pkg.sv"
)

# number of lines to skip in vlog_files.list (usually size of VLOG_ORDER+2)
VLOG_SKIP=$(( ${#VLOG_ORDER[@]} + 2 ))

# time to wait for simulator to come up (to avoid hanging on missin license)
SIM_TIMEOUT=60

# configure shell for test run
ulimit -n 4096
if [ -e /opt/synopsys/vcs-mx/vcs.sh ]; then
   source /opt/synopsys/vcs-mx/vcs.sh
fi

if [ -e /opt/altera/quartus_15.sh ]; then
   source /opt/altera/quartus_15.sh
fi
# =============================================================================
# no need to edit below here
# =============================================================================

CCI_MPF_LIBS=$CCI_MPF/sw
CCI_MPF_INCLUDE=$CCI_MPF/sw/include
BUILDDIR=$CCI_MPF/build
NLB_MPF=$BUILDDIR/hw/NLB
VLOG_FILES=$ASE_DIR/vlog_files.list
VLOG_FILES_TEMP=/tmp/new_vlog_files.list
SIM_READY=$ASE_DIR/work/.ase_ready.pid

# safety check, don't clobber HDL
if [ -e $BUILDDIR ]; then
   echo "Build dir \"$BUILDDIR\" exists, please delete."
   exit -1
fi
mkdir $BUILDDIR

# -----------------------------------------------------------------------------
# assemble NLB HW
# -----------------------------------------------------------------------------
echo "=== ASSEMBLE NLB HW ==="
# copy test AFU RTL
mkdir -p $NLB_MPF
cp -r $NLB_MPF_SRC/* $NLB_MPF/
cd $NLB_MPF
# rm QSYS instantiation templates
find . -name "*_inst\.*" -exec rm "{}" \;
find . -name "sim" -type d | xargs rm -rf
find . -name "*_bb\.*" -exec rm "{}" \;
# link in MPF RTL
ln -s $CCI_MPF/hw/rtl
# fix up CSRs for DFH list
sed -i "s/^\s*localparam\s\+END_OF_LIST.*$/localparam END_OF_LIST = 1'h0;/" nlb_csr.sv
sed -i "s/^\s*localparam\s\+NEXT_DFH_BYTE_OFFSET.*$/localparam NEXT_DFH_BYTE_OFFSET = { 8'h00 + 16'h1000 };/" nlb_csr.sv

# -----------------------------------------------------------------------------
# set up ASE
# -----------------------------------------------------------------------------
echo "=== SET UP ASE ==="
cd $ASE_DIR
scripts/generate_ase_environment.py $NLB_MPF -t VCS
make clean

# fix build order
rm -f $VLOG_FILES_TEMP
# first add packages in order
for i in ${VLOG_ORDER[@]}; do
   grep "$i" $VLOG_FILES >> $VLOG_FILES_TEMP
done
# then add rest of file
tail -n +$VLOG_SKIP $VLOG_FILES >> $VLOG_FILES_TEMP
cp $VLOG_FILES_TEMP $VLOG_FILES

# set ASE to regression mode, one run
sed -i "s/ASE_MODE = .*$/ASE_MODE = 4/;s/ASE_NUM_TESTS = .*$/ASE_NUM_TESTS = 1/" ase.cfg

# build simulation
make

# run simulation
if [ -e $SIM_READY ]; then
   rm $SIM_READY
fi
make sim &> sim.out &
SIM_PID=$!

# -----------------------------------------------------------------------------
# build SW
# -----------------------------------------------------------------------------
echo "=== BUILD SW ==="
cd $CCI_MPF_LIBS
make clean
make prefix=$AALSDK_INST_PREFIX
export LD_LIBRARY_PATH=$CCI_MPF_LIBS:$LD_LIBRARY_PATH

# -----------------------------------------------------------------------------
# build sample
# -----------------------------------------------------------------------------
echo "=== BUILD SAMPLE ==="
cd $SAMPLE_DIR
make clean
make CFLAGS=-I$CCI_MPF_INCLUDE prefix=$AALSDK_INST_PREFIX

# -----------------------------------------------------------------------------
# run sample
# -----------------------------------------------------------------------------
echo "=== RUN SAMPLE ==="
echo -n "Waiting for simulation..."
while [ ! -e $SIM_READY ]; do
   sleep 1
   echo -n "."
   SIM_TIMEOUT=$(( $SIM_TIMEOUT - 1 ))
   if [ $SIM_TIMEOUT -eq 0 ]; then
      echo  "timed out"
      # FIXME: if baling out, there may be a vcs process left
      exit -1
   fi
done
echo "up."
export ASE_WORKDIR=$ASE_DIR/work
./$SAMPLE_BIN
RETVAL=$?

# -----------------------------------------------------------------------------
# check result
# -----------------------------------------------------------------------------
if [ "$RETVAL" -eq "0" ]; then
   echo " _____  _______ _______ _______"
   echo "|_____] |_____| |______ |______"
   echo "|       |     | ______| ______|"
else
   echo "_______ _______ _____       "
   echo "|______ |_____|   |   |     "
   echo "|       |     | __|__ |_____"
fi

# -----------------------------------------------------------------------------
# clean up
# -----------------------------------------------------------------------------
cd $OLDDIR
exit $RETVAL
