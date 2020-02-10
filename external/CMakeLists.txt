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

project(external)

################################################################################
## opae-test ###################################################################
################################################################################
if(OPAE_BUILD_TESTS)
    configure_file(opae-test-CMakeLists.txt.in
                   ${CMAKE_CURRENT_BINARY_DIR}/opae-test/download/CMakeLists.txt)
    
    execute_process(
        COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/opae-test/download)
    if(result)
        message(FATAL_ERROR "CMake step for opae-test failed: ${result}")
    endif()
    
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/opae-test/download)
    if(result)
        message(FATAL_ERROR "Build step for opae-test failed: ${result}")
    endif()
    
    # Add opae-test directly to our build.
    # This defines the opae-test targets.
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/opae-test
                     ${CMAKE_CURRENT_BINARY_DIR}/opae-test
                     EXCLUDE_FROM_ALL)
endif()

################################################################################
## opae-legacy #################################################################
################################################################################
configure_file(opae-legacy-CMakeLists.txt.in
               ${CMAKE_CURRENT_BINARY_DIR}/opae-legacy/download/CMakeLists.txt)

execute_process(
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/opae-legacy/download)
if(result)
    message(FATAL_ERROR "CMake step for opae-legacy failed: ${result}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/opae-legacy/download)
if(result)
    message(FATAL_ERROR "Build step for opae-legacy failed: ${result}")
endif()

# Add opae-legacy directly to our build.
# This defines the opae-legacy targets.
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/opae-legacy
                 ${CMAKE_CURRENT_BINARY_DIR}/opae-legacy
                 EXCLUDE_FROM_ALL)
