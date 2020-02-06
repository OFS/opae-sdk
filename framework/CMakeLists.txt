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
## POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 2.8.12)

# projectname is the same as the main-executable
project(framework)

set(CMAKE_CXX_STANDARD 11)

# Disable some warnings that fire in system libraries
check_cxx_compiler_flag("-Wno-error=frame-address"
  CXX_SUPPORTS_NO_ERROR_FRAME_ADDRESS)

opae_add_shared_library(TARGET test_system
    SOURCE mock/test_system.cpp mock/ioctl_handlers.cpp
    LIBS fpga_db dl
)

if (CXX_SUPPORTS_NO_ERROR_FRAME_ADDRESS)
  set_target_properties(test_system PROPERTIES COMPILE_FLAGS "-Wno-error=frame-address")
endif()

target_include_directories(test_system PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_PATH}>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${OPAE_LIBS_ROOT}/libopae-c>
          $<BUILD_INTERFACE:${OPAE_LIBS_ROOT}/plugins/xfpga>)

opae_add_shared_library(TARGET fpga_db
    SOURCE platform/fpga_hw.cpp
)

if(OPAE_ENABLE_MOCK)
    target_compile_definitions(fpga_db PUBLIC -DOPAE_ENABLE_MOCK=1)
    
    add_custom_command(TARGET fpga_db
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-1socket-nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-1socket-nlb0-vf.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dcp-rc-nlb3.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dfl0-nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dcp-vc.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dfl0_patchset2-nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dcp-rc-dfl0_patchset2-nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
    )
endif(OPAE_ENABLE_MOCK)
