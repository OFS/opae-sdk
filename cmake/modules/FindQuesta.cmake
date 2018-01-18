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

find_package(Perl REQUIRED)

set(MTI_HOME $ENV{MTI_HOME})
if(MTI_HOME)
  message("-- Using MTI_HOME: ${MTI_HOME}")
  set(VSIM_EXECUTABLE "${MTI_HOME}/bin/vsim" CACHE PATH "Path to vsim")
  set(VLOG_EXECUTABLE "${MTI_HOME}/bin/vlog" CACHE PATH "Path to vlog")
  set(VLIB_EXECUTABLE "${MTI_HOME}/bin/vlib" CACHE PATH "Path to vlib")
  set(VMAP_EXECUTABLE "${MTI_HOME}/bin/vmap" CACHE PATH "Path to vlib")
  set(VPI_INCLUDE_DIR "${MTI_HOME}/include"  CACHE PATH "Path to dpi.h file")
else()
  set(MTI_HOME "" CACHE PATH "Path to Questa Verilog simulator")
endif()

set(libsvdpi_LIBRARIES 1)
find_path(libsvdpi_INCLUDE_DIRS
  NAMES "svdpi.h"
  PATHS "${MTI_HOME}/include")

if(libsvdpi_LIBRARIES AND libsvdpi_INCLUDE_DIRS)
  set(libsvdpi_FOUND true)
endif(libsvdpi_LIBRARIES AND libsvdpi_INCLUDE_DIRS)
