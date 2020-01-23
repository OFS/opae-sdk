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

if (QUESTA_FOUND)
    return()
endif()

find_package(PackageHandleStandardArgs REQUIRED)
if(NOT ${MTI_HOME})
  set(MTI_HOME $ENV{MTI_HOME})
endif()

set(QUESTA_HINTS
  $ENV{MTI_HOME}
  $ENV{MODELSIM_ROOTDIR}
  $ENV{MODELSIM_HOME}
  $ENV{MODELSIM_ROOT}
  $ENV{MODELSIM_DIR}
  $ENV{MODELSIM}
  $ENV{MODELSIM_ROOT}
  $ENV{QUARTUS_ROOTDIR}
  $ENV{QUARTUS_HOME}
  $ENV{QUARTUS_ROOT}
  $ENV{QUARTUS_DIR}
  $ENV{QUARTUS})

set(QUESTA_PATH_SUFFIXES
  bin
  ../modelsim_ae/bin
  ../modelsim_ase/bin)

find_program(QUESTA_VLIB_EXECUTABLE vlib
    HINTS ${QUESTA_HINTS}
    PATH_SUFFIXES bin ${QUESTA_PATH_SUFFIXES}
    DOC "Path to the vlib executable")

find_program(QUESTA_VMAP_EXECUTABLE vmap
    HINTS ${QUESTA_HINTS}
    PATH_SUFFIXES bin ${QUESTA_PATH_SUFFIXES}
    DOC "Path to the vmap executable")

find_program(QUESTA_VCOM_EXECUTABLE vcom
    HINTS ${QUESTA_HINTS}
    PATH_SUFFIXES bin ${QUESTA_PATH_SUFFIXES}
    DOC "Path to the vcom executable")

find_program(QUESTA_VLOG_EXECUTABLE vlog
    HINTS ${QUESTA_HINTS}
    PATH_SUFFIXES bin ${QUESTA_PATH_SUFFIXES}
    DOC "Path to the vlog executable")

find_program(QUESTA_VSIM_EXECUTABLE vsim
    HINTS ${QUESTA_HINTS}
    PATH_SUFFIXES bin ${QUESTA_PATH_SUFFIXES}
    DOC "Path to the vsim executable")

mark_as_advanced(QUESTA_VLIB_EXECUTABLE)
mark_as_advanced(QUESTA_VMAP_EXECUTABLE)
mark_as_advanced(QUESTA_VCOM_EXECUTABLE)
mark_as_advanced(QUESTA_VLOG_EXECUTABLE)
mark_as_advanced(QUESTA_VSIM_EXECUTABLE)

set(libsvdpi_LIBRARIES 1)
find_path(libsvdpi_INCLUDE_DIRS
  NAMES "svdpi.h"
  PATHS ${QUESTA_HINTS}
  PATH_SUFFIXES include)

if(libsvdpi_LIBRARIES AND libsvdpi_INCLUDE_DIRS)
  set(libsvdpi_FOUND true)
endif(libsvdpi_LIBRARIES AND libsvdpi_INCLUDE_DIRS)

find_package_handle_standard_args(Questa REQUIRED_VARS
    QUESTA_VSIM_EXECUTABLE
    QUESTA_VMAP_EXECUTABLE
    QUESTA_VCOM_EXECUTABLE
    QUESTA_VLOG_EXECUTABLE
    QUESTA_VLIB_EXECUTABLE
    libsvdpi_INCLUDE_DIRS)
