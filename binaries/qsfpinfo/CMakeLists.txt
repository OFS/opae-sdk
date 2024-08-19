## Copyright(c) 2024, Silciom Denmark A/S
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

if (OPAE_WITH_QSFPINFO_QSFPPRINT)

    opae_add_executable(TARGET qsfpinfo
        SOURCE
            main.c
            cmis_wrap.c
            sff-common_wrap.c
        LIBS
            argsfilter
            opae-c
            m
        COMPONENT toolqsfpinfo
    )

    include(ExternalProject)

    ExternalProject_Add(
        ethtool_project
        GIT_REPOSITORY "https://git.kernel.org/pub/scm/network/ethtool/ethtool.git"
        GIT_TAG "v6.7"
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        UPDATE_COMMAND ""
    )

    ExternalProject_Get_Property(ethtool_project SOURCE_DIR)

    add_custom_target(get-ethtool-files
        DEPENDS ethtool_project
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SOURCE_DIR}/cmis.c"
            "${SOURCE_DIR}/cmis.h"
            "${SOURCE_DIR}/qsfp.c"
            "${SOURCE_DIR}/qsfp.h"
            "${SOURCE_DIR}/sff-common.c"
            "${SOURCE_DIR}/sff-common.h"
            "${SOURCE_DIR}/internal.h"
            "${SOURCE_DIR}/json_print.h"
            "${SOURCE_DIR}/json_writer.h" "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SOURCE_DIR}/netlink/extapi.h" "${CMAKE_CURRENT_SOURCE_DIR}/netlink/extapi.h"
    )

    target_compile_options(qsfpinfo PUBLIC
        -D_QSFPPRINT
    )

    add_dependencies(qsfpinfo get-ethtool-files)

else(OPAE_WITH_QSFPINFO_QSFPPRINT)

    opae_add_executable(TARGET qsfpinfo
        SOURCE
            main.c
        LIBS
            argsfilter
            opae-c
            m
        COMPONENT toolqsfpinfo
    )

endif(OPAE_WITH_QSFPINFO_QSFPPRINT)

target_include_directories(qsfpinfo
    PRIVATE
        ${OPAE_LIB_SOURCE}/argsfilter
)

