#!/bin/bash
## Copyright(c) 2013-2017, Intel Corporation
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

##########################################################
#
#  ASE Setup file
#  Author: Rahul R Sharma <rahul.r.sharma@intel.com>
#  
#  INSTRUCTIONS:
#  * This file is a one-time per site setup
#  * Before starting, it is a good idea to backup the file
#
##########################################################

# Step 1: Setup License Server
#         Contact your EDA Admin and set this to required values
#         Modify following line
export LM_LICENSE_FILE=".."
if ! [ $LM_LICENSE_FILE ] ; then
    echo "License variable not set, set this in line 43. Please check with your EDA admin"
    exit
elif [ $LM_LICENSE_FILE == "" ] ;  then
    echo "License variable not set, set this in line 43. Please check with your EDA admin"
    exit    
fi

# Step 2: Select one of the following tools
#         - VCS
#         - QuestaSim
#         Uncomment setenv line with correct tool string
#
export MY_SIM_TOOL="VCS"
if ! [ $MY_SIM_TOOL ] ; then
    echo "MY_SIM_TOOL has not been set up, please set this in line 57"
    exit
fi


#
# Step 3: Set up the correct Tool strings 
#         Follow your sub-case closely and fill in the details
#         Contact your EDA admin for more details 
#
if [ $MY_SIM_TOOL == "VCS" ] ; then
    # VCS setup
    echo "VCS Tool setup"
    # Modify this line
    export VCS_HOME=""
    export PATH="${VCS_HOME}/bin:${PATH}"
    if ! [ $VCS_HOME ] ; then
	echo "A) VCS_HOME has not been set up. Contact EDA Admin"
	exit
    elif [ $VCS_HOME == "" ] ; then
    	echo "B) VCS_HOME is empty. Contact EDA Admin"
    	exit    
    fi
elif [ $MY_SIM_TOOL == "QuestaSim" ] ; then
    # QuestaSim setup
    echo "QuestaSim tool setup"
    # Modify this line. Contact EDA Admin
    export QUESTA_HOME=""
    if ! [ $QUESTA_HOME ] ; then
	echo "A) QUESTA_HOME has not been set up. Contact EDA Admin"
	exit
    elif [ $QUESTA_HOME == "" ] ; then
    	echo "B) QUESTA_HOME is empty. Contact EDA Admin"
    	exit    
    fi
    # Modify this line. Contact EDA Admin
    export MGLS_LICENSE_FILE="" 
    if ! [ $MGLS_LICENSE_FILE ] ; then
	echo "A) MGLS_LICENSE_FILE has not been set up. Contact EDA Admin"
	exit
    elif [ $MGLS_LICENSE_FILE == "" ] ; then
    	echo "B) MGLS_LICENSE_FILE is empty. Contact EDA Admin"
    	exit    
    fi
    # LM_PROJECT env
    export LM_PROJECT="PutMyLM_PROJECThere"
    export MENTOR_TOP=$QUESTA_HOME
    export MDLTECH=$QUESTA_HOME
    export MTI_VCO_MODE=64
    export COMP64=1
else
    echo "The Simulation toolname supplied is not supported"
    echo "ASE mode is supported in VCS and QuestaSim tools only"
fi



