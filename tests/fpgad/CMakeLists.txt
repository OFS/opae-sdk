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

opae_test_add_static_lib(TARGET fpgad-static
    SOURCE
        ${OPAE_BIN_SOURCE}/fpgad/command_line.c
	${OPAE_BIN_SOURCE}/fpgad/config_file.c
	${OPAE_BIN_SOURCE}/fpgad/daemonize.c
	${OPAE_BIN_SOURCE}/fpgad/event_dispatcher_thread.c
	${OPAE_BIN_SOURCE}/fpgad/events_api_thread.c
	${OPAE_BIN_SOURCE}/fpgad/fpgad.c
	${OPAE_BIN_SOURCE}/fpgad/monitored_device.c
	${OPAE_BIN_SOURCE}/fpgad/monitor_thread.c
    LIBS
        bitstream-static
        ${json-c_LIBRARIES}
        ${uuid_LIBRARIES}
)

target_compile_definitions(fpgad-static PRIVATE main=fpgad_main)

target_include_directories(fpgad-static
    PRIVATE
        ${OPAE_BIN_SOURCE}
	${OPAE_LIB_SOURCE}/libbitstream
)

opae_test_add_static_lib(TARGET fpgad-api-static
    SOURCE
        ${OPAE_BIN_SOURCE}/fpgad/api/logging.c
	${OPAE_BIN_SOURCE}/fpgad/api/opae_events_api.c
	${OPAE_BIN_SOURCE}/fpgad/api/device_monitoring.c
	${OPAE_BIN_SOURCE}/fpgad/api/sysfs.c
    LIBS
        ${CMAKE_THREAD_LIBS_INIT}
        ${json-c_LIBRARIES}
)

target_include_directories(fpgad-api-static
    PRIVATE
        ${OPAE_BIN_SOURCE}
	${OPAE_LIB_SOURCE}/libbitstream
)

opae_test_add_static_lib(TARGET fpgad-xfpga-static
    SOURCE ${OPAE_BIN_SOURCE}/fpgad/plugins/fpgad-xfpga/fpgad-xfpga.c
)

target_include_directories(fpgad-xfpga-static
    PRIVATE
        ${OPAE_BIN_SOURCE}
	${OPAE_LIB_SOURCE}/libbitstream
) 

opae_test_add_static_lib(TARGET fpgad-vc-static
    SOURCE ${OPAE_BIN_SOURCE}/fpgad/plugins/fpgad-vc/fpgad-vc.c
)

target_include_directories(fpgad-vc-static
    PRIVATE
        ${OPAE_BIN_SOURCE}
	${OPAE_LIB_SOURCE}/libbitstream
)

function(add_fpgad_test target source)
    opae_test_add(TARGET ${target}
        SOURCE ${source}
        LIBS
            opae-c-static
            fpgad-static
            fpgad-api-static
    )
    target_include_directories(${target}
        PRIVATE
            ${OPAE_BIN_SOURCE}
            ${OPAE_LIB_SOURCE}/libbitstream
    )
endfunction()

add_fpgad_test(test_fpgad_command_line_c            test_command_line_c.cpp)
add_fpgad_test(test_fpgad_config_file_c             test_config_file_c.cpp)
add_fpgad_test(test_fpgad_fpgad_c                   test_fpgad_c.cpp)
add_fpgad_test(test_fpgad_daemonize_c               test_daemonize_c.cpp)
add_fpgad_test(test_fpgad_event_dispatcher_thread_c test_event_dispatcher_thread_c.cpp)
add_fpgad_test(test_fpgad_events_api_thread_c       test_events_api_thread_c.cpp)
add_fpgad_test(test_fpgad_monitor_thread_c          test_monitor_thread_c.cpp)
add_fpgad_test(test_fpgad_monitored_device_c        test_monitored_device_c.cpp)

function(add_fpgad_api_test target source)
    opae_test_add(TARGET ${target}
        SOURCE ${source}
        LIBS
            fpgad-static
            fpgad-api-static
    )
    target_include_directories(${target}
        PRIVATE
            ${OPAE_BIN_SOURCE}
	    ${OPAE_LIB_SOURCE}/libbitstream
    )
endfunction()

add_fpgad_api_test(test_fpgad_api_logging_c           test_api_logging_c.cpp)
add_fpgad_api_test(test_fpgad_api_device_monitoring_c test_api_device_monitoring_c.cpp)
add_fpgad_api_test(test_fpgad_api_opae_events_api_c   test_api_opae_events_api_c.cpp)
add_fpgad_api_test(test_fpgad_api_sysfs_c             test_api_sysfs_c.cpp)

function(add_fpgad_xfpga_test target source)
    opae_test_add(TARGET ${target}
        SOURCE ${source}
        LIBS
            fpgad-static
            fpgad-xfpga-static
            fpgad-api-static
    )
    target_include_directories(${target}
        PRIVATE
            ${OPAE_BIN_SOURCE}
	    ${OPAE_LIB_SOURCE}/libbitstream
    )
endfunction()

add_fpgad_xfpga_test(test_fpgad_plugin_fpgad_xfpga_c test_plugin_fpgad_xfpga_c.cpp)
