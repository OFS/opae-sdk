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

#set(SRC
#  enum.c
#  buffer.c
#  plugin.c
#  close.c
#  error.c
#  event.c
#  hostif.c
#  manage.c
#  metrics.c
#  mmio.c
#  open.c
#  properties.c
#  reconf.c
#  reset.c
#  sysobject.c
#  userclk.c
#  version.c
#  ${opae-test_ROOT}/framework/mock/opae_std.c)

set(SRC
    enum.cpp
    buffer.cpp
    plugin.c
    close.cpp
    error.cpp
    ${opae-test_ROOT}/framework/mock/opae_std.c
    opae.proto
)

opae_add_module_library(TARGET opae-grpc
    SOURCE ${SRC}
    LIBS
        dl
        ${CMAKE_THREAD_LIBS_INIT}
        opae-c
        ${libjson-c_LIBRARIES}
        ${libuuid_LIBRARIES}
        ${gRPC_LIBRARIES}
    COMPONENT opaeclib
)

target_include_directories(opae-grpc
    PRIVATE
        ${OPAE_LIB_SOURCE}/libopae-c
        ${gRPC_INCLUDE_DIRS}
)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET opae-grpc
    LANGUAGE cpp
)
protobuf_generate(TARGET opae-grpc
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
)
