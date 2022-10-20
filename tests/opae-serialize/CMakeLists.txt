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

opae_test_add_static_lib(TARGET opae-serialize-static
    SOURCE
        ${OPAE_LIB_SOURCE}/libopae-serialize/action.c
        ${OPAE_LIB_SOURCE}/libopae-serialize/request.c
        ${OPAE_LIB_SOURCE}/libopae-serialize/response.c
        ${OPAE_LIB_SOURCE}/libopae-serialize/serialize.c
    LIBS
        ${CMAKE_THREAD_LIBS_INIT}
        ${libjson-c_LIBRARIES}
	${libuuid_LIBRARIES}
)

#opae_test_add(TARGET test_ser_action_c
#    SOURCE test_action_c.cpp
#    LIBS opae-serialize-static
#)

opae_test_add(TARGET test_ser_request_c
    SOURCE test_request_c.cpp
    LIBS
        opae-c-static
        opae-serialize-static
)

target_include_directories(test_ser_request_c
    PRIVATE
        ${OPAE_LIB_SOURCE}/libopae-serialize
)

#opae_test_add(TARGET test_ser_response_c
#    SOURCE test_response_c.cpp
#    LIBS opae-serialize-static
#)

opae_test_add(TARGET test_ser_serialize_c
    SOURCE test_serialize_c.cpp
    LIBS
        opae-c-static
        opae-serialize-static
)

target_include_directories(test_ser_serialize_c
    PRIVATE
        ${OPAE_LIB_SOURCE}/libopae-serialize
)
