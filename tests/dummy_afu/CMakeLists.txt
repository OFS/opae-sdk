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

opae_test_add_static_lib(TARGET dummy_afu-static
    SOURCE ${OPAE_SDK_SOURCE}/samples/dummy_afu/dummy_afu.cpp
    LIBS
        opae-c
        opae-cxx-core
)

target_compile_definitions(dummy_afu-static
    PRIVATE main=dummy_afu_main
)

target_include_directories(dummy_afu-static
    PRIVATE
        ${CMAKE_SOURCE_DIR}/samples/dummy_afu
        ${CMAKE_SOURCE_DIR}/external/CLI11/include
        ${CMAKE_SOURCE_DIR}/external/spdlog/include
)

opae_test_add(TARGET test_dummy_afu
    SOURCE test_dummy_afu.cpp
    LIBS
        dummy_afu-static
        test-fpgad-static
    TEST_FPGAD
)

target_include_directories(test_dummy_afu
    PRIVATE
        ${CMAKE_SOURCE_DIR}/external/CLI11/include
        ${CMAKE_SOURCE_DIR}/external/spdlog/include
        ${CMAKE_SOURCE_DIR}/samples/dummy_afu
        ${CMAKE_SOURCE_DIR}/external/opae-test/framework/mock/test_fpgad
)

