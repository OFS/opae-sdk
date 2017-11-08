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

find_package(GTest REQUIRED)

	include_directories(${GTEST_INCLUDE_DIRS}
			    ${OPAE_INCLUDE_DIR}
			    ${COMMON_DIR}/fptutils
			    ${CMAKE_SOURCE_DIR}/ase/api/src
			    ${CMAKE_SOURCE_DIR}/src
			    ${CMAKE_BINARY_DIR}/../tests/ASE)

set(HEADER_FILES ${CMAKE_BINARY_DIR}/../tests/ASE/globals.h)
	set(SRC gtmain.cpp ${HEADER_FILES}   
	    unit_tests/gtProperties_base.cpp   
	    unit_tests/gtOpenClose_base.cpp       
	    functional_tests/gtEnumerate.cpp  
#    functional_tests/gtHostif.cpp  
	    functional_tests/gtOpenClose.cpp   
#    functional_tests/gtReconf.cpp  
#    functional_tests/gtUmsg.cpp
	    functional_tests/gtBuffer.cpp    
#    functional_tests/gtEvent.cpp      
	    functional_tests/gtMMIO.cpp    
	    functional_tests/gtReset.cpp
	   )

add_executable(gtase ${SRC})

	target_include_directories(gtase PUBLIC
				   $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
				   $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
				   $<INSTALL_INTERFACE:include>
				   PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/../../../ase/api/src>)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(PUBLIC gtase "-pthread")
endif()

target_link_libraries(gtase opae-c-ase json-c uuid ${GTEST_BOTH_LIBRARIES})
# unit_tests functional_tests)

if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(gtase "${CMAKE_THREAD_LIBS_INIT}")
endif()

# build mock driver library
#add_library(mock SHARED mock.c)
#target_include_directories( $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
# PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/../../ase/api/src>)
#target_link_libraries(mock dl)

	if (BUILD_OPAE_CPP)
	add_subdirectory(c++)
endif()
