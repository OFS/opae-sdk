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

    if(${LSB_DISTRIBUTOR_ID} STREQUAL "Ubuntu")

        if(${LSB_RELEASE_NUMBER} STREQUAL "16.04")
            set(JSON_C_DEBIAN_PACKAGE "libjson-c2")
        elseif(${LSB_RELEASE_NUMBER} STREQUAL "18.04" OR ${LSB_RELEASE_NUMBER} STREQUAL "19.04")
            set(JSON_C_DEBIAN_PACKAGE "libjson-c3")
        elseif(${LSB_RELEASE_NUMBER} STREQUAL "19.10" OR ${LSB_RELEASE_NUMBER} STREQUAL "20.04")
            set(JSON_C_DEBIAN_PACKAGE "libjson-c4")
        else()
            message(WARNING "Unrecognized Ubuntu version: ${LSB_RELEASE_NUMBER}. Defaulting to ${JSON_C_DEBIAN_PACKAGE}")
        endif()

    endif(${LSB_DISTRIBUTOR_ID} STREQUAL "Ubuntu")
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



macro(CREATE_PYTHON_EXE EXE_NAME MAIN_MODULE)
    message(WARNING "This macro will be deprecated in a future release.")

    file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
    set(PACKAGER_BIN ${PROJECT_BINARY_DIR}/bin/${EXE_NAME})

    # Generate a __main__.py that loads the target module
    set(BUILD_DIR_MAIN "${CMAKE_CURRENT_BINARY_DIR}/${EXE_NAME}_main")
    file(MAKE_DIRECTORY "${BUILD_DIR_MAIN}")
    file(WRITE "${BUILD_DIR_MAIN}/__main__.py"
        "import sys\n"
        "from ${MAIN_MODULE} import main\n"
        "if __name__ == '__main__':\n"
        "    sys.exit(main())\n")

    # Generate a Python script to zip the sources.
    #   *** We could use writepy() for Python files, but this introduces the
    #   *** potential for compatibility problems, especially in RPMs.
    file(WRITE "${BUILD_DIR_MAIN}/do_zip.py"
        "import os\n"
        "import stat\n"
        "import zipfile\n"
        "from io import BytesIO\n"
        "\n"
        "# Write to a buffer so that the shebang can be prepended easily\n"
        "wr_buf = BytesIO()\n"
        "wr_buf.write('#!/usr/bin/env python{0}'.format(os.linesep).encode())\n"
        "\n"
        "z = zipfile.PyZipFile(wr_buf, 'w')\n")

    # Emit the list of files to include in the zipped file.  Entries in the ${ARGN}
    # list may either be actual names of files to zip or entries may be the names
    # of sub-lists.  The sub-lists are tuples, holding the path to the file to zip
    # and the name to call the file in the zipped file.
    foreach(PYFILE ${ARGN})
        # Is this entry a list or a file?
        if (DEFINED ${PYFILE})
            # It's a list.  Extract the source path and the name to call the
            # file inside the zipped file.
            list(GET ${PYFILE} 0 F_PATH)
            list(GET ${PYFILE} 1 Z_NAME)
            file(APPEND "${BUILD_DIR_MAIN}/do_zip.py"
                 "z.write('${F_PATH}', '${Z_NAME}')\n")
        else()
            # Entry is just a file name.
            file(APPEND "${BUILD_DIR_MAIN}/do_zip.py"
                 "z.write('${PYFILE}')\n")
        endif()
    endforeach(PYFILE)

    file(APPEND "${BUILD_DIR_MAIN}/do_zip.py"
        "z.write('${BUILD_DIR_MAIN}/__main__.py', '__main__.py')\n"
        "z.close()\n"
        "\n"
        "# Write out the buffer\n"
        "with open('${PACKAGER_BIN}', 'wb') as f:\n"
        "  f.write(wr_buf.getvalue())\n"
        "  # Mark the file executable\n"
        "  mode = os.fstat(f.fileno()).st_mode\n"
        "  mode |= stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH\n"
        "  os.fchmod(f.fileno(), stat.S_IMODE(mode))\n"
        "\n"
        "f.close()\n")

    # Run Python to generate the zipped file
    execute_process(COMMAND ${PYTHON_EXECUTABLE} "${BUILD_DIR_MAIN}/do_zip.py"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

endmacro(CREATE_PYTHON_EXE)
