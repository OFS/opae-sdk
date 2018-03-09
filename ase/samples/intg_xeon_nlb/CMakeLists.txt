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

project("opae-c-ase-samples-nlb")

configure_file(${PROJECT_SOURCE_DIR}/sources.txt.in
  ${PROJECT_BINARY_DIR}/sources.txt)

configure_file(${PROJECT_SOURCE_DIR}/sources.txt.in
  ${PROJECT_BINARY_DIR}/to_install/sources.txt)
install(FILES ${PROJECT_BINARY_DIR}/to_install/sources.txt
  DESTINATION ${ASE_SAMPLES}/intg_xeon_nlb
  COMPONENT aseextra)

configure_file(${PROJECT_SOURCE_DIR}/ccip_std_afu.json
  ${PROJECT_BINARY_DIR}/ccip_std_afu.json)
install(FILES ${PROJECT_BINARY_DIR}/ccip_std_afu.json
  DESTINATION ${ASE_SAMPLES}/intg_xeon_nlb
  COMPONENT aseextra)

file(COPY ${PROJECT_SOURCE_DIR}/src
  DESTINATION ${PROJECT_BINARY_DIR})
install(DIRECTORY ${PROJECT_BINARY_DIR}/src
  DESTINATION ${ASE_SAMPLES}/intg_xeon_nlb
  COMPONENT aseextra)

set(intg_xeon_nlb_SOURCES
  sources.txt
  ${PROJECT_BINARY_DIR}/ccip_std_afu.json)
ase_add_modelsim_module(intg_xeon_nlb
  ${intg_xeon_nlb_SOURCES})
