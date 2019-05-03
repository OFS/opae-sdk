## Copyright(c) 2017-2019, Intel Corporation
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

add_subdirectory(api)
add_subdirectory(plugins/fpgad-xfpga)
add_subdirectory(plugins/fpgad-vc)
add_subdirectory(plugins/fpgad-dc)

include_directories(${OPAE_INCLUDE_DIR}
                    ${libjson-c_INCLUDE_DIRS}
		    ${CMAKE_BINARY_DIR}/include
		    ${OPAE_SDK_SOURCE}/libopae
                    ${OPAE_SDK_SOURCE}/libbitstream
                    ${OPAE_SDK_SOURCE}/tools/base)

set(SRC
  command_line.c
  config_file.c
  event_dispatcher_thread.c
  events_api_thread.c
  fpgad.c
  monitor_thread.c
  daemonize.c
  monitored_device.c
)
add_executable(fpgad ${SRC})

set_install_rpath(fpgad)

target_link_libraries(fpgad
  safestr
  opae-c
  bitstream
  fpgad-api
  dl
  rt
  ${CMAKE_THREAD_LIBS_INIT}
  ${libjson-c_LIBRARIES}
  uuid)

set(CMAKE_C_FLAGS "-DHAVE_CONFIG_H ${CMAKE_C_FLAGS} -pthread")

configure_file(fpgad.service.in fpgad.service @ONLY NEWLINE_STYLE UNIX)

install(TARGETS fpgad
        RUNTIME DESTINATION bin
        COMPONENT toolfpgad)

if ("${CPACK_GENERATOR}" STREQUAL "RPM")

  if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fpgad.service DESTINATION /etc/systemd/system COMPONENT toolfpgad)
  else()
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fpgad.service DESTINATION ${CMAKE_INSTALL_PREFIX}/etc/systemd/system COMPONENT toolfpgad)
  endif()

else()

  # DEB
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/fpgad.service DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/system COMPONENT toolfpgad)

endif()

if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
  install(FILES fpgad.cfg DESTINATION /etc/opae COMPONENT toolfpgad)
  install(FILES fpgad.conf DESTINATION /etc/sysconfig COMPONENT toolfpgad)
else()
  install(FILES fpgad.cfg DESTINATION ${CMAKE_INSTALL_PREFIX}/etc/opae COMPONENT toolfpgad)
  install(FILES fpgad.conf DESTINATION ${CMAKE_INSTALL_PREFIX}/etc/sysconfig COMPONENT toolfpgad)
endif()

