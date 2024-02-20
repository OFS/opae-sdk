## Copyright(c) 2017-2023, Intel Corporation
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

cmake_minimum_required(VERSION 3.14)
project(opae)

set(OPAE_SDK_SOURCE ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Root directory of opae-sdk project" FORCE)
set(OPAE_LIB_SOURCE ${OPAE_SDK_SOURCE}/libraries CACHE PATH "Library source directory" FORCE)
set(OPAE_BIN_SOURCE ${OPAE_SDK_SOURCE}/binaries CACHE PATH "Binary source directory" FORCE)
set(OPAE_INCLUDE_PATH ${OPAE_SDK_SOURCE}/include CACHE PATH "Include directory for opae-sdk")
set(opae-test_ROOT ${OPAE_SDK_SOURCE}/tests CACHE PATH "Test directory for opae-sdk")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${OPAE_SDK_SOURCE}/cmake/modules")

############################################################################
## Place all executables and libraries under the same directories ##########
############################################################################
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin CACHE PATH "Exe Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib CACHE PATH "Lib Build directory" FORCE)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

############################################################################
## External projects #######################################################
############################################################################
option(OPAE_WITH_CLI11 "Enable use of CLI11" ON)
mark_as_advanced(OPAE_WITH_CLI11)

option(OPAE_WITH_SPDLOG "Enable use of spdlog" ON)
mark_as_advanced(OPAE_WITH_SPDLOG)

option(OPAE_WITH_LIBEDIT "Enable use of libedit" ON)
mark_as_advanced(OPAE_WITH_LIBEDIT)

option(OPAE_WITH_PYBIND11 "Enable use of pybind11" ON)
mark_as_advanced(OPAE_WITH_PYBIND11)

option(OPAE_WITH_HWLOC "Enable use of hwloc" ON)
mark_as_advanced(OPAE_WITH_HWLOC)

option(OPAE_WITH_TBB "Enable use of tbb" ON)
mark_as_advanced(OPAE_WITH_TBB)

option(OPAE_WITH_NUMA "Enable use of numa" ON)
mark_as_advanced(OPAE_WITH_NUMA)

option(OPAE_MINIMAL_BUILD "Enable minimal build" OFF)
mark_as_advanced(OPAE_MINIMAL_BUILD)

option(OPAE_BUILD_TESTS "Enable building of OPAE unit tests" OFF)
mark_as_advanced(OPAE_BUILD_TESTS)

############################################################################
## Python Interpreter/Build Env  ###########################################
############################################################################
set(OPAE_PYTHON_VERSION 3 CACHE STRING "Python version to use for building/distributing pyopae")
set_property(CACHE OPAE_PYTHON_VERSION PROPERTY STRINGS 3 3.6 3.7 3.8 3.9 3.10 3.11)
find_package(Python3 COMPONENTS Interpreter Development)

################################################################################
# util-linux/libuuid
################################################################################

set(UTIL_LINUX_URL
        https://github.com/util-linux/util-linux
        CACHE STRING "URL for util-linux")
set(UTIL_LINUX_VERSION
        2.38
        CACHE STRING "Version for util-linux")
set(UTIL_LINUX_TAG
        v${UTIL_LINUX_VERSION}
        CACHE STRING "Tag for util-linux")

if (NOT uuid_FOUND)
    find_package(uuid)
endif(NOT uuid_FOUND)

if (NOT uuid_FOUND)
    include(ExternalProject)

    message(WARNING "Building libuuid.so from source is experimental. "
                    "Be sure to install all prerequisite packages, eg bison. "
                    "To avoid this build path, please install "
                    "the util-linux package, which provides libuuid.so.")

    ExternalProject_Add(uuid
        GIT_REPOSITORY ${UTIL_LINUX_URL}
        GIT_TAG ${UTIL_LINUX_TAG}
        CONFIGURE_COMMAND ./autogen.sh && ./configure --disable-all-programs --enable-libuuid
        BUILD_COMMAND make libuuid.la
        INSTALL_COMMAND ""
        BUILD_IN_SOURCE ON
    )

    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/uuid-prefix/src/uuid/libuuid/uuid
        COMMAND
            ln -sf ${CMAKE_CURRENT_BINARY_DIR}/uuid-prefix/src/uuid/libuuid/src ${CMAKE_CURRENT_BINARY_DIR}/uuid-prefix/src/uuid/libuuid/uuid
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/uuid-prefix/src/uuid/libuuid/src
    )

    add_custom_target(uuid_symlink
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/uuid-prefix/src/uuid/libuuid/uuid
        COMMENT "Creating include symlink for uuid"
    )
    add_dependencies(uuid_symlink uuid)

    set(uuid_ROOT ${CMAKE_CURRENT_BINARY_DIR}/uuid-prefix/src/uuid/libuuid CACHE PATH "base dir for uuid" FORCE)

    set(uuid_INCLUDE_DIRS ${uuid_ROOT}
        CACHE STRING "Path to uuid" FORCE)
    set(uuid_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/uuid-prefix/src/uuid/.libs/libuuid.so
        CACHE STRING "Path to uuid libraries" FORCE)
    set(uuid_DEFINITIONS "" CACHE STRING "uuid built from source" FORCE)
    set(uuid_FOUND TRUE CACHE BOOL "uuid built from source" FORCE)

    add_library(uuid_IMPORT SHARED IMPORTED)
    target_include_directories(uuid_IMPORT
        INTERFACE
            ${uuid_INCLUDE_DIRS}
    )
    set_property(
        TARGET uuid_IMPORT
        PROPERTY
            IMPORTED_LOCATION "${uuid_LIBRARIES}"
    )
    add_dependencies(uuid_IMPORT uuid_symlink)
    set(uuid_IMPORTED TRUE CACHE BOOL "uuid imported from source" FORCE)

endif(NOT uuid_FOUND)

message(STATUS "uuid_ROOT ${uuid_ROOT}")
message(STATUS "uuid_INCLUDE_DIRS ${uuid_INCLUDE_DIRS}")
message(STATUS "uuid_LIBRARIES ${uuid_LIBRARIES}")
message(STATUS "uuid_DEFINITIONS ${uuid_DEFINITIONS}")

################################################################################
# json-c
################################################################################

set(JSON_C_URL
        https://github.com/json-c/json-c
        CACHE STRING "URL for json-c")
set(JSON_C_TAG
        json-c-0.16-20220414
        CACHE STRING "Tag for json-c")

FetchContent_Declare(json-c
    GIT_REPOSITORY ${JSON_C_URL}
    GIT_TAG ${JSON_C_TAG}
)

if (NOT json-c_FOUND)
    find_package(json-c)
endif(NOT json-c_FOUND)

if (NOT json-c_FOUND)
    set(json-c_ROOT ${FETCHCONTENT_BASE_DIR} CACHE PATH "base dir for json-c" FORCE)

    FetchContent_MakeAvailable(json-c)

    add_custom_command(
        OUTPUT
            ${FETCHCONTENT_BASE_DIR}/json-c-src/json.h
        COMMAND
            ln -sf ${FETCHCONTENT_BASE_DIR}/json-c-build/json.h ${FETCHCONTENT_BASE_DIR}/json-c-src/json.h
        DEPENDS
            ${FETCHCONTENT_BASE_DIR}/json-c-build/json.h
    )

    add_custom_target(json_h
        DEPENDS ${FETCHCONTENT_BASE_DIR}/json-c-src/json.h
        COMMENT "Creating json-c-src/json.h"
    )

    add_custom_command(
        OUTPUT
            ${FETCHCONTENT_BASE_DIR}/json-c
        COMMAND
            ln -sf ${FETCHCONTENT_BASE_DIR}/json-c-src ${FETCHCONTENT_BASE_DIR}/json-c
        DEPENDS
            ${FETCHCONTENT_BASE_DIR}/json-c-src
            ${FETCHCONTENT_BASE_DIR}/json-c-src/json.h
    )

    add_custom_target(json_c_symlink
        DEPENDS ${FETCHCONTENT_BASE_DIR}/json-c
        COMMENT "Creating symlink for json-c include dir"
    )

    add_custom_command(
        OUTPUT
            ${FETCHCONTENT_BASE_DIR}/json-c/json.h
            ${FETCHCONTENT_BASE_DIR}/json-c/json_config.h
        COMMAND
            ln -sf ${FETCHCONTENT_BASE_DIR}/json-c-build/json.h ${FETCHCONTENT_BASE_DIR}/json-c/json.h
        COMMAND
            ln -sf ${FETCHCONTENT_BASE_DIR}/json-c-build/json_config.h ${FETCHCONTENT_BASE_DIR}/json-c/json_config.h
        DEPENDS
            ${FETCHCONTENT_BASE_DIR}/json-c-build/json.h
            ${FETCHCONTENT_BASE_DIR}/json-c-build/json_config.h
            ${FETCHCONTENT_BASE_DIR}/json-c
    )

    add_custom_target(json_c_headers
        DEPENDS
            ${FETCHCONTENT_BASE_DIR}/json-c/json.h
            ${FETCHCONTENT_BASE_DIR}/json-c/json_config.h
        COMMENT "Creating symlinks for json-c headers"
    )

    set(json-c_INCLUDE_DIRS ${json-c_ROOT}
        CACHE STRING "Path to FetchContent json-c" FORCE)
    set(json-c_LIBRARIES json-c
        CACHE STRING "Path to json-c libraries" FORCE)
    set(json-c_DEFINITIONS ""
        CACHE STRING "json-c preprocessor definitions" FORCE)
    set(json-c_FOUND TRUE CACHE BOOL "json-c found by FetchContent" FORCE)
    set(json-c_IMPORTED TRUE CACHE BOOL "json-c imported from source" FORCE)
endif(NOT json-c_FOUND)

message(STATUS "json-c_ROOT ${json-c_ROOT}")
message(STATUS "json-c_INCLUDE_DIRS ${json-c_INCLUDE_DIRS}")
message(STATUS "json-c_LIBRARIES ${json-c_LIBRARIES}")
message(STATUS "json-c_DEFINITIONS ${json-c_DEFINITIONS}")

################################################################################
# CLI11
################################################################################

set(CLI11_URL
        https://github.com/CLIUtils/CLI11.git
        CACHE STRING "URL for CLI11")
set(CLI11_VERSION
        2.1.2
        CACHE STRING "Version for CLI11")
set(CLI11_TAG
        v${CLI11_VERSION}
        CACHE STRING "Tag for CLI11")

FetchContent_Declare(CLI11
    GIT_REPOSITORY ${CLI11_URL}
    GIT_TAG ${CLI11_TAG}
)

if (OPAE_WITH_CLI11)

    if (NOT CLI11_FOUND)
        find_package(CLI11 ${CLI11_VERSION})
    endif(NOT CLI11_FOUND)

    if (NOT CLI11_FOUND)
        set(CLI11_ROOT ${FETCHCONTENT_BASE_DIR}/cli11-src CACHE PATH "base dir for CLI11" FORCE)

        FetchContent_MakeAvailable(CLI11)

        set(CLI11_INCLUDE_DIRS ${CLI11_ROOT}/include
            CACHE STRING "Path to FetchContent CLI11" FORCE)
        set(CLI11_LIBRARIES ""
            CACHE STRING "Do not link against CLI11 library, using header-only" FORCE)
        set(CLI11_DEFINITIONS ""
            CACHE STRING "Do not link against CLI11 library, using header-only" FORCE)
        set(CLI11_FOUND TRUE CACHE BOOL "CLI11 found by FetchContent" FORCE)
    endif(NOT CLI11_FOUND)

    message(STATUS "CLI11_ROOT ${CLI11_ROOT}")
    message(STATUS "CLI11_INCLUDE_DIRS ${CLI11_INCLUDE_DIRS}")
    #message(STATUS "CLI11_LIBRARIES ${CLI11_LIBRARIES}")
    #message(STATUS "CLI11_DEFINITIONS ${CLI11_DEFINITIONS}")

endif(OPAE_WITH_CLI11)

################################################################################
# spdlog
################################################################################

set(SPDLOG_URL
        https://github.com/gabime/spdlog.git
        CACHE STRING "URL for spdlog")
set(SPDLOG_VERSION
        1.8.0
        CACHE STRING "Version for spdlog")
set(SPDLOG_TAG
        v${SPDLOG_VERSION}
        CACHE STRING "Tag for spdlog")

FetchContent_Declare(spdlog
    GIT_REPOSITORY ${SPDLOG_URL}
    GIT_TAG ${SPDLOG_TAG}
)

if (OPAE_WITH_SPDLOG)

    if (NOT spdlog_FOUND)
        find_package(spdlog ${SPDLOG_VERSION})
    endif(NOT spdlog_FOUND)

    if (NOT spdlog_FOUND)
        set(spdlog_ROOT ${FETCHCONTENT_BASE_DIR}/spdlog-src CACHE PATH "base dir for spdlog" FORCE)

        FetchContent_MakeAvailable(spdlog)

        set(spdlog_INCLUDE_DIRS ${spdlog_ROOT}/include
            CACHE STRING "Path to external spdlog" FORCE)
        set(spdlog_LIBRARIES ""
            CACHE STRING "Do not link against spdlog library, using header-only" FORCE)
        set(spdlog_DEFINITIONS ""
            CACHE STRING "Do not link against spdlog library, using header-only" FORCE)
        set(spdlog_FOUND TRUE CACHE BOOL "spdlog found by FetchContent" FORCE)
    else(NOT spdlog_FOUND)
        if(spdlog_DEFINITIONS)
            string(REGEX MATCH "-DSPDLOG_FMT_EXTERNAL" HAVE_FMT ${spdlog_DEFINITIONS})
            if (HAVE_FMT)
                find_package(fmt)
                set(fmt_LIBRARIES "fmt::fmt" CACHE STRING "fmt library link interface")
            endif (HAVE_FMT)
        endif(spdlog_DEFINITIONS)
    endif(NOT spdlog_FOUND)

    message(STATUS "spdlog_ROOT ${spdlog_ROOT}")
    message(STATUS "spdlog_INCLUDE_DIRS ${spdlog_INCLUDE_DIRS}")
    message(STATUS "spdlog_LIBRARIES ${spdlog_LIBRARIES}")
    message(STATUS "spdlog_DEFINITIONS ${spdlog_DEFINITIONS}")

endif(OPAE_WITH_SPDLOG)

################################################################################
# libedit
################################################################################

set(LIBEDIT_URL
        https://src.fedoraproject.org/repo/pkgs/libedit/libedit-20221030-3.1.tar.gz/sha512/41eb46feaffa909e8790b9a9e304d5246e82ab366721196126a923d68b4d4964d0a433fe238f9d5e0a00aefb5c8cb66132150792929a793785ad091d91016f97/libedit-20221030-3.1.tar.gz
        CACHE STRING "URL for libedit")
set(LIBEDIT_VERSION
        3.1
        CACHE STRING "Version of libedit")

if (OPAE_WITH_LIBEDIT)

    if (NOT libedit_FOUND)
        find_package(libedit ${LIBEDIT_VERSION})
    endif(NOT libedit_FOUND)

    if (NOT libedit_FOUND)
        include(ExternalProject)

        message(WARNING "Building libedit.so from source is experimental. "
                        "Be sure to install all prerequisite packages, eg libncurses5-dev. "
                        "To avoid this build path, please install the libedit-dev / "
                        "libedit-devel package, which provides libedit.so.")

        ExternalProject_Add(libedit
            URL ${LIBEDIT_URL}
            CONFIGURE_COMMAND ./configure
            BUILD_COMMAND make
            INSTALL_COMMAND ""
            BUILD_IN_SOURCE ON
        )

        set(libedit_ROOT ${CMAKE_CURRENT_BINARY_DIR}/libedit-prefix/src/libedit
            CACHE PATH "base dir for libedit" FORCE)
        set(libedit_INCLUDE_DIRS ${libedit_ROOT}/src
            CACHE STRING "Path to libedit" FORCE)
        set(libedit_LIBRARIES ${libedit_ROOT}/src/.libs/libedit.so
            CACHE STRING "Path to libedit libraries" FORCE)
        set(libedit_DEFINITIONS "" CACHE STRING "libedit built from source" FORCE)
        set(libedit_FOUND TRUE CACHE BOOL "libedit built from source" FORCE)

        add_library(libedit_IMPORT SHARED IMPORTED)
        set_property(
            TARGET libedit_IMPORT
            PROPERTY
                IMPORTED_LOCATION "${libedit_LIBRARIES}"
        )
        set(libedit_IMPORTED TRUE CACHE BOOL "libedit imported from source" FORCE)
        add_dependencies(libedit_IMPORT libedit)

    endif(NOT libedit_FOUND)

    message(STATUS "libedit_ROOT ${libedit_ROOT}")
    message(STATUS "libedit_INCLUDE_DIRS ${libedit_INCLUDE_DIRS}")
    message(STATUS "libedit_LIBRARIES ${libedit_LIBRARIES}")
    message(STATUS "libedit_DEFINITIONS ${libedit_DEFINITIONS}")

endif(OPAE_WITH_LIBEDIT)

################################################################################
# pybind11
################################################################################

set(PYBIND11_URL
        https://github.com/pybind/pybind11.git
        CACHE STRING "URL for pybind11")

if(${CMAKE_VERSION} VERSION_LESS "3.4.0" AND ${Python3_VERSION} VERSION_LESS "3.9.0")
    set(PYBIND11_VERSION
        2.4.3
        CACHE STRING "Version for pybind11")
else()
    # Otherwise, pull recent pybind11 tag to enable Python 3.9 support.
    set(PYBIND11_VERSION
        2.10.2
        CACHE STRING "Version for pybind11")
endif()

set(PYBIND11_TAG
        v${PYBIND11_VERSION}
        CACHE STRING "Tag for pybind11")

FetchContent_Declare(pybind11
    GIT_REPOSITORY ${PYBIND11_URL}
    GIT_TAG ${PYBIND11_TAG}
)

if (OPAE_WITH_PYBIND11)

    if (NOT pybind11_FOUND)
        find_package(pybind11 CONFIG)
    endif(NOT pybind11_FOUND)

    if (NOT pybind11_FOUND)
        set(pybind11_ROOT ${FETCHCONTENT_BASE_DIR}/pybind11-src CACHE PATH "base dir for pybind11" FORCE)

        FetchContent_MakeAvailable(pybind11)

        set(pybind11_INCLUDE_DIRS ${pybind11_ROOT}/include
            CACHE STRING "Path to external pybind11" FORCE)
        set(pybind11_DEFINITIONS ""
            CACHE STRING "Do not link against pybind11 library, using header-only" FORCE)
        set(pybind11_FOUND TRUE CACHE BOOL "pybind11 found by FetchContent" FORCE)
    endif(NOT pybind11_FOUND)

    message(STATUS "pybind11_ROOT ${pybind11_ROOT}")
    message(STATUS "pybind11_INCLUDE_DIRS ${pybind11_INCLUDE_DIRS}")
    message(STATUS "pybind11_DEFINITIONS ${pybind11_DEFINITIONS}")

endif(OPAE_WITH_PYBIND11)

################################################################################
# hwloc
################################################################################

set(HWLOC_URL
        https://github.com/open-mpi/hwloc.git
        CACHE STRING "URL for hwloc")
set(HWLOC_VERSION
        2.8.0
        CACHE STRING "Version for hwloc")
set(HWLOC_TAG
        hwloc-${HWLOC_VERSION}
        CACHE STRING "Tag for hwloc")

if (OPAE_WITH_HWLOC)

    if (NOT hwloc_FOUND)
        find_package(hwloc ${HWLOC_VERSION})
    endif(NOT hwloc_FOUND)

    if (NOT hwloc_FOUND)
        include(ExternalProject)

        message(WARNING "Building libhwloc.so from source is experimental. "
                        "To avoid this build path, please install the libhwloc-dev "
                        "/ hwloc-devel package, which provides libhwloc.so.")

        ExternalProject_Add(hwloc
            GIT_REPOSITORY ${HWLOC_URL}
            GIT_TAG ${HWLOC_TAG}
            CONFIGURE_COMMAND ./autogen.sh && ./configure
            BUILD_COMMAND make
            INSTALL_COMMAND ""
            BUILD_IN_SOURCE ON
        )

        set(hwloc_ROOT ${CMAKE_CURRENT_BINARY_DIR}/hwloc-prefix/src/hwloc CACHE PATH "base dir for hwloc" FORCE)
        set(hwloc_INCLUDE_DIRS ${hwloc_ROOT}/include
            CACHE STRING "Path to hwloc" FORCE)
        set(hwloc_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/hwloc-prefix/src/hwloc/hwloc/.libs/libhwloc.so
            CACHE STRING "Path to hwloc libraries" FORCE)
        set(hwloc_DEFINITIONS "" CACHE STRING "hwloc built from source" FORCE)
        set(hwloc_FOUND TRUE CACHE BOOL "hwloc built from source" FORCE)

        add_library(hwloc_IMPORT SHARED IMPORTED)
        set_property(
            TARGET hwloc_IMPORT
            PROPERTY
                IMPORTED_LOCATION "${hwloc_LIBRARIES}"
        )
        set(hwloc_IMPORTED TRUE CACHE BOOL "hwloc imported from source" FORCE)
        add_dependencies(hwloc_IMPORT hwloc)

    endif(NOT hwloc_FOUND)

    message(STATUS "hwloc_ROOT ${hwloc_ROOT}")
    message(STATUS "hwloc_INCLUDE_DIRS ${hwloc_INCLUDE_DIRS}")
    message(STATUS "hwloc_LIBRARIES ${hwloc_LIBRARIES}")
    message(STATUS "hwloc_DEFINITIONS ${hwloc_DEFINITIONS}")

endif(OPAE_WITH_HWLOC)

################################################################################
# tbb
################################################################################

set(TBB_URL
        https://github.com/oneapi-src/oneTBB.git
        CACHE STRING "URL for tbb")
set(TBB_TAG
        v2021.7.0
        CACHE STRING "Tag for tbb")

FetchContent_Declare(tbb
    GIT_REPOSITORY ${TBB_URL}
    GIT_TAG ${TBB_TAG}
)

if (OPAE_WITH_TBB)

    if (NOT tbb_FOUND)
        find_package(tbb)
    endif(NOT tbb_FOUND)

    if (NOT tbb_FOUND)
        set(tbb_ROOT ${FETCHCONTENT_BASE_DIR} CACHE PATH "base dir for tbb" FORCE)
        set(TBB_TEST OFF CACHE BOOL "skip building tests" FORCE)

        FetchContent_MakeAvailable(tbb)

        set(tbb_INCLUDE_DIRS ${tbb_ROOT}/tbb-src/include
            CACHE STRING "Path to FetchContent tbb" FORCE)
        set(tbb_LIBRARIES tbb
            CACHE STRING "Path to tbb libraries" FORCE)
        set(tbb_DEFINITIONS ""
            CACHE STRING "tbb preprocessor definitions" FORCE)
        set(tbb_FOUND TRUE CACHE BOOL "tbb found by FetchContent" FORCE)
    endif(NOT tbb_FOUND)

    message(STATUS "tbb_ROOT ${tbb_ROOT}")
    message(STATUS "tbb_INCLUDE_DIRS ${tbb_INCLUDE_DIRS}")
    message(STATUS "tbb_LIBRARIES ${tbb_LIBRARIES}")
    message(STATUS "tbb_DEFINITIONS ${tbb_DEFINITIONS}")

endif(OPAE_WITH_TBB)

################################################################################
# numa
################################################################################

set(NUMA_URL
        https://github.com/numactl/numactl.git
        CACHE STRING "URL for numa")
set(NUMA_VERSION
        2.0.16
        CACHE STRING "Version for numa")
set(NUMA_TAG
	v${NUMA_VERSION}
        CACHE STRING "Tag for numa")

if (OPAE_WITH_NUMA)

    if (NOT numa_FOUND)
        find_package(numa ${NUMA_VERSION})
    endif(NOT numa_FOUND)

    if (NOT numa_FOUND)
        include(ExternalProject)

        message(WARNING "Building libnuma.so from source is experimental. "
                        "To avoid this build path, please install the libnuma-dev "
                        "/ numactl-devel package, which provides libnuma.so.")

        ExternalProject_Add(numa
            GIT_REPOSITORY ${NUMA_URL}
            GIT_TAG ${NUMA_TAG}
            CONFIGURE_COMMAND ./autogen.sh && ./configure
            BUILD_COMMAND make libnuma.la
            INSTALL_COMMAND ""
            BUILD_IN_SOURCE ON
        )

        set(numa_ROOT ${CMAKE_CURRENT_BINARY_DIR}/numa-prefix/src/numa CACHE PATH "base dir for numa" FORCE)
        set(numa_INCLUDE_DIRS ${numa_ROOT}
            CACHE STRING "Path to numa" FORCE)
        set(numa_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/numa-prefix/src/numa/.libs/libnuma.so
            CACHE STRING "Path to numa libraries" FORCE)
        set(numa_DEFINITIONS "" CACHE STRING "numa built from source" FORCE)
        set(numa_FOUND TRUE CACHE BOOL "numa built from source" FORCE)

        add_library(numa_IMPORT SHARED IMPORTED)
        set_property(
            TARGET numa_IMPORT
            PROPERTY
                IMPORTED_LOCATION "${numa_LIBRARIES}"
        )
        set(numa_IMPORTED TRUE CACHE BOOL "numa imported from source" FORCE)
        add_dependencies(numa_IMPORT numa)

    endif(NOT numa_FOUND)

    message(STATUS "numa_ROOT ${numa_ROOT}")
    message(STATUS "numa_INCLUDE_DIRS ${numa_INCLUDE_DIRS}")
    message(STATUS "numa_LIBRARIES ${numa_LIBRARIES}")
    message(STATUS "numa_DEFINITIONS ${numa_DEFINITIONS}")

endif(OPAE_WITH_NUMA)

################################################################################
# googletest
################################################################################

set(GTEST_URL
        https://github.com/google/googletest
        CACHE STRING "URL for googletest")
set(GTEST_VERSION
        1.11.0
        CACHE STRING "Version for googletest")
set(GTEST_TAG
        release-${GTEST_VERSION}
        CACHE STRING "Tag for googletest")

FetchContent_Declare(gtest
    GIT_REPOSITORY ${GTEST_URL}
    GIT_TAG ${GTEST_TAG}
)

if (OPAE_BUILD_TESTS)
    enable_testing()

    if (NOT GTest_FOUND)
        find_package(GTest ${GTEST_VERSION})
    endif(NOT GTest_FOUND)

    if (NOT GTest_FOUND)
        set(GTEST_ROOT ${FETCHCONTENT_BASE_DIR}/gtest-src CACHE PATH "gtest root directory" FORCE)

        set(BUILD_SHARED_LIBS_ORIG ${BUILD_SHARED_LIBS})
        set(BUILD_SHARED_LIBS ON)

        FetchContent_MakeAvailable(gtest)

        set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_ORIG})

        set(GTEST_INCLUDE_DIR ${GTEST_ROOT}/googletest/include CACHE PATH "gtest include directory" FORCE)
        set(GTEST_LIBRARY ${LIBRARY_OUTPUT_PATH}/libgtest.so CACHE PATH "path to gtest library" FORCE)
        set(GTEST_MAIN_LIBRARY ${LIBRARY_OUTPUT_PATH}/libgtest_main.so CACHE PATH "path to gtest main library" FORCE)
        set(GTEST_LIBRARY_DEBUG ${LIBRARY_OUTPUT_PATH}/libgtestd.so CACHE PATH "path to (debug) gtest library" FORCE)
        set(GTEST_MAIN_LIBRARY_DEBUG ${LIBRARY_OUTPUT_PATH}/libgtest_maind.so CACHE PATH "path to (debug) gtest main library" FORCE)
        set(GTest_FOUND TRUE CACHE BOOL "GTest found by FetchContent" FORCE)

        add_library(gtest_IMPORT SHARED IMPORTED)
        set_property(
            TARGET gtest_IMPORT
            PROPERTY
                IMPORTED_LOCATION "${GTEST_LIBRARY}"
        )
        add_dependencies(gtest_IMPORT gtest)

        add_library(gtest_MAIN_IMPORT SHARED IMPORTED)
        set_property(
            TARGET gtest_MAIN_IMPORT
            PROPERTY
                IMPORTED_LOCATION "${GTEST_MAIN_LIBRARY}"
        )
        add_dependencies(gtest_MAIN_IMPORT gtest)

        add_library(gtest_IMPORT_DEBUG SHARED IMPORTED)
        set_property(
            TARGET gtest_IMPORT_DEBUG
            PROPERTY
                IMPORTED_LOCATION "${GTEST_LIBRARY_DEBUG}"
        )
        add_dependencies(gtest_IMPORT_DEBUG gtest)

        add_library(gtest_MAIN_IMPORT_DEBUG SHARED IMPORTED)
        set_property(
            TARGET gtest_MAIN_IMPORT_DEBUG
            PROPERTY
                IMPORTED_LOCATION "${GTEST_MAIN_LIBRARY_DEBUG}"
        )
        add_dependencies(gtest_MAIN_IMPORT_DEBUG gtest)

        set(GTEST_IMPORTED TRUE CACHE BOOL "gtest imported from source" FORCE)

    endif(NOT GTest_FOUND)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(GTEST_LIBRARIES ${GTEST_LIBRARY_DEBUG} ${GTEST_MAIN_LIBRARY_DEBUG}
            CACHE STRING "gtest link libraries" FORCE)
    else()
        set(GTEST_LIBRARIES ${GTEST_LIBRARY} ${GTEST_MAIN_LIBRARY}
            CACHE STRING "gtest link libraries" FORCE)
    endif()

    message(STATUS "GTEST_INCLUDE_DIR: ${GTEST_INCLUDE_DIR}")
    message(STATUS "GTEST_LIBRARY: ${GTEST_LIBRARY}")
    message(STATUS "GTEST_MAIN_LIBRARY: ${GTEST_MAIN_LIBRARY}")
    message(STATUS "GTEST_LIBRARY_DEBUG: ${GTEST_LIBRARY_DEBUG}")
    message(STATUS "GTEST_MAIN_LIBRARY_DEBUG: ${GTEST_MAIN_LIBRARY_DEBUG}")

endif(OPAE_BUILD_TESTS)

option(OPAE_ENABLE_MOCK "Enable building of test infrastructure with mock" OFF)
mark_as_advanced(OPAE_ENABLE_MOCK)

include(OPAE)

############################################################################
## Whether to build opae-legacy ############################################
############################################################################
option(OPAE_BUILD_LEGACY "Enable building of OPAE legacy tools" OFF)
mark_as_advanced(OPAE_BUILD_LEGACY)

set(OPAE_LEGACY_TAG "master" CACHE STRING "Desired branch for opae-legacy")
mark_as_advanced(OPAE_LEGACY_TAG)

FetchContent_Declare(opae-legacy
    GIT_REPOSITORY https://github.com/OFS/opae-legacy.git
    GIT_TAG ${OPAE_LEGACY_TAG}
)

if (OPAE_BUILD_LEGACY)
    set(opae-legacy_ROOT ${FETCHCONTENT_BASE_DIR}/opae-legacy-src
        CACHE PATH "base dir for opae-legacy" FORCE)
    FetchContent_MakeAvailable(opae-legacy)
endif(OPAE_BUILD_LEGACY)


if (OPAE_MINIMAL_BUILD)
    set(OPAE_WITH_CLI11 OFF CACHE BOOL "Enable use of CLI11" FORCE)
    set(OPAE_WITH_SPDLOG OFF CACHE BOOL "Enable use of spdlog" FORCE)
    set(OPAE_WITH_LIBEDIT OFF CACHE BOOL "Enable use of libedit" FORCE)
    set(OPAE_WITH_PYBIND11 OFF CACHE BOOL "Enable use of pybind11" FORCE)
    set(OPAE_WITH_HWLOC OFF CACHE BOOL "Enable use of hwloc" FORCE)
    set(OPAE_WITH_TBB OFF CACHE BOOL "Enable use of tbb" FORCE)
    set(OPAE_WITH_NUMA OFF CACHE BOOL "Enable use of numa" FORCE)
    set(CMAKE_INSTALL_PREFIX "/usr" CACHE STRING "" FORCE)
endif(OPAE_MINIMAL_BUILD)

############################################################################
## CMAKE_INSTALL_PREFIX check ##############################################
############################################################################

string(LENGTH ${CMAKE_INSTALL_PREFIX} INSTALL_PREFIX_LEN)
if (${INSTALL_PREFIX_LEN} GREATER 127)
    message(FATAL_ERROR "The length of CMAKE_INSTALL_PREFIX (${INSTALL_PREFIX_LEN})"
                        " has exceeded a limit mandated by the Linux kernel"
                        " regarding the length of a script shebang. As a result,"
                        " the build has been stopped. Please choose a more suitable"
                        " CMAKE_INSTALL_PREFIX and try again.")
endif ()

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

# begin opae-libs

set(OPAE_VERSION_LOCAL   "" CACHE STRING "OPAE local version")
set(OPAE_VERSION_MAJOR    2 CACHE STRING "OPAE major version" FORCE)
set(OPAE_VERSION_MINOR   12 CACHE STRING "OPAE minor version" FORCE)
set(OPAE_VERSION_REVISION 0${OPAE_VERSION_LOCAL} CACHE STRING "OPAE revision version" FORCE)
set(OPAE_VERSION ${OPAE_VERSION_MAJOR}.${OPAE_VERSION_MINOR}.${OPAE_VERSION_REVISION}
    CACHE STRING "OPAE version" FORCE)

set(SYSFS_PATH_MAX 256 CACHE STRING "Maximum size of sysfs paths")
set(DEV_PATH_MAX 256 CACHE STRING "Maximum size of device paths")

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
## Conditional Build Steps and Features ####################################
############################################################################
option(OPAE_BUILD_LIBOPAE_CXX "Enable building of C++ bindings around libopae-c." ON)
mark_as_advanced(OPAE_BUILD_LIBOPAE_CXX)

option(OPAE_BUILD_PYTHON_DIST "Enable building of Python source and binary distribution" OFF)
mark_as_advanced(OPAE_BUILD_PYTHON_DIST)

option(OPAE_BUILD_LIBOPAEVFIO "Enable building of the opaevfio library" ON)
mark_as_advanced(OPAE_BUILD_LIBOPAEVFIO)

option(OPAE_BUILD_PLUGIN_VFIO "Enable building of the vfio plugin module" ON)
mark_as_advanced(OPAE_BUILD_PLUGIN_VFIO)

option(OPAE_BUILD_LIBOPAEUIO "Enable building of the opaeuio library" ON)
mark_as_advanced(OPAE_BUILD_LIBOPAEUIO)

option(OPAE_BUILD_LIBOFS "Enable building of the libofs library and framework" ON)
mark_as_advanced(OPAE_BUILD_LIBOFS)

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
            This most likely means that kernel( version >= 5.4) headers aren't installed.
            See errors in platform_vfio_errors.txt")
        file(WRITE ${CMAKE_BINARY_DIR}/platform_vfio_errors.txt ${TRY_COMPILE_OUTPUT})
    endif(VFIO_CHECK)
endif (OPAE_BUILD_LIBOPAEVFIO)

if (NOT DEFINED OPAE_MINIMAL_BUILD)
    option(OPAE_MINIMAL_BUILD "Enable minimal build" OFF)
    mark_as_advanced(OPAE_MINIMAL_BUILD)
endif(NOT DEFINED OPAE_MINIMAL_BUILD)

############################################################################
## Install headers #########################################################
############################################################################
install(DIRECTORY include/opae
    DESTINATION include
    COMPONENT libopaeheaders
    PATTERN .clang-format EXCLUDE)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/cmake/config/config.h.in"
               "${CMAKE_BINARY_DIR}/include/config.h")

# end opae-libs

############################################################################
## Install configuration file ##############################################
############################################################################
if ("${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
  install(FILES opae.cfg DESTINATION /etc/opae)
else()
  install(FILES opae.cfg DESTINATION ${CMAKE_INSTALL_PREFIX}/etc/opae)
endif()

############################################################################
## Install cmake modules ###################################################
############################################################################
install(DIRECTORY cmake/modules
    DESTINATION src/opae/cmake)

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
set(CPACK_PACKAGE_CONTACT "opae@lists.linuxfoundation.org")
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

install(FILES
    ${OPAE_SDK_SOURCE}/cmake/modules/OPAEGit.cmake
    ${OPAE_SDK_SOURCE}/cmake/modules/OPAEPlugin.cmake
    DESTINATION lib/opae-${OPAE_VERSION}/modules
)
