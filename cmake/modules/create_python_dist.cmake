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

## create_python_dist is a cmake function that can be used to create a Python
## distribution. The first argument is the build target to create to install
## the package in the build tree. The second argument is the Python module or
## Python package that is expected to exist in the current source directory.
## Currently, the package/module distinction is made if the second argument is
## a directory or not. If it is a directory, then it must contain an
## __init__.py file to be treated as a Python package. It is up to the
## developer of the package to ensure that the contents of the directory
## contain all required modules and the setup.py is correct. It is assumed that
## the name of the module or package given as the second argument is the name
## of the package listed in the setup.py file. In either case (if the input is
## a package or a module), all input files will be processed with
## 'configure_file' to replace any CMake variables that use the @ symbol
function(create_python_dist target mod_or_pkg)
    set(CONFIGURE_FILES .py .json .JSON)
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${mod_or_pkg})
        # check that the namespace package has a __init__.py file
        # we will only check at this level - any subdirectories missing this
        # may be invalid nested namespace packages
        if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${mod_or_pkg}/__init__.py)
            message(SEND_ERROR "${mod_or_pkg} is a directory but missing __init__.py")
        endif (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${mod_or_pkg}/__init__.py)
        # stage a files in this namespace package in the binary tree
        file(GLOB_RECURSE py_src
            RELATIVE
            ${CMAKE_CURRENT_SOURCE_DIR}/${mod_or_pkg}
            ${mod_or_pkg}/*)
        foreach(py_file ${py_src})
            # check if the file is a text file we want to run through configure_file
            get_filename_component(file_ext ${py_file} EXT)
            list(FIND CONFIGURE_FILES ${file_ext} list_index)
            if (${list_index} GREATER -1)
                configure_file(
                    ${mod_or_pkg}/${py_file}
                    ${CMAKE_CURRENT_BINARY_DIR}/${mod_or_pkg}/${py_file}
                    @ONLY)
            else (${list_index} GREATER -1)
                # we don't configure the file, simply copy to the binary staging area
                # also, .pyc files may exist in the source tree if a user runs Python
                # modules from there so let's not copy those
                if (NOT ${file_ext} STREQUAL ".pyc")
                    file(COPY ${mod_or_pkg}/${py_file}
                              ${CMAKE_CURRENT_BINARY_DIR}/${mod_or_pkg}/${py_file})
                endif (NOT ${file_ext} STREQUAL ".pyc")
            endif (${list_index} GREATER -1)
        endforeach(py_file ${py_src})
    else(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${mod_or_pkg})
        configure_file(
            ${mod_or_pkg}
            ${CMAKE_CURRENT_BINARY_DIR}/${mod_or_pkg}
            @ONLY)
    endif(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${mod_or_pkg})

    configure_file(
        setup.py
        ${CMAKE_CURRENT_BINARY_DIR}/setup.py
        @ONLY)

    add_custom_command(
        OUTPUT  dist
        COMMAND ${PYTHON_EXECUTABLE} setup.py sdist
        COMMAND ${PYTHON_EXECUTABLE} setup.py bdist_wheel
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})


    if(PYTHON_VIRTUALENV)
        ## install the source distribution to the virtual python environment
        add_custom_target(${target} ALL
            COMMAND ${PYTHON_VIRTUALENV} -m pip install ${CMAKE_CURRENT_BINARY_DIR}/dist/${mod_or_pkg}-${INTEL_FPGA_API_VERSION}.tar.gz -I
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dist
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    else(PYTHON_VIRTUALENV)
        ## install the source distribution to the user area
        message(WARNING "Installing ${mod_or_pkg} to user area. You may uninstall with 'pip uninstall ${mod_or_pkg}'")
        add_custom_target(${target} ALL
            COMMAND ${PYTHON_EXECUTABLE} -m pip install ${CMAKE_CURRENT_BINARY_DIR}/dist/${mod_or_pkg}-${INTEL_FPGA_API_VERSION}.tar.gz -I --user --install-option="--install-scripts=${EXECUTABLE_OUTPUT_PATH}"
            DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/dist
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
    endif(PYTHON_VIRTUALENV)


endfunction(create_python_dist mod_or_pkg setup_py)
