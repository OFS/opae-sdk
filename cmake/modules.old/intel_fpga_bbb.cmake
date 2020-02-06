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
## POSSIBILITY OF SUCH DAMAGE

function (Build_Intel_FPGA_BBB)

  message(STATUS "Trying to fetch intel-fpga-bbb throught git")
  find_package(Git REQUIRED)

  # Enable ExternalProject CMake module
  include(ExternalProject)

  # Download Intel BBB repository
  set(GCOV_COMPILE_FLAGS "-g -O0 --coverage -fprofile-arcs -ftest-coverage")
  set(GCOV_LINK_FLAGS "-lgcov")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GCOV_COMPILE_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${GCOV_LINK_FLAGS}")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/modules/intel_fpga_bbb_include.cmake
    ${PROJECT_BINARY_DIR}/intel_fpga_bbb_include.cmake)
  if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    ExternalProject_Add(
      intel-fpga-bbb
      GIT_REPOSITORY "https://github.com/OPAE/intel-fpga-bbb"
      GIT_TAG "feature/opae_samples_2"
      UPDATE_COMMAND ""
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/intel-fpga-bbb
      CMAKE_ARGS -DCMAKE_POSITION_INDEPENDENT_CODE=ON
      -DCMAKE_PROJECT_mpf_INCLUDE=${PROJECT_BINARY_DIR}/intel_fpga_bbb_include.cmake
      -DCMAKE_C_FLAGS=${CMAKE_C_FLAGS} -I${CMAKE_BINARY_DIR}/include
      -DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}
      -DOPAELIB_INC_PATH=${CMAKE_SOURCE_DIR}/common/include
      # Disable install step
      INSTALL_COMMAND "")
  else()
    ExternalProject_Add(
      intel-fpga-bbb
      GIT_REPOSITORY "https://github.com/OPAE/intel-fpga-bbb"
      GIT_TAG "feature/opae_samples_2"
      UPDATE_COMMAND ""
      PREFIX ${CMAKE_CURRENT_BINARY_DIR}/intel-fpga-bbb
      CMAKE_ARGS -DCMAKE_POSITION_INDEPENDENT_CODE=ON
      -DCMAKE_PROJECT_mpf_INCLUDE=${PROJECT_BINARY_DIR}/intel_fpga_bbb_include.cmake
      -DOPAELIB_INC_PATH=${CMAKE_SOURCE_DIR}/common/include
      # Disable install step
      INSTALL_COMMAND "")
  endif()
  set (mpf_root "${CMAKE_CURRENT_BINARY_DIR}/intel-fpga-bbb/BBB_cci_mpf/sw")
  message(STATUS "MPF locatet at: ${mpf_root}")

  # Create a libMPF target to be used as a dependency by sample programs
  add_library(MPF SHARED IMPORTED GLOBAL)
  add_dependencies(MPF intel-fpga-bbb)

  # Get MPF source and binary directories from CMake project
  ExternalProject_Get_Property(intel-fpga-bbb source_dir binary_dir)

  # Set libMPF properties
  set_target_properties(MPF PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/BBB_cci_mpf/sw/libMPF.so"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")

  # Export MPF variables
  set(FPGA_BBB_CCI_SRC "${source_dir}" PARENT_SCOPE)
  set(MPF_ROOT ${mpf_root} PARENT_SCOPE)
  set(MPF_INCLUDE_DIRS ${source_dir}/BBB_cci_mpf/sw/include)
  set(MPF_INCLUDE_DIRS ${source_dir}/BBB_cci_mpf/sw/include PARENT_SCOPE)
  set(MPF_MAIN_LIBRARY   libMPF PARENT_SCOPE)
  set(MPF_BOTH_LIBRARIES libMPF PARENT_SCOPE)
  set(MPF_FOUND true PARENT_SCOPE)
  message(STATUS "MPF include dir: ${MPF_INCLUDE_DIRS}")

endfunction(Build_Intel_FPGA_BBB)
