## Copyright(c) 2018, Intel Corporation
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
project(fpgadiag)

set(PYFILES
    setup.py
    test_fpgadiag.py
    opae/__init__.py
    opae/utils/__init__.py
    opae/utils/utils.py
    opae/utils/byteutils.py
    opae/utils/csr.py
    opae/tools/__init__.py
    opae/tools/fpgadiag/__init__.py
    opae/tools/fpgadiag/diagtest.py
    opae/tools/fpgadiag/nlb.py
    opae/tools/fpgadiag/nlb0.py
    opae/tools/fpgadiag/nlb3.py
    opae/tools/fpgadiag/nlb7.py
    opae/tools/fpgadiag/fpgadiag.py
    )

set (PYDIST_STAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/stage)

configure_file(README.md ${PYDIST_STAGE_DIR}/README.md @ONLY)
foreach(pyfile ${PYFILES})
    configure_file(${pyfile} ${PYDIST_STAGE_DIR}/${pyfile} @ONLY)
endforeach(pyfile ${PYFILES})

add_custom_target(pyfpgadiag-dist ALL
    COMMAND ${PYTHON_EXECUTABLE} setup.py sdist
    COMMAND ${PYTHON_EXECUTABLE} setup.py bdist_wheel
    DEPENDS ${PYFILES}
    WORKING_DIRECTORY ${PYDIST_STAGE_DIR}
    COMMENT "Building Python distrubutions of pyfpgadiag")

install(CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} -m pip install --user dist/opae.fpgadiag-${INTEL_FPGA_API_VERSION}.tar.gz
                              WORKING_DIRECTORY ${PYDIST_STAGE_DIR})"
        COMPONENT toolfpgadiagapps)
