@echo off
rem $Header: //acds/main/quartus/devices/firmware/tools/sign/ndsign_servermode.bat#2 $
set WRAPPER_ROOT=%~dp0
set WRAPPER_ROOT=%WRAPPER_ROOT:\=/%

rem --css-keyhash="TIqHIwDOhgg4tdAGTUyjGXOHZsM=" --> for sj-jdasilva-620.altera.priv.altera.com
rem --css-keyhash="0VZyoNq5ChG5+f++hHPUxMk4YvA=" --> for jdasilva-mobl.amr.corp.intel.com

%WRAPPER_ROOT%/ndsign.bat --debug --verbose servermode --port=1750
