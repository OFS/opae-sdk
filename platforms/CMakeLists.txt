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

set(PLATFORM_DIRS
        afu_top_ifc_db
        platform_db
        platform_if)

## This paths may be used in scripts to point to the installed platform database.
## The configure_file rule below handles the substitution.
set(PLATFORM_SHARE_DIR share/opae/platform)

install(DIRECTORY ${PLATFORM_DIRS} DESTINATION ${PLATFORM_SHARE_DIR} COMPONENT platform)

## Update directory paths in the platform configuration script
configure_file(
        scripts/afu_platform_config afu_platform_config
        @ONLY NEWLINE_STYLE UNIX)
add_custom_target(afu_platform_config ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/afu_platform_config)

set(PLATFORM_SCRIPTS
        ${CMAKE_CURRENT_BINARY_DIR}/afu_platform_config
        scripts/rtl_src_config)

install(PROGRAMS ${PLATFORM_SCRIPTS}
        DESTINATION bin
        COMPONENT platform)
