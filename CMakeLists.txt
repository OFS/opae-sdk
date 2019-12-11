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

  execute_process(COMMAND ${GIT_EXECUTABLE} diff --stat
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DIFF_OUTPUT
    RESULT_VARIABLE GIT_DIFF_RESULT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT ${GIT_DIFF_RESULT} EQUAL 0)
    set(INTEL_FPGA_TREE_DIRTY 0)
  else(NOT ${GIT_DIFF_RESULT} EQUAL 0)
    if(GIT_DIFF_OUTPUT)
      set(INTEL_FPGA_TREE_DIRTY 1)
    else(GIT_DIFF_OUTPUT)
      set(INTEL_FPGA_TREE_DIRTY 0)
    endif(GIT_DIFF_OUTPUT)
  endif(NOT ${GIT_DIFF_RESULT} EQUAL 0)
else(EXISTS ${GIT_EXECUTABLE})
  set(GIT_COMMIT_HASH unknown)
  set(INTEL_FPGA_TREE_DIRTY 0)
endif(EXISTS ${GIT_EXECUTABLE})

############################################################################
## Add 'versioning' library ################################################
############################################################################

set(INTEL_FPGA_API_VER_MAJOR 1  CACHE STRING "Intel FPGA API major version")
set(INTEL_FPGA_API_VER_MINOR 4  CACHE STRING "Intel FPGA API minor version")
set(INTEL_FPGA_API_VER_REV   0  CACHE STRING "Intel FPGA API revision version")
set(INTEL_FPGA_API_VERSION   ${INTEL_FPGA_API_VER_MAJOR}.${INTEL_FPGA_API_VER_MINOR}.${INTEL_FPGA_API_VER_REV})
set(INTEL_FPGA_API_HASH      ${GIT_COMMIT_HASH})

############################################################################
## Compilation configuration ###############################################
############################################################################
set(OPAE_SDK_SOURCE ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Root directory of opae-sdk project" FORCE)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${OPAE_SDK_SOURCE}/cmake/modules")

############################################################################
## Find Python by version     ##############################################
############################################################################
set(OPAE_PYTHON_VERSION 2.7 CACHE STRING "Python version to use for building/distributing pyopae")
set_property(CACHE OPAE_PYTHON_VERSION PROPERTY STRINGS 2.7 3.6 3.5 3.4 3.3)
find_package(PythonInterp ${OPAE_PYTHON_VERSION})
find_package(PythonLibs ${OPAE_PYTHON_VERSION})

############################################################################
## Other setup and dependencies ############################################
############################################################################
include(compiler_config)
include(libraries_config)
include(fpga_functions)
find_package(Doxygen)
find_package(Threads REQUIRED)
find_package(Sphinx)
include(dependency_notifier)

############################################################################
## set libs install location ###############################################
############################################################################

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS) 	
if ("${LIB64}" STREQUAL "TRUE")
    set_cached_variable(OPAE_LIB_INSTALL_DIR "lib64")
else()
    set_cached_variable(OPAE_LIB_INSTALL_DIR "lib")
endif()

if(CPACK_GENERATOR)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()

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

if (DOXYGEN_FOUND)
  add_subdirectory(doc)
endif()


############################################################################
## Sub-projects ############################################################
############################################################################

add_subdirectory(safe_string)
add_subdirectory(libbitstream)

option(BUILD_LIBOPAE_C "Enable building of libopae-c. This is the default OPAE API implementation." ON)
mark_as_advanced(BUILD_LIBOPAE_C)
if(BUILD_LIBOPAE_C)
  add_subdirectory(libopae)
endif()

option(BUILD_LIBOPAE_CXX "Enable building of C++ bindings around libopae-c." ON)
mark_as_advanced(BUILD_LIBOPAE_CXX)
if(BUILD_LIBOPAE_CXX)
  add_subdirectory(libopaecxx)
endif()

option(BUILD_LIBOPAE_PY "Enable building of Python bindings around libopae-cxx-core." ON)
mark_as_advanced(BUILD_LIBOPAE_PY)
if(BUILD_LIBOPAE_PY AND BUILD_LIBOPAE_CXX)
  add_subdirectory(pyopae)
endif()

option(BUILD_PYTHON_DIST "Enable building of Python source and binary distribution" OFF)
mark_as_advanced(BUILD_PYTHON_DIST)

add_subdirectory(platforms)
add_subdirectory(tools)

option(BUILD_ASE "Enable ASE compilation" ON)
mark_as_advanced(BUILD_ASE)

if (NOT BUILD_ASE AND NOT BUILD_LIBOPAE_C)
  message(FATAL_ERROR "Not building any OPAE libraries")
endif()


############################################################################
## Add 'samples' ###########################################################
############################################################################

add_subdirectory(samples)

############################################################################
## Add 'tests' #############################################################
############################################################################

option(BUILD_TESTS "Enable tests compilation" OFF)
mark_as_advanced(BUILD_TESTS)

option(BUILD_ASE_TESTS "Enable ASE test compilation." OFF)
mark_as_advanced(BUILD_ASE_TESTS)

option(ENABLE_MOCK "Enable mock driver for testing." ON)
mark_as_advanced(ENABLE_MOCK)

if(BUILD_TESTS OR BUILD_ASE_TESTS)
  enable_testing()
  include(CTest)
  add_subdirectory(testing)
endif()

############################################################################
## RPATH Handling ##########################################################
############################################################################

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${OPAE_LIB_INSTALL_DIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

############################################################################
## Packaging ###############################################################
############################################################################

option(HASH_ARCHIVES "Add git commit hash to archive names" OFF)
mark_as_advanced(HASH_ARCHIVES)
#use to set doc dir for installing license file
include(GNUInstallDirs)

set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open Programmable Acceleration Engine")
SET(CPACK_PACKAGE_VENDOR "Intel Corporation")
set(CPACK_PACKAGE_VERSION_MAJOR "${INTEL_FPGA_API_VER_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${INTEL_FPGA_API_VER_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${INTEL_FPGA_API_VER_REV}")
set(CPACK_PACKAGE_VERSION ${INTEL_FPGA_API_VERSION})
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "opae@lists.01.org")
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
#Hashing the package components
 if(HASH_ARCHIVES)
  set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}")
else()
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
endif()

if("${CPACK_GENERATOR}" STREQUAL "DEB")
include(packaging)

#install LICENSE file to copyright
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DOCDIR}
COMPONENT licensefile
RENAME copyright)

#list of components to be included in the package
set(CPACK_COMPONENTS_ALL 
	toolfpgaconf
	toolfpgainfo 
	toolfpgaport
	toolfpgametrics
	toolfpgadiag 
	toolfpga_dma_test 
	toolfpgabist
	toolfpgaflash
	toolfpgadiagapps 
	toolpackager 
	tooluserclk 
	toolras 
	toolcoreidle 
	toolmmlink 
	jsonschema 
	opaecase 
	asertl 
	asescripts 
	aseextra 
	asesw 
	libopaeheaders 
	dochtml 
	doclatex 
	docrtf 
	docman 
	docxml 
	platform 
	samplesrc 
	safestrlib 
	safestrheaders 
	opaeclib 
	opaecxxcorelib 
	opaetoolslibs 
	opaecxxutils 
	opaecxxlib 
	opaecxxnlb 
	toolfpgaconf 
	toolbist_app 
	toolbist 
	toolfpgad
	toolfpgad_api
	toolfpgad_xfpga
	toolfpgad_vc
	)


# Component definition
define_pkg(tools
  COMPONENTS
  toolfpgad
  toolfpgad_api
  toolfpgad_xfpga
  toolfpgad_vc
  toolfpgaconf
  toolfpgainfo
  licensefile
  GROUP "tools"
  DISPLAY_NAME "opae-tools"
  DESCRIPTION "OPAE base tool binaries"
  DEB_DEPENDS "opae-libs , opae-devel"
  )

  define_pkg(tools-extra
  COMPONENTS
  tooluserclk
  toolras
  toolmmlink
  toolcoreidle
  toolfpgadiag
  toolfpgabist
  toolfpga_dma_test
  toolbist_app
  toolbist
  toolfpgaflash
  toolfpgadiagapps
  toolpachssiconfig
  opaecxxutils
  toolpackager
  jsonschema
  opaecxxlib
  opaecxxnlb
  hssiiolib
  hssiprograms
  licensefile
  GROUP "tools-extra"
  DISPLAY_NAME "opae-tools-extra"
  DESCRIPTION "OPAE extra tool binaries"
  DEB_DEPENDS "opae-libs , opae-devel"
  )

define_pkg(ase
  COMPONENTS
  opaecase
  asertl
  asescripts
  aseextra
  asesw
  licensefile
  GROUP "ase"
  DISPLAY_NAME "opae-ase"
  DESCRIPTION "OPAE AFU Simulation Environment"
  DEB_DEPENDS "opae-libs , opae-devel"
  )

define_pkg(devel
  COMPONENTS
  libopaeheaders
  dochtml
  doclatex
  docrtf
  docman
  docxml
  platform
  samplesrc
  samplebin
  safestrlib
  safestrheaders
  licensefile
  GROUP "devel"
  DISPLAY_NAME "opae-devel"
  DESCRIPTION "OPAE headers, sample source, and documentation"
  DEB_DEPENDS "uuid-dev , libjson0 , opae-libs"
  )

  define_pkg(libs
  COMPONENTS
  opaeclib
  opaecxxcorelib
  licensefile
  GROUP "libs"
  DISPLAY_NAME "opae-libs"
  DESCRIPTION "OPAE runtime"
  DEB_DEPENDS "uuid-dev , libjson0"
  )

#Binary DEB packaging
set(CPACK_COMPONENTS_GROUPING ONE_PER_GROUP)
#set(CPACK_COMPONENT_GROUPS_ALL libs devel tools ase all)
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_DEB_PACKAGE_COMPONENT ON)

# Run ldconfig after installation
option(RUN_LDCONFIG "Enable execution of ldconfig after installation" ON)
mark_as_advanced(RUN_LDCONFIG)

if (RUN_LDCONFIG)
  if (NOT CMAKE_INSTALL_LIBDIR)
    set(CMAKE_INSTALL_LIBDIR "lib")
  endif()

  set(LDCONFIG_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})

  file(WRITE ${PROJECT_BINARY_DIR}/scripts/postinst "
mkdir -p /etc/ld.so.conf.d
echo \"${LDCONFIG_DIR}\" > /etc/ld.so.conf.d/opae-c.conf
ldconfig
")

  file(WRITE ${PROJECT_BINARY_DIR}/scripts/prerm "
rm -f -- /etc/ld.so.conf.d/opae-c.conf
ldconfig
")

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${PROJECT_BINARY_DIR}/scripts/postinst;${PROJECT_BINARY_DIR}/scripts/prerm")
endif(RUN_LDCONFIG)
endif()

if("${CPACK_GENERATOR}" STREQUAL "RPM")
# Binary RPM packaging
set(CPACK_RPM_PACKAGE_LICENSE "BSD 3.0")
set(CPACK_RPM_COMPONENT_INSTALL OFF)
set(CPACK_RPM_PACKAGE_COMPONENT OFF)
set(CPACK_RPM_PACKAGE_RELEASE ${CPACK_PACKAGE_RELEASE})
set(CPACK_RPM_PACKAGE_LICENSE "BSD 3.0")
set(CPACK_TEMPORARY_PACKAGE_FILE_NAME "${CMAKE_BINARY_DIR}/_CPack_Packages/Linux/RPM/RPMS/x86_64/${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.rpm")
set(CPACK_META_GROUP_NAME "opae")
set(CPACK_RPM_PACKAGE_DESCRIPTION "This package contains the Open Programmable Acceleration Engine (OPAE) components ")
#install license file
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DOCDIR}
COMPONENT licensefile)
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


configure_file("${CMAKE_CURRENT_SOURCE_DIR}/opae.spec.in" "${CMAKE_CURRENT_BINARY_DIR}/opae.spec" @ONLY)
set(CPACK_RPM_USER_BINARY_SPECFILE "${CMAKE_CURRENT_BINARY_DIR}/opae.spec")
endif()


# Source code packaging target
if(CPACK_GENERATOR STREQUAL "TGZ")
set(CPACK_SOURCE_GENERATOR "TGZ")
if(HASH_ARCHIVES)
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
  "/mybuild/"
  "/build/"
  "/.git"
  "~$"
  ${CPACK_SOURCE_IGNORE_FILES})
endif()

#Target to generate tarball
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

if(HASH_ARCHIVES)
#Target to generate RPM package
	add_custom_target(package_rpm
		  COMMAND ${CMAKE_MAKE_PROGRAM}
		  COMMAND ${CMAKE_CPACK_COMMAND}
		  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/_CPack_Packages/Linux/RPM/RPMS/x86_64 ${CMAKE_CURRENT_BINARY_DIR}
		  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.rpm ${CMAKE_CURRENT_BINARY_DIR}/opae-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}.rpm
      COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-libs-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.rpm ${CMAKE_CURRENT_BINARY_DIR}/opae-libs-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}.rpm
		  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-devel-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.rpm ${CMAKE_CURRENT_BINARY_DIR}/opae-devel-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}.rpm
		  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-tools-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.rpm ${CMAKE_CURRENT_BINARY_DIR}/opae-tools-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}.rpm
      COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-tools-extra-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.rpm ${CMAKE_CURRENT_BINARY_DIR}/opae-tools-extra-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}.rpm
      COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-ase-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.rpm ${CMAKE_CURRENT_BINARY_DIR}/opae-ase-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}.rpm
      )
else()
	add_custom_target(package_rpm
		  COMMAND ${CMAKE_MAKE_PROGRAM}
		  COMMAND ${CMAKE_CPACK_COMMAND}
		  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/_CPack_Packages/Linux/RPM/RPMS/x86_64 ${CMAKE_CURRENT_BINARY_DIR}
	  )
endif()

if(HASH_ARCHIVES)
#Target to generate DEB package
  add_custom_target(package_deb
  COMMAND ${CMAKE_MAKE_PROGRAM}
  COMMAND ${CMAKE_CPACK_COMMAND}
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}-libs.deb ${CMAKE_CURRENT_BINARY_DIR}/opae-libs-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}.deb
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}-devel.deb ${CMAKE_CURRENT_BINARY_DIR}/opae-devel-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}.deb
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}-tools.deb ${CMAKE_CURRENT_BINARY_DIR}/opae-tools-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}.deb
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}-tools-extra.deb ${CMAKE_CURRENT_BINARY_DIR}/opae-tools-extra-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}.deb
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}-ase.deb ${CMAKE_CURRENT_BINARY_DIR}/opae-ase-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-git${GIT_COMMIT_HASH}.deb

  )
else()
  add_custom_target(package_deb
  COMMAND ${CMAKE_MAKE_PROGRAM}
  COMMAND ${CMAKE_CPACK_COMMAND}
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-libs.deb ${CMAKE_CURRENT_BINARY_DIR}/opae-libs-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.deb
	COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-devel.deb ${CMAKE_CURRENT_BINARY_DIR}/opae-devel-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.deb
	COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-tools.deb ${CMAKE_CURRENT_BINARY_DIR}/opae-tools-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.deb
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-tools-extra.deb ${CMAKE_CURRENT_BINARY_DIR}/opae-tools-extra-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.deb
  COMMAND ${CMAKE_COMMAND} -E rename ${CMAKE_CURRENT_BINARY_DIR}/opae-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}-ase.deb ${CMAKE_CURRENT_BINARY_DIR}/opae-ase-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.deb
  )
endif()
include(CPack)

configure_file("${CMAKE_SOURCE_DIR}/cmake/config/config.h.in"
               "${PROJECT_BINARY_DIR}/include/config.h")
