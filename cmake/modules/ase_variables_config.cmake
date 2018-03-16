## Copyright(c) 2017, 2018, Intel Corporation
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

if(ASE_POST_INSTALL)
  set(ASE_SHARE_DIR            ${OPAE_BASE_DIR}/${OPAE_SHARE_DIR}/ase CACHE STRING "Directory containing shared ASE files")
else()
  set(ASE_SHARE_DIR            ${CMAKE_SOURCE_DIR}/ase)
endif()
set(ASE_SAMPLES                ${ASE_SHARE_DIR}/samples)
set(ASE_SCRIPTS_IN             ${ASE_SHARE_DIR}/in)
set(ASE_RTL_SOURCE_FILE_DIR    ${ASE_SHARE_DIR}/rtl)

set(PLATFORM_SHARE_DIR         ${OPAE_BASE_DIR}/${OPAE_SHARE_DIR}/platform)
set(PLATFORM_IF_DIR            ${PLATFORM_SHARE_DIR}/platform_if)
set(PLATFORM_IF_RTL            ${PLATFORM_IF_DIR}/rtl)

set(ASE_SIMULATOR "QUESTA" CACHE STRING "SystemVerilog simulator tool")
set(ASE_PLATFORM "FPGA_PLATFORM_INTG_XEON" CACHE STRING "FPGA platform")
set(ASE_PLATFORM_IF "ccip_std_afu" CACHE STRING "AFU interface type")
set(ASE_TIMESCALE "1ps/1ps" CACHE STRING "Simulator time-scale")
set(ASE_PROJECT_SOURCES "sources.txt" CACHE STRING "AFU sources input file")
set(ASE_DISCRETE_EMIF_MODEL "EMIF_MODEL_BASIC" CACHE STRING "ASE EMIF discrete memory model")
set(ASE_TOP_ENTITY "ase_top" CACHE STRING "Top entity for the ASE testbench")

if(ASE_PLATFORM STREQUAL "FPGA_PLATFORM_INTG_XEON")
    set(ASE_PLATFORM_ABBREV "intg_xeon")
else()
    set(ASE_PLATFORM_ABBREV "discrete")
endif()
