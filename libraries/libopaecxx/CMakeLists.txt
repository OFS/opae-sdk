## Copyright(c) 2018-2023, Intel Corporation
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

set(CMAKE_CXX_STANDARD 11)

set(OPAECXXCORE_SRC
    src/properties.cpp
    src/token.cpp
    src/handle.cpp
    src/shared_buffer.cpp
    src/events.cpp
    src/except.cpp
    src/errors.cpp
    src/sysobject.cpp
    src/version.cpp
)

opae_add_shared_library(TARGET opae-cxx-core
    EXPORT opae-targets
    SOURCE ${OPAECXXCORE_SRC}
    LIBS
        ${uuid_LIBRARIES}
    VERSION ${OPAE_VERSION}
    SOVERSION ${OPAE_VERSION_MAJOR}
    COMPONENT opaecxxcorelib
)

opae_add_executable(TARGET hello_cxxcore
    SOURCE samples/hello_fpga-1.cpp
    LIBS
        opae-c
        opae-cxx-core
        ${json-c_LIBRARIES}
    COMPONENT samples
)
