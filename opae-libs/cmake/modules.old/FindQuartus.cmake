## Copyright(c) 2017, Intel Corporation
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

if (QUARTUS_FOUND)
    return()
endif()

find_package(PackageHandleStandardArgs REQUIRED)

set(QUARTUS_HINTS
  $ENV{QUARTUS_ROOTDIR}
  $ENV{QUARTUS_HOME}
  $ENV{QUARTUS_ROOT}
  $ENV{QUARTUS_DIR}
  $ENV{QUARTUS})

find_program(QUARTUS_EXECUTABLE quartus
  HINTS ${QUARTUS_HINTS}
  PATH_SUFFIXES bin bin64
  DOC "Path to the Quartus executable")

find_program(QUARTUS_MAP quartus_map
  HINTS ${QUARTUS_HINTS}
  PATH_SUFFIXES bin bin64
  DOC "Path to the Quartus map executable")

find_program(QUARTUS_SYN quartus_syn
  HINTS ${QUARTUS_HINTS}
  PATH_SUFFIXES bin bin64
  DOC "Path to the Quartus syn executable")

find_program(QUARTUS_SH quartus_sh
  HINTS ${QUARTUS_HINTS}
  PATH_SUFFIXES bin bin64
  DOC "Path to the Quartus sh executable")

find_program(QUARTUS_QSYS_GENERATE qsys-generate
  HINTS ${QUARTUS_HINTS}
  PATH_SUFFIXES ../qsys/bin ../qsys/bin64 ../sopc_builder/bin
  DOC "Path to the Quartus Qsys generate")

find_program(QUARTUS_QSYS_SCRIPT qsys-script
  HINTS ${QUARTUS_HINTS}
  PATH_SUFFIXES ../qsys/bin ../qsys/bin64 ../sopc_builder/bin
  DOC "Path to the Quartus Qsys script")

if (QUARTUS_SH)
  execute_process(COMMAND ${QUARTUS_SH}
    --tcl_eval puts "$::quartus(version)"
    OUTPUT_VARIABLE quartus_version
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  if (quartus_version MATCHES Pro)
    set(QUARTUS_EDITION Pro)
  elseif (quartus_version MATCHES Lite)
    set(QUARTUS_EDITION Lite)
  else ()
    set(QUARTUS_EDITION Standard)
  endif()

  string(REGEX REPLACE " " ";" quartus_version ${quartus_version})

  list(GET quartus_version 1 QUARTUS_VERSION)
endif()

get_filename_component(QUARTUS_DIR ${QUARTUS_EXECUTABLE} DIRECTORY)
get_filename_component(QUARTUS_DIR ${QUARTUS_DIR}/.. REALPATH)

mark_as_advanced(QUARTUS_EXECUTABLE)
mark_as_advanced(QUARTUS_SH)
mark_as_advanced(QUARTUS_MAP)
mark_as_advanced(QUARTUS_SYN)
mark_as_advanced(QUARTUS_QSYS_SCRIPT)
mark_as_advanced(QUARTUS_QSYS_GENERATE)

find_package_handle_standard_args(Quartus REQUIRED_VARS
    QUARTUS_EXECUTABLE
    QUARTUS_MAP
    QUARTUS_SYN
    QUARTUS_SH
    QUARTUS_QSYS_SCRIPT
    QUARTUS_QSYS_GENERATE)
