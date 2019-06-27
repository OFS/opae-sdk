@echo off
rem $Header: //acds/main/quartus/devices/firmware/tools/sign/ndsign.bat#3 $
rem Use this arc command to setup your env
rem  % arc shell --devtool-cache python/2.7.3b ndsign

rem OR...
rem  - download the latest version of python 2.7 from here:
rem      https://www.python.org/downloads/release
rem  - installer should be named "python-2.7.13.amd64.msi" (or something close to that 2.7.14, 2.7.15, etc)
rem  - install into default install location of c:\Python27\
rem  - accept all defaults

setlocal enableDelayedExpansion

set WRAPPER_ROOT=%~dp0
set WRAPPER_ROOT=%WRAPPER_ROOT:\=/%

REM Blindly add some likely Python paths to the PATH
set PATH=%PATH%;s:\tools\python\2.7.3b\windows64
set PATH=c:\arc\cache\tools\python\2.7.3b\windows64;%PATH%
set PATH=c:\Python27;%PATH%
set PATH=.\cmf_sign;%PATH%

set WRAPPER_INTERP=python.exe
if not "%ARC_PICE%"=="1" if not ""=="" set PYTHONHOME=
%WRAPPER_INTERP% %WRAPPER_ROOT%/ndsign.py %*
endlocal & exit /B %ERRORLEVEL%


rem In order to run the server with ephemeral signing you need to have cmf_sign in your path. You can do
rem this 2 ways - run from an arc system, or by creating a cmf_sign directory here and copying the following
rem files to that directory. Test by adding it to your path and running cmf_sign. If it reports missing dlls
rem then you may need to run it in Powershell to see the actual dlls that are missing.
