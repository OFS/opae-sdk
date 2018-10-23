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

add_subdirectory(dma_common)
add_subdirectory(memcpy)
add_subdirectory(mmio)
add_subdirectory(pool)
add_subdirectory(queue)
add_subdirectory(buffer)

set_property(TARGET dma_common PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET memcpy PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET mmio PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET pool PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET queue PROPERTY POSITION_INDEPENDENT_CODE ON)
set_property(TARGET buffer PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(utils SHARED
    $<TARGET_OBJECTS:dma_common>
    $<TARGET_OBJECTS:mmio>
    $<TARGET_OBJECTS:pool>
    $<TARGET_OBJECTS:queue>
    $<TARGET_OBJECTS:buffer>
    $<TARGET_OBJECTS:memcpy>)

set_install_rpath(utils)

install(TARGETS utils
    LIBRARY DESTINATION ${OPAE_LIB_INSTALL_DIR}
    COMPONENT toolfpgainfo)
