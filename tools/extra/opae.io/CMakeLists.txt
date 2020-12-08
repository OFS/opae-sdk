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
## POSSIBILITY OF SUCH DAMAGE.

if(PLATFORM_SUPPORTS_VFIO)

    find_package(edit REQUIRED)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    set(PYBIND11_CPP_STANDARD -std=c++11)

    add_executable(opae.io main.cpp)

    if (NOT (CMAKE_VERSION VERSION_LESS 3.0))
        target_link_libraries(opae.io PRIVATE pybind11::embed dl util ${libedit_LIBRARIES} opaevfio)
    else()
        target_link_libraries(opae.io PRIVATE ${PYTHON_LIBRARIES} dl util ${libedit_LIBRARIES} opaevfio)
    endif()
    
    target_include_directories(opae.io
        PUBLIC ${OPAE_INCLUDE_DIR}
        PRIVATE ${PYTHON_INCLUDE_DIRS}
        PRIVATE ${PYBIND11_INCLUDE_DIR}
    )
    target_compile_definitions(opae.io
        PRIVATE LIBVFIO_EMBED
    )
    install(TARGETS opae.io
            RUNTIME DESTINATION bin
            COMPONENT opae.io
    )
    
endif(PLATFORM_SUPPORTS_VFIO)
