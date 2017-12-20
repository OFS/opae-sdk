::#########################################################################
::#                       ASE location settings                           #
::#########################################################################
::# Work directory
SET WORK=work

::# ASE Source directory
SET ASE_SRCDIR=%cd%
SET ASE_WORKDIR=%ASE_SRCDIR%/%WORK%

::#########################################################################
::#                            Build options                              #
::#########################################################################

::## C Compiler options

SET CC=gcc
SET CC_OPT=-std=c99 -I %ASE_SRCDIR%/../common/include/ -I %MTI_HOME%/include/ -I %ASE_SRCDIR%/sw/

::#########################################################################
::#                          ASE HW/SW settings                           #
::########################################################################

::## ASE SW file setup
SET ASESW_FILE_LIST=%ASE_SRCDIR%/sw/app_backend.c %ASE_SRCDIR%/sw/ase_ops.c %ASE_SRCDIR%/sw/error_report.c %ASE_SRCDIR%/sw/mqueue_ops.c %ASE_SRCDIR%/sw/tstamp_ops.c %ASE_SRCDIR%\api\src\buffer.c %ASE_SRCDIR%\api\src\close.c %ASE_SRCDIR%/api\src\common.c %ASE_SRCDIR%/api\src\enum.c %ASE_SRCDIR%/api\src\event.c %ASE_SRCDIR%/api\src\manage.c %ASE_SRCDIR%/api\src\mmio.c %ASE_SRCDIR%/api\src\open.c %ASE_SRCDIR%/api\src\umsg.c
SET ASESW_OBJ_LIST=app_backend.o ase_ops.o error_report.o mqueue_ops.o tstamp_ops.o buffer.o close.o common.o enum.o event.o manage.o mmio.o open.o umsg.o

set ASE_WORKDIR=%ASE_SRCDIR%\%WORK%

cd %WORK%

%CC% -c -DBUILD_FPGA_DLL %CC_OPT% %ASESW_FILE_LIST%
%CC% -shared -o libopae_ase.dll %ASESW_OBJ_LIST% -lws2_32 -lRpcrt4 -Wl,--out-implib,libopae_ase.lib

%CC% -c %CC_OPT% %ASE_SRCDIR%\..\samples\hello_fpga.c
%CC% -o hello_fpga.exe hello_fpga.o -L. -lws2_32 -lRpcrt4 -llibopae_ase




