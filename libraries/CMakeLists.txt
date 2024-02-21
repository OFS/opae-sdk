## Copyright(c) 2022, Intel Corporation
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

# extra
# Top level OPAE_BUILD_EXTRA_TOOLS controls building all the extra tools
# Each extra tool gets its own conditional building variable
# format is OPAE_BUILD_<TOOL_NAME>
option(OPAE_BUILD_EXTRA_TOOLS "Enable building extra tools" ON)
mark_as_advanced(OPAE_BUILD_EXTRA_TOOLS)

option(OPAE_BUILD_CXXUTILS "Enable building extra tool c++utils" ON)
mark_as_advanced(OPAE_BUILD_CXXUTILS)

if(OPAE_BUILD_EXTRA_TOOLS)
  if(OPAE_BUILD_CXXUTILS)
    opae_add_subdirectory(c++utils)
  endif()
endif()

opae_add_subdirectory(argsfilter)
opae_add_subdirectory(afu-test)
opae_add_subdirectory(libopaemem)
opae_add_subdirectory(libopae-c)
if (OPAE_BUILD_LIBOFS)
    opae_add_subdirectory(libofs)
endif (OPAE_BUILD_LIBOFS)

if (OPAE_BUILD_LIBOPAEUIO)
    opae_add_subdirectory(libopaeuio)
    opae_add_subdirectory(pyopaeuio)
endif()

if (OPAE_BUILD_LIBOPAEVFIO AND PLATFORM_SUPPORTS_VFIO)
    opae_add_subdirectory(libopaevfio)
endif()

opae_add_subdirectory(libbitstream)
opae_add_subdirectory(plugins)

if(OPAE_BUILD_LIBOPAE_CXX)
    opae_add_subdirectory(libopaecxx)
endif()

if(OPAE_WITH_PYBIND11 AND OPAE_BUILD_LIBOPAE_CXX)
    opae_add_subdirectory(pyopae)
endif()

opae_add_subdirectory(libboard/board_common)
opae_add_subdirectory(libboard/board_a10gx)
opae_add_subdirectory(libboard/board_n3000)
opae_add_subdirectory(libboard/board_d5005)
opae_add_subdirectory(libboard/board_n6000)
opae_add_subdirectory(libboard/board_n5010)
opae_add_subdirectory(libboard/board_c6100)
opae_add_subdirectory(libboard/board_jtag_pci_dk)
opae_add_subdirectory(libboard/board_cmc)
