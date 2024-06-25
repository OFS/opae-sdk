## Copyright(c) 2017-2022, Intel Corporation
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

opae_add_subdirectory(fpgaconf)
opae_add_subdirectory(fpgainfo)
opae_add_subdirectory(fpgametrics)

option(OPAE_BUILD_USERCLK "Enable building extra tool userclk" ON)
mark_as_advanced(OPAE_BUILD_USERCLK)

option(OPAE_BUILD_QSFPINFO "Enable building extra tool qsfpinfo" ON)
mark_as_advanced(OPAE_BUILD_QSFPINFO)

option(OPAE_BUILD_FPGADIAG "Enable building extra tool fpgadiag" ON)
mark_as_advanced(OPAE_BUILD_FPGADIAG)

option(OPAE_BUILD_MMLINK "Enable building extra tool mmlink" ON)
mark_as_advanced(OPAE_BUILD_MMLINK)

option(OPAE_BUILD_PAC_HSSI_CONFIG  "Enable building extra tool pac_hssi_config" ON)
mark_as_advanced(OPAE_BUILD_PAC_HSSI_CONFIG)

option(OPAE_BUILD_FPGABIST "Enable building extra tool fpgabist" OFF)
mark_as_advanced(OPAE_BUILD_FPGABIST)

option(OPAE_BUILD_OPAEIO "Enable building extra tool opae.io" ON)
mark_as_advanced(OPAE_BUILD_OPAEIO)

opae_add_subdirectory(hssi)

if(OPAE_BUILD_EXTRA_TOOLS)
  if(OPAE_BUILD_USERCLK)
    opae_add_subdirectory(userclk)
  endif()
  if(OPAE_BUILD_QSFPINFO)
    opae_add_subdirectory(qsfpinfo)
  endif()
  if(OPAE_BUILD_FPGADIAG)
    set(OPAE_BUILD_CXXUTILS ON)
    opae_add_subdirectory(fpgadiag)
  endif()
  if(OPAE_BUILD_MMLINK)
    opae_add_subdirectory(mmlink)
  endif()
  if(OPAE_BUILD_PAC_HSSI_CONFIG)
    opae_add_subdirectory(pac_hssi_config)
  endif()
  if(OPAE_BUILD_FPGABIST)
    opae_add_subdirectory(fpgabist)
  endif()
  if(OPAE_BUILD_OPAEIO)
    opae_add_subdirectory(opae.io)
  endif()
endif()

if (OPAE_WITH_PYBIND11)
  opae_add_subdirectory(ofs.uio)
endif()

opae_add_subdirectory(vabtool)
opae_add_subdirectory(fpgad)
