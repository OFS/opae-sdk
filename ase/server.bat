::#########################################################################
::#                 Argument processing					                  #
::#########################################################################
::# Count args and process

@echo off

set argc=0
for %%i in (%*) do Set /A argc+=1

IF %argc% EQU 0 (
	echo Usage: server option. option: clean/compile/sim
	exit /b
)
IF %argc% GTR 1 (
	echo Too many arguments. Usage: server option. option: clean/compile/sim
	exit /b
)
IF %argc% EQU 1 (
	IF %1% NEQ clean (
		IF %1% NEQ compile (
			IF %1% NEQ sim (
				echo Invalid argument. Usage: server option. option: clean/compile/sim
				exit /b
			)
		)
	)
)

call ase_sources.bat

:: Base directory
SET ASE_BASEDIR=%cd%

::###############################################################
::# ASE switches (disabling logger and checker may speed up     #
::# simulation in assumption that protocol errors don't exist   #
::# in design                                                   #
::###############################################################
SET ASE_DISABLE_LOGGER=0
SET ASE_DISABLE_CHECKER=0

::#########################################################################
::#                 Enable Altera gates library in ASE                    #
::#########################################################################
::# Enable Altera Gates
SET GLS_SIM=1

::# ASE GLS_SIM path check test
SET GLS_SAMPLE_LIB=%QUARTUS_HOME%/eda/sim_lib/altera_primitives.v

::# Gate level libraries to add to simulation
SET GLS_VERILOG_OPT=%QUARTUS_HOME%/eda/sim_lib/altera_primitives.v %QUARTUS_HOME%/eda/sim_lib/220model.v %QUARTUS_HOME%/eda/sim_lib/sgate.v %QUARTUS_HOME%/eda/sim_lib/altera_mf.v %QUARTUS_HOME%/eda/sim_lib/stratixv_hssi_atoms.v %QUARTUS_HOME%/eda/sim_lib/stratixv_pcie_hip_atoms.v %QUARTUS_HOME%/eda/sim_lib/altera_lnsim.sv

::#########################################################################
::#                       ASE location settings                           #
::#########################################################################
::# Work directory
SET WORK=work

::# ASE Source directory
SET ASE_SRCDIR=%cd%
SET ASE_WORKDIR=%ASE_SRCDIR%/%WORK%

::# Configuration & regression file inputs
SET ASE_CONFIG=%ASE_SRCDIR%/ase.cfg
::SET ASE_SCRIPT=%ASE_SRCDIR%/ase_regress.sh

::#########################################################################
::#                          ASE HW/SW settings                           #
::########################################################################
::## Timescale
SET TIMESCALE=1ps/1ps

::## ASE HW file setup
SET ASEHW_FILE_LIST=%ASE_SRCDIR%/rtl/ccip_if_pkg.sv %ASE_SRCDIR%/rtl/ase_pkg.sv %ASE_SRCDIR%/rtl/outoforder_wrf_channel.sv %ASE_SRCDIR%/rtl/inorder_wrf_channel.sv %ASE_SRCDIR%/rtl/latency_pipe.sv %ASE_SRCDIR%/rtl/ccip_emulator.sv %ASE_SRCDIR%/rtl/ase_svfifo.sv %ASE_SRCDIR%/rtl/ccip_logger.sv %ASE_SRCDIR%/rtl/ccip_checker.sv %ASE_SRCDIR%/rtl/ase_top.sv

::## ASE HW include directory setup
SET ASE_INCDIR=%ASE_SRCDIR%/rtl/+

::## ASE SW file setup
SET ASESW_FILE_LIST=%ASE_SRCDIR%/sw/ase_ops.c %ASE_SRCDIR%/sw/ipc_mgmt_ops.c %ASE_SRCDIR%/sw/mem_model.c %ASE_SRCDIR%/sw/protocol_backend.c %ASE_SRCDIR%/sw/tstamp_ops.c %ASE_SRCDIR%/sw/mqueue_ops.c %ASE_SRCDIR%/sw/error_report.c %ASE_SRCDIR%/sw/linked_list_ops.c %ASE_SRCDIR%/sw/randomness_control.c

echo %ASEHW_FILE_LIST%

::## ASE top level module
SET ASE_TOP=ase_top

::#########################################################################
::#                            Build options                              #
::#########################################################################

::## C Compiler options
SET CC_OPT=-g -D SIM_SIDE -I %ASE_SRCDIR%/sw/ -D SIMULATOR=%SIMULATOR% -D %ASE_PLATFORM% -I %MTI_HOME%/include/

::#########################################################################
::#                         Questa Build Switches                         #
::#########################################################################
::## VHDL compile
SET MENT_VCOM_OPT=-nologo -work $(WORK) 
SET MENT_VLOG_OPT=%MENT_VLOG_OPT% -nologo +librescan -work %WORK% +define+%SIMULATOR% -novopt -dpiheader work/dpiheader.h +incdir+%ASE_INCDIR%+%DUT_INCDIR%+%WORK% -sv -timescale %TIMESCALE% -l vlog.log +define+%ASE_PLATFORM% %GLS_VERILOG_OPT%

::## VSIM elaboration, and run options
SET MENT_VSIM_OPT=-c -l run.log -dpioutoftheblue 1 -novopt -do %ASE_SRCDIR%/vsim_run.tcl -sv_seed 1234

IF %1 EQU clean (
	cd %WORK%
	echo Y | rmdir /s work
	del *.log *.tsv AN.DB
	del transcript modelsim.ini vsim.wlf ucli.key vsim_stacktrace.vstf
	del profile* simprofile* scanbuild-app scanbuild-sim
	echo Y | rmdir DVEfiles/ csrc/
	del .ase_* *.o ase_seed.txt warnings.txt
	del transcript *.log .ase_ipc_local ase_seed.txt
	del vsim.wlf *_smq __hdl_xmr.tab
	
	cd %ASE_SRCDIR%
::	del ase_sources.bat ase_files.files synopsys_sim.setup vlog_files.list vhdl_files.list ucli.key
::	del scripts/*.pyc vcs_run.tcl vsim_run.tcl
::	del ase_seed.txt run vlog-ase
	exit /b
	
)

IF %1 EQU compile (
	mkdir %WORK%
	cd %WORK%
	vlib %WORK%
	vlog -ccflags "%CC_OPT%" %ASESW_FILE_LIST%
	vlog %MENT_VLOG_OPT% %ASEHW_FILE_LIST% -l vlog-ase.log 
	vlog %MENT_VLOG_OPT% -f %DUT_VLOG_SRC_LIST% -l vlog-afu.log
	cd %ASE_SRCDIR%
	exit /b
)

IF %1 EQU sim (
	cd %ASE_WORKDIR%
	vsim %MENT_VSIM_OPT% +CONFIG=%ASE_CONFIG% -ldflags "-lws2_32" %ASE_TOP%
	exit /bat
	cd %ASE_SRCDIR
)