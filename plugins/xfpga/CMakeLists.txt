## Copyright(c) 2017-2020, Intel Corporation
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

project(libxfpga)

set(SRC
  common.c
  enum.c
  error.c
  umsg.c
  reconf.c
  open.c
  close.c
  reset.c
  mmio.c
  buffer.c
  bitstream.c
  hostif.c
  event.c
  properties.c
  init.c
  opae_drv.c
  sysfs.c
  wsid_list.c
  token_list.c
  mmap.c
  version.c
  userclk.c
  usrclk/user_clk_pgm_uclock.c
  plugin.c
  sysobject.c
  manage.c
  metrics/metrics.c
  metrics/metrics_utils.c
  metrics/afu_metrics.c
  metrics/vector.c
  metrics/metrics_max10.c
  metrics/threshold.c)

opae_add_module_library(xfpga SRC)

target_link_libraries(xfpga m opae-c)

target_include_directories(xfpga PRIVATE ${OPAE_LIBS_ROOT}/libopae-c)

# Hide non-public symbols when building release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message("!! Building for release, may break internal tests")
  set_property(TARGET xfpga PROPERTY C_VISIBILITY_PRESET hidden)
endif(CMAKE_BUILD_TYPE STREQUAL "Release")

install(TARGETS xfpga
  LIBRARY DESTINATION ${OPAE_LIB_INSTALL_DIR}
  COMPONENT opaeclib)


############################################################################
## Add 'coverage' ##########################################################
############################################################################

#if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
#  if(BUILD_TESTS AND GTEST_FOUND)
#    set_target_for_coverage(xfpga bin/gtapi -p)
#    add_dependencies(coverage_xfpga gtapi)
#  endif(BUILD_TESTS AND GTEST_FOUND)
#endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")
