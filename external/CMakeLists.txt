## Copyright(c) 2020-2022, Intel Corporation
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

cmake_minimum_required(VERSION 3.11)

if(OPAE_WITH_PYBIND11)

    if (NOT pybind11_FOUND)
        set(PYBIND11_INSTALLATION_FOUND FALSE CACHE BOOL "Found installed pybind11 pkg" FORCE)

        if(${CMAKE_VERSION} VERSION_LESS "3.4.0" AND ${PYTHONLIBS_VERSION_STRING} VERSION_LESS "3.9.0")
            set(PYBIND11_TAG "v2.4.3")
        else()
        # Otherwise, pull recent pybind11 tag to enable Python 3.9 support.
            set(PYBIND11_TAG "v2.10.0")
        endif()
        message(STATUS "Fetching pybind11 ${PYBIND11_TAG}")

        set(pybind11_ROOT ${FETCHCONTENT_BASE_DIR}/pybind11-src CACHE PATH "base dir for pybind11" FORCE)

        FetchContent_Declare(pybind11
            GIT_REPOSITORY https://github.com/pybind/pybind11.git
            GIT_TAG ${PYBIND11_TAG}
        )

        FetchContent_MakeAvailable(pybind11)
    endif(NOT pybind11_FOUND)

endif(OPAE_WITH_PYBIND11)

if (OPAE_WITH_CLI11)
    if(NOT CLI11_FOUND)
        find_package(CLI11 1.8)
    endif(NOT CLI11_FOUND)

    if(NOT CLI11_FOUND)
        set(CLI11_URL
            https://github.com/CLIUtils/CLI11.git
            CACHE STRING "URL for CLI11 Project")
        set(CLI11_TAG
            v2.1.2
            CACHE STRING "Tag for CLI11")

        set(CLI11_ROOT ${FETCHCONTENT_BASE_DIR}/cli11-src CACHE PATH "base dir for cli11" FORCE)

        FetchContent_Declare(CLI11
            GIT_REPOSITORY ${CLI11_URL}
            GIT_TAG ${CLI11_TAG}
        )

        FetchContent_MakeAvailable(CLI11)

        set(CLI11_INCLUDE_DIRS ${CLI11_ROOT}/include
            CACHE STRING "Path to external CLI11" FORCE)
        set(CLI11_LIBRARIES ""
            CACHE STRING "Do not link against CLI11 library, using header-only" FORCE)
        set(CLI11_DEFINITIONS ""
            CACHE STRING "Do not link against CLI11 library, using header-only" FORCE)
        set(CLI11_FOUND TRUE CACHE BOOL "CLI11 found as external")
    endif(NOT CLI11_FOUND)
endif(OPAE_WITH_CLI11)

if (OPAE_WITH_SPDLOG)
    if(NOT spdlog_FOUND)
        find_package(spdlog 1.8)
    endif(NOT spdlog_FOUND)

    if(NOT spdlog_FOUND)
        set(SPDLOG_URL
            https://github.com/gabime/spdlog.git
            CACHE STRING "URL for spdlog Project")
        set(SPDLOG_TAG
            v1.8.0
            CACHE STRING "Tag for spdlog")

        set(spdlog_ROOT ${FETCHCONTENT_BASE_DIR}/spdlog-src CACHE PATH "base dir for spdlog" FORCE)

        FetchContent_Declare(spdlog
            GIT_REPOSITORY ${SPDLOG_URL}
            GIT_TAG ${SPDLOG_TAG}
        )

        FetchContent_MakeAvailable(spdlog)

        set(spdlog_INCLUDE_DIRS ${spdlog_ROOT}/include
            CACHE STRING "Path to external spdlog" FORCE)
        set(spdlog_LIBRARIES ""
            CACHE STRING "Do not link against spdlog library, using header-only" FORCE)
        set(spdlog_DEFINITIONS ""
            CACHE STRING "Do not link against spdlog library, using header-only" FORCE)
        set(spdlog_FOUND TRUE CACHE BOOL "spdlog found as external")
    else(NOT spdlog_FOUND)
        if(spdlog_DEFINITIONS)
            string(REGEX MATCH "-DSPDLOG_FMT_EXTERNAL" have_fmt ${spdlog_DEFINITIONS})
            if (have_fmt)
                find_package(fmt)
                set(fmt_LIBRARIES "fmt::fmt" CACHE STRING "fmt library link interface")
            endif (have_fmt)
        endif(spdlog_DEFINITIONS)
    endif(NOT spdlog_FOUND)
endif(OPAE_WITH_SPDLOG)

if(OPAE_BUILD_LEGACY)
    set(opae-legacy_ROOT ${FETCHCONTENT_BASE_DIR}/opae-legacy-src CACHE PATH "base dir for opae-legacy" FORCE)

    FetchContent_Declare(opae-legacy
        GIT_REPOSITORY https://github.com/OFS/opae-legacy.git
        GIT_TAG ${OPAE_LEGACY_TAG}
    )

    FetchContent_MakeAvailable(opae-legacy)
endif(OPAE_BUILD_LEGACY)
