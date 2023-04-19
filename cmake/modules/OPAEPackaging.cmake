#!/usr/bin/cmake -P
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

set(JSON_C_DEBIAN_PACKAGE "libjson0")

find_program(LSB_RELEASE_EXE lsb_release)
if(LSB_RELEASE_EXE)
    execute_process(COMMAND ${LSB_RELEASE_EXE} -is
        OUTPUT_VARIABLE LSB_DISTRIBUTOR_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(COMMAND ${LSB_RELEASE_EXE} -rs
        OUTPUT_VARIABLE LSB_RELEASE_NUMBER
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    message(STATUS "Detecting distribution - ${LSB_DISTRIBUTOR_ID} ${LSB_RELEASE_NUMBER}")

    if("${LSB_DISTRIBUTOR_ID}" STREQUAL "" OR "${LSB_RELEASE_NUMBER}" STREQUAL "")
        set(JSON_C_DEBIAN_PACKAGE "libjson-c5")
        message(WARNING "Unrecognized distribution: Defaulting to ${JSON_C_DEBIAN_PACKAGE}")
    elseif(${LSB_DISTRIBUTOR_ID} STREQUAL "Ubuntu")

        if(${LSB_RELEASE_NUMBER} STREQUAL "16.04")
            set(JSON_C_DEBIAN_PACKAGE "libjson-c2")
        elseif(${LSB_RELEASE_NUMBER} STREQUAL "18.04" OR ${LSB_RELEASE_NUMBER} STREQUAL "19.04")
            set(JSON_C_DEBIAN_PACKAGE "libjson-c3")
        elseif(${LSB_RELEASE_NUMBER} STREQUAL "19.10" OR ${LSB_RELEASE_NUMBER} STREQUAL "20.04")
            set(JSON_C_DEBIAN_PACKAGE "libjson-c4")
        elseif(${LSB_RELEASE_NUMBER} STREQUAL "22.04")
            set(JSON_C_DEBIAN_PACKAGE "libjson-c5")
        else()
            message(WARNING "Unrecognized Ubuntu version: ${LSB_RELEASE_NUMBER}. Defaulting to ${JSON_C_DEBIAN_PACKAGE}")
        endif()

    endif()
endif(LSB_RELEASE_EXE)


function(DEFINE_PKG name)
  set(_components "COMPONENTS")

  # Parse all these entries
  set(_entries "GROUP;DISPLAY_NAME;DESCRIPTION;DEB_DEPENDS")

  # Only valid options for a component
  set(_component_entries "GROUP;DISPLAY_NAME;DESCRIPTION;DEB_DEPENDS")

  # Define parsing order
  cmake_parse_arguments(DEFINE_PKG
    ""
    "${_entries}"
    "${_components}"
    ${ARGN})

  # Iterate over 2-valued entries
  foreach(_component ${DEFINE_PKG_COMPONENTS})
    string(TOUPPER "${_component}" _component_upper)

    # Assume all entrys refer to component variables
    foreach(_entry ${_component_entries})
      if(DEFINE_PKG_${_entry})
        set(CPACK_COMPONENT_${_component_upper}_${_entry} "${DEFINE_PKG_${_entry}}"
            CACHE STRING "component" FORCE)
      endif()
    endforeach()
  endforeach()


  if(DEFINE_PKG_DEB_DEPENDS)
  string(TOUPPER "${DEFINE_PKG_GROUP}" _group_upper)
    set(CPACK_DEBIAN_${_group_upper}_PACKAGE_DEPENDS ${DEFINE_PKG_DEB_DEPENDS}
        CACHE STRING "depends" FORCE)
  endif()

  if(DEFINE_PKG_DESCRIPTION)
  string(TOUPPER "${DEFINE_PKG_GROUP}" _group_upper)
    set(CPACK_COMPONENT_${_group_upper}_DESCRIPTION ${DEFINE_PKG_DESCRIPTION}
        CACHE STRING "descr" FORCE)
  endif()


endfunction(DEFINE_PKG)


function(opae_python_install)
    set(options )
    set(oneValueArgs COMPONENT RECORD_FILE SOURCE_DIR RPM_PACKAGE)
    set(multiValueArgs)
    cmake_parse_arguments(OPAE_PYTHON_INSTALL "${options}"
        "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    string(COMPARE EQUAL
        "${OPAE_PYTHON_INSTALL_RPM_PACKAGE}" "" rpm_package_empty)
    if(NOT ${rpm_package_empty} AND "${CPACK_GENERATOR}" STREQUAL "RPM")
        if(CPACK_PACKAGE_DIRECTORY)
            set(build_root
                ${CPACK_PACKAGE_DIRECTORY}/_CPack_Packages/Linux/RPM/BUILD
            )
        else(CPACK_PACKAGE_DIRECTORY)
            set(build_root
                ${CMAKE_BINARY_DIR}/_CPack_Packages/Linux/RPM/BUILD
            )
        endif(CPACK_PACKAGE_DIRECTORY)
        set(APPEND_CODE
            "
            file(READ
                ${CMAKE_BINARY_DIR}/${OPAE_PYTHON_INSTALL_RECORD_FILE} RECORD_FILE
            )
            file(APPEND
                ${build_root}/${OPAE_PYTHON_INSTALL_RPM_PACKAGE}-files.txt
                \"\${RECORD_FILE}\"
            )
            "
        )
    endif(NOT ${rpm_package_empty} AND "${CPACK_GENERATOR}" STREQUAL "RPM")

    # this install target generates cmake code that is executed by a make/ninja
    # target rule at install time. In that context the $DESTDIR environment
    # variable might be empty (i.e. `make install`) or set (i.e.
    # `make install DESTDIR=/mnt/root`). Avoid passing an empty variable to
    # --root= by checking (and possibly set) the $DESTDIR variable before
    # calling python setuptools.

    #install(
    #    CODE "
    #        if (\"\$ENV{DESTDIR}\" STREQUAL \"\")
    #            set(ENV{DESTDIR} /)
    #        endif()
    #        execute_process(
    #            COMMAND ${PYTHON_EXECUTABLE} setup.py install -O1 \
    #                --single-version-externally-managed \
    #                --root=\$ENV{DESTDIR} \
    #                --prefix=${CMAKE_INSTALL_PREFIX} \
    #                --record=${CMAKE_BINARY_DIR}/${OPAE_PYTHON_INSTALL_RECORD_FILE}
    #            WORKING_DIRECTORY ${OPAE_PYTHON_INSTALL_SOURCE_DIR}
    #        )
    #    ${APPEND_CODE}
    #    "
    #    COMPONENT ${OPAE_PYTHON_INSTALL_COMPONENT}
    #)

    install(
        CODE "
            if (\"\$ENV{DESTDIR}\" STREQUAL \"\")
                set(ENV{DESTDIR} /)
            endif()
            execute_process(
                COMMAND ${PYTHON_EXECUTABLE} -m pip install \
                    --root=\$ENV{DESTDIR} \
                    --prefix=${CMAKE_INSTALL_PREFIX} \
		    --no-warn-script-location .
                WORKING_DIRECTORY ${OPAE_PYTHON_INSTALL_SOURCE_DIR}
            )
        ${APPEND_CODE}
        "
        COMPONENT ${OPAE_PYTHON_INSTALL_COMPONENT}
    )

endfunction(opae_python_install)
