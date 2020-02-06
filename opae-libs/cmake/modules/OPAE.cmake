#!/usr/bin/cmake -P
## Copyright(c) 2020, Intel Corporation
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

include(OPAEGit)

include(Findjson-c)

if(NOT libjson-c_FOUND)
    message("-- json-c not found. Please install json-c package for you respective distribution:
    DEB: apt install libjson-c-dev
    RPM: yum install json-c-devel
   If you have already installed this package in a non-standard location 
   please specify the location by defining the variable LIBJSON-C_ROOT in 
   your cmake command as follows: cmake <path to clone dir> -DLIBJSON-C_ROOT=<path to json-c install location>")
   set(REQUIRED_DEPENDENCIES "libjson-c ${REQUIRED_DEPENDENCIES}")
endif()

include(FindUUID)

if(NOT libuuid_FOUND)
    message("-- uuid not found. Please install uuid package for your respective distribution:
    DEB: apt install uuid-dev
    RPM: yum install libuuid-devel
   If you have already installed this package in a non-standard location 
   please specify the location by defining the variable LIBUUID_ROOT in 
   your cmake command as follows: cmake <path to clone dir> -DLIBUUID_ROOT=<path to uuid install location>")
   set(REQUIRED_DEPENDENCIES "libuuid ${REQUIRED_DEPENDENCIES}")
endif()

include(OPAECompiler)
include(OPAETest)
