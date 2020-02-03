#!/usr/bin/cmake -P
## Copyright(c) 2017-2020, Intel Corporation
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
## POSSIBILITY OF SUCH DAMAGE

find_path(opae_test
    NAMES framework/mock/test_system.cpp
    PATHS ${opae_test_framework}
          ${CMAKE_BINARY_DIR}/opae-test/src/opae-test
          ${CMAKE_SOURCE_DIR})

if (NOT ${opae_test})
    include(ExternalProject)
    ExternalProject_Add(opae-test
          GIT_REPOSITORY    https://github.com/OPAE/opae-test.git
          PREFIX ${CMAKE_BINARY_DIR}/opae-test
          CMAKE_ARGS -DOPAE_INCLUDE_PATH=${OPAE_INCLUDE_PATH} -DOPAE_LIBS_ROOT=${OPAE_LIBS_ROOT}
          INSTALL_COMMAND ""
          BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --config $<CONFIG> --target test_system
          COMMENT "adding opae-test"
    )
endif (NOT ${opae_test})

if (${opae_test_framework})
    set(opae_test_framework_FOUND true)
    message(FATAL_ERROR "TBD")
endif (${opae_test_framework})
