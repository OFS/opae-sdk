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

cmake_minimum_required(VERSION 2.8.12)
project(opae)

############################################################################
## Get git hash info, if available #########################################
############################################################################
find_program(GIT_EXECUTABLE git)
if(EXISTS ${GIT_EXECUTABLE})
  execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    RESULT_VARIABLE GIT_LOG_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT ${GIT_LOG_RESULT} EQUAL 0)
      set(GIT_COMMIT_HASH unknown)
    endif(NOT ${GIT_LOG_RESULT} EQUAL 0)
else(EXISTS ${GIT_EXECUTABLE})
  set(GIT_COMMIT_HASH unknown)
endif(EXISTS ${GIT_EXECUTABLE})

############################################################################
## Add 'versioning' library ################################################
############################################################################

set(INTEL_FPGA_API_VER_MAJOR 0  CACHE STRING "Intel FPGA API major version")
set(INTEL_FPGA_API_VER_MINOR 13 CACHE STRING "Intel FPGA API minor version")
set(INTEL_FPGA_API_VER_REV   0  CACHE STRING "Intel FPGA API revision version")
set(INTEL_FPGA_API_VERSION   ${INTEL_FPGA_API_VER_MAJOR}.${INTEL_FPGA_API_VER_MINOR}.${INTEL_FPGA_API_VER_REV})
set(INTEL_FPGA_API_HASH      ${GIT_COMMIT_HASH})

############################################################################
## Compilation configuration ###############################################
############################################################################
set(OPAE_SDK_SOURCE ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Root directory of opae-sdk project" FORCE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${OPAE_SDK_SOURCE}/cmake/modules")

include(compiler_config)
include(libraries_config)
include(fpga_functions)

############################################################################
## Target configuration ####################################################
############################################################################

# Place all executables and libraries under same directories
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH "Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE PATH "Build directory" FORCE)

############################################################################
## Compilation configuration ###############################################
############################################################################

set(COMMON_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/common)
set(OPAE_INCLUDE_DIR  ${COMMON_DIR}/include CACHE PATH "Path to include directory" FORCE)
add_custom_target(copy-common-opae-header-files ALL
  COMMAND cmake -E copy_directory ${OPAE_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/include)

# Install common header files
install(DIRECTORY common/include/opae
  DESTINATION include
  COMPONENT libopaeheaders)

install(DIRECTORY common/include/safe_string
  DESTINATION include
  COMPONENT safestrheaders)

############################################################################
## Add 'documentation' target ##############################################
############################################################################
option(BUILD_SPHINX_DOC "Enable building of Sphinx documentation." OFF)
mark_as_advanced(BUILD_SPHINX_DOC)

find_package(Doxygen)
if (DOXYGEN_FOUND)
  add_subdirectory(doc)
endif()

############################################################################
## Sub-projects ############################################################
############################################################################

add_subdirectory(safe_string)

option(BUILD_LIBOPAE_C "Enable building of libopae-c. This is the default OPAE API implementation." ON)
mark_as_advanced(BUILD_LIBOPAE_C)
if(BUILD_LIBOPAE_C)
  add_subdirectory(libopae)
endif()

add_subdirectory(platforms)
add_subdirectory(tools/userclk)
add_subdirectory(tools/fpgad)
add_subdirectory(tools/ras)
add_subdirectory(tools/mmlink)
add_subdirectory(tools/coreidle)
add_subdirectory(tools/fpgaconf)
add_subdirectory(tools/fpgabist)
add_subdirectory(tools/fpgaflash)
add_subdirectory(tools/fpgainfo)
add_subdirectory(tools/c++utils)
add_subdirectory(tools/libopae++)
add_subdirectory(tools/fpgadiag)
add_subdirectory(tools/fpgaport)
add_subdirectory(tools/hssi)
add_subdirectory(tools/packager)

option(BUILD_ASE "Enable ASE compilation" ON)
mark_as_advanced(BUILD_ASE)
if(BUILD_ASE)
  add_subdirectory(ase/api)
  add_subdirectory(ase)
endif()

if (NOT BUILD_ASE AND NOT BUILD_LIBOPAE_C)
  message(FATAL_ERROR "Not building any OPAE libraries")
endif()

find_package(Threads REQUIRED)

############################################################################
## Add 'samples' ###########################################################
############################################################################

add_subdirectory(samples)

############################################################################
## Add 'tests' #############################################################
############################################################################

option(BUILD_TESTS "Enable tests compilation" OFF)
mark_as_advanced(BUILD_TESTS)

if(BUILD_TESTS)
  enable_testing()
  include(CTest)
  add_subdirectory(tests)
endif()

############################################################################
## RPATH Handling ##########################################################
############################################################################

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS) 
if ("${LIB64}" STREQUAL "TRUE")
      set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib64")
else()
      set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif() 
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
############################################################################
## libs install location ###################################################
############################################################################

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS) 
if ("${LIB64}" STREQUAL "TRUE")
    set(LIB_INSTALL_DIR "lib64")
else()
    set(LIB_INSTALL_DIR "lib")
endif() 


############################################################################
## Packaging ###############################################################
############################################################################

option(HASH_ARCHIVES "Add git commit hash to archive names" OFF)
mark_as_advanced(HASH_ARCHIVES)

set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open Programmable Acceleration Engine")
SET(CPACK_PACKAGE_VENDOR "Intel Corporation")
set(CPACK_PACKAGE_VERSION_MAJOR "${INTEL_FPGA_API_VER_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${INTEL_FPGA_API_VER_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${INTEL_FPGA_API_VER_REV}")
set(CPACK_PACKAGE_VERSION ${INTEL_FPGA_API_VERSION})
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_RPM_PACKAGE_DESCRIPTION "This package contains the Open Programmable Acceleration Engine (OPAE) components ")

#Hashing the package components
 if(HASH_ARCHIVES)
  set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}")
else()
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
endif()

set(CPACK_META_GROUP_NAME "meta")
set(CPACK_LIBS_GROUP_NAME "libs")

# Binary packaging target
set(CPACK_PACKAGE_CONTACT "opae@lists.01.org")
set(CPACK_RPM_COMPONENT_INSTALL OFF)
set(CPACK_RPM_PACKAGE_COMPONENT OFF)
set(CPACK_RPM_PACKAGE_RELEASE ${CPACK_PACKAGE_RELEASE})
set(CPACK_RPM_PACKAGE_LICENSE "BSD 3.0")
set(CPACK_TEMPORARY_PACKAGE_FILE_NAME "${CMAKE_BINARY_DIR}/_CPack_Packages/Linux/RPM/RPMS/x86_64/${CPACK_PACKAGE_FILE_NAME}.rpm")


# /usr, /usr/lib are already present in CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST,
# but some Linux distributions complain without this explicit suppression
set(CPACK_RPM_SPEC_MORE_DEFINE "%define ignore \#")
set(CPACK_RPM_USER_FILELIST
  "%ignore /"
  "%ignore /usr"
  "%ignore /usr/bin"
  "%ignore /usr/lib"
  "%ignore /usr/share"
  "%ignore /usr/include"
  "%ignore /usr/src"
  "%ignore /usr/doc"
  "%ignore /usr/lib64")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST
  "/"
  "/usr"
  "/usr/bin"
  "/usr/lib"
  "/usr/share"
  "/usr/include"
  "/usr/src"
  "/usr/doc"
  "/usr/lib64")


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/opae.spec.in" "${CMAKE_CURRENT_BINARY_DIR}/opae-try.spec" @ONLY)
set(CPACK_RPM_USER_BINARY_SPECFILE "${CMAKE_CURRENT_BINARY_DIR}/opae-try.spec")

# Source code packaging target
set(CPACK_SOURCE_GENERATOR "TGZ")
if (HASH_ARCHIVES)
  set(CPACK_SOURCE_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}_git${GIT_COMMIT_HASH}")
  set(DEFINE_RPM_NAME "%define _rpmfilename %%{ARCH}/%%{NAME}-%%{VERSION}-%%{RELEASE}_git${GIT_COMMIT_HASH}.%%{ARCH}.rpm")
else()
  set(CPACK_SOURCE_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}")
  set(DEFINE_RPM_NAME "")
endif()

# Ignore following files in the final package
set(CPACK_SOURCE_IGNORE_FILES
  "coverage.cmake"
  "/mybuild/"
  "/build/"
  "/.git"
  "~$"
  ${CPACK_SOURCE_IGNORE_FILES})

include(CPack)

add_custom_target(package_rpm
  COMMAND ${CMAKE_MAKE_PROGRAM}
  COMMAND ${CMAKE_CPACK_COMMAND}
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/_CPack_Packages/Linux/RPM/RPMS/x86_64 ${CMAKE_CURRENT_BINARY_DIR}
  )

add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)
