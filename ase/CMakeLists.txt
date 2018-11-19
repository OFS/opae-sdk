## Copyright(c) 2014-2018, Intel Corporation
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
include(opae_devpkgs_cmake_install)

############################################################################
## Create config_int.h #####################################################
############################################################################

configure_file ("${CMAKE_SOURCE_DIR}/cmake/config/config.h.in"
                "${PROJECT_BINARY_DIR}/include/config.h" )

############################################################################
## Set base directories ####################################################
############################################################################

# Set root ASE directories correctly now
set(OPAE_BASE_DIR      ${OPAE_SDK_SOURCE})
set(OPAE_SHARE_DIR     ${OPAE_SDK_SOURCE})
set(OPAE_SHARE_DIR     ${OPAE_SDK_SOURCE} PARENT_SCOPE) # do not remove, they are not the same
include(ase_variables_config)

# This is used in scripts to find the relative installed path to ASE files
set(ASE_INST_SHARE_DIR share/opae/ase)

############################################################################
## Install some directories ################################################
############################################################################

# Install some directories
install(DIRECTORY rtl          DESTINATION ${ASE_INST_SHARE_DIR} COMPONENT asertl)
install(DIRECTORY sw           DESTINATION ${ASE_INST_SHARE_DIR} COMPONENT asesw)
install(DIRECTORY scripts      DESTINATION ${ASE_INST_SHARE_DIR} USE_SOURCE_PERMISSIONS COMPONENT asescripts)

############################################################################
## Add sub-project (ASE client, ASE server Verilog code)       #############
## (Exclude ASE server C code, as it is compiled per each AFU) #############
############################################################################

# ASE client
add_subdirectory(api)

# ASE RTL code
add_subdirectory(rtl)

###########################################################################
## Extra platform scripts #################################################
###########################################################################

set(EXTRA_ASE_FILES
  Makefile
  ase.cfg
  ase_regress.sh)

install(FILES ${EXTRA_ASE_FILES}
  DESTINATION ${ASE_INST_SHARE_DIR}
  COMPONENT aseextra)

## Some ASE scripts are installed in bin
set(PLATFORM_SCRIPTS
  afu_sim_setup
  with_ase)

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
