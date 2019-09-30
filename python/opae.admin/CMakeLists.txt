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
set(OPAE_ADMIN_RPM_RELEASE 3)
set (ADMIN_STAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/stage)
find_program (RPM_BUILD
    rpmbuild
    )
find_program (DEB_BUILD
    debuild
    )

configure_file(setup.py.in ${CMAKE_CURRENT_SOURCE_DIR}/setup.py @ONLY)
configure_file(setup.py.in ${ADMIN_STAGE_DIR}/setup.py @ONLY)
configure_file(opae/admin/version.py.in ${CMAKE_CURRENT_SOURCE_DIR}/opae/admin/version.py @ONLY)
configure_file(opae/admin/version.py.in ${ADMIN_STAGE_DIR}/opae/admin/version.py @ONLY)
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
        DEPENDS opae.admin
        WORKING_DIRECTORY ${ADMIN_STAGE_DIR}
        COMMENT "Building opae.admin Python RPM distribution"
        )
endif (RPM_BUILD)

if (DEB_BUILD)
    add_custom_target(opae.admin.deb.tarball)
    add_custom_command(TARGET opae.admin.deb.tarball PRE_BUILD
                       COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/opae ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION}/opae
                       COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/tests ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION}/tests
                       COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/setup.py ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION}/setup.py
                       COMMAND find ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION} -type f -name '*.pyc' -delete
                       COMMAND find ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION} -type d -name '__pycache__' -delete
                       COMMAND find ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION} -type f -name 'version.py.in' -delete
                       COMMAND tar -C ${CMAKE_CURRENT_BINARY_DIR}/deb -zcf ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin_${INTEL_SECURITY_TOOLS_VERSION}.orig.tar.gz opae.admin-${INTEL_SECURITY_TOOLS_VERSION}
                       DEPENDS opae tests
                       COMMENT "Setting up debian tarball for Python")

    add_custom_target(opae.admin.deb.debian
                      DEPENDS opae.admin.deb.tarball)
    add_custom_command(TARGET opae.admin.deb.debian PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION}/debian
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/debian/deb-changelog ${INTEL_SECURITY_TOOLS_VERSION} >${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION}/debian/changelog
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/debian/compat ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION}/debian/compat
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/debian/control ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION}/debian/control
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/debian/copyright ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION}/debian/copyright
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/debian/rules ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION}/debian/rules
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION}/debian/source
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/debian/format ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION}/debian/source/format
        DEPENDS debian/changelog.in debian/compat debian/control debian/copyright debian/rules debian/format
        COMMENT "Setting up debuild directory")

    add_custom_target(opae.admin.deb
        COMMAND ${DEB_BUILD} -us -uc
        DEPENDS opae.admin.deb.debian
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/deb/opae.admin-${INTEL_SECURITY_TOOLS_VERSION}
        COMMENT "Building opae.admin Python DEB distribution"
        )

endif (DEB_BUILD)
