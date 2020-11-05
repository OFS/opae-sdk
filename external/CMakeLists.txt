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

cmake_minimum_required(VERSION 2.8.12)

if(OPAE_BUILD_TESTS)
    opae_external_project_add(PROJECT_NAME opae-test
                              GIT_URL https://github.com/OPAE/opae-test.git
                              GIT_TAG ${OPAE_TEST_TAG}
                              PRESERVE_REPOS ${OPAE_PRESERVE_REPOS})
endif(OPAE_BUILD_TESTS)

if(OPAE_BUILD_SIM)
    opae_external_project_add(PROJECT_NAME opae-sim
                              GIT_URL https://github.com/OPAE/opae-sim.git
                              GIT_TAG ${OPAE_SIM_TAG}
                              PRESERVE_REPOS ${OPAE_PRESERVE_REPOS})
endif(OPAE_BUILD_SIM)

if(OPAE_BUILD_LIBOPAE_PY)
    if(${CMAKE_VERSION} VERSION_LESS "3.4.0" AND ${PYTHONLIBS_VERSION_STRING} VERSION_LESS "3.9.0")
        set(PYBIND11_TAG "v2.4.3")
    else()
    # Otherwise, pull recent pybind11 tag to enable Python 3.9 support.
        set(PYBIND11_TAG "v2.6.0")
    endif()
    message(STATUS "Using pybind11 ${PYBIND11_TAG}")
    opae_external_project_add(PROJECT_NAME pybind11
                              GIT_URL https://github.com/pybind/pybind11.git
                              GIT_TAG "${PYBIND11_TAG}"
    )

endif(OPAE_BUILD_LIBOPAE_PY)
