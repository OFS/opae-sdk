## Copyright(c) 2018-2022, Intel Corporation
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

set(SRC
    pluginmgr.c
    api-shell.c
    init.c
    props.c
    cfg-file.c
    fpgad-cfg.c
    fpgainfo-cfg.c
    opae-cfg.c
    hash_map.c
    ${opae-test_ROOT}/framework/mock/opae_std.c
)

if (OPAE_BUILD_REMOTE)
    list(APPEND SRC
        uds-ifc.c
        inet-ifc.c
        rmt-ifc.c
        rmt-cfg.c
        pollsrv.c)
endif (OPAE_BUILD_REMOTE)

opae_add_shared_library(TARGET opae-c
    EXPORT opae-targets
    SOURCE ${SRC}
    LIBS
        dl
        ${CMAKE_THREAD_LIBS_INIT}
        ${libjson-c_LIBRARIES}
        ${libuuid_LIBRARIES}
    VERSION ${OPAE_VERSION}
    SOVERSION ${OPAE_VERSION_MAJOR}
    COMPONENT opaeclib
)

if (OPAE_BUILD_REMOTE)
    target_compile_definitions(opae-c
        PRIVATE
            OPAE_BUILD_REMOTE=1)
endif (OPAE_BUILD_REMOTE)

opae_add_static_library(TARGET opae-c-archive
    EXPORT opae-targets
    SOURCE ${SRC}
    LIBS
        dl
        ${CMAKE_THREAD_LIBS_INIT}
        ${libjson-c_LIBRARIES}
        ${libuuid_LIBRARIES}
    COMPONENT opaeclib
)

if (OPAE_BUILD_REMOTE)
    target_compile_definitions(opae-c-archive
        PRIVATE
            OPAE_BUILD_REMOTE=1)
endif (OPAE_BUILD_REMOTE)

install(FILES adapter.h props.h opae_int.h
    DESTINATION include/opae/plugin
)
install(FILES plugin.h
    DESTINATION include/opae
)
install(FILES ${OPAE_SDK_SOURCE}/tests/framework/mock/opae_std.h
    DESTINATION include/mock
)
