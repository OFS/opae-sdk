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

project(sphinx)

# configured documentation tools and intermediate build results
set(BINARY_BUILD_DIR "${CMAKE_BINARY_DIR}/sphinx/_build")

# Sphinx cache with pickled ReST documents
set(SPHINX_CACHE_DIR "${CMAKE_BINARY_DIR}/sphinx/_doctrees")

# Documentation number
if(HASH_ARCHIVES)
    set(SPHINX_DOC_RELEASE "${OPAE_VERSION}.${OPAE_GIT_COMMIT_HASH}")
else()
  set(SPHINX_DOC_RELEASE "${OPAE_VERSION}")
endif()

# HTML output directory
set(SPHINX_HTML_DIR "${CMAKE_BINARY_DIR}/sphinx/html/${SPHINX_DOC_RELEASE}")

# MAN output directory
set(SPHINX_MAN_DIR "${CMAKE_BINARY_DIR}/sphinx/man/${SPHINX_DOC_RELEASE}")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in"
  "${BINARY_BUILD_DIR}/conf.py"
  @ONLY)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/index.rst.in"
  "${BINARY_BUILD_DIR}/index.rst"
  @ONLY)

add_custom_target(copy-doxygen-files ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/doc/xml ${BINARY_BUILD_DIR}/doxygen_xml
  DEPENDS doxygen)

add_custom_target(copy-markdown-files ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/doc/src ${BINARY_BUILD_DIR}/docs)

add_custom_target(copy-include-files ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/opae-libs/include ${BINARY_BUILD_DIR}/docs/fpga_api/include
  DEPENDS copy-markdown-files)

add_custom_target(copy-sample-files ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/samples ${BINARY_BUILD_DIR}/docs/fpga_api/samples
  DEPENDS copy-include-files)

add_custom_target(copy-python-files ALL
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_SOURCE_DIR}/opae-libs/pyopae/README.md
  ${BINARY_BUILD_DIR}/docs/pyopae/README.md
  DEPENDS copy-markdown-files)


add_custom_target(docs ALL
  COMMAND ${CMAKE_COMMAND} -E env "PYTHONPATH=${LIBRARY_OUTPUT_PATH}/python${OPAE_PYTHON_VERSION}"	
  ${SPHINX_EXECUTABLE}
  -q -b html
  -c "${BINARY_BUILD_DIR}"
  -d "${SPHINX_CACHE_DIR}"
  "${BINARY_BUILD_DIR}"
  "${SPHINX_HTML_DIR}"
  WORKING_DIRECTORY "${BINARY_BUILD_DIR}"
  COMMENT "Building HTML documentation with Sphinx")

add_custom_target(manpages ALL
  COMMAND ${CMAKE_COMMAND} -E env "PYTHONPATH=${LIBRARY_OUTPUT_PATH}/python${OPAE_PYTHON_VERSION}"	
  ${SPHINX_EXECUTABLE}
  -q -b man
  -c "${BINARY_BUILD_DIR}"
  -d "${SPHINX_CACHE_DIR}"
  "${BINARY_BUILD_DIR}"
  "${SPHINX_MAN_DIR}"
  WORKING_DIRECTORY "${BINARY_BUILD_DIR}"
  COMMENT "Building manpages with Sphinx")

add_dependencies(docs
  copy-doxygen-files
  copy-markdown-files
  copy-include-files
  copy-sample-files
  copy-python-files
  doxygen
  _opae)
