## Copyright(c) 2017-2023, Intel Corporation
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
cmake_minimum_required(VERSION 3.14)

if (OPAE_WITH_CLI11 AND OPAE_WITH_SPDLOG)
    project(afu-test)
    add_library(afu-test INTERFACE)

    if (fmt_LIBRARIES)
        # if we found fmt before (from CMakeLists.txt)
        # then we need to find it again from this directory
        # so we can "import" the fmt::fmt link target
        find_package(fmt)
    endif (fmt_LIBRARIES)

    target_include_directories(afu-test INTERFACE
        ${OPAE_INCLUDE_PATHS}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CLI11_INCLUDE_DIRS}
        ${spdlog_INCLUDE_DIRS}
    )

    target_link_libraries(afu-test INTERFACE
        opae-c opae-cxx-core
        ${spdlog_LIBRARIES}
        ${fmt_LIBRARIES}
    )

    target_compile_options(afu-test INTERFACE
        -Wno-unused-result
    )

    target_compile_definitions(afu-test INTERFACE
        ${spdlog_DEFINITIONS}
    )
endif(OPAE_WITH_CLI11 AND OPAE_WITH_SPDLOG)
