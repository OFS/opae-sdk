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

opae_add_subdirectory(argsfilter)
opae_add_subdirectory(libboard/board_common)
opae_add_subdirectory(libboard/board_a10gx)
opae_add_subdirectory(libboard/board_n3000)
opae_add_subdirectory(libboard/board_d5005)
opae_add_subdirectory(fpgaconf)
opae_add_subdirectory(fpgainfo)
opae_add_subdirectory(fpgametrics)
opae_add_subdirectory(libopaevfio)
opae_add_subdirectory(libopaeuio)

# extra
# Top level OPAE_BUILD_EXTRA_TOOLS controls building all the extra tools
# Each extra tool gets its own conditional building variable
# format is OPAE_BUILD_EXTRA_TOOLS_<TOOL_NAME>
option(OPAE_BUILD_EXTRA_TOOLS "Enable building extra tools" ON)
mark_as_advanced(OPAE_BUILD_EXTRA_TOOLS)

option(OPAE_BUILD_EXTRA_TOOLS_USERCLK "Enable building extra tool userclk" ON)
mark_as_advanced(OPAE_BUILD_EXTRA_TOOLS_USERCLK)

option(OPAE_BUILD_EXTRA_TOOLS_PACKAGER "Enable building extra tool packager" ON)
mark_as_advanced(OPAE_BUILD_EXTRA_TOOLS_PACKAGER)

option(OPAE_BUILD_EXTRA_TOOLS_CXXUTILS "Enable building extra tool c++utils" ON)
mark_as_advanced(OPAE_BUILD_EXTRA_TOOLS_CXXUTILS)

option(OPAE_BUILD_EXTRA_TOOLS_FPGADIAG "Enable building extra tool fpgadiag" ON)
mark_as_advanced(OPAE_BUILD_EXTRA_TOOLS_FPGADIAG)

option(OPAE_BUILD_EXTRA_TOOLS_MMLINK "Enable building extra tool mmlink" ON)
mark_as_advanced(OPAE_BUILD_EXTRA_TOOLS_MMLINK)

option(OPAE_BUILD_EXTRA_TOOLS_RAS "Enable building extra tool ras" ON)
mark_as_advanced(OPAE_BUILD_EXTRA_TOOLS_RAS)

option(OPAE_BUILD_EXTRA_TOOLS_PAC_HSSI_CONFIG  "Enable building extra tool pac_hssi_config" ON)
mark_as_advanced(OPAE_BUILD_EXTRA_TOOLS_PAC_HSSI_CONFIG)

option(OPAE_BUILD_EXTRA_TOOLS_FPGABIST "Enable building extra tool fpgabist" OFF)
mark_as_advanced(OPAE_BUILD_EXTRA_TOOLS_FPGABIST)

option(OPAE_BUILD_EXTRA_TOOLS_OPAEIO "Enable building extra tool opae.io" ON)
mark_as_advanced(OPAE_BUILD_EXTRA_TOOLS_OPAEIO)

if(OPAE_BUILD_EXTRA_TOOLS)
  if(OPAE_BUILD_EXTRA_TOOLS_USERCLK)
    opae_add_subdirectory(extra/userclk)
  endif()
  if(OPAE_BUILD_EXTRA_TOOLS_PACKAGER)
    opae_add_subdirectory(extra/packager)
  endif()
  if(OPAE_BUILD_EXTRA_TOOLS_FPGADIAG)
    set(OPAE_BUILD_EXTRA_TOOLS_CXXUTILS ON)
    opae_add_subdirectory(extra/fpgadiag)
  endif()
  if(OPAE_BUILD_EXTRA_TOOLS_CXXUTILS)
    opae_add_subdirectory(extra/c++utils)
  endif()
  if(OPAE_BUILD_EXTRA_TOOLS_MMLINK)
    opae_add_subdirectory(extra/mmlink)
  endif()
  if(OPAE_BUILD_EXTRA_TOOLS_RAS)
    opae_add_subdirectory(extra/ras)
  endif()
  if(OPAE_BUILD_EXTRA_TOOLS_PAC_HSSI_CONFIG)
    opae_add_subdirectory(extra/pac_hssi_config)
  endif()
  if(OPAE_BUILD_EXTRA_TOOLS_FPGABIST)
    opae_add_subdirectory(extra/fpgabist)
  endif()
  if(OPAE_BUILD_EXTRA_TOOLS_OPAEIO)
    opae_add_subdirectory(extra/opae.io)
  endif()
endif()
