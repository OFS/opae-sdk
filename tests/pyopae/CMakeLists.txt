## Copyright(c) 2020-2023, Intel Corporation
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

try_compile(SUPPORTS_EMBEDDED_PYTHON
    ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test_embed.cpp
    CMAKE_FLAGS
    "-DINCLUDE_DIRECTORIES=${Python3_INCLUDE_DIRS};${pybind11_INCLUDE_DIRS}"
    LINK_LIBRARIES ${Python3_LIBRARIES}
    OUTPUT_VARIABLE TRY_COMPILE_OUTPUT
)

if (SUPPORTS_EMBEDDED_PYTHON)
    set(SRC
         test_pyopae.cpp
	 ${OPAE_LIB_SOURCE}/pyopae/opae.cpp
	 ${OPAE_LIB_SOURCE}/pyopae/pycontext.h
	 ${OPAE_LIB_SOURCE}/pyopae/pycontext.cpp
	 ${OPAE_LIB_SOURCE}/pyopae/pyproperties.h
	 ${OPAE_LIB_SOURCE}/pyopae/pyproperties.cpp
	 ${OPAE_LIB_SOURCE}/pyopae/pyhandle.h
	 ${OPAE_LIB_SOURCE}/pyopae/pyhandle.cpp
	 ${OPAE_LIB_SOURCE}/pyopae/pytoken.h
	 ${OPAE_LIB_SOURCE}/pyopae/pytoken.cpp
	 ${OPAE_LIB_SOURCE}/pyopae/pyshared_buffer.h
	 ${OPAE_LIB_SOURCE}/pyopae/pyshared_buffer.cpp
	 ${OPAE_LIB_SOURCE}/pyopae/pyevents.h
	 ${OPAE_LIB_SOURCE}/pyopae/pyevents.cpp
	 ${OPAE_LIB_SOURCE}/pyopae/pyerrors.h
	 ${OPAE_LIB_SOURCE}/pyopae/pyerrors.cpp
	 ${OPAE_LIB_SOURCE}/pyopae/pysysobject.h
	 ${OPAE_LIB_SOURCE}/pyopae/pysysobject.cpp
    )
    if(OPAE_ENABLE_MOCK)
        set(SRC ${SRC} ${opae-test_ROOT}/framework/mock/opae_mock.cpp)
    else(OPAE_ENABLE_MOCK)
        set(SRC ${SRC} ${opae-test_ROOT}/framework/mock/opae_std.c)
    endif(OPAE_ENABLE_MOCK)

    add_executable(test_pyopae ${SRC})
    target_compile_definitions(test_pyopae PRIVATE
        OPAE_EMBEDDED)
    target_include_directories(test_pyopae
        PRIVATE ${pybind11_INCLUDE_DIRS}
        PRIVATE ${Python3_INCLUDE_DIRS}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE ${opae-test_ROOT}/framework)

    target_link_libraries(test_pyopae
        PUBLIC
            opae-c
            opae-cxx-core
            ${OPAE_TEST_LIBRARIES}
            ${json-c_LIBRARIES}
            ${Python3_LIBRARIES})

    macro(add_pyopae_test pytest)
        add_custom_command(TARGET test_pyopae
           POST_BUILD
           COMMAND ${CMAKE_COMMAND} -E copy
           ${CMAKE_CURRENT_SOURCE_DIR}/${pytest}
           ${CMAKE_BINARY_DIR}
           )
        add_test(
            NAME ${pytest}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND $<TARGET_FILE:test_pyopae> test ${pytest}
        )
    endmacro(add_pyopae_test pytest)

    add_pyopae_test(test_properties.py)
    add_pyopae_test(test_shared_buffers.py)
    add_pyopae_test(test_sysobject.py)

else(SUPPORTS_EMBEDDED_PYTHON)
    message(WARNING
        "Could not compile embedded Python. See errors in embed_errors.txt")
    file(WRITE ${CMAKE_BINARY_DIR}/embed_errors.txt ${TRY_COMPILE_OUTPUT})
endif(SUPPORTS_EMBEDDED_PYTHON)
