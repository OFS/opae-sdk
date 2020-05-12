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


opae_test_add_static_lib(TARGET board-common-static
    SOURCE ${OPAE_SDK_SOURCE}/tools/libboard/board_common/board_common.c
    LIBS
        opae-c
)

opae_test_add_static_lib(TARGET board-n3000-static
    SOURCE ${OPAE_SDK_SOURCE}/tools/libboard/board_n3000/board_n3000.c
    LIBS
        opae-c
)

opae_test_add_static_lib(TARGET board-a10gx-static
    SOURCE ${OPAE_SDK_SOURCE}/tools/libboard/board_a10gx/board_a10gx.c
    LIBS
        opae-c
)

opae_test_add_static_lib(TARGET board-d5005-static
    SOURCE ${OPAE_SDK_SOURCE}/tools/libboard/board_d5005/board_d5005.c
    LIBS
        opae-c
)

opae_test_add(TARGET test_board_n3000_c
    SOURCE test_board_n3000.cpp
    LIBS
        board-n3000-static
        board-common-static
)

target_include_directories(test_board_n3000_c
    PRIVATE ${OPAE_SDK_SOURCE}/tools
)

opae_test_add(TARGET test_board_a10gx_c
    SOURCE test_board_a10gx.cpp
    LIBS
        board-a10gx-static
        board-common-static
)

target_include_directories(test_board_a10gx_c
    PRIVATE ${OPAE_SDK_SOURCE}/tools
)

opae_test_add(TARGET test_board_d5005_c
    SOURCE test_board_d5005.cpp
    LIBS
        board-d5005-static
        board-common-static
)

target_include_directories(test_board_d5005_c
    PRIVATE ${OPAE_SDK_SOURCE}/tools
)
