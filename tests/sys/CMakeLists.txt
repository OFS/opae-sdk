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

##### Setup Build Configuration #######################################
set(OPAE_INCLUDE_DIR ${OPAE_SDK_SOURCE}/common/include
                     ${CMAKE_SOURCE_DIR}
                     ${CMAKE_SOURCE_DIR}/utils
                     ${CMAKE_SOURCE_DIR}/../safe_string)

set (OPAE_LIB_DIR 
         "/usr/include/lib"
         "/usr/lib")

link_directories(${OPAE_LIB_DIR})

set(GCC_COMPILER_FLAGS "-Wno-write-strings")

# Check if support for C++ 11 is available
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag("-std=c++14" COMPILER_SUPPORTS_CXX14)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX14)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
  set(CMAKE_CXX_STANDARD 14)
elseif(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${GCC_COMPILER_FLAGS} ")
  set(CMAKE_CXX_STANDARD 11)
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x ${GCC_COMPILER_FLAGS} ")
endif()

find_package(Doxygen
	REQUIRED dot
	OPTIONAL_COMPONENTS	mscgen dia)

if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
	${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(docs
	${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	COMMENT "Generating test lists and descriptions with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)

find_package(GTest REQUIRED)

############ Build GTAPI_SYS #################################################
include_directories(${GTEST_INCLUDE_DIRS}
            ${OPAE_INCLUDE_DIR}
            ${CMAKE_SOURCE_DIR}
            ${CMAKE_SOURCE_DIR}/..
            ${OPAE_LIB_DIR}
            ${CMAKE_INSTALL_PREFIX}/utils
            ${CMAKE_INSTALL_PREFIX}/lib)

set(SRC gtmain_sys.cpp 
        utils/jsonParser.cpp
        function/gtEnumerate_sys.cpp
        function/gtBuffer_sys.cpp
        function/gtMMIO_sys.cpp
        function/gtOpenClose_sys.cpp
  	    stress/gtBuf_sys.cpp
        stress/gtMMIO_sys.cpp
        stress/gtOpen_sys.cpp
        stress/gtPR_sys.cpp
        stress/gtPerfCounters_sys.cpp
        stress/gtRASPower_sys.cpp)

add_library(commonlib SHARED
            ${CMAKE_SOURCE_DIR}/utils/common_utils.cpp
            ${CMAKE_SOURCE_DIR}/utils/common_sys.cpp)

add_executable(foapp  ${CMAKE_SOURCE_DIR}/utils/fpgaopen_app.cpp)

target_include_directories(foapp PUBLIC
            $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
            $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
            $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/utils>
            $<INSTALL_INTERFACE:include>)

add_executable(gtapi_sys ${SRC})

target_include_directories(gtapi_sys PUBLIC
            $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
            $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
            $<INSTALL_INTERFACE:include>)

if(THREADS_HAVE_PTHREAD_ARG)
            target_compile_options(PUBLIC commonlib "-pthread")
            target_compile_options(PUBLIC foapp "-pthread")
            target_compile_options(PUBLIC gtapi "-pthread")
endif()

#intra-process tests
target_link_libraries(commonlib opae-c)
target_link_libraries(foapp commonlib safestr opae-c uuid ${GTEST_BOTH_LIBRARIES})
target_link_libraries(gtapi_sys commonlib safestr opae-c ${libjson-c_LIBRARIES} uuid ${GTEST_BOTH_LIBRARIES})


#################### Common ###################################################
if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(foapp "${CMAKE_THREAD_LIBS_INIT}")
    target_link_libraries(gtapi "${CMAKE_THREAD_LIBS_INIT}")
endif()


