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

cmake_minimum_required(VERSION 2.8.11)
project(opae)

############################################################################
## Add 'versioning' library ################################################
############################################################################

set(INTEL_FPGA_API_VER_MAJOR 0 CACHE STRING "Intel FPGA API major version")
set(INTEL_FPGA_API_VER_MINOR 9 CACHE STRING "Intel FPGA API minor version")
set(INTEL_FPGA_API_VER_REV   0 CACHE STRING "Intel FPGA API revision version")
set(INTEL_FPGA_API_VERSION   ${INTEL_FPGA_API_VER_MAJOR}.${INTEL_FPGA_API_VER_MINOR}.${INTEL_FPGA_API_VER_REV})

############################################################################
## Compilation configuration ###############################################
############################################################################

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

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

set(COMMON_DIR        ${CMAKE_SOURCE_DIR}/common)
set(OPAE_INCLUDE_DIR  ${COMMON_DIR}/include)
add_custom_target(copy-common-opae-header-files ALL
  COMMAND cmake -E copy_directory ${OPAE_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/include)

# Install common header files
install(DIRECTORY common/include/opae
  DESTINATION include
  COMPONENT libopaeheaders)

############################################################################
## Sub-projects ############################################################
############################################################################

add_subdirectory(safe_string)

option(BUILD_LIBOPAE_C "Enable building of libopae-c. This is the default OPAE API implementation." ON)
mark_as_advanced(BUILD_LIBOPAE_C)
if(BUILD_LIBOPAE_C)
  add_subdirectory(libopae)
endif()

add_subdirectory(tools/userclk)
add_subdirectory(tools/fpgad)
add_subdirectory(tools/mmlink)
add_subdirectory(tools/fpgaconf)
add_subdirectory(tools/fpgainfo)
add_subdirectory(tools/c++utils)
add_subdirectory(tools/libopae++)
add_subdirectory(tools/fpgadiag)

option(BUILD_ASE "Enable ASE compilation" OFF)
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
## Add 'samples' ###################################################
############################################################################
add_subdirectory(samples)

############################################################################
## RPATH Handling ##########################################################
############################################################################
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


############################################################################
## Packaging ###############################################################
############################################################################

option(HASH_ARCHIVES "Add git commit hash to archive names" OFF)
mark_as_advanced(HASH_ARCHIVES)

find_program(GIT_EXECUTABLE git)
if(EXISTS ${GIT_EXECUTABLE})
   execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
     OUTPUT_VARIABLE GIT_COMMIT_HASH
     OUTPUT_STRIP_TRAILING_WHITESPACE)
else(EXISTS ${GIT_EXECUTABLE})
   set(GIT_COMMIT_HASH unknown)
endif(EXISTS ${GIT_EXECUTABLE})

set(CPACK_COMPONENT_OPAECLIB_GROUP         "libs")

set(CPACK_COMPONENT_LIBOPAEHEADERS_GROUP   "devel")
set(CPACK_COMPONENT_DOCHTML_GROUP          "devel")
set(CPACK_COMPONENT_DOCLATEX_GROUP         "devel")
set(CPACK_COMPONENT_DOCRTF_GROUP           "devel")
set(CPACK_COMPONENT_DOCMAN_GROUP           "devel")
set(CPACK_COMPONENT_DOCXML_GROUP           "devel")
set(CPACK_COMPONENT_SAMPLESRC_GROUP        "devel")

set(CPACK_COMPONENT_TOOLUSERCLK_GROUP      "tools")
set(CPACK_COMPONENT_TOOLFPGAD_GROUP        "tools")
set(CPACK_COMPONENT_TOOLRAS_GROUP          "tools")
set(CPACK_COMPONENT_TOOLMMLINK_GROUP       "tools")
set(CPACK_COMPONENT_TOOLCOREIDLE_GROUP     "tools")
set(CPACK_COMPONENT_TOOLFPGACONF_GROUP     "tools")
set(CPACK_COMPONENT_TOOLFPGADIAG_GROUP     "tools")
set(CPACK_COMPONENT_TOOLFPGADIAGAPPS_GROUP "tools")
set(CPACK_COMPONENT_TOOLFPGAINFO_GROUP     "tools")
set(CPACK_COMPONENT_OPAECXXUTILS_GROUP     "tools")
set(CPACK_COMPONENT_OPAECXXLIB_GROUP       "tools")
set(CPACK_COMPONENT_OPAECXXNLB_GROUP       "tools")

set(CPACK_COMPONENT_OPAECASE_GROUP         "ase")
set(CPACK_COMPONENT_ASERTL_GROUP           "ase")
set(CPACK_COMPONENT_ASESW_GROUP            "ase")
set(CPACK_COMPONENT_ASESCRIPTS_GROUP       "ase")
set(CPACK_COMPONENT_ASESAMPLECONFIG_GROUP  "ase")
set(CPACK_COMPONENT_ASEEXTRA_GROUP         "ase")

set(CPACK_COMPONENT_GROUP_LIBS_DESCRIPTION
	"libopae-c")
set(CPACK_COMPONENT_GROUP_DEVEL_DESCRIPTION
	"OPAE headers, sample source, and documentation")
set(CPACK_COMPONENT_GROUP_TOOLS_DESCRIPTION
	"OPAE tool binaries")
set(CPACK_COMPONENT_GROUP_ASE_DESCRIPTION
	"OPAE AFU Simulation Environment")
set(CPACK_COMPONENT_GROUP_ALL_DESCRIPTION
        "OPAE meta package")

set(CPACK_COMPONENT_GROUP_DEVEL_DEPENDS libs)
set(CPACK_COMPONENT_GROUP_TOOLS_DEPENDS libs)
set(CPACK_COMPONENT_GROUP_ASE_DEPENDS   devel)
set(CPACK_COMPONENT_GROUP_ALL_DEPENDS   devel tools ase)

set(CPACK_COMPONENTS_GROUPING ONE_PER_GROUP)
set(CPACK_COMPONENT_GROUPS_ALL libs devel tools ase all)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open Programmable Acceleration Engine")
SET(CPACK_PACKAGE_VENDOR "Intel Corporation")
set(CPACK_PACKAGE_VERSION_MAJOR "${INTEL_FPGA_API_VER_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${INTEL_FPGA_API_VER_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${INTEL_FPGA_API_VER_REV}")
set(CPACK_PACKAGE_RELEASE 1)

if (HASH_ARCHIVES)
   set(CPACK_RPM_LIBS_FILE_NAME "${CMAKE_PROJECT_NAME}-libs-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.x86_64_git${GIT_COMMIT_HASH}.rpm")
   set(CPACK_RPM_DEVEL_FILE_NAME "${CMAKE_PROJECT_NAME}-devel-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.x86_64_git${GIT_COMMIT_HASH}.rpm")
   set(CPACK_RPM_TOOLS_FILE_NAME "${CMAKE_PROJECT_NAME}-tools-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.x86_64_git${GIT_COMMIT_HASH}.rpm")
   set(CPACK_RPM_ASE_FILE_NAME "${CMAKE_PROJECT_NAME}-ase-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.x86_64_git${GIT_COMMIT_HASH}.rpm")
else(HASH_ARCHIVES)
   set(CPACK_RPM_LIBS_FILE_NAME "${CMAKE_PROJECT_NAME}-libs-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.x86_64.rpm")
   set(CPACK_RPM_DEVEL_FILE_NAME "${CMAKE_PROJECT_NAME}-devel-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.x86_64.rpm")
   set(CPACK_RPM_TOOLS_FILE_NAME "${CMAKE_PROJECT_NAME}-tools-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.x86_64.rpm")
   set(CPACK_RPM_ASE_FILE_NAME "${CMAKE_PROJECT_NAME}-ase-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.x86_64.rpm")
endif(HASH_ARCHIVES)

# Source code packaging target
set(CPACK_SOURCE_GENERATOR "TGZ")
if (HASH_ARCHIVES)
   set(CPACK_SOURCE_PACKAGE_FILE_NAME
     "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}_git${GIT_COMMIT_HASH}")
   set(DEFINE_RPM_NAME "%define _rpmfilename %%{ARCH}/%%{NAME}-%%{VERSION}-%%{RELEASE}_git${GIT_COMMIT_HASH}.%%{ARCH}.rpm")
else(HASH_ARCHIVES)
   set(CPACK_SOURCE_PACKAGE_FILE_NAME
     "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
   set(DEFINE_RPM_NAME "")
endif(HASH_ARCHIVES)

set(CPACK_SOURCE_IGNORE_FILES
  "coverage.cmake"
  "/mybuild/"
  "/build/"
  "/.git"
  "~$"
  ${CPACK_SOURCE_IGNORE_FILES})

# Binary packaging target
set(CPACK_PACKAGE_CONTACT "opae@lists.01.org")
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_PACKAGE_COMPONENT ON)
set(CPACK_PACKAGE_VERSION ${INTEL_FPGA_API_VERSION})
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_RPM_PACKAGE_REQUIRES "libuuid, libuuid-devel")

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
  "%ignore /usr/doc")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST
  "/"
  "/usr"
  "/usr/bin"
  "/usr/lib"
  "/usr/share"
  "/usr/include"
  "/usr/src"
  "/usr/doc")

include(CPack)
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

############################################################################
## Meta Package RPM ########################################################
############################################################################
set(PACKAGE_PACKAGER_NAME "The OPAE Development Team")
set(PACKAGE_PACKAGER_EMAIL ${CPACK_PACKAGE_CONTACT})
set(MODULE_NAME libopae-all)
set(PACKAGE_NAME opae-all)
set(PACKAGE_VERSION ${CPACK_PACKAGE_VERSION})
set(PACKAGE_SUMMARY "OPAE meta-package")
set(PACKAGE_DESCRIPTION "OPAE meta-package")
set(PACKAGE_VENDOR "Intel Corporation")
set(PACKAGE_URL http://www.intel.com)
set(PACKAGE_SRC_TAR_NAME ${CPACK_SOURCE_PACKAGE_FILE_NAME})

set(CONFIG_TEMPLATE_PATH "${CMAKE_SOURCE_DIR}/cmake/config")
configure_file(${CONFIG_TEMPLATE_PATH}/libopae-all.spec.in
  ${CMAKE_CURRENT_BINARY_DIR}/libopae-all.spec @ONLY)

find_program(RPMBUILD_EXECUTABLE rpmbuild)
if(EXISTS ${RPMBUILD_EXECUTABLE})
   add_custom_target(metarpm
                      COMMAND ${RPMBUILD_EXECUTABLE} -bb libopae-all.spec
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif(EXISTS ${RPMBUILD_EXECUTABLE})
