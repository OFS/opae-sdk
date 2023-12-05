## Copyright(c) 2021-2023, Intel Corporation
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

set(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/timestamp)
file(GLOB_RECURSE PKG_FILES ${CMAKE_CURRENT_SOURCE_DIR}/ethernet/*.py)

add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND ${Python3_EXECUTABLE} -m pip install --root=${CMAKE_CURRENT_SOURCE_DIR}/build .
    COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS pyproject.toml ${PKG_FILES}
)

set_property(DIRECTORY PROPERTY
    ADDITIONAL_MAKE_CLEAN_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/build"
        "${CMAKE_CURRENT_SOURCE_DIR}/dist"
)

add_custom_target(build ALL DEPENDS ${OUTPUT})

opae_python_install(
    COMPONENT toolhssi
    RECORD_FILE hssi-install.txt
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    RPM_PACKAGE tools-extra
)
