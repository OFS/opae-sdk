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

set(SRC
    opae.proto
    grpc_client.cpp
    grpc_server.cpp
    convert.cpp
    event_notifier.cpp
)

opae_add_shared_library(TARGET opae-clisrv
    SOURCE ${SRC}
    LIBS
        opae-c
        ${CMAKE_THREAD_LIBS_INIT}
        ${json-c_LIBRARIES}
        ${uuid_LIBRARIES}
        ${gRPC_LIBRARIES}
    VERSION ${OPAE_VERSION}
    SOVERSION ${OPAE_VERSION_MAJOR}
    COMPONENT opaeremote
)

target_include_directories(opae-clisrv
    PRIVATE
        ${OPAE_LIB_SOURCE}/libopae-c
        ${OPAE_LIB_SOURCE}/plugins/grpc
        ${gRPC_INCLUDE_DIRS}
        ${CMAKE_BINARY_DIR}/protos
)

add_custom_command(
    OUTPUT
        ${CMAKE_BINARY_DIR}/protos
    COMMAND
        mkdir -p ${CMAKE_BINARY_DIR}/protos
    DEPENDS
        ${CMAKE_BINARY_DIR}
)

add_custom_target(bin_protos
    DEPENDS
        ${CMAKE_BINARY_DIR}/protos
    COMMENT
        "Creating protos output directory"
)

add_dependencies(opae-clisrv bin_protos)

get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)
protobuf_generate(TARGET opae-clisrv
    LANGUAGE cpp
    PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/protos
)
protobuf_generate(TARGET opae-clisrv
    LANGUAGE grpc
    GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
    PROTOC_OUT_DIR ${CMAKE_BINARY_DIR}/protos
    PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}"
)
