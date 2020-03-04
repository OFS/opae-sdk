#!/usr/bin/cmake -P
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

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

if(NOT CMAKE_C_COMPILER)
    message("-- No C compiler was found. Please install the gcc package for your distribution:
    DEB: apt install gcc
    RPM: yum install gcc")
endif()

if(NOT CMAKE_CXX_COMPILER)
    message("-- No C++ compiler was found. Please install the g++ package for your distribution:
    DEB: apt install g++
    RPM: yum install gcc-c++")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads)

############################################################################
## Set the default build type to Release with debug info. ##################
############################################################################
if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE RelWithDebInfo
        CACHE STRING
        "Type of build: {Debug Release RelWithDebInfo MinSizeRel Coverage}"
        FORCE)
    function(opae_coverage_build)
    endfunction()
elseif(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    find_program(OPAE_GCOV_EXECUTABLE gcov)
    if(NOT OPAE_GCOV_EXECUTABLE)
        message(FATAL_ERROR "Coverage requested, but gcov not found. Aborting...")
    endif()
    find_program(OPAE_LCOV_EXECUTABLE lcov)
    find_program(OPAE_GENHTML_EXECUTABLE genhtml)

    # example:
    #   opae_coverage_build(TARGET opae-c SOURCE a.c b.c)
    function(opae_coverage_build)
        set(options )
        set(oneValueArgs TARGET)
        set(multiValueArgs SOURCE)
        cmake_parse_arguments(OPAE_COVERAGE_BUILD "${options}"
            "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        set_property(SOURCE ${OPAE_COVERAGE_BUILD_SOURCE} APPEND_STRING PROPERTY COMPILE_FLAGS
                     " -g -O0 -Wall -Wextra -Werror -pthread --coverage -fprofile-arcs -ftest-coverage")
        target_link_libraries(${OPAE_COVERAGE_BUILD_TARGET} "-lgcov")
    endfunction()
else()
    function(opae_coverage_build)
    endfunction()
endif()

set(CMAKE_C_FLAGS_DEBUG            "-g -O0 -Wall -Wextra -Werror -pthread")
set(CMAKE_CXX_FLAGS_DEBUG          "-g -O0 -Wall -Wextra -Werror -pthread")

set(CMAKE_C_FLAGS_RELEASE          "-O2 -Wall -Wextra -Werror -pthread")
set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -Wall -Wextra -Werror -pthread")

set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-g -O2 -Wall -Wextra -Werror -pthread")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -Wall -Wextra -Werror -pthread")

set(CMAKE_C_FLAGS_MINSIZEREL       "-Os -Wall -Wextra -Werror -pthread")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -Wall -Wextra -Werror -pthread")

############################################################################
## Enable defensive options for Release builds. ############################
############################################################################
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    # C options
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat -Wformat-security")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -D_FORTIFY_SOURCE=2")
    if(GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -z noexecstack -z relro -z now")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all")
    endif()

    # C++ options
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat -Wformat-security")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -D_FORTIFY_SOURCE=2")
    if(GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -z noexecstack -z relro -z now")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all")
    endif()

    # Linker options
    if (NOT ${CMAKE_C_COMPILER} MATCHES  "clang")
         set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pie")
         set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
    endif()
endif()

# Check if support for C++ 11/14/0x is available
check_cxx_compiler_flag("-std=c++14" COMPILER_SUPPORTS_CXX14)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX14)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    set(CMAKE_CXX_STANDARD 14)
elseif(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_CXX_STANDARD 11)
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

# If building on a 32-bit system, make sure off_t can store offsets > 2GB.
if(CMAKE_COMPILER_IS_GNUCC)
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        add_definitions(-D_LARGEFILE_SOURCE)
        add_definitions(-D_FILE_OFFSET_BITS=64)
    endif()
endif()

macro(set_install_rpath target_name)
    if(OPAE_INSTALL_RPATH)
        set_target_properties(${target_name} PROPERTIES
            INSTALL_RPATH "\$ORIGIN/../${OPAE_LIB_INSTALL_DIR}"
            INSTALL_RPATH_USE_LINK_PATH TRUE
            SKIP_BUILD_RPATH FALSE
            BUILD_WITH_INSTALL_RPATH FALSE)
    endif()
endmacro()

function(opae_add_subdirectory directory_name)
    get_filename_component(full_dir_path "${directory_name}" REALPATH)
    if(EXISTS "${full_dir_path}" AND IS_DIRECTORY "${full_dir_path}")
       add_subdirectory(${directory_name})
    else()
       message("Directory not found: ${full_dir_path}")
    endif()
endfunction()
# example:
#   opae_add_executable(TARGET fpgaconf SOURCE a.c b.c LIBS safestr)
function(opae_add_executable)
    set(options )
    set(oneValueArgs TARGET COMPONENT DESTINATION)
    set(multiValueArgs SOURCE LIBS)
    cmake_parse_arguments(OPAE_ADD_EXECUTABLE "${options}"
        "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${OPAE_ADD_EXECUTABLE_TARGET} ${OPAE_ADD_EXECUTABLE_SOURCE})

    target_include_directories(${OPAE_ADD_EXECUTABLE_TARGET} PUBLIC
        $<BUILD_INTERFACE:${OPAE_LIBS_ROOT}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${OPAE_LIBS_ROOT}
        PUBLIC ${libjson-c_INCLUDE_DIRS}
        PUBLIC ${libuuid_INCLUDE_DIRS})

    set_property(TARGET ${OPAE_ADD_EXECUTABLE_TARGET} PROPERTY C_STANDARD 99)
    target_compile_definitions(${OPAE_ADD_EXECUTABLE_TARGET}
        PRIVATE
            HAVE_CONFIG_H=1)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${OPAE_ADD_EXECUTABLE_TARGET}
            PRIVATE
                LIBOPAE_DEBUG=1)
    endif()

    target_link_libraries(${OPAE_ADD_EXECUTABLE_TARGET} ${OPAE_ADD_EXECUTABLE_LIBS})

    opae_coverage_build(TARGET ${OPAE_ADD_EXECUTABLE_TARGET} SOURCE ${OPAE_ADD_EXECUTABLE_SOURCE})
    set_install_rpath(${OPAE_ADD_EXECUTABLE_TARGET})

    if(OPAE_ADD_EXECUTABLE_COMPONENT)
        if(OPAE_ADD_EXECUTABLE_DESTINATION)
            set(dest ${OPAE_ADD_EXECUTABLE_DESTINATION})
        else(OPAE_ADD_EXECUTABLE_DESTINATION)
            set(dest bin)
        endif(OPAE_ADD_EXECUTABLE_DESTINATION)

        install(TARGETS ${OPAE_ADD_EXECUTABLE_TARGET}
                RUNTIME DESTINATION ${dest}
                COMPONENT ${OPAE_ADD_EXECUTABLE_COMPONENT})
    endif(OPAE_ADD_EXECUTABLE_COMPONENT)
endfunction()

# example:
#   opae_add_shared_library(TARGET opae-c SOURCE a.c b.c LIBS safestr)
function(opae_add_shared_library)
    set(options )
    set(oneValueArgs TARGET VERSION SOVERSION COMPONENT DESTINATION)
    set(multiValueArgs SOURCE LIBS)
    cmake_parse_arguments(OPAE_ADD_SHARED_LIBRARY "${options}"
        "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_library(${OPAE_ADD_SHARED_LIBRARY_TARGET} SHARED ${OPAE_ADD_SHARED_LIBRARY_SOURCE})

    target_include_directories(${OPAE_ADD_SHARED_LIBRARY_TARGET} PUBLIC
        $<BUILD_INTERFACE:${OPAE_LIBS_ROOT}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${OPAE_LIBS_ROOT}
        PUBLIC ${libjson-c_INCLUDE_DIRS}
        PUBLIC ${libuuid_INCLUDE_DIRS})

    set_property(TARGET ${OPAE_ADD_SHARED_LIBRARY_TARGET} PROPERTY C_STANDARD 99)
    target_compile_definitions(${OPAE_ADD_SHARED_LIBRARY_TARGET}
        PRIVATE
            HAVE_CONFIG_H=1)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${OPAE_ADD_SHARED_LIBRARY_TARGET}
            PRIVATE
                LIBOPAE_DEBUG=1)
    endif()

    if(OPAE_ADD_SHARED_LIBRARY_VERSION AND OPAE_ADD_SHARED_LIBRARY_SOVERSION)
        set_target_properties(${OPAE_ADD_SHARED_LIBRARY_TARGET} PROPERTIES
            VERSION ${OPAE_ADD_SHARED_LIBRARY_VERSION}
            SOVERSION ${OPAE_ADD_SHARED_LIBRARY_SOVERSION})
    endif()

    target_link_libraries(${OPAE_ADD_SHARED_LIBRARY_TARGET} ${OPAE_ADD_SHARED_LIBRARY_LIBS})

    opae_coverage_build(TARGET ${OPAE_ADD_SHARED_LIBRARY_TARGET} SOURCE ${OPAE_ADD_SHARED_LIBRARY_SOURCE})
    set_install_rpath(${OPAE_ADD_SHARED_LIBRARY_TARGET})

    if(OPAE_ADD_SHARED_LIBRARY_COMPONENT)
        if(OPAE_ADD_SHARED_LIBRARY_DESTINATION)
            set(dest ${OPAE_ADD_SHARED_LIBRARY_DESTINATION})
        else(OPAE_ADD_SHARED_LIBRARY_DESTINATION)
            set(dest ${OPAE_LIB_INSTALL_DIR})
        endif(OPAE_ADD_SHARED_LIBRARY_DESTINATION)

        install(TARGETS ${OPAE_ADD_SHARED_LIBRARY_TARGET}
                LIBRARY DESTINATION ${dest}
                COMPONENT ${OPAE_ADD_SHARED_LIBRARY_COMPONENT})
    endif(OPAE_ADD_SHARED_LIBRARY_COMPONENT)
endfunction()

# example:
#   opae_add_module_library(TARGET xfpga SOURCE a.c b.c LIBS safestr)
function(opae_add_module_library)
    set(options )
    set(oneValueArgs TARGET COMPONENT DESTINATION)
    set(multiValueArgs SOURCE LIBS)
    cmake_parse_arguments(OPAE_ADD_MODULE_LIBRARY "${options}"
        "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_library(${OPAE_ADD_MODULE_LIBRARY_TARGET} MODULE ${OPAE_ADD_MODULE_LIBRARY_SOURCE})

    target_include_directories(${OPAE_ADD_MODULE_LIBRARY_TARGET} PUBLIC
        $<BUILD_INTERFACE:${OPAE_LIBS_ROOT}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${OPAE_LIBS_ROOT}
        PUBLIC ${libjson-c_INCLUDE_DIRS}
        PUBLIC ${libuuid_INCLUDE_DIRS})

    set_property(TARGET ${OPAE_ADD_MODULE_LIBRARY_TARGET} PROPERTY C_STANDARD 99)
    target_compile_definitions(${OPAE_ADD_MODULE_LIBRARY_TARGET}
        PRIVATE
            HAVE_CONFIG_H=1)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${OPAE_ADD_MODULE_LIBRARY_TARGET}
            PRIVATE
                LIBOPAE_DEBUG=1)
    endif()

    target_link_libraries(${OPAE_ADD_MODULE_LIBRARY_TARGET} ${OPAE_ADD_MODULE_LIBRARY_LIBS})

    opae_coverage_build(TARGET ${OPAE_ADD_MODULE_LIBRARY_TARGET} SOURCE ${OPAE_ADD_MODULE_LIBRARY_SOURCE})

    if(OPAE_ADD_MODULE_LIBRARY_COMPONENT)
        if(OPAE_ADD_MODULE_LIBRARY_DESTINATION)
            set(dest ${OPAE_ADD_MODULE_LIBRARY_DESTINATION})
        else(OPAE_ADD_MODULE_LIBRARY_DESTINATION)
            set(dest ${OPAE_LIB_INSTALL_DIR}/opae)
        endif(OPAE_ADD_MODULE_LIBRARY_DESTINATION)

        install(TARGETS ${OPAE_ADD_MODULE_LIBRARY_TARGET}
                LIBRARY DESTINATION ${dest}
                COMPONENT ${OPAE_ADD_MODULE_LIBRARY_COMPONENT})
    endif(OPAE_ADD_MODULE_LIBRARY_COMPONENT)
endfunction()

# example:
#   opae_add_static_library(TARGET safestr SOURCE ${SRC})
function(opae_add_static_library)
    set(options )
    set(oneValueArgs TARGET COMPONENT DESTINATION)
    set(multiValueArgs SOURCE LIBS)
    cmake_parse_arguments(OPAE_ADD_STATIC_LIBRARY "${options}"
        "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_library(${OPAE_ADD_STATIC_LIBRARY_TARGET} STATIC ${OPAE_ADD_STATIC_LIBRARY_SOURCE})

    target_include_directories(${OPAE_ADD_STATIC_LIBRARY_TARGET} PUBLIC
        $<BUILD_INTERFACE:${OPAE_LIBS_ROOT}/include>
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    set_property(TARGET ${OPAE_ADD_STATIC_LIBRARY_TARGET} PROPERTY C_STANDARD 99)
    set_property(TARGET ${OPAE_ADD_STATIC_LIBRARY_TARGET}
        PROPERTY
            POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(${OPAE_ADD_STATIC_LIBRARY_TARGET}
        PRIVATE
            PIC=1
            HAVE_CONFIG_H=1)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(${OPAE_ADD_STATIC_LIBRARY_TARGET}
            PRIVATE
                LIBOPAE_DEBUG=1)
    endif()

    target_link_libraries(${OPAE_ADD_STATIC_LIBRARY_TARGET}
        ${OPAE_ADD_STATIC_LIBRARY_LIBS})

    opae_coverage_build(TARGET ${OPAE_ADD_STATIC_LIBRARY_TARGET} SOURCE ${OPAE_ADD_STATIC_LIBRARY_SOURCE})

    if(OPAE_ADD_STATIC_LIBRARY_COMPONENT)
        if(OPAE_ADD_STATIC_LIBRARY_DESTINATION)
            set(dest ${OPAE_ADD_STATIC_LIBRARY_DESTINATION})
        else(OPAE_ADD_STATIC_LIBRARY_DESTINATION)
            set(dest ${OPAE_LIB_INSTALL_DIR})
        endif(OPAE_ADD_STATIC_LIBRARY_DESTINATION)

        install(TARGETS ${OPAE_ADD_STATIC_LIBRARY_TARGET}
                ARCHIVE DESTINATION ${dest}
                COMPONENT ${OPAE_ADD_STATIC_LIBRARY_COMPONENT})
    endif(OPAE_ADD_STATIC_LIBRARY_COMPONENT)
endfunction()
