## Copyright(c) 2017-2023, Intel Corporation
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

cmake_minimum_required(VERSION 3.14)
project(testing)

if(GTest_FOUND)
    message(STATUS "Found GTest")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

# Disable some warnings that fire during gtest compilation
check_cxx_compiler_flag("-Wno-sign-compare" CXX_SUPPORTS_NO_SIGN_COMPARE)
if(CXX_SUPPORTS_NO_SIGN_COMPARE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
endif()

add_subdirectory(framework)

add_subdirectory(bitstream)
add_subdirectory(opae-c)
add_subdirectory(opae-cxx)
add_subdirectory(pyopae)
add_subdirectory(xfpga)
add_subdirectory(opaemem)
if (OPAE_BUILD_LIBOFS)
    add_subdirectory(libofs)
    add_subdirectory(ofs_driver)
endif (OPAE_BUILD_LIBOFS)

add_subdirectory(argsfilter)
add_subdirectory(board)
add_subdirectory(dummy_afu)
add_subdirectory(fpgaconf)
add_subdirectory(fpgainfo)
add_subdirectory(hello_events)
add_subdirectory(hello_fpga)
add_subdirectory(object_api)
add_subdirectory(userclk)
add_subdirectory(fpgametrics)
if (OPAE_BUILD_LIBOFS)
    add_subdirectory(ofs_cpeng)
endif (OPAE_BUILD_LIBOFS)
add_subdirectory(fpgad)
