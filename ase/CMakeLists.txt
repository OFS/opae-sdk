## Copyright(c) 2017, 2018, Intel Corporation
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

## Install target for ASE support files
project(ase)
set(OPAE_SHARE_DIR share/opae)
set(ASE_SHARE_DIR  share/opae/ase)
set(ASE_SAMPLES    ${ASE_SHARE_DIR}/samples)
add_subdirectory(api)

install(DIRECTORY rtl          DESTINATION share/opae/ase COMPONENT asertl)
install(DIRECTORY sw           DESTINATION share/opae/ase COMPONENT asesw)
install(DIRECTORY scripts      DESTINATION share/opae/ase USE_SOURCE_PERMISSIONS COMPONENT asescripts)

# Post-installation cmake project
configure_file(${PROJECT_SOURCE_DIR}/in/CMakeLists.txt.in
  ${PROJECT_BINARY_DIR}/CMakeLists.txt)
install(DIRECTORY in DESTINATION share/opae/ase COMPONENT asescripts)
install(PROGRAMS ${PROJECT_BINARY_DIR}/CMakeLists.txt
  DESTINATION ${ASE_SHARE_DIR}
  COMPONENT asescripts)

# CCIP base RTL code
configure_file(${PROJECT_SOURCE_DIR}/rtl/sources_abs.txt.in
  ${PROJECT_BINARY_DIR}/rtl/sources_abs.txt)
install(FILES ${PROJECT_BINARY_DIR}/rtl/sources_abs.txt
  DESTINATION ${ASE_SHARE_DIR}/rtl
  COMPONENT aseextra)

# Samples
configure_file(${PROJECT_SOURCE_DIR}/samples/intg_xeon_nlb/sources.txt
  ${PROJECT_BINARY_DIR}/samples/intg_xeon_nlb/sources.txt)
configure_file(${PROJECT_SOURCE_DIR}/samples/intg_xeon_nlb/ccip_std_afu.json
  ${PROJECT_BINARY_DIR}/samples/intg_xeon_nlb/ccip_std_afu.json)
install(FILES ${PROJECT_BINARY_DIR}/samples/intg_xeon_nlb/sources.txt
  DESTINATION ${ASE_SAMPLES}/intg_xeon_nlb
  COMPONENT aseextra)
install(FILES ${PROJECT_BINARY_DIR}/samples/intg_xeon_nlb/ccip_std_afu.json
  DESTINATION ${ASE_SAMPLES}/intg_xeon_nlb
  COMPONENT aseextra)
file(COPY ${PROJECT_SOURCE_DIR}/samples/intg_xeon_nlb/src
  DESTINATION ${PROJECT_BINARY_DIR}/samples/intg_xeon_nlb)
install(DIRECTORY ${PROJECT_BINARY_DIR}/samples/intg_xeon_nlb/src
  DESTINATION ${ASE_SAMPLES}/intg_xeon_nlb
  COMPONENT aseextra)

# Post-installation Development scripts
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/compiler_config.cmake
  DESTINATION ${OPAE_SHARE_DIR}/cmake/modules
  COMPONENT asescripts)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/libraries_config.cmake
  DESTINATION ${OPAE_SHARE_DIR}/cmake/modules
  COMPONENT asescripts)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/Findjson-c.cmake
  DESTINATION ${OPAE_SHARE_DIR}/cmake/modules
  COMPONENT asescripts)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/FindRT.cmake
  DESTINATION ${OPAE_SHARE_DIR}/cmake/modules
  COMPONENT asescripts)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/FindUUID.cmake
  DESTINATION ${OPAE_SHARE_DIR}/cmake/modules
  COMPONENT asescripts)

install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/FindOPAE.cmake
  DESTINATION ${OPAE_SHARE_DIR}/cmake/modules
  COMPONENT asescripts)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/FindQuesta.cmake
  DESTINATION ${OPAE_SHARE_DIR}/cmake/modules
  COMPONENT asescripts)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/FindQuartus.cmake
  DESTINATION ${OPAE_SHARE_DIR}/cmake/modules
  COMPONENT asescripts)

# Extra platform scripts
set(EXTRA_ASE_FILES
  Makefile
  ase.cfg
  ase_regress.sh)

install(FILES ${EXTRA_ASE_FILES}
  DESTINATION ${ASE_SHARE_DIR}
  COMPONENT aseextra)

## Some ASE scripts are installed in bin
set(PLATFORM_SCRIPTS
  afu_sim_setup)

foreach(SRC ${PLATFORM_SCRIPTS})
  ## Substitute CMake variables of the form @<var>@ in the source file
  ## and store the file in the build tree's bin directory.
  configure_file(
    scripts/${SRC} ${PROJECT_BINARY_DIR}/bin/${SRC}
    @ONLY NEWLINE_STYLE UNIX)
  ## This forces the configure_file to be built for all targets
  add_custom_target(${SRC} ALL DEPENDS ${PROJECT_BINARY_DIR}/bin/${SRC})

  install(PROGRAMS ${PROJECT_BINARY_DIR}/bin/${SRC}
    DESTINATION bin
    COMPONENT asescripts)
endforeach(SRC)
