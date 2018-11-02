## Copyright(c) 2018, Intel Corporation
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

include_directories(
	${OPAE_INCLUDE_DIR} )

add_library(bmc SHARED
	bmc.c
	bmcdata.c
	bmc_ioctl.c
	bmcinfo.c)

add_library(staticbmc STATIC
	bmc.c
	bmcdata.c
	bmc_ioctl.c
	bmcinfo.c)

set_install_rpath(bmc)

set_property(TARGET staticbmc PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(bmc safestr m rt ${CMAKE_THREAD_LIBS_INIT})

set_target_properties(bmc PROPERTIES
  VERSION ${INTEL_FPGA_API_VERSION}
  SOVERSION ${INTEL_FPGA_API_VER_MAJOR})


install(TARGETS bmc
    LIBRARY DESTINATION ${OPAE_LIB_INSTALL_DIR}
    COMPONENT opaetoolslibs)