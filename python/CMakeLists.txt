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

project(python)

############################################################################
## Find Python by version     ##############################################
############################################################################
set(OPAE_PYTHON_VERSION 3.6 CACHE STRING "Python version to use for building/distributing pyopae")
set_property(CACHE OPAE_PYTHON_VERSION PROPERTY STRINGS 2.7 3.6 3.5 3.4 3.3)

find_package(PythonInterp ${OPAE_PYTHON_VERSION})
find_package(PythonLibs ${OPAE_PYTHON_VERSION})

set(INTEL_SECURITY_TOOLS_VERSION 1.0.3 CACHE STRING "Security Tools Version string")

add_subdirectory(opae.admin)

execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import virtualenv"
    RESULT_VARIABLE HAS_VIRTUALENV)

if (HAS_VIRTUALENV)
    message("no virtualenv found, use 'pip install virtualenv' to install it")
else(HAS_VIRTUALENV)
    add_custom_target(pydev
        COMMAND ${PYTHON_EXECUTABLE} -m virtualenv ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/bin/pip install -e ${CMAKE_CURRENT_SOURCE_DIR}/opae.admin
        COMMAND echo "Please source ${CMAKE_CURRENT_BINARY_DIR}/bin/activate to use this virtual environment"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Setting up virtualenv..."
        )
endif(HAS_VIRTUALENV)
