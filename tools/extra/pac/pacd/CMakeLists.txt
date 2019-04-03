## Copyright(c) 2017, Intel Corporation
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


include_directories(${OPAE_INCLUDE_DIR}
                    ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga
                    ${OPAE_SDK_SOURCE}/libopae
                    ${OPAE_SDK_SOURCE}/libbitstream
                    ${OPAE_SDK_SOURCE}/tools/include)

set(SERVICE_FILE pacd.service)
set(SERVICE_FILE_DIR ${CMAKE_INSTALL_PREFIX}/usr/lib/systemd/system)
set(SERVICE_CONF_FILE pacd.conf)
set(SERVICE_CONF_DIR ${CMAKE_INSTALL_PREFIX}/etc/sysconfig)

set(PACD_ENVIRONMENT_FILE ${SERVICE_CONF_DIR}/${SERVICE_CONF_FILE})
set(PACD_ROOT_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)
set(PACD_BINARY ${PACD_ROOT_DIRECTORY}/pacd)

configure_file(pacd.service.in pacd.service @ONLY NEWLINE_STYLE UNIX)

set(SRC pacd.c daemonize.c log.c enumerate.c sysfs.c bmc_thermal.c reset_bmc.c)
add_executable(pacd ${SRC})

set_install_rpath(pacd)

target_link_libraries(pacd bmc safestr bitstream uuid opae-c pthread rt)

if (IS_DIRECTORY /usr/lib/systemd/system)
#    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SERVICE_FILE} DESTINATION ${SERVICE_FILE_DIR} COMPONENT toolpacd)
#    install(FILES ${SERVICE_CONF_FILE} DESTINATION ${SERVICE_CONF_DIR} COMPONENT toolpacd)
     install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SERVICE_FILE} DESTINATION ${PACD_ROOT_DIRECTORY} COMPONENT toolpacd)
     install(FILES ${SERVICE_CONF_FILE} DESTINATION ${PACD_ROOT_DIRECTORY} COMPONENT toolpacd)
else()
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${SERVICE_FILE} ${SERVICE_CONF_FILE}
        DESTINATION ${PACD_ROOT_DIRECTORY} COMPONENT toolpacd)
endif()

install(TARGETS pacd
        RUNTIME DESTINATION ${PACD_ROOT_DIRECTORY}
        COMPONENT toolpacd)
