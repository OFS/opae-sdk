## Copyright(c) 2023, Intel Corporation
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

opae_test_add_static_lib(TARGET opae-u-static
    SOURCE
        ${OPAE_LIB_SOURCE}/plugins/uio/dfl.c
        ${OPAE_LIB_SOURCE}/plugins/uio/opae_uio.c
        ${OPAE_LIB_SOURCE}/plugins/uio/plugin.c
    LIBS
        dl
        m
        ${CMAKE_THREAD_LIBS_INIT}
        opae-c
        opaeuio
        ${json-c_LIBRARIES}
        ${uuid_LIBRARIES}
)

opae_test_add(TARGET test_opae_u_dfl_c
    SOURCE test_dfl_c.cpp
    LIBS opae-u-static
)

target_include_directories(test_opae_u_dfl_c
    PRIVATE
        ${OPAE_LIB_SOURCE}/plugins/uio
)

opae_test_add(TARGET test_opae_u_opae_uio_c
    SOURCE test_opae_uio_c.cpp
    LIBS opae-u-static
)

target_include_directories(test_opae_u_opae_uio_c
    PRIVATE
        ${OPAE_LIB_SOURCE}/plugins/uio
)

opae_test_add(TARGET test_opae_u_plugin_c
    SOURCE test_plugin_c.cpp
    LIBS opae-u-static
)

target_include_directories(test_opae_u_plugin_c
    PRIVATE
        ${OPAE_LIB_SOURCE}/plugins/uio
)
