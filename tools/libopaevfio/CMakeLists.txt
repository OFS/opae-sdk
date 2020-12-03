## Copyright(c) 2020, Intel Corporation
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

try_compile(PLATFORM_SUPPORTS_VFIO
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/vfiocheck.c
    OUTPUT_VARIABLE TRY_COMPILE_OUTPUT
)

if (PLATFORM_SUPPORTS_VFIO)
    set(PLATFORM_SUPPORTS_VFIO TRUE CACHE BOOL "Platform supports vfio driver")
    opae_add_shared_library(TARGET opaevfio
        SOURCE opaevfio.c
        LIBS
            ${CMAKE_THREAD_LIBS_INIT}
        VERSION ${OPAE_VERSION}
        SOVERSION ${OPAE_VERSION_MAJOR}
        COMPONENT vfiolib
    )
    
    opae_add_executable(TARGET opaevfiotest
        SOURCE opaevfiotest.c
        LIBS opaevfio
        COMPONENT vfiotest
    )
else(PLATFORM_SUPPORTS_VFIO)
    message(WARNING
        "Could not compile VFIO. See errors in platform_vfio_errors.txt")
    file(WRITE ${CMAKE_BINARY_DIR}/platform_vfio_errors.txt ${TRY_COMPILE_OUTPUT})
endif(PLATFORM_SUPPORTS_VFIO)
