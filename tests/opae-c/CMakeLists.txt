## Copyright(c) 2020-2023, Intel Corporation
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

opae_test_add_static_lib(TARGET opae-c-static
    SOURCE
        ${OPAE_LIB_SOURCE}/libopae-c/api-shell.c
        ${OPAE_LIB_SOURCE}/libopae-c/init.c
        ${OPAE_LIB_SOURCE}/libopae-c/pluginmgr.c
        ${OPAE_LIB_SOURCE}/libopae-c/props.c
        ${OPAE_LIB_SOURCE}/libopae-c/cfg-file.c
        ${OPAE_LIB_SOURCE}/libopae-c/fpgad-cfg.c
        ${OPAE_LIB_SOURCE}/libopae-c/fpgainfo-cfg.c
        ${OPAE_LIB_SOURCE}/libopae-c/opae-cfg.c
    LIBS
        ${CMAKE_THREAD_LIBS_INIT}
        ${json-c_LIBRARIES}
)

opae_test_add(TARGET self_test
    SOURCE self_test.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_enum_c
    SOURCE test_enum_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_open_c
    SOURCE test_open_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_props_c
    SOURCE test_props_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_reset_c
    SOURCE test_reset_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_mmio_c
    SOURCE test_mmio_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_metrics_c
    SOURCE test_metrics_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_umsg_c
    SOURCE test_umsg_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_buffer_c
    SOURCE test_buffer_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_version_c
    SOURCE test_version_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_error_c
    SOURCE test_error_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_event_c 
    SOURCE test_event_c.cpp
    LIBS
        opae-c-static
        test-fpgad-static
    TEST_FPGAD
)

target_include_directories(test_opae_event_c
    PRIVATE
        ${OPAE_LIB_SOURCE}/libbitstream
)

opae_test_add(TARGET test_opae_sdl_c
    SOURCE test_sdl_c.cpp
    LIBS
        opae-c-static
        bmc-static
        xfpga-static
)

target_include_directories(test_opae_sdl_c
    PRIVATE
        ${OPAE_LIB_SOURCE}/plugins/xfpga
	${OPAE_LIB_SOURCE}/plugins/xfpga/metrics/bmc
)

opae_test_add(TARGET test_opae_hostif_c
    SOURCE test_hostif_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_reconf_c
    SOURCE test_reconf_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_object_c
    SOURCE test_object_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_usrclk_c
    SOURCE test_usrclk_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_init_c
    SOURCE test_init_c.cpp
    LIBS opae-c-static
)

opae_test_add(TARGET test_opae_pluginmgr_c
    SOURCE test_pluginmgr_c.cpp
    LIBS
	opae-c-static
)

set_tests_properties(test_opae_pluginmgr_c
    PROPERTIES
        ENVIRONMENT "LD_LIBRARY_PATH=${LIBRARY_OUTPUT_PATH}")

opae_test_add(TARGET test_cfg_file_c
    SOURCE test_cfg_file_c.cpp
    LIBS
	opae-c-static
)

opae_test_add(TARGET test_opae_cfg_c
    SOURCE test_opae_cfg_c.cpp
    LIBS
	opae-c-static
)

opae_test_add(TARGET test_fpgainfo_cfg_c
    SOURCE test_fpgainfo_cfg_c.cpp
    LIBS
	opae-c-static
)

opae_test_add(TARGET test_fpgad_cfg_c
    SOURCE test_fpgad_cfg_c.cpp
    LIBS
	opae-c-static
)

add_library(dummy_plugin MODULE dummy_plugin.c)

target_include_directories(dummy_plugin
    PRIVATE
        ${OPAE_INCLUDE_PATH}
        ${OPAE_LIB_SOURCE}/libopae-c
        ${opae-test_ROOT}/framework
        ${json-c_INCLUDE_DIRS}
        ${uuid_INCLUDE_DIRS}
)

target_link_libraries(dummy_plugin ${json-c_LIBRARIES})
add_dependencies(test_opae_pluginmgr_c dummy_plugin)
