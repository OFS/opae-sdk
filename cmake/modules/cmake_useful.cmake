#!/usr/bin/cmake -P
## Copyright(c) 2017-2018, Intel Corporation
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

# Create rule for obtain one file by copying another one
function(rule_copy_file target_file source_file)
    add_custom_command(OUTPUT ${target_file}
        COMMAND cp -p ${source_file} ${target_file}
        DEPENDS ${source_file})
endfunction(rule_copy_file target_file source_file)

#  rule_copy_source([source_dir] file ...)
#
# Create rule for obtain file(s) in binary tree by copiing it from source tree.
#
# Files are given relative to ${source_dir}, if it is set, or
# relative to ${CMAKE_CURRENT_SOURCE_DIR}.
#
# Files will be copied into ${CMAKE_CURRENT_BINARY_DIR} with same
# relative paths.
#
# ${source_dir} should be absolute path(that is, starts from '/').
# Otherwise first argument is treated as first file to copy.
function(rule_copy_source file)
    string(REGEX MATCH "^/" is_abs_path ${file})
    if(is_abs_path)
        set(source_dir ${file})
        set(files ${ARGN})
    else(is_abs_path)
        set(source_dir ${CMAKE_CURRENT_SOURCE_DIR})
        set(files ${file} ${ARGN})
    endif(is_abs_path)

    foreach(file_real ${files})
        rule_copy_file("${CMAKE_CURRENT_BINARY_DIR}/${file_real}"
            ${source_dir}/${file_real})
    endforeach(file_real ${files})
endfunction(rule_copy_source file)

#  to_abs_path(output_var path [...])
#
# Convert relative path of file to absolute path:
# use path in source tree, if file already exist there.
# otherwise use path in binary tree.
# If initial path already absolute, return it.
function(to_abs_path output_var)
    set(result)
    foreach(path ${ARGN})
        string(REGEX MATCH "^/" _is_abs_path ${path})
        if(_is_abs_path)
            list(APPEND result ${path})
        else(_is_abs_path)
            file(GLOB to_abs_path_file
                "${CMAKE_CURRENT_SOURCE_DIR}/${path}"
            )
            if(NOT to_abs_path_file)
                set (to_abs_path_file "${CMAKE_CURRENT_BINARY_DIR}/${path}")
            endif(NOT to_abs_path_file)
            list(APPEND result ${to_abs_path_file})
        endif(_is_abs_path)
    endforeach(path ${ARGN})
    set("${output_var}" ${result} PARENT_SCOPE)
endfunction(to_abs_path output_var path)

#  is_path_inside_dir(output_var dir path)
#
# Set output_var to true if path is absolute path inside given directory.
# NOTE: Path should be absolute.
macro(is_path_inside_dir output_var dir path)
    file(RELATIVE_PATH _rel_path ${dir} ${path})
    string(REGEX MATCH "^\\.\\." _is_not_inside_dir ${_rel_path})
    if(_is_not_inside_dir)
        set(${output_var} "FALSE")
    else(_is_not_inside_dir)
        set(${output_var} "TRUE")
    endif(_is_not_inside_dir)
endmacro(is_path_inside_dir output_var dir path)

# Write given content to the file.
#
# If file is already exists and its content is same as written one, file
# is not rewritten, so its write timestamp remains unchanged.
function(file_update filename content)
    if(EXISTS "${filename}")
        file(READ "${filename}" old_content)
        if(old_content STREQUAL "${content}")
            return()
        endif(old_content STREQUAL "${content}")
    endif(EXISTS "${filename}")
    # File doesn't exists or its content differ.
    file(WRITE "${filename}" "${content}")
endfunction(file_update filename content)

# Write given content to the file in APPEND mode.
#
# For append we use position variable <pos> which should be initialized
# to 0 for every new build process and which is updated every time when
# this function is called.
# If file is already exists and its content at given <pos> is same as
# written one, file is not rewritten, so its write timestamp remains unchanged.
#
# Note, that file will not be rewritten if new build process issues less
# APPEND actions than one which create file.
# But similar problem exists for file_update() and even for built-in
# configure_file() command: file will not be removed if new build process
# do not call configure_file() for it.
function(file_update_append filename content POS)
    set(pos "${${POS}}")
    string(LENGTH "${content}" len)
    # Update output variable first.
    # This allows to use return() when need not to do anything
    math(EXPR pos_new "${pos}+${len}")
    set(${POS} "${pos_new}" PARENT_SCOPE)
    if(EXISTS "${filename}")
        file(READ "${filename}" old_content LIMIT "${len}" OFFSET "${pos}")
        if(old_content STREQUAL "${content}")
            return()
        elseif(old_content STREQUAL "")
            file(APPEND "${filename}" "${content}")
            return()
        endif(old_content STREQUAL "${content}")
    else(EXISTS "${filename}")
        if(NOT pos EQUAL 0)
            message(FATAL_ERROR "Appending to non-zero position to non-existent file.")
        endif(NOT pos EQUAL 0)
    endif(EXISTS "${filename}")
    # File doesn't exists or its content differ.
    if(NOT pos EQUAL 0)
        file(READ "${filename}" prefix LIMIT "${pos}")
    else(NOT pos EQUAL 0)
        set(prefix)
    endif(NOT pos EQUAL 0)
    file(WRITE "${filename}" "${prefix}${content}")
endfunction(file_update_append filename content POS)

# Common mechanism for output status message
# when checking different aspects.
#
# Normal using:
#
#  check_begin("Checking <...>")
#  if(NOT <check-already-has-been-done>)
#     check_try() # Here message is printed
#     # Perform check(try_compile(), etc.)
#  endif(NOT <check-already-has-been-done>)
#  check_end("<check-result>") # Here message is printed with result.

# Should be called (unconditionally) when new cheking is issued.
# Note, that given @status_msg is not printed at that step.
function(check_begin status_msg)
    set(_check_status_msg ${status_msg} PARENT_SCOPE)
    set(_check_has_tries PARENT_SCOPE)
endfunction(check_begin status_msg)

# Should be called before every real checking, that is which is not come
# from cache variables comparision.
#
# First call of that function is trigger printing of @status_msg,
# passed to check_begin().
function(check_try)
    if(NOT _check_has_tries)
        message(STATUS "${_check_status_msg}")
        set(_check_has_tries "1" PARENT_SCOPE)
    endif(NOT _check_has_tries)
endfunction(check_try)

# Should be called when cheking is end, and @result_msg should be short
# description of check result.
# If any check_try() has been issued before,
#   "@status_msg - @result_msg"
# will be printed.
# Otherwise,
#   "@status_msg - [cached] @result_msg"
# will be printed, at it will be the only message for that check.
function(check_end result_msg)
    if(NOT _check_has_tries)
        message(STATUS "${_check_status_msg} [cached] - ${result_msg}")
    else(NOT _check_has_tries)
        message(STATUS "${_check_status_msg} - ${result_msg}")
    endif(NOT _check_has_tries)
    set(_check_status_msg PARENT_SCOPE)
    set(_check_has_tries PARENT_SCOPE)
endfunction(check_end result_msg)

#  set_bool_string(var true_string false_string value [CACHE ... | PARENT_SCOPE])
#
# Set variable to one of two string according to true property of some value.
#
# If 'value' is true-evaluated, 'var' will be set to 'true_string',
# otherwise to 'var' will be set to 'false_string'.
# Like standard set(), macro accept CACHE and PARENT_SCOPE modifiers.
#
# Useful for form meaningful value for cache variables, contained result
# of some operation.
# Also may be used for form message for check_end().
macro(set_bool_string var true_string false_string value)
    # Macro parameters are not a variables, so them cannot be tested
    # using 'if(value)'.
    # Usage 'if(${value})' leads to warnings since 2.6.4 when ${val}
    # is boolean constant
    # (because until 2.6.4 'if(value)' always dereference val).
    # So copy 'value' to local variable for test it.
    set(_set_bool_string_value ${value})
    if(_set_bool_string_value)
        set(${var} "${true_string}" ${ARGN})
    else()
        set(${var} "${false_string}" ${ARGN})
    endif()
endmacro(set_bool_string)

#  set_zero_string(var true_string false_string value [CACHE ... | PARENT_SCOPE])
#
# Set variable to one of two string according to zero property of some value.
#
# If 'value' is '0' (presizely), 'var' will be set to 'zero_string',
# otherwise to 'var' will be set to 'nonzero_string'.
# Like standard set(), macro accept CACHE and PARENT_SCOPE modifiers.
#
# Useful for form meaningful value for cache variables, contained result
# of execute_process().
# Also may be used for form message for check_end().
macro(set_zero_string var zero_string nonzero_string value)
    set(_set_zero_string_value ${value})
    if(_set_zero_string_value EQUAL "0")
        set(${var} ${zero_string} ${ARGN})
    else()
        set(${var} ${nonzero_string} ${ARGN})
    endif()
endmacro(set_zero_string)
