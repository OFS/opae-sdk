## Copyright(c) 2017, Intel Corporation
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

set(ASE_LIBRARIES "" CACHE INTERNAL "ase libraries")

function(add_fpga_executable)
    list(GET ARGV 0 target_name)
    if (BUILD_LIBFPGA)
        add_executable(${ARGV})
        target_link_libraries(${target_name} fpga)
    endif(BUILD_LIBFPGA)
    if (BUILD_ASE)
        list(REMOVE_AT ARGV 0)
        add_executable(${target_name}-ASE ${ARGV})
    endif(BUILD_ASE)
endfunction(add_fpga_executable)

function(add_fpga_library)
    list(GET ARGV 0 target_name)
    if (BUILD_LIBFPGA)
        add_library(${ARGV})
        target_link_libraries(${target_name} fpga)
    endif(BUILD_LIBFPGA)
    if (BUILD_ASE)
        list(REMOVE_AT ARGV 0)
        add_library(${target_name}-ASE ${ARGV})
    endif(BUILD_ASE)
endfunction(add_fpga_library)

function(fpga_target_link_libraries)
    list(GET ARGV 0 target_name)
    list(REMOVE_AT ARGV 0)
    list(LENGTH ARGV length)
    list(FIND ARGV "FPGA_LIBS" fpga_libs_index)
    set(fpga_libs "")
    set(ase_libs "")
    if (fpga_libs_index GREATER -1)
        math(EXPR start "${fpga_libs_index}+1")
        math(EXPR end   "${length}-1")
        foreach(idx RANGE ${start} ${end})
            list(GET ARGV ${idx} item)
            list(APPEND fpga_libs ${item})
            list(APPEND ase_libs ${item}-ASE)
        endforeach()
        list(REMOVE_ITEM ARGV FPGA_LIBS ${fpga_libs})
    endif()
    if (BUILD_LIBFPGA)
        target_link_libraries(${target_name} ${ARGV} ${fpga_libs})
    endif(BUILD_LIBFPGA)
    if (BUILD_ASE)
        target_link_libraries(${target_name}-ASE ${ARGV} ${ase_libs})
    endif(BUILD_ASE)
endfunction(fpga_target_link_libraries)


get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS) 
if ("${LIB64}" STREQUAL "TRUE")
    set(FPGA_FUNCTIONS_LIB_DIR "lib64")
else()
    set(FPGA_FUNCTIONS_LIB_DIR "lib")
endif() 

macro(set_install_rpath target_name)
    set_target_properties(${target_name} PROPERTIES INSTALL_RPATH "\$ORIGIN/../${FPGA_FUNCTIONS_LIB_DIR}"
                                                    INSTALL_RPATH_USE_LINK_PATH TRUE
                                                    SKIP_BUILD_RPATH FALSE
                                                    BUILD_WITH_INSTALL_RPATH FALSE)
endmacro(set_install_rpath target_name)
