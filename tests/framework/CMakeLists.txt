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

# projectname is the same as the main-executable
project(framework)

set(CMAKE_CXX_STANDARD 11)

# Disable some warnings that fire in system libraries
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-Wno-error=frame-address"
  CXX_SUPPORTS_NO_ERROR_FRAME_ADDRESS)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(GTEST_LIB ${GTEST_LIBRARY_DEBUG})
else()
    set(GTEST_LIB ${GTEST_LIBRARY})
endif()

opae_add_shared_library(TARGET test_system
    SOURCE
        mock/test_system.cpp
	mock/ioctl_handlers.cpp
    LIBS
        opae-c
        fpga_db
        dl
	${GTEST_LIB}
)

if (GTEST_IMPORTED)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_dependencies(test_system gtest_IMPORT_DEBUG)
        add_dependencies(test_system gtest_MAIN_IMPORT_DEBUG)
    else()
        add_dependencies(test_system gtest_IMPORT)
        add_dependencies(test_system gtest_MAIN_IMPORT)
    endif()
endif(GTEST_IMPORTED)

if (CXX_SUPPORTS_NO_ERROR_FRAME_ADDRESS)
  set_target_properties(test_system PROPERTIES COMPILE_FLAGS "-Wno-error=frame-address")
endif()

target_include_directories(test_system
    PUBLIC
        $<BUILD_INTERFACE:${OPAE_INCLUDE_PATH}>
        $<BUILD_INTERFACE:${GTEST_INCLUDE_DIR}>
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${OPAE_LIB_SOURCE}/libopae-c>
        $<BUILD_INTERFACE:${OPAE_LIB_SOURCE}/plugins/xfpga>
)

opae_add_shared_library(TARGET fpga_db
    SOURCE platform/fpga_hw.cpp
)

opae_test_add_static_lib(TARGET test-fpgad-static
    SOURCE
        mock/test_fpgad/logging.c
        mock/test_fpgad/event_dispatcher_thread.c
        mock/test_fpgad/monitor_thread.c
        mock/test_fpgad/monitored_device.c
        mock/test_fpgad/events_api_thread.c
        mock/test_fpgad/opae_events_api.c
        mock/test_fpgad/device_monitoring.c
    LIBS
        opae-c
        ${json-c_LIBRARIES}
)

target_include_directories(test-fpgad-static
    PRIVATE
        ${OPAE_LIB_SOURCE}
        ${CMAKE_CURRENT_SOURCE_DIR}/mock/test_fpgad
        ${CMAKE_CURRENT_SOURCE_DIR}/mock/test_fpgad/fpgad
        ${CMAKE_CURRENT_SOURCE_DIR}/mock/test_fpgad/fpgad/api
)

opae_add_module_library(TARGET test_fpgad-xfpga
    SOURCE mock/test_fpgad/fpgad-xfpga.c
    LIBS test-fpgad-static
)

target_include_directories(test_fpgad-xfpga
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/mock/test_fpgad
        ${CMAKE_CURRENT_SOURCE_DIR}/mock/test_fpgad/fpgad
        ${CMAKE_CURRENT_SOURCE_DIR}/mock/test_fpgad/fpgad/api
)

add_dependencies(fpga_db test_fpgad-xfpga)

if(OPAE_ENABLE_MOCK)
    target_compile_definitions(fpga_db PUBLIC -DOPAE_ENABLE_MOCK=1)
    
    add_custom_command(TARGET fpga_db
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-1socket-nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dcp-rc-nlb3.tar.gz
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
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_dfl_d5005_nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_dfl_n3000_nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_dfl_n6000_sku0_nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_dfl_n6000_sku1_nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_dfl_c6100_nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
    )
endif(OPAE_ENABLE_MOCK)
