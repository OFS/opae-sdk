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

opae_test_add_static_lib(TARGET opae-cxx-core-static
    SOURCE
        ${OPAE_LIBS_ROOT}/libopaecxx/src/errors.cpp
        ${OPAE_LIBS_ROOT}/libopaecxx/src/events.cpp
        ${OPAE_LIBS_ROOT}/libopaecxx/src/except.cpp
        ${OPAE_LIBS_ROOT}/libopaecxx/src/handle.cpp
        ${OPAE_LIBS_ROOT}/libopaecxx/src/properties.cpp
        ${OPAE_LIBS_ROOT}/libopaecxx/src/shared_buffer.cpp
        ${OPAE_LIBS_ROOT}/libopaecxx/src/token.cpp
        ${OPAE_LIBS_ROOT}/libopaecxx/src/sysobject.cpp
        ${OPAE_LIBS_ROOT}/libopaecxx/src/version.cpp
    LIBS
        opae-c
        ${libjson-c_LIBRARIES}
)

opae_test_add(TARGET test_opae_handle_cxx_core
    SOURCE test_handle_cxx_core.cpp
    LIBS opae-cxx-core-static
)

opae_test_add(TARGET test_opae_buffer_cxx_core
    SOURCE test_buffer_cxx_core.cpp
    LIBS opae-cxx-core-static
)

opae_test_add(TARGET test_opae_errors_cxx_core
    SOURCE test_errors_cxx_core.cpp
    LIBS opae-cxx-core-static
)

opae_test_add(TARGET test_opae_events_cxx_core
    SOURCE test_events_cxx_core.cpp
    LIBS
        safestr
        test-fpgad-static
        opae-cxx-core-static
    TEST_FPGAD
)

target_include_directories(test_opae_events_cxx_core
    PRIVATE
        ${OPAE_LIBS_ROOT}/libbitstream
)

opae_test_add(TARGET test_opae_properties_cxx_core
    SOURCE test_properties_cxx_core.cpp
    LIBS opae-cxx-core-static
)

opae_test_add(TARGET test_opae_version_cxx_core
    SOURCE test_version_cxx_core.cpp
    LIBS opae-cxx-core-static
)

opae_test_add(TARGET test_opae_except_cxx_core
    SOURCE test_except_cxx_core.cpp
    LIBS opae-cxx-core-static
)

opae_test_add(TARGET test_opae_object_cxx_core
    SOURCE test_object_cxx_core.cpp
    LIBS opae-cxx-core-static
)
