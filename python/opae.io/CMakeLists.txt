## Copyright(c) 2019-2020, Intel Corporation
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

cmake_minimum_required (VERSION 2.8.12)

# projectname is the same as the main-executable
project(opae.io)

set(libvfio_URL
    https://github.com/r-rojo/libvfio.git
    CACHE STRING "URL for libvfio Project")
set(libvfio_TAG
    rrojo/libvfio-cfg-dma
    CACHE STRING "Tag to checkout for libvfio")


opae_external_project_add(PROJECT_NAME libvfio
                          GIT_URL ${libvfio_URL}
                          GIT_TAG ${libvfio_TAG}
)

set(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/build/timestamp)
file(GLOB_RECURSE PKG_FILES ${CMAKE_CURRENT_SOURCE_DIR}/opae/*)
add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND ${PYTHON_EXECUTABLE} setup.py build -b ${CMAKE_CURRENT_BINARY_DIR}/build
    COMMAND ${CMAKE_COMMAND} -E touch ${OUTPUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS setup.py ${PKG_FILES}
)

add_custom_target(opae.io ALL DEPENDS ${OUTPUT})
install(
    CODE "execute_process(
        COMMAND ${PYTHON_EXECUTABLE} setup.py install
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )"
    COMPONENT opae.io
)

find_package(Threads)
find_package(libeditline)

add_executable(opae-io main.cpp ${libvfio_ROOT}/src/vfio_driver.cpp)
target_include_directories(opae-io
    PRIVATE ${CMAKE_SOURCE_DIR}/external/pybind11/include
            ${PYTHON_INCLUDE_DIRS}
            ${libvfio_ROOT}/include
)

if (NOT (CMAKE_VERSION VERSION_LESS 3.0))
    set(PYLINK pybind11::embed)
else()
    set(PYLINK ${PYTHON_LIBRARIES})
endif()

target_link_libraries(opae-io
    PRIVATE vfiocpp-static rt dl ${CMAKE_THREAD_LIBS_INIT} util ${PYLINK} ${libedit_LIBRARIES}
)

target_compile_definitions(opae-io
    PRIVATE LIBVFIO_EMBED
)

install(TARGETS opae-io
        RUNTIME DESTINATION bin
        COMPONENT opae.io)
