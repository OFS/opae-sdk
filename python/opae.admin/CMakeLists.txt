## Copyright(c) 2019, Intel Corporation
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
cmake_minimum_required (VERSION 2.8)

# projectname is the same as the main-executable
project(opae.admin)
set(OPAE_ADMIN_RPM_RELEASE 1)
set (ADMIN_STAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/stage)
find_program (RPM_BUILD
    rpmbuild
    )

configure_file(setup.py.in ${CMAKE_CURRENT_SOURCE_DIR}/setup.py @ONLY)
configure_file(setup.py.in ${ADMIN_STAGE_DIR}/setup.py @ONLY)
configure_file(tox.ini ${ADMIN_STAGE_DIR}/tox.ini @ONLY)
configure_file(py27-nose2.cfg ${ADMIN_STAGE_DIR}/py27-nose2.cfg @ONLY)
configure_file(py36-nose2.cfg ${ADMIN_STAGE_DIR}/py36-nose2.cfg @ONLY)

add_custom_target(opae.admin)

add_custom_command(TARGET opae.admin PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_CURRENT_SOURCE_DIR}/opae ${CMAKE_CURRENT_BINARY_DIR}/stage/opae
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_CURRENT_SOURCE_DIR}/tests ${CMAKE_CURRENT_BINARY_DIR}/stage/tests
                   DEPENDS opae tests
                   COMMENT "Setting up staging area")


add_custom_target(opae.admin.src
    COMMAND ${PYTHON_EXECUTABLE} setup.py sdist
    WORKING_DIRECTORY ${ADMIN_STAGE_DIR}
    COMMENT "Building opae.admin Python source distribution"
    )
add_custom_target(opae.admin.wheel
    COMMAND ${PYTHON_EXECUTABLE} setup.py bdist_wheel --universal
    WORKING_DIRECTORY ${ADMIN_STAGE_DIR}
    COMMENT "Building opae.admin Python wheel distribution"
    )
add_custom_target(opae.admin.tox
    COMMAND ${PYTHON_EXECUTABLE} -m tox opae
    DEPENDS opae.admin
    WORKING_DIRECTORY ${ADMIN_STAGE_DIR}
    COMMENT "Running tox on opae.admin"
    )

if (RPM_BUILD)
    add_custom_target(opae.admin.rpm
        COMMAND ${PYTHON_EXECUTABLE} setup.py bdist_rpm --release ${OPAE_ADMIN_RPM_RELEASE}
        WORKING_DIRECTORY ${ADMIN_STAGE_DIR}
        COMMENT "Building opae.admin Python RPM distribution"
        )
endif (RPM_BUILD)

