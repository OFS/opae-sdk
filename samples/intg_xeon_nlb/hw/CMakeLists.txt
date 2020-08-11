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

######################################################################
# ASE module definition ##############################################
######################################################################

if(OPAE_BUILD_ASE_SAMPLES)
  include(ase_add_modelsim)
  set(intg_xeon_nlb_SOURCES
    sources.txt
    intg_xeon_nlb.json)
  ase_add_afu_module(intg_xeon_nlb
    ${intg_xeon_nlb_SOURCES})
  file(COPY ${PROJECT_SOURCE_DIR}/hw/rtl
    DESTINATION ${PROJECT_BINARY_DIR}/hw)

  # Set platform
  ase_module_set_platform_name(intg_xeon_nlb intg_xeon)

  # Add definitions
  ase_module_add_definitions(intg_xeon_nlb NUM_AFUS=1)
  ase_module_add_definitions(intg_xeon_nlb NLB400_MODE_0)
  ase_module_add_definitions(intg_xeon_nlb FPGA_PLATFORM_INTG_XEON)

  # Configure files and finalize target rules
  ase_module_finalize_modelsim_linking(intg_xeon_nlb)

endif()

######################################################################
# Installation files #################################################
######################################################################

install(FILES ${PROJECT_SOURCE_DIR}/hw/sources.txt
  DESTINATION ${OPAE_SAMPLES}/intg_xeon_nlb/hw
  COMPONENT samplesrc)

install(FILES ${PROJECT_SOURCE_DIR}/hw/intg_xeon_nlb.json
  DESTINATION ${OPAE_SAMPLES}/intg_xeon_nlb/hw
  COMPONENT samplesrc)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/hw/rtl
  DESTINATION ${OPAE_SAMPLES}/intg_xeon_nlb/hw
  COMPONENT samplesrc)
