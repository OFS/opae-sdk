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
## POSSIBILITY OF SUCH DAMAGE.

find_program(VERILATOR_EXECUTABLE verilator
  HINTS $ENV{VERILATOR_ROOT}
  /usr/local/bin
  /usr/bin
  PATH_SUFFIXES bin
  DOC "Path to the Verilator executable")

find_program(VERILATOR_COVERAGE_EXECUTABLE verilator_coverage
  HINTS $ENV{VERILATOR_ROOT}
  /usr/local/bin
  /usr/bin
  PATH_SUFFIXES bin
  DOC "Path to the Verilator coverage executable")

get_filename_component(VERILATOR_EXECUTABLE_DIR ${VERILATOR_EXECUTABLE}
  DIRECTORY)

find_path(VERILATOR_INCLUDE_DIR verilated.h
  HINTS $ENV{VERILATOR_ROOT} ${VERILATOR_EXECUTABLE_DIR}/..
  PATH_SUFFIXES include share/verilator/include
  DOC "Path to the Verilator headers")

mark_as_advanced(VERILATOR_EXECUTABLE)
mark_as_advanced(VERILATOR_COVERAGE_EXECUTABLE)
mark_as_advanced(VERILATOR_INCLUDE_DIR)

find_package(PackageHandleStandardArgs REQUIRED)
find_package_handle_standard_args(Verilator REQUIRED_VARS
  VERILATOR_EXECUTABLE VERILATOR_COVERAGE_EXECUTABLE VERILATOR_INCLUDE_DIR)

if (WIN32)
  set(library_policy STATIC)
else()
  set(library_policy SHARED)
endif()

add_library(verilated ${library_policy}
  ${VERILATOR_INCLUDE_DIR}/verilated.cpp
  ${VERILATOR_INCLUDE_DIR}/verilated_cov.cpp
  ${VERILATOR_INCLUDE_DIR}/verilated_dpi.cpp
  ${VERILATOR_INCLUDE_DIR}/verilated_vcd_c.cpp)

set_target_properties(verilated PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set_source_files_properties(
  ${VERILATOR_INCLUDE_DIR}/verilated.cpp
  PROPERTIES
  COMPILE_DEFINITIONS "VL_USER_STOP;VL_USER_FINISH")

set_source_files_properties(
  ${VERILATOR_INCLUDE_DIR}/verilated_cov.cpp
  PROPERTIES
  COMPILE_DEFINITIONS "VM_COVERAGE=1")

set_target_properties(verilated PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

target_include_directories(verilated SYSTEM PRIVATE
  ${VERILATOR_INCLUDE_DIR}
  ${VERILATOR_INCLUDE_DIR}/vltstd)

set(VERILATOR_FOUND ${VERILATOR_FOUND} PARENT_SCOPE)
set(VERILATOR_EXECUTABLE "${VERILATOR_EXECUTABLE}" PARENT_SCOPE)
set(VERILATOR_INCLUDE_DIR "${VERILATOR_INCLUDE_DIR}" PARENT_SCOPE)
set(VERILATOR_COVERAGE_EXECUTABLE "${VERILATOR_COVERAGE_EXECUTABLE}" PARENT_SCOPE)
