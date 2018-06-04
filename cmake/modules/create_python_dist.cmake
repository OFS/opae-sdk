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
function(create_python_dist)
    set(options BUILD_WHEEL)
    set(one_value_args
        TARGET MODULE
        PACKAGE
        PY_SETUP_COMMAND
        BUILD_COMMAND
        BUILD_INCLUDE_DIRS
        BUILD_LIBRARY_DIRS)
    set(multi_value_args
        DEPENDS
        CONFIGURE_TYPES
        COPY_FILES
        CONFIGURE_FILES)

    cmake_parse_arguments(create_python_dist "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})
    if (NOT create_python_dist_CONFIGURE_TYPES)
        set(create_python_dist_CONFIGURE_TYPES .py .json .JSON)
    endif (NOT create_python_dist_CONFIGURE_TYPES)

    if (create_python_dist_PACKAGE)
        # check that the namespace package has a __init__.py file
        # we will only check at this level - any subdirectories missing this
        # may be invalid nested namespace packages
        if (NOT EXISTS
		${CMAKE_CURRENT_SOURCE_DIR}/${create_python_dist_PACKAGE}/__init__.py)
            message(SEND_ERROR "${mod_or_pkg} is a directory but missing __init__.py")
        endif (NOT EXISTS
		${CMAKE_CURRENT_SOURCE_DIR}/${create_python_dist_PACKAGE}/__init__.py)
        # stage a files in this namespace package in the binary tree
        file(GLOB_RECURSE py_src
            RELATIVE
            ${CMAKE_CURRENT_SOURCE_DIR}/${create_python_dist_PACKAGE} *)
        foreach(py_file ${py_src})
            # check if the file is a text file we want to run through configure_file
            get_filename_component(file_ext ${py_file} EXT)
            list(FIND create_python_dist_CONFIGURE_TYPES ${file_ext} list_index)
            if (${list_index} GREATER -1)
                configure_file(
                    ${create_python_dist_PACKAGE}/${py_file}
                    ${CMAKE_CURRENT_BINARY_DIR}/${create_python_dist_PACKAGE}/${py_file}
                    @ONLY)
            else (${list_index} GREATER -1)
                # we don't configure the file, simply copy to the binary staging area
                # also, .pyc files may exist in the source tree if a user runs Python
                # modules from there so let's not copy those
                if (NOT ${file_ext} STREQUAL ".pyc")
                    file(COPY ${create_python_dist_PACKAGE}/${py_file}
                         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/${create_python_dist_PACKAGE})
                endif (NOT ${file_ext} STREQUAL ".pyc")
            endif (${list_index} GREATER -1)
        endforeach(py_file ${py_src})
    endif (create_python_dist_PACKAGE)

    if (create_python_dist_MODULE)
        configure_file(
            ${create_python_dist_MODULE}
            ${CMAKE_CURRENT_BINARY_DIR}/${create_python_dist_MODULE}
            @ONLY)
    endif (create_python_dist_MODULE)

    configure_file(
        setup.py
        ${CMAKE_CURRENT_BINARY_DIR}/setup.py
        @ONLY)

    set(copy_list "")
    foreach(to_copy ${create_python_dist_COPY_FILES})
        add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${to_copy}
            COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_SOURCE_DIR}/${to_copy}
                    ${CMAKE_CURRENT_BINARY_DIR}/${to_copy}
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${to_copy})
        set(copy_list ${CMAKE_CURRENT_BINARY_DIR}/${to_copy} ${copy_list})
    endforeach(to_copy ${create_python_dist_COPY_FILES})

    if (create_python_dist_BUILD_INCLUDE_DIRS)
        set(py_include_dirs "--include-dirs=${create_python_dist_BUILD_INCLUDE_DIRS}")
    endif (create_python_dist_BUILD_INCLUDE_DIRS)

    if (create_python_dist_BUILD_LIBRARY_DIRS)
        set(py_library_dirs "--library-dirs=${create_python_dist_BUILD_LIBRARY_DIRS}")
    endif (create_python_dist_BUILD_LIBRARY_DIRS)

    if ("${create_python_dist_BUILD_COMMAND}" STREQUAL "build_ext")
        set(py_rpath "--rpath=${LIBRARY_OUTPUT_PATH}")
    endif ("${create_python_dist_BUILD_COMMAND}" STREQUAL "build_ext")

    add_custom_command(
        OUTPUT  build
        COMMAND ${PYTHON_VIRTUALENV} setup.py ${create_python_dist_BUILD_COMMAND} ${py_include_dirs} ${py_library_dirs} ${py_rpath}
        COMMAND ${PYTHON_VIRTUALENV} setup.py develop
        DEPENDS ${create_python_dist_DEPENDS} ${create_python_dist_COPY_FILES} ${copy_list}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Making development build")


    set(depends_list ${CMAKE_CURRENT_BINARY_DIR}/build)

    if (create_python_dist_BUILD_WHEEL)
        add_custom_command(
            OUTPUT dist
            COMMAND ${PYTHON_VIRTUALENV} setup.py bdist_wheel
            DEPENDS ${create_python_dist_DEPENDS} build
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

        set(depends_list ${depends_list} ${CMAKE_CURRENT_BINARY_DIR}/dist)
    endif (create_python_dist_BUILD_WHEEL)

    add_custom_target(${create_python_dist_TARGET} ALL
        DEPENDS ${depends_list})

endfunction(create_python_dist)
