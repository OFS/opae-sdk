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

# Post-installation Development scripts
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/cmake_useful.cmake
  DESTINATION src/opae/cmake/modules
  COMPONENT samplesrc)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/compiler_config.cmake
  DESTINATION src/opae/cmake/modules
  COMPONENT samplesrc)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/libraries_config.cmake
  DESTINATION src/opae/cmake/modules
  COMPONENT samplesrc)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/Findjson-c.cmake
  DESTINATION src/opae/cmake/modules
  COMPONENT samplesrc)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/FindRT.cmake
  DESTINATION src/opae/cmake/modules
  COMPONENT samplesrc)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/FindUUID.cmake
  DESTINATION src/opae/cmake/modules
  COMPONENT samplesrc)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/FindDBus.cmake
  DESTINATION src/opae/cmake/modules
  COMPONENT samplesrc)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/FindGLIB.cmake
  DESTINATION src/opae/cmake/modules
  COMPONENT samplesrc)

install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/FindOPAE.cmake
  DESTINATION src/opae/cmake/modules
  COMPONENT samplesrc)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/FindQuesta.cmake
  DESTINATION src/opae/cmake/modules
  COMPONENT samplesrc)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/FindQuartus.cmake
  DESTINATION src/opae/cmake/modules
  COMPONENT samplesrc)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/cmake/modules/FindVerilator.cmake
  DESTINATION src/opae/cmake/modules
  COMPONENT samplesrc)
