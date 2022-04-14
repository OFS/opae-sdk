## Copyright(c) 2017-2022, Intel Corporation
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

cmake_minimum_required(VERSION 3.10)
project(opae)

set(OPAE_SDK_SOURCE ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Root directory of opae-sdk project" FORCE)
set(OPAE_LIB_SOURCE ${OPAE_SDK_SOURCE}/libraries CACHE PATH "Library source directory" FORCE)
set(OPAE_BIN_SOURCE ${OPAE_SDK_SOURCE}/binaries CACHE PATH "Binary source directory" FORCE)
set(OPAE_INCLUDE_PATH ${OPAE_SDK_SOURCE}/include CACHE PATH "Include directory for opae-sdk")
set(opae-test_ROOT ${OPAE_SDK_SOURCE}/tests CACHE PATH "Test directory for opae-sdk")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${OPAE_SDK_SOURCE}/cmake/modules")
include(OPAE)

############################################################################
## Whether to use external projects ########################################
############################################################################
option(OPAE_WITH_CLI11 "Enable use of CLI11" ON)
mark_as_advanced(OPAE_WITH_CLI11)

option(OPAE_WITH_SPDLOG "Enable use of spdlog" ON)
mark_as_advanced(OPAE_WITH_SPDLOG)

option(OPAE_WITH_LIBEDIT "Enable use of libedit" ON)
mark_as_advanced(OPAE_WITH_LIBEDIT)

option(OPAE_WITH_HWLOC "Enable use of hwloc" ON)
mark_as_advanced(OPAE_WITH_HWLOC)

option(OPAE_WITH_TBB "Enable use of tbb" ON)
mark_as_advanced(OPAE_WITH_TBB)

option(OPAE_MINIMAL_BUILD "Enable minimal build" OFF)
mark_as_advanced(OPAE_MINIMAL_BUILD)

############################################################################
## Whether to build opae-legacy ############################################
############################################################################
option(OPAE_BUILD_LEGACY "Enable building of OPAE legacy tools" OFF)
mark_as_advanced(OPAE_BUILD_LEGACY)

if(OPAE_BUILD_LEGACY)
    set(OPAE_LEGACY_TAG "master" CACHE STRING "Desired branch for opae-legacy")
    mark_as_advanced(OPAE_LEGACY_TAG)
endif(OPAE_BUILD_LEGACY)

if (OPAE_MINIMAL_BUILD)
    set(OPAE_WITH_CLI11 OFF CACHE BOOL "Enable use of CLI11" FORCE)
    set(OPAE_WITH_SPDLOG OFF CACHE BOOL "Enable use of spdlog" FORCE)
    set(OPAE_WITH_LIBEDIT OFF CACHE BOOL "Enable use of libedit" FORCE)
    set(OPAE_WITH_HWLOC OFF CACHE BOOL "Enable use of hwloc" FORCE)
    set(OPAE_WITH_TBB OFF CACHE BOOL "Enable use of tbb" FORCE)
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE STRING "" FORCE)
endif(OPAE_MINIMAL_BUILD)

############################################################################
## Other setup and dependencies ############################################
############################################################################
find_package(Doxygen)
find_package(Sphinx)

if(CPACK_GENERATOR)
  set(CMAKE_INSTALL_PREFIX "/usr")
endif()

############################################################################
## Sub-projects ############################################################
############################################################################

set(GTEST_VERSION "1.11.0" CACHE STRING "googletest version in use" FORCE)

# begin opae-libs

set(OPAE_VERSION_LOCAL   "" CACHE STRING "OPAE local version")
set(OPAE_VERSION_MAJOR    2 CACHE STRING "OPAE major version" FORCE)
set(OPAE_VERSION_MINOR    1 CACHE STRING "OPAE minor version" FORCE)
set(OPAE_VERSION_REVISION 0${OPAE_VERSION_LOCAL} CACHE STRING "OPAE revision version" FORCE)
set(OPAE_VERSION ${OPAE_VERSION_MAJOR}.${OPAE_VERSION_MINOR}.${OPAE_VERSION_REVISION}
    CACHE STRING "OPAE version" FORCE)

set(SYSFS_PATH_MAX 256 CACHE STRING "Maximum size of sysfs paths")
set(DEV_PATH_MAX 256 CACHE STRING "Maximum size of device paths")

############################################################################
## Python Interpreter/Build Env  ###########################################
############################################################################
set(OPAE_PYTHON_VERSION 3 CACHE STRING "Python version to use for building/distributing pyopae")
set_property(CACHE OPAE_PYTHON_VERSION PROPERTY STRINGS 2.7 3 3.6 3.7 3.8 3.9)
find_package(PythonInterp ${OPAE_PYTHON_VERSION})
find_package(PythonLibs ${OPAE_PYTHON_VERSION})

option(OPAE_BUILD_TESTS "Enable building of OPAE unit tests" OFF)
mark_as_advanced(OPAE_BUILD_TESTS)

include(OPAE)
include(OFS)

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if ("${LIB64}" STREQUAL "TRUE")
    set(OPAE_LIB_INSTALL_DIR "lib64" CACHE INTERNAL "arch specific library")
else()
    set(OPAE_LIB_INSTALL_DIR "lib" CACHE INTERNAL "arch specific library")
endif()

############################################################################
## RPATH Handling ##########################################################
############################################################################
option(OPAE_INSTALL_RPATH "Enable to create RPATH during install" OFF)
mark_as_advanced(OPAE_INSTALL_RPATH)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
if(OPAE_INSTALL_RPATH)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${OPAE_LIB_INSTALL_DIR}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

############################################################################
## Place all executables and libraries under the same directories ##########
############################################################################
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH "Exe Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE PATH "Lib Build directory" FORCE)

############################################################################
## Conditional Build Steps and Features ####################################
############################################################################
option(OPAE_BUILD_LIBOPAE_CXX "Enable building of C++ bindings around libopae-c." ON)
mark_as_advanced(OPAE_BUILD_LIBOPAE_CXX)

option(OPAE_WITH_PYBIND11 "Enable use of pybind11." ON)
mark_as_advanced(OPAE_WITH_PYBIND11)

option(OPAE_BUILD_PYTHON_DIST "Enable building of Python source and binary distribution" OFF)
mark_as_advanced(OPAE_BUILD_PYTHON_DIST)

option(OPAE_ENABLE_MOCK "Enable building of test infrastructure with mock" OFF)
mark_as_advanced(OPAE_ENABLE_MOCK)

option(OPAE_BUILD_LIBOPAEVFIO "Enable building of the opaevfio library" ON)
mark_as_advanced(OPAE_BUILD_LIBOPAEVFIO)

option(OPAE_BUILD_PLUGIN_VFIO "Enable building of the vfio plugin module" ON)
mark_as_advanced(OPAE_BUILD_PLUGIN_VFIO)

option(OPAE_BUILD_LIBOPAEUIO "Enable building of the opaeuio library" ON)
mark_as_advanced(OPAE_BUILD_LIBOPAEUIO)

option(OPAE_PRESERVE_REPOS "Disable refresh of external repos" OFF)
mark_as_advanced(OPAE_PRESERVE_REPOS)

option(OPAE_BUILD_LIBOFS "Enable building of the libofs library and framework" ON)
mark_as_advanced(OPAE_BUILD_LIBOFS)

if(OPAE_BUILD_TESTS)
    set(OPAE_TEST_TAG "master" CACHE STRING "Desired branch for opae-test")
    mark_as_advanced(OPAE_TEST_TAG)
endif(OPAE_BUILD_TESTS)

if (OPAE_BUILD_PLUGIN_VFIO AND NOT OPAE_BUILD_LIBOPAEVFIO)
    message(STATUS
        "Enabling vfio plugin automatically enables libopaevfio")
    set(OPAE_BUILD_LIBOPAEVFIO ON)
endif (OPAE_BUILD_PLUGIN_VFIO AND NOT OPAE_BUILD_LIBOPAEVFIO)

if (OPAE_BUILD_LIBOPAEVFIO)
    try_compile(VFIO_CHECK
        ${CMAKE_BINARY_DIR}
	${OPAE_LIB_SOURCE}/libopaevfio/vfiocheck.c
        OUTPUT_VARIABLE TRY_COMPILE_OUTPUT
    )

    if (VFIO_CHECK)
        set(PLATFORM_SUPPORTS_VFIO TRUE CACHE BOOL "Platform supports vfio driver" FORCE)
        message(STATUS
            "Compatible VFIO headers found")
    else(VFIO_CHECK)
        set(PLATFORM_SUPPORTS_VFIO FALSE CACHE BOOL "Platform supports vfio driver" FORCE)
        message(WARNING
            "Could not compile VFIO.
            This most likely means that kernel( version >= 4.6) headers aren't installed.
            See errors in platform_vfio_errors.txt")
        file(WRITE ${CMAKE_BINARY_DIR}/platform_vfio_errors.txt ${TRY_COMPILE_OUTPUT})
    endif(VFIO_CHECK)
endif (OPAE_BUILD_LIBOPAEVFIO)

if (NOT DEFINED OPAE_MINIMAL_BUILD)
    option(OPAE_MINIMAL_BUILD "Enable minimal build" OFF)
    mark_as_advanced(OPAE_MINIMAL_BUILD)
endif(NOT DEFINED OPAE_MINIMAL_BUILD)

if (OPAE_MINIMAL_BUILD)
    set(OPAE_WITH_PYBIND11 OFF CACHE BOOL "Enable use of pybind11" FORCE)
    set(OPAE_PRESERVE_REPOS ON CACHE BOOL "Disable refresh of external repos" FORCE)
endif(OPAE_MINIMAL_BUILD)

install(DIRECTORY include/opae
    DESTINATION include
    COMPONENT libopaeheaders)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/config/config.h.in"
               "${CMAKE_BINARY_DIR}/include/config.h")

# end opae-libs

install(DIRECTORY cmake/modules
    DESTINATION src/opae/cmake)

opae_add_subdirectory(external)
opae_add_subdirectory(libraries)
opae_add_subdirectory(platforms)
opae_add_subdirectory(binaries)
opae_add_subdirectory(python)
if (OPAE_BUILD_LIBOFS)
    opae_add_subdirectory(ofs)
endif (OPAE_BUILD_LIBOFS)

option(OPAE_BUILD_SAMPLES "Enable building of OPAE samples" ON)
mark_as_advanced(OPAE_BUILD_SAMPLES)
if(OPAE_BUILD_SAMPLES)
    opae_add_subdirectory(samples)
endif()

if(OPAE_BUILD_TESTS)
    enable_testing()
    opae_add_subdirectory(tests)
endif()

############################################################################
## Add 'documentation' target ##############################################
############################################################################
option(OPAE_BUILD_SPHINX_DOC "Enable building of Sphinx documentation." OFF)
mark_as_advanced(OPAE_BUILD_SPHINX_DOC)

if(DOXYGEN_FOUND)
    add_subdirectory(doc)
else(DOXYGEN_FOUND)
    message(STATUS "Documentation cannot be built without doxygen")
endif(DOXYGEN_FOUND)

############################################################################
## Packaging ###############################################################
############################################################################

option(HASH_ARCHIVES "Add git commit hash to archive names" OFF)
mark_as_advanced(HASH_ARCHIVES)
set(CPACK_RPM_RELOCATION_PATHS "/etc")

# use to set doc dir for installing license file
include(GNUInstallDirs)

set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Open Programmable Acceleration Engine")
SET(CPACK_PACKAGE_VENDOR "Intel Corporation")
set(CPACK_PACKAGE_VERSION_MAJOR "${OPAE_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${OPAE_VERSION_MINOR}")
if(HASH_ARCHIVES)
  set(CPACK_PACKAGE_VERSION_PATCH "${OPAE_VERSION_REVISION}.g${OPAE_GIT_COMMIT_HASH}")
else()
  set(CPACK_PACKAGE_VERSION_PATCH "${OPAE_VERSION_REVISION}")
endif()
set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "opae@lists.01.org")
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

if("${CPACK_GENERATOR}" STREQUAL "DEB")
    #include(packaging)

# install LICENSE file to copyright
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DOCDIR}
COMPONENT licensefile
RENAME copyright)

# list of components to be included in the package
set(CPACK_COMPONENTS_ALL
	toolfpgaconf
	toolargsfilter
	toolfpgainfo
	toolfpgametrics
	samplehssi
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
	opaeclib
	opaecxxcorelib
	afutest
	opaetoolslibs
	vfiolib
	vfiotest
	uiolib
	uiotest
	memlib
	memtest
	opaecxxutils
	opaecxxlib
	opaecxxnlb
	opaeboardlib
	toolfpgaconf
	toolbist_app
	toolbist
	testsopae
	vabtool
	)

# move component part of file name from *after* version to *before* version
foreach(comp IN ITEMS tools tools-extra tests libs devel ase)
  string(TOUPPER ${comp} COMP)
  set(CPACK_DEBIAN_${COMP}_FILE_NAME ${CPACK_PACKAGE_NAME}-${comp}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}.deb)
endforeach()

# Component definition
define_pkg(tools
  COMPONENTS
  toolfpgaconf
  opaeboardlib
  vfiolib
  vfiotest
  uiolib
  uiotest
  memlib
  memtest
  toolargsfilter
  toolfpgainfo
  toolfpgametrics
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
  opaecxxlib
  opaecxxnlb
  vabtool
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
  samplehssi
  licensefile
  GROUP "devel"
  DISPLAY_NAME "opae-devel"
  DESCRIPTION "OPAE headers, sample source, and documentation"
  DEB_DEPENDS "uuid-dev , ${JSON_C_DEBIAN_PACKAGE} , opae-libs"
  )

  define_pkg(libs
  COMPONENTS
  opaeclib
  opaecxxcorelib
  opaetoolslibs
  afutest
  licensefile
  GROUP "libs"
  DISPLAY_NAME "opae-libs"
  DESCRIPTION "OPAE runtime"
  DEB_DEPENDS "uuid-dev , ${JSON_C_DEBIAN_PACKAGE}"
  )

  define_pkg(tests
  COMPONENTS
  testsopae
  GROUP "tests"
  DISPLAY_NAME "opae-tests"
  DESCRIPTION "OPAE test package"
  DEB_DEPENDS "opae-libs"
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
set(CPACK_SOURCE_PACKAGE_FILE_NAME
  "${CMAKE_PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}")

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

#Target to generate rpm package
add_custom_target(package_rpm
  COMMAND ${CMAKE_MAKE_PROGRAM}
  COMMAND ${CMAKE_CPACK_COMMAND}
)

#Target to generate DEB package
add_custom_target(package_deb
  COMMAND ${CMAKE_MAKE_PROGRAM}
  COMMAND ${CMAKE_CPACK_COMMAND}
)

include(CPack)

install(EXPORT opae-targets
    DESTINATION lib/opae-${OPAE_VERSION})
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/pkg/opae-config.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/pkg/opae-config.cmake @ONLY)

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/pkg/opae-config-version.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/pkg/opae-config-version.cmake @ONLY)

install(FILES
    ${CMAKE_BINARY_DIR}/cmake/pkg/opae-config.cmake
    ${CMAKE_BINARY_DIR}/cmake/pkg/opae-config-version.cmake
    DESTINATION lib/opae-${OPAE_VERSION})

