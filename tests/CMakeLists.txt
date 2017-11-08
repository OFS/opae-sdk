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

if (NOT OPAE_SDK_SOURCE)
	set(OPAE_BASE_DIR ${CMAKE_SOURCE_DIR}/../../fpga-sw)
endif(NOT OPAE_SDK_SOURCE)

set(OPAE_INCLUDE_DIR ${OPAE_SDK_SOURCE}/common/include)

if (NOT OPAE_LIB_DIR)
	set (OPAE_LIB_DIR ${OPAE_SDK_SOURCE}/build/lib)
endif()

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
if(BUILD_ASE)
	add_subdirectory(ASE)
else(BUILD_ASE)
	include_directories(${GTEST_INCLUDE_DIRS}
			    ${OPAE_INCLUDE_DIR}
			    ${OPAE_SDK_SOURCE}/libopae/src
			    ${OPAE_SDK_SOURCE}/tools/libopae++
			    ${OPAE_SDK_SOURCE}/tools/c++utils
                            ${Boost_INCLUDE_DIRS}
                            ${OPAE_SDK_SOURCE}/common/include
                            ${OPAE_SDK_SOURCE}/tests
			    ${CMAKE_BINARY_DIR}/..)

add_subdirectory(safe_string)

	set(SRC gtmain.cpp jsonParser.cpp
	    unit/gtProperties_base.cpp
	    unit/gtOpenClose_base.cpp
            unit/gtEnumerate.cpp
            unit/gtOpen.cpp
            unit/gtOptionParser.cpp
	    function/gtEnumerate.cpp
	    function/gtHostif.cpp
	    function/gtOpenClose.cpp
	    function/gtReconf.cpp
	    function/gtUmsg.cpp
	    function/gtBuffer.cpp
	    function/gtEvent.cpp
	    function/gtMMIO.cpp
	    function/gtReset.cpp
        function/gtMMLink.cpp
  	    stress/gtBuf_stress.cpp
        stress/gtMMIO_stress.cpp
        stress/gtOpen_stress.cpp
        stress/gtPR_stress.cpp
        stress/gtPerfCounters_stress.cpp
        stress/gtRASPower_stress.cpp)

add_library(commonlib SHARED common_test.h common_test.cpp)

add_executable(foapp
	fpgaopen_app.cpp
	)

	target_include_directories(foapp PUBLIC
				   $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
				   $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
				   $<INSTALL_INTERFACE:include>
				   PRIVATE $<BUILD_INTERFACE:${OPAE_SDK_SOURCE}/libopae/src>)

add_executable(gtapi ${SRC})

	target_include_directories(gtapi PUBLIC
				   $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
				   $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
				   $<INSTALL_INTERFACE:include>
				   PRIVATE $<BUILD_INTERFACE:${OPAE_SDK_SOURCE}/libopae/src>)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(PUBLIC commonlib "-pthread")
	target_compile_options(PUBLIC foapp "-pthread")
	target_compile_options(PUBLIC gtapi "-pthread")
endif()

#intra-process tests
target_link_libraries(commonlib opae-c)
target_link_libraries(foapp commonlib safestr opae-c uuid ${GTEST_BOTH_LIBRARIES})
target_link_libraries(gtapi commonlib safestr opae-c json-c uuid ${GTEST_BOTH_LIBRARIES}
                      opae-c++-utils opae-c++)
# unit_tests functional_tests)

if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(foapp "${CMAKE_THREAD_LIBS_INIT}")
	target_link_libraries(gtapi "${CMAKE_THREAD_LIBS_INIT}")
endif()

# build mock driver library
add_library(mock SHARED mock.c)
	target_include_directories(mock PUBLIC
				   $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
				   PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libopae/src>)
target_link_libraries(mock dl safestr)

        add_subdirectory(dcp)

endif(BUILD_ASE)


