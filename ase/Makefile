# #############################################################################
# Copyright(c) 2011-2016, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# * Neither the name of Intel Corporation nor the names of its contributors
# may be used to endorse or promote products derived from this software
# without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
# #############################################################################
#
# Module Info:
# Language   : System{Verilog} | C/C++
# Owner      : Rahul R Sharma
#              rahul.r.sharma@intel.com
#              Intel Corporation
#
# ASE environment build - Makefile
#
#########################################################################
# Provides a template for compiling ASE with RTL CAFU sources in VCS
# and Questasim
# For a full experience, roll your own Makefile
#
##########################################################################

include ase_sources.mk

###############################################################
##                                                           ##
##         ASE Platform value (selection method)             ##
##                                                           ##
## * FPGA_PLATFORM_INTG_XEON:                                ##
##       Sets ASE to emulate one socket of Integrated        ##
##       platform                                            ##
## * FPGA_PLATFORM_DISCRETE:                                 ##
##       Sets ASE to emulate a discrete PCIe attached FPGA   ##
##                                                           ##
## Operation:                                                ##
## - FPGA_PLATFORM_INTG_XEON or FPGA_PLATFORM_DISCRETE       ##
##   selection drives compiler macros  to the individual SV  ##
##   and C components                                        ##
## - ASE_PLATFORM will be used as the differentiating macro  ##
##   during the compilation stage                            ##
## - This methos allows for newer platforms to be added as   ##
##   ASE_PLATFORM, reduces accidental settings failure       ##
##                                                           ##
###############################################################
ASE_PLATFORM?=FPGA_PLATFORM_INTG_XEON

###############################################################
# ASE switches (disabling logger and checker may speed up     #
# simulation in assumption that protocol errors don't exist   #
# in design                                                   #
###############################################################
ASE_DISABLE_LOGGER ?= 0
ASE_DISABLE_CHECKER ?= 0

#########################################################################
#                 Enable Altera gates library in ASE                    #
#########################################################################
# Enable Altera Gates
GLS_SIM = 1

# ASE GLS_SIM path check test
GLS_SAMPLE_LIB = $(QUARTUS_HOME)/eda/sim_lib/altera_primitives.v

# Gate level libraries to add to simulation
GLS_VERILOG_OPT = $(QUARTUS_HOME)/eda/sim_lib/altera_primitives.v
GLS_VERILOG_OPT+= $(QUARTUS_HOME)/eda/sim_lib/220model.v
GLS_VERILOG_OPT+= $(QUARTUS_HOME)/eda/sim_lib/sgate.v
GLS_VERILOG_OPT+= $(QUARTUS_HOME)/eda/sim_lib/altera_mf.v
GLS_VERILOG_OPT+= $(QUARTUS_HOME)/eda/sim_lib/stratixv_hssi_atoms.v
GLS_VERILOG_OPT+= $(QUARTUS_HOME)/eda/sim_lib/stratixv_pcie_hip_atoms.v
GLS_VERILOG_OPT+= $(QUARTUS_HOME)/eda/sim_lib/altera_lnsim.sv

# add required files for local memory model
ifeq ($(ASE_PLATFORM), FPGA_PLATFORM_DISCRETE)
GLS_VERILOG_OPT+= $(QUARTUS_HOME)/eda/sim_lib/twentynm_atoms.v
ifeq ($(SIMULATOR), VCS)
  GLS_VERILOG_OPT+= $(QUARTUS_HOME)/eda/sim_lib/synopsys/twentynm_atoms_ncrypt.v
endif
ifeq ($(SIMULATOR), QUESTA)
  GLS_VERILOG_OPT+= $(QUARTUS_HOME)/eda/sim_lib/mentor/twentynm_atoms_ncrypt.v
endif
endif

#########################################################################
#                       ASE location settings                           #
#########################################################################
# Work directory
WORK = work

# ASE Source directory
ASE_SRCDIR = $(shell pwd)
OPAE_BASEDIR ?= $(ASE_SRCDIR)/../
ASE_WORKDIR = $(PWD)/$(WORK)

# Configuration & regression file inputs
ASE_CONFIG ?= $(PWD)/ase.cfg
ASE_SCRIPT ?= $(PWD)/ase_regress.sh

# Configuration for discrete Memory Model
ASE_MEM_SRC = $(ASE_SRCDIR)/rtl/device_models/dcp_emif_model

#########################################################################
#                          ASE HW/SW settings                           #
########################################################################
## Timescale
TIMESCALE = 1ps/1ps

## ASE HW file setup
ASEHW_FILE_LIST = -F $(ASE_SRCDIR)/rtl/sources.txt

## ASE platform-specific HW
ASE_PLATFORM_FILE_LIST ?=
ifeq ($(ASE_PLATFORM), FPGA_PLATFORM_DISCRETE)
  ASE_PLATFORM_FILE_LIST += -F $(ASE_MEM_SRC)/sources.txt
endif

## ASE SW file setup
ASESW_FILE_LIST = \
	$(ASE_SRCDIR)/sw/ase_ops.c \
	$(ASE_SRCDIR)/sw/ipc_mgmt_ops.c \
	$(ASE_SRCDIR)/sw/mem_model.c \
	$(ASE_SRCDIR)/sw/protocol_backend.c \
	$(ASE_SRCDIR)/sw/tstamp_ops.c \
	$(ASE_SRCDIR)/sw/mqueue_ops.c \
	$(ASE_SRCDIR)/sw/error_report.c \
	$(ASE_SRCDIR)/sw/linked_list_ops.c \
	$(ASE_SRCDIR)/sw/randomness_control.c \

## Safe string sources
SAFESTR_SRC_LIST = $(wildcard ${OPAE_BASEDIR}/safe_string/*.c)

## ASE top level module
ASE_TOP = ase_top

#########################################################################
#                            Build options                              #
#########################################################################
## Choice of VCS or QUESTA ##
SIMULATOR?=VCS
CC=gcc

## RTL command
SNPS_COMMAND = $(shell command -v vcs)
MENT_COMMAND = $(shell command -v vsim)

## GCC version
GCC_VERSION_GT_49 = $(shell gcc -dumpversion | gawk '{print $$1>=4.9?"1":"0"}')

## For ModelSim figure out whether it is a 32 bit edition
CC_INT_SIZE=-m64
ifeq ($(SIMULATOR), QUESTA)
  ifdef MENT_COMMAND
    # Assume 32 bit edition unless "vsim -version" indicates -64
    CC_INT_SIZE=-m32
    MENT_VERSION=$(shell vsim -version)
    ifneq (,$(findstring -64, $(MENT_VERSION)))
      CC_INT_SIZE=-m64
    endif
  endif
endif

## C Compiler options
CC_OPT?=
CC_OPT+= -g $(CC_INT_SIZE) -fPIC -D SIM_SIDE=1 -I $(ASE_SRCDIR)/sw/ -I $(OPAE_BASEDIR)/common/include/
CC_OPT+= -D SIMULATOR=$(SIMULATOR) -D $(ASE_PLATFORM)
CC_OPT+= -Wall -Wformat -Wformat-security
CC_OPT+= -O2 -D_FORTIFY_SOURCE=2
ifeq ($(GCC_VERSION_GT_49),1)
  CC_OPT+= -fstack-protector-strong
  CC_OPT+= -z noexecstack -z relro -z now
else
  CC_OPT+= -fstack-protector-all
endif
## CC_OPT, simulator specific include paths
ifeq ($(SIMULATOR), VCS)
  CC_OPT+= -I $(VCS_HOME)/include/
endif
ifeq ($(SIMULATOR), QUESTA)
  CC_OPT+= -I $(MTI_HOME)/include/
endif

## Print information ##
 $(info SIMULATOR=$(SIMULATOR))
 $(info CC=$(CC))

## ASE Link options
ASE_LD_SWITCHES?=
ASE_LD_SWITCHES+= -lrt -lpthread

## Library names
ASE_SHOBJ_NAME = ase_libs
ASE_SHOBJ_SO = $(ASE_SHOBJ_NAME).so

#########################################################################
#                          VCS Build Switches                           #
#########################################################################
## VHDL compile
SNPS_VHDLAN_OPT?=
SNPS_VHDLAN_OPT+= -nc -verbose -full64
SNPS_VHDLAN_OPT+= -work $(WORK)

## Verilog compile
SNPS_VLOGAN_OPT?=
SNPS_VLOGAN_OPT+= -nc -V -full64 +v2k -sverilog +define+$(SIMULATOR)
SNPS_VLOGAN_OPT+= +incdir+$(DUT_INCDIR)
SNPS_VLOGAN_OPT+= +librescan -work $(WORK) -override_timescale=$(TIMESCALE)
SNPS_VLOGAN_OPT+= +define+$(ASE_PLATFORM)
SNPS_VLOGAN_OPT+= +lint=all,noVCDE,noNS,NCEID,CAWM,TFIPC,IPDP,PCWM
ifeq ($(ASE_DISABLE_LOGGER), 1)
  SNPS_VLOGAN_OPT+= +define+ASE_DISABLE_LOGGER=1
endif
ifeq ($(ASE_DISABLE_CHECKER), 1)
  SNPS_VLOGAN_OPT+= +define+ASE_DISABLE_CHECKER=1
endif
ifeq ($(GLS_SIM), 1)
  SNPS_VLOGAN_OPT+= $(GLS_VERILOG_OPT)
endif

## VCS elaboration
SNPS_VCS_OPT?=
SNPS_VCS_OPT+= -nc -V +vcs+lic+wait -full64 -debug_pp -Mupdate -lca
SNPS_VCS_OPT+= -j 4
SNPS_VCS_OPT+= -l vcs_elab.log +vhdllib+$(WORK) -Mlib=$(WORK) +lint=TFIPC-L
SNPS_VCS_OPT+= -override_timescale=$(TIMESCALE) -o $(WORK)/ase_simv
# SNPS_VCS_OPT+= -LDFLAGS="-m64"             #  !!! DO NOT EDIT !!!

## Simulation options
SNPS_SIM_OPT+= -ucli -do $(PWD)/vcs_run.tcl
# SNPS_SIM_OPT+= -l run.log
SNPS_SIM_OPT+= +ntb_random_seed=1234


#########################################################################
#                         Questa Build Switches                         #
#########################################################################
## VHDL compile
MENT_VCOM_OPT?=
MENT_VCOM_OPT+= -nologo -work $(WORK)

## VLOG compile
# MENT_VLOG_OPT = -64
MENT_VLOG_OPT?=
MENT_VLOG_OPT+= -nologo +librescan -work $(WORK) +define+$(SIMULATOR) -novopt
MENT_VLOG_OPT+= -dpiheader work/dpiheader.h +incdir+$(DUT_INCDIR)+$(WORK)
MENT_VLOG_OPT+= -sv -timescale $(TIMESCALE) -l vlog.log
MENT_VLOG_OPT+= +define+$(ASE_PLATFORM)
ifeq ($(GLS_SIM), 1)
  MENT_VLOG_OPT+= $(GLS_VERILOG_OPT)
endif

## VSIM elaboration, and run options
# MENT_VSIM_OPT = -64
MENT_VSIM_OPT?=
MENT_VSIM_OPT+= -c -l run.log -dpioutoftheblue 1 -novopt
MENT_VSIM_OPT+= -sv_lib $(ASE_SHOBJ_NAME) -do $(PWD)/vsim_run.tcl
MENT_VSIM_OPT+= -sv_seed 1234
# -voptargs="+acc"


#########################################################################
#                            Build Targets                              #
#########################################################################
# Default
all: check build

# Check
check: header
# Check that only INTG_XEON or DISCRETE is selected, not both or neither
ifeq ($(ASE_PLATFORM), FPGA_PLATFORM_INTG_XEON)
	@echo "ASE platform set to INTG_XEON mode"
else
  ifeq ($(ASE_PLATFORM), FPGA_PLATFORM_DISCRETE)
	@echo "ASE platform set to DISCRETE mode"
  else
	@echo "ASE platform set to '$(ASE_PLATFORM)' -- this is an illegal value"
	@echo "    Valid values - {FPGA_PLATFORM_INTG_XEON, FPGA_PLATFORM_DISCRETE}"
	exit 1
  endif
endif
# Ensure GCC is available
ifneq ($(CC), gcc)
	@echo "#                                                          #"
	@echo "# CC=$(CC) not supported                                   #"
	@echo "# Run 'make help' for more information                     #"
	@echo "#                                                          #"
	@echo "############################################################"
	exit 1
endif
# Echo simulator setting
ifneq ($(SIMULATOR), VCS)
  ifneq ($(SIMULATOR), QUESTA)
	@echo "#                                                          #"
	@echo "# SIMULATOR=$(SIMULATOR) not supported                     #"
	@echo "# Run 'make help' for more information                     #"
	@echo "#                                                          #"
	@echo "############################################################"
	exit 1
  endif
endif
# Check gate simulation libraries
ifeq ($(GLS_SIM), 1)
  ifndef QUARTUS_HOME
	@echo "** ERROR **: env(QUARTUS_HOME) has not been set."
	@echo "** ERROR **: GLS_SIM needs QUARTUS_HOME environment variable to be set."
    ifdef QUARTUS_ROOT
	@echo "** ERROR **: env(QUARTUS_ROOT) is set up. Please set env(QUARTUS_HOME) to same location."
	@echo "** ERROR **: This makefile references libraries based on env(QUARTUS_HOME)."
    endif
  else
    ifeq ($(wildcard $(GLS_SAMPLE_LIB)),)
	@echo "** ERROR **: env(QUARTUS_HOME) not set up correctly."
	@echo "** ERROR **: Cannot find Quartus library files in known locations"
    endif
  endif
endif
# Check if some RTL simulator exists
ifeq ($(SIMULATOR), VCS)
  ifndef VCS_HOME
	@echo "**ERROR** : env(VCS_HOME) is not set ! svdpi.h cannot be found !"
  endif
  ifndef SNPS_COMMAND
	@echo "**ERROR** : Synopsys commands (vlogan, vcs) not found !"
  endif
else
  ifeq ($(SIMULATOR), QUESTA)
    ifndef MTI_HOME
	@echo "**ERROR** : env(MTI_HOME) is not set ! svdpi.h cannot be found !"
    endif
    ifndef MENT_COMMAND
	@echo "**ERROR** : Modelsim commands (vlog, vsim) not found !"
    endif
  else
     @echo "**ERROR**: Unknown RTL simulator tool in use --- this is unsupported !"
  endif
endif

# Print version number
header:
	@echo "#################################################################"
	@echo "#                                                               #"
	@echo "#             OPAE Intel(R) Xeon(R) + FPGA Library              #"
	@echo "#               AFU Simulation Environment (ASE)                #"
	@echo "#                                                               #"
	@echo "#################################################################"

# Help information
help: header
	@echo "#                     |                                         #"
	@echo "#        COMMAND      |               DESCRIPTION               #"
	@echo "# --------------------|---------------------------------------- #"
	@echo "# make                | Build the HW Model using RTL supplied   #"
	@echo "#                     |                                         #"
	@echo "# make sim            | Run simulator                           #"
	@echo "#                     | - ASE can be run in one of 4 modes set  #"
	@echo "#                     |   in ase.cfg                            #"
	@echo "#                     | - A regression mode can be enabled by   #"
	@echo "#                     |   writing ASE_MODE = 4 in ase.cfg and   #"
	@echo "#                     |   supplying an ase_regress.sh script    #"
	@echo "#                     |                                         #"
	@echo "# make wave           | Open the waveform (if created)          #"
	@echo "#                     | To be run after simulation completes    #"
	@echo "#                     |                                         #"
	@echo "# make clean          | Clean simulation files                  #"
	@echo "#                     |                                         #"
	@echo "# make distclean      | Clean ASE sub-distribution              #"
	@echo "#                     |                                         #"
	@echo "# ====================|======================================== #"
	@echo "#    Makefile switch  |               DESCRIPTION               #"
	@echo "# --------------------|---------------------------------------- #"
	@echo "# ASE_CONFIG          | Directly input an ASE configuration     #"
	@echo "#                     |   file path (ase.cfg)                   #"
	@echo "#                     |                                         #"
	@echo "# ASE_SCRIPT          | Directly input an ASE regression file   #"
	@echo "#                     |   path (ase_regress.sh, for ASE_MODE=4) #"
	@echo "#                     |                                         #"
	@echo "# SIMULATOR           | Directly input a simulator brand        #"
	@echo "#                     |   (select between 'VCS' or 'QUESTA')    #"
	@echo "#                     |                                         #"
	@echo "# ASE_DISABLE_CHECKER | Disable CCI-P protocol checker module   #"
	@echo "#                     |  (set to '1' might speed up simulation) #"
	@echo "#                     | **WARNING** => NO warnings on hazards,  #"
	@echo "#                     |   protocol checks, timeouts will be     #"
	@echo "#                     |   generated. This option must be ONLY   #"
	@echo "#                     |   used if the design is already CCI-P   #"
	@echo "#                     |   compliant and fast simulation of      #"
	@echo "#                     |   app-specific logic is needed          #"
	@echo "#                     |                                         #"
	@echo "#################################################################"

## Build ASE Software objects and shared library ##
sw_build:
	make header
	mkdir -p $(WORK)
	cd $(WORK) ; $(CC) $(CC_OPT) -c $(SAFESTR_SRC_LIST) || exit 1 ; cd -
	cd $(WORK) ; $(CC) $(CC_OPT) -c $(ASESW_FILE_LIST) || exit 1 ; cd -
	cd $(WORK) ; $(CC) $(CC_INT_SIZE) -g -shared -o $(ASE_SHOBJ_SO) `ls *.o` $(ASE_LD_SWITCHES) ; cd -
	nm $(WORK)/$(ASE_SHOBJ_SO) > $(WORK)/$(ASE_SHOBJ_NAME).nm

## VCS build template ##
vcs_build: sw_build
	@echo "############################################################"
	@echo "#                                                          #"
	@echo "#                VCS-GCC build initiated                   #"
	@echo "#                                                          #"
	@echo "############################################################"
ifeq ($(ASE_PLATFORM), FPGA_PLATFORM_DISCRETE)
	cp -f $(ASE_MEM_SRC)/*.hex $(ASE_WORKDIR)
	vlogan $(SNPS_VLOGAN_OPT) $(ASEHW_FILE_LIST) $(ASE_PLATFORM_FILE_LIST) -l vlogan-ase.log
else
	vlogan $(SNPS_VLOGAN_OPT) $(ASEHW_FILE_LIST) -l vlogan-ase.log
endif

ifdef DUT_VHD_SRC_LIST
	vhdlan $(SNPS_VHDLAN_OPT) -F $(DUT_VHD_SRC_LIST)
endif
ifdef DUT_VLOG_SRC_LIST
	vlogan $(SNPS_VLOGAN_OPT) -F $(DUT_VLOG_SRC_LIST) -l vlogan-afu.log
endif
ifeq ($(CC), gcc)
	vcs $(SNPS_VCS_OPT) $(ASE_TOP) $(WORK)/$(ASE_SHOBJ_SO) $(ASE_LD_SWITCHES)
endif

## Questasim template ##
questa_build: sw_build
	@echo "############################################################"
	@echo "#                                                          #"
	@echo "#              QuestaSim-GCC build initiated               #"
	@echo "#                                                          #"
	@echo "############################################################"
	cd $(WORK) ; vlib $(WORK) ; vmap work $(WORK) ; cd -
	cp -f $(ASE_MEM_SRC)/*.hex $(ASE_WORKDIR)
	cd $(WORK) ; vlog $(MENT_VLOG_OPT) $(ASEHW_FILE_LIST) $(ASE_PLATFORM_FILE_LIST) -l vlog-ase.log ; cd -
ifdef DUT_VHD_SRC_LIST
	cd $(WORK) ; vcom $(MENT_VCOM_OPT) -F $(DUT_VHD_SRC_LIST) -l vcom-afu.log ; cd -
endif
ifdef DUT_VLOG_SRC_LIST
	cd $(WORK) ; vlog $(MENT_VLOG_OPT) -F $(DUT_VLOG_SRC_LIST) -l vlog-afu.log ; cd -
endif

## Build Simulator objects ##
build:
ifeq ($(SIMULATOR), VCS)
	make vcs_build
else
  ifeq ($(SIMULATOR), QUESTA)
	make questa_build
  else
	@echo "############################################################"
	@echo "#         SIMULATOR=$(SIMULATOR) not supported             #"
	@echo "############################################################"
  endif
endif

## Run ASE Simulator ##
sim: check
ifeq ($(SIMULATOR), VCS)
	cd $(ASE_WORKDIR) ; ./ase_simv $(SNPS_SIM_OPT) +CONFIG=$(ASE_CONFIG) +SCRIPT=$(ASE_SCRIPT) ; cd -
else
  ifeq ($(SIMULATOR), QUESTA)
	cd $(ASE_WORKDIR) ; vsim $(MENT_VSIM_OPT) +CONFIG=$(ASE_CONFIG) +SCRIPT=$(ASE_SCRIPT) $(ASE_TOP) ; cd -
  else
	@echo "############################################################"
	@echo "#         SIMULATOR=$(SIMULATOR) not supported             #"
	@echo "############################################################"
  endif
endif

# Open Wave file
wave: check
ifeq ($(SIMULATOR), VCS)
	dve -vpd `find . -name inter.vpd` || dve -vpd `find . -name inter.vpd` -full64
else
  ifeq ($(SIMULATOR), QUESTA)
	vsim -view `find . -name vsim.wlf`
  endif
endif


#########################################################################
#                            Clean all                                  #
#########################################################################
clean: header
	rm -rf work/ *.log *.tsv AN.DB/ || echo "make: Didn't clean up work"
	rm -rf csrc/ vc_hdrs.h .vlogansetup.* *.vpd app_build/
	rm -rf transcript modelsim.ini vsim.wlf ucli.key vsim_stacktrace.vstf
	rm -rf profile* simprofile* scanbuild-app scanbuild-sim DVEfiles/ csrc/
	rm -rf .ase_* *.o ase_seed.txt warnings.txt
	rm -rf transcript *.log .ase_ipc_local ase_seed.txt
	rm -rf vsim.wlf *_smq __hdl_xmr.tab

distclean: header clean
	./distclean.sh
