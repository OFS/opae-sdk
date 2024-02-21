## Copyright(c) 2020-2022, Intel Corporation
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

opae_test_add_static_lib(TARGET fpgainfo-static
    SOURCE
        ${OPAE_BIN_SOURCE}/fpgainfo/bmcdata.c
	${OPAE_BIN_SOURCE}/fpgainfo/bmcinfo.c
	${OPAE_BIN_SOURCE}/fpgainfo/errors.c
        ${OPAE_BIN_SOURCE}/fpgainfo/events.c
        ${OPAE_BIN_SOURCE}/fpgainfo/fmeinfo.c
        ${OPAE_BIN_SOURCE}/fpgainfo/fpgainfo.c
        ${OPAE_BIN_SOURCE}/fpgainfo/portinfo.c
        ${OPAE_BIN_SOURCE}/fpgainfo/powerinfo.c
        ${OPAE_BIN_SOURCE}/fpgainfo/tempinfo.c
        ${OPAE_BIN_SOURCE}/fpgainfo/board.c
        ${OPAE_BIN_SOURCE}/fpgainfo/main.c
    LIBS
        argsfilter-static
)

target_compile_definitions(fpgainfo-static
    PRIVATE main=fpgainfo_main
)

target_include_directories(fpgainfo-static
    PRIVATE ${OPAE_LIB_SOURCE}/argsfilter
)

opae_test_add(TARGET test_fpgainfo_c
    SOURCE test_fpgainfo_c.cpp
    LIBS fpgainfo-static
         board-common-static
         opaeuio
)

opae_test_add(TARGET test_fpgainfo_board_c 
    SOURCE test_board_c.cpp
    LIBS fpgainfo-static
         board-common-static
         opaeuio
)
