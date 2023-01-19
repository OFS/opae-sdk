#!/usr/bin/cmake -P
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
function(opae_add_shared_plugin)
    set(options )
    set(oneValueArgs TARGET PLUGIN PRIORITY)
    set(multiValueArgs SOURCE)
    cmake_parse_arguments(OPAE_ADD_SHARED_PLUGIN  "${options}"
        "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    set(register_c ${OPAE_ADD_SHARED_PLUGIN_PLUGIN}_register)
    set(plugin_so lib${OPAE_ADD_SHARED_PLUGIN_PLUGIN}.so)
    if (OPAE_ADD_SHARED_PLUGIN_PRIORITY)
        set(priority ${OPAE_ADD_SHARED_PLUGIN_PRIORITY})
    else()
        set(priority 1100)
    endif()

    set(source_code
"#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif // _GNU_SOURCE
#ifndef __USE_GNU
#define __USE_GNU 1
#endif // __USE_GNU
#include <opae/plugin.h>
__attribute__ ((constructor(${priority}))) static void ${register_c}(void)
{
    opae_plugin_mgr_register_plugin(\"${plugin_so}\", NULL)\;
}"
    )

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/${register_c}.c ${source_code})
    add_library(${OPAE_ADD_SHARED_PLUGIN_TARGET} SHARED
        ${register_c}.c
        ${OPAE_ADD_SHARED_PLUGIN_SOURCE}
    )
    target_link_libraries(${OPAE_ADD_SHARED_PLUGIN_TARGET}
        PUBLIC opae-c
    )
endfunction()
