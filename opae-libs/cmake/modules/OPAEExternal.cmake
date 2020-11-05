#!/usr/bin/cmake -P
## Copyright(c) 2020, Intel Corporation
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

cmake_minimum_required (VERSION 2.8.12)

macro(opae_external_project_add)
    set(options EXCLUDE_FROM_ALL NO_ADD_SUBDIRECTORY DEFER)
    set(oneValueArgs PROJECT_NAME GIT_URL GIT_TAG PRESERVE_REPOS)
    set(multiValueArgs)
    cmake_parse_arguments(OPAE_EXTERNAL_PROJECT_ADD "${options}"
        "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    if(NOT OPAE_EXTERNAL_PROJECT_ADD_GIT_TAG)
        set(OPAE_EXTERNAL_PROJECT_ADD_GIT_TAG "master")
    endif(NOT OPAE_EXTERNAL_PROJECT_ADD_GIT_TAG)

    set(${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME}_ROOT
        ${CMAKE_SOURCE_DIR}/external/${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME}
        CACHE PATH "Root directory to ${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME} external" FORCE)

    set(download_dir
        ${CMAKE_CURRENT_BINARY_DIR}/${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME}/download)
    file(WRITE ${download_dir}/CMakeLists.txt
        "cmake_minimum_required(VERSION 2.8.12)\n"
        "project(${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME}-download)\n"
        "include(ExternalProject)\n"
        "ExternalProject_Add(${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME}\n"
        "    GIT_REPOSITORY ${OPAE_EXTERNAL_PROJECT_ADD_GIT_URL}\n"
        "    GIT_TAG ${OPAE_EXTERNAL_PROJECT_ADD_GIT_TAG}\n"
        "    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME}\n"
        "    BINARY_DIR ${CMAKE_BINARY_DIR}/external/${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME}\n"
        "    CONFIGURE_COMMAND \"\"\n"
        "    BUILD_COMMAND \"\"\n"
        "    INSTALL_COMMAND \"\"\n"
        "    TEST_COMMAND \"\"\n"
        "    COMMENT \"adding ${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME}\"\n"
        ")\n"
    )

    if (${OPAE_EXTERNAL_PROJECT_ADD_DEFER})
        set(timestamp ${download_dir}/timestamp)
        add_custom_command(
            OUTPUT ${timestamp}
            COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
            COMMAND ${CMAKE_COMMAND} --build .
            COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E touch ${timestamp}
            COMMENT "fetching ${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME} ${download_dir}"
            WORKING_DIRECTORY ${download_dir})

        add_custom_target(${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME}-fetch
            ALL DEPENDS ${timestamp}
        )
    else (${OPAE_EXTERNAL_PROJECT_ADD_DEFER})
        if(NOT EXISTS ${CMAKE_SOURCE_DIR}/external/${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME} OR NOT ${OPAE_EXTERNAL_PROJECT_ADD_PRESERVE_REPOS})
            execute_process(
                COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${download_dir})
            if(result)
                message(FATAL_ERROR "CMake step for ${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME} failed: ${result}")
            endif(result)

            execute_process(
                COMMAND ${CMAKE_COMMAND} --build .
                RESULT_VARIABLE result
                WORKING_DIRECTORY ${download_dir})
            if(result)
                message(FATAL_ERROR "Build step for ${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME} failed: ${result}")
            endif(result)
        endif()
        add_custom_target(${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME}-fetch
        )
    endif (${OPAE_EXTERNAL_PROJECT_ADD_DEFER})

    set(src_dir
        ${CMAKE_SOURCE_DIR}/external/${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME})
    set(bin_dir
        ${CMAKE_BINARY_DIR}/external/${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME})

    if(NOT ${OPAE_EXTERNAL_PROJECT_ADD_NO_ADD_SUBDIRECTORY} AND EXISTS ${src_dir}/CMakeLists.txt)
        message(DEBUG "adding subdirectory: ${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME}")
        if(${OPAE_EXTERNAL_PROJECT_ADD_EXCLUDE_FROM_ALL})
            add_subdirectory(${src_dir} ${bin_dir} EXCLUDE_FROM_ALL)
        else(${OPAE_EXTERNAL_PROJECT_ADD_EXCLUDE_FROM_ALL})
            add_subdirectory(${src_dir} ${bin_dir})
        endif(${OPAE_EXTERNAL_PROJECT_ADD_EXCLUDE_FROM_ALL})
    else(NOT ${OPAE_EXTERNAL_PROJECT_ADD_NO_ADD_SUBDIRECTORY} AND EXISTS ${src_dir}/CMakeLists.txt)
        message(DEBUG "NOT adding subdirectory: ${OPAE_EXTERNAL_PROJECT_ADD_PROJECT_NAME}")
    endif(NOT ${OPAE_EXTERNAL_PROJECT_ADD_NO_ADD_SUBDIRECTORY} AND EXISTS ${src_dir}/CMakeLists.txt)

endmacro(opae_external_project_add)

macro(opae_target_depends_external target external)
    add_dependencies(${target} ${external}-fetch)
endmacro(opae_target_depends_external target external)
