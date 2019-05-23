## Copyright(c) 2014-2018, Intel Corporation
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

cmake_minimum_required(VERSION 2.8.11)
include(cmake_useful)
include(CMakeParseArguments)

############################################################################
## Fetch tool and script locations #########################################
############################################################################

# find afu_sim_setup
find_program(AFU_PLATFORM_CONFIG
  NAMES
  afu_platform_config
  PATHS
  ${CMAKE_BINARY_DIR}/bin
  ${CMAKE_SOURCE_DIR}/platforms/scripts
  /usr/bin
  /usr/local/bin
  /opt/local/bin
  DOC "AFU platform configuration utility")

# find afu_sim_setup
find_program(AFU_JSON_MGR
  NAMES
  afu_json_mgr
  PATHS
  ${CMAKE_BINARY_DIR}/bin
  ${CMAKE_SOURCE_DIR}/platforms/scripts
  /usr/bin
  /usr/local/bin
  /opt/local/bin
  DOC "AFU json manager configuration utility")

############################################################################
## Global ASE variables ####################################################
############################################################################

# Default directory and model variables
set(ASE_SHARE_DIR              ${OPAE_SDK_SOURCE}/libopae/plugins/ase CACHE STRING "Directory containing shared ASE files")
set(ASE_SAMPLES                ${OPAE_SHARE_DIR}/samples)
set(ASE_SCRIPTS_IN             ${ASE_SHARE_DIR}/in)
set(ASE_SERVER_RTL             ${ASE_SHARE_DIR}/rtl               CACHE STRING "Location of ASE server SystemVerilog code")
set(ASE_DISCRETE_EMIF_MODEL    "EMIF_MODEL_BASIC"                 CACHE STRING "ASE EMIF discrete memory model")
set(ASE_TOP_ENTITY             "ase_top"                          CACHE STRING "Top entity for the ASE testbench")

# Default values for project variables (should be modified for each target!)
set(ASE_MODULE_TYPE            "AFU"                              CACHE STRING "Default ASE module type (AFU, BBB, LIBSIM)")
set(ASE_SIMULATOR              "QUESTA"                           CACHE STRING "Default ASE simulator")
set(ASE_PLATFORM               "FPGA_PLATFORM_INTG_XEON"          CACHE STRING "Default ASE platform (fullname)")
set(ASE_PLATFORM_NAME          "intg_xeon"                        CACHE STRING "Default ASE platform (abbreviation)")
set(ASE_PLATFORM_IF            "ccip_std_afu"                     CACHE STRING "Default ASE AFU interface")
set(ASE_TIMESCALE              "1ps/1ps"                          CACHE STRING "Default ASE timescale")
set(ASE_MODE                   2                                  CACHE STRING "Default ASE mode (1=Ctrl-C server kill, 2=timeout server kill)")
set(ASE_TIMEOUT                10000                              CACHE STRING "Default ASE timeout cycles")
set(ASE_USR_CLK_MHZ            312.50000                          CACHE STRING "Default ASE AFU clock frequency")

# Default AFU json metadata
set(ASE_AFU_JSON               "ccip_std_afu.json")

# Default SW library name for simulator
set(ASE_SHOBJ_NAME             "libopae-c-ase-server")
set(ASE_SHOBJ_SO               ${ASE_SHOBJ_NAME}.so)

############################################################################
## Global platform variables ###############################################
############################################################################

set(PLATFORM_SHARE_DIR         ${OPAE_BASE_DIR}/${OPAE_SHARE_DIR}/platform)
set(PLATFORM_IF_DIR            ${PLATFORM_SHARE_DIR}/platform_if)
set(PLATFORM_IF_RTL            ${PLATFORM_IF_DIR}/rtl)
set(PLATFORM_PKG_FILE          ${PLATFORM_IF_RTL}/ccip_if_pkg.sv)

############################################################################
## ASE module properties ###################################################
############################################################################

# Name of the given module, like "intg_xeon_nlb".
define_property(TARGET PROPERTY ASE_MODULE_NAME
  BRIEF_DOCS "Name of the ASE module."
  FULL_DOCS "Name of the ASE module.")

# Type of ASE module (e.g. afu, platform, interface)
define_property(TARGET PROPERTY ASE_MODULE_TYPE
  BRIEF_DOCS "Type of the ASE module (e.g. afu, platform, interface)."
  FULL_DOCS "Type of the ASE module (e.g. afu, platform, interface).")

# Name of used simulation tool (e.g. questa, vcs).
define_property(TARGET PROPERTY ASE_MODULE_SIMULATOR
  BRIEF_DOCS "SystemVerilog simulator (e.g. questa, vcs)."
  FULL_DOCS "SystemVerilog simulator (e.g. questa, vcs).")

# Simulation tool timescale (e.g. 1ps/1ps)
define_property(TARGET PROPERTY ASE_MODULE_TIMESCALE
  BRIEF_DOCS "SystemVerilog simulator timescale (e.g. 1ps/1ps)."
  FULL_DOCS "SystemVerilog simulator timescale (e.g. 1ps/1ps).")

# Absolute location of the work library for the AFU.
define_property(TARGET PROPERTY ASE_MODULE_WORKLIB_LOCATION
  BRIEF_DOCS "Location of the built ASE module work library."
  FULL_DOCS "Location of the built ASE module work library.")

# Name of the JSON file which contains the AFU metadata, defaults to ccip_std_afu.json
define_property(TARGET PROPERTY ASE_MODULE_AFU_JSON
  BRIEF_DOCS "Name of the JSON file which contains the AFU metadata, defaults to ccip_std_afu.json"
  FULL_DOCS "Name of the JSON file which contains the AFU metadata, defaults to ccip_std_afu.json")

# Absolute filename of libopae-c-ase-server.so file which is built.
define_property(TARGET PROPERTY ASE_MODULE_LIBOPAE_LOCATION
  BRIEF_DOCS "Location of the libopae-c-ase-server library of the AFU module."
  FULL_DOCS "Location of the libopae-c-ase-server library of the AFU module.")

# ASE platform name (e.g. intg_xeon, discrete, etc)
define_property(TARGET PROPERTY ASE_MODULE_PLATFORM_NAME
  BRIEF_DOCS "Platform utilized between AFU and CPU (e.g. intg_xeon, discrete_pcie3, etc)."
  FULL_DOCS "Platform utilized between AFU and CPU (e.g. intg_xeon, discrete_pcie3, etc).")

# ASE platform fullname (e.g. FPGA_PLATFORM_INTG_XEON, etc)
define_property(TARGET PROPERTY ASE_MODULE_PLATFORM_FULLNAME
  BRIEF_DOCS "Platform (fullname) utilized between AFU and CPU (e.g. FPGA_PLATFORM_INTG_XEON)."
  FULL_DOCS "Platform (fullname) utilized between AFU and CPU (e.g. FPGA_PLATFORM_INTG_XEON).")

# ASE module interface (e.g. ccip_std_afu)
define_property(TARGET PROPERTY ASE_MODULE_PLATFORM_IF
  BRIEF_DOCS "AFU platform interface (e.g. ccip_std_afu)."
  FULL_DOCS "AFU platform interface (e.g. ccip_std_afu.)")

# ASE simulator mode (e.g. 1, 2)
define_property(TARGET PROPERTY ASE_MODULE_USR_CLOCK_MHZ
  BRIEF_DOCS "ASE simulation clock frequency for AFU in MHz (e.g. 312.500)."
  FULL_DOCS "ASE simulation clock frequency for AFU in MHz (e.g. 312.500).")

# ASE simulator mode (e.g. 1, 2)
define_property(TARGET PROPERTY ASE_MODULE_ASE_MODE
  BRIEF_DOCS "ASE simulation mode (e.g. 1, 2)."
  FULL_DOCS "ASE simulation mode (e.g. 1, 2).")

# ASE simulator timeout in clock cycles (when ASE_MODE=2)
define_property(TARGET PROPERTY ASE_MODULE_ASE_TIMEOUT
  BRIEF_DOCS "ASE simulator timeout in clock cycles (when ASE_MODE=2)"
  FULL_DOCS "ASE simulator timeout in clock cycles (when ASE_MODE=2)")

# Required BBBs (AFU basic building blocks)
define_property(TARGET PROPERTY ASE_MODULE_INCLUDE_BBBS
  BRIEF_DOCS "ASE module included BBBs (AFU basic building blocks)"
  FULL_DOCS "ASE module included BBBs (AFU basic building blocks)")

# Compile definitions for the ASE module
define_property(TARGET PROPERTY ASE_MODULE_COMPILE_DEFINITIONS
  BRIEF_DOCS "ASE module compile definitions for Verilog compiler."
  FULL_DOCS "ASE module compile definitions for Verilog compiler.")

# Include directories for the ASE module
define_property(TARGET PROPERTY ASE_MODULE_INCLUDE_DIRECTORIES
  BRIEF_DOCS "ASE module include directories for Verilog compiler."
  FULL_DOCS "ASE module include directories for Verilog compiler.")

# Source directory for the ASE module
define_property(TARGET PROPERTY ASE_MODULE_SOURCE_DIR
  BRIEF_DOCS "Root source directory for the ASE module."
  FULL_DOCS "Root source directory for the ASE module.")

# List of user provided ASE module sources
define_property(TARGET PROPERTY ASE_MODULE_SOURCES
  BRIEF_DOCS "List of user provided ASE module sources (with respect to ASE_MODULE_SOURCE_DIR)."
  FULL_DOCS "List of user provided ASE module sources (with respect to ASE_MODULE_SOURCE_DIR).")

# Target root directory for the ASE module binaries
define_property(TARGET PROPERTY ASE_MODULE_BINARY_DIR
  BRIEF_DOCS "Target root directory for building the ASE module binaries."
  FULL_DOCS "Target root directory for building the ASE module binaries.")

# List of final processed list of ASE module sources, relative to ASE_MODULE_BINARY_DIR
define_property(TARGET PROPERTY ASE_MODULE_SOURCES_REL
  BRIEF_DOCS "List of final processed list of ASE module sources, relative to ASE_MODULE_BINARY_DIR."
  FULL_DOCS "List of final processed list of ASE module sources, relative to ASE_MODULE_BINARY_DIR.")

############################################################################
## Helpers to fetch ASE module properties ##################################
############################################################################

# ase_module_get_platform_name(RESULT_VAR name)
# Return platform name (e.g. intg_xeon, discrete, etc)
# ASE_MODULE_PLATFORM_NAME
function(ase_module_get_platform_name RESULT_VAR name)
  if(NOT TARGET ${name})
    message(FATAL_ERROR "\"${name}\" is not really a target.")
  endif(NOT TARGET ${name})
  get_property(ase_module_type TARGET ${name} PROPERTY ASE_MODULE_TYPE)
  if(NOT ase_module_type)
    message(FATAL_ERROR "\"${name}\" is not really a target for ASE module.")
  endif(NOT ase_module_type)
  get_property(platform_name TARGET ${name} PROPERTY ASE_MODULE_PLATFORM_NAME)
  set(${RESULT_VAR} ${platform_name} PARENT_SCOPE)
endfunction(ase_module_get_platform_name RESULT_VAR name)

# ase_module_set_platform_name(name RESULT_VAR)
# Return platform name (e.g. intg_xeon, discrete, etc)
# ASE_MODULE_PLATFORM_NAME
function(ase_module_set_platform_name name RESULT_VAR)
  if(NOT TARGET ${name})
    message(FATAL_ERROR "\"${name}\" is not really a target.")
  endif(NOT TARGET ${name})
  get_property(ase_module_type TARGET ${name} PROPERTY ASE_MODULE_TYPE)
  if(NOT ase_module_type)
    message(FATAL_ERROR "\"${name}\" is not really a target for ASE module.")
  endif(NOT ase_module_type)
  set_property(TARGET ${name} PROPERTY ASE_MODULE_PLATFORM_NAME  ${RESULT_VAR})
endfunction(ase_module_set_platform_name name RESULT_VAR)

# ase_module_get_platform_fullname(RESULT_VAR name)
# Return platform name (e.g. FPGA_PLATFORM_INTG_XEON, etc)
# ASE_MODULE_PLATFORM_FULLNAME
function(ase_module_get_platform_fullname RESULT_VAR name)
  if(NOT TARGET ${name})
    message(FATAL_ERROR "\"${name}\" is not really a target.")
  endif(NOT TARGET ${name})
  get_property(ase_module_type TARGET ${name} PROPERTY ASE_MODULE_TYPE)
  if(NOT ase_module_type)
    message(FATAL_ERROR "\"${name}\" is not really a target for ASE module.")
  endif(NOT ase_module_type)
  get_property(module_fullname TARGET ${name} PROPERTY ASE_MODULE_PLATFORM_FULLNAME)
  set(${RESULT_VAR} ${module_fullname} PARENT_SCOPE)
endfunction(ase_module_get_platform_fullname RESULT_VAR name)

# ase_module_get_platform_if(RESULT_VAR name)
# Return platform interface name (e.g. ccip_std_afu)
# ASE_MODULE_PLATFORM_IF
function(ase_module_get_platform_if RESULT_VAR name)
  if(NOT TARGET ${name})
    message(FATAL_ERROR "\"${name}\" is not really a target.")
  endif(NOT TARGET ${name})
  get_property(ase_module_type TARGET ${name} PROPERTY ASE_MODULE_TYPE)
  if(NOT ase_module_type)
    message(FATAL_ERROR "\"${name}\" is not really a target for ASE module.")
  endif(NOT ase_module_type)
  get_property(platform_if_name TARGET ${name} PROPERTY ASE_MODULE_PLATFORM_IF)
  set(${RESULT_VAR} ${platform_if_name} PARENT_SCOPE)
endfunction(ase_module_get_platform_if RESULT_VAR name)

# ase_module_get_afu_json(RESULT_VAR name)
# Return AFU json metadata file (e.g. ccip_std_afu.json, etc)
# ASE_MODULE_AFU_JSON
function(ase_module_get_afu_json RESULT_VAR name)
  if(NOT TARGET ${name})
    message(FATAL_ERROR "\"${name}\" is not really a target.")
  endif(NOT TARGET ${name})
  get_property(ase_module_type TARGET ${name} PROPERTY ASE_MODULE_TYPE)
  if(NOT ase_module_type)
    message(FATAL_ERROR "\"${name}\" is not really a target for ASE module.")
  endif(NOT ase_module_type)
  get_property(module_fullname TARGET ${name} PROPERTY ASE_MODULE_AFU_JSON)
  set(${RESULT_VAR} ${module_fullname} PARENT_SCOPE)
endfunction(ase_module_get_afu_json RESULT_VAR name)

# ase_module_set_afu_json(name RESULT_VAR)
# Set AFU json metadata file
# ASE_MODULE_AFU_JSON
function(ase_module_set_afu_json name RESULT_VAR)
  if(NOT TARGET ${name})
    message(FATAL_ERROR "\"${name}\" is not really a target.")
  endif(NOT TARGET ${name})
  get_property(ase_module_type TARGET ${name} PROPERTY ASE_MODULE_TYPE)
  if(NOT ase_module_type)
    message(FATAL_ERROR "\"${name}\" is not really a target for ASE module.")
  endif(NOT ase_module_type)
  set_property(TARGET ${name} PROPERTY ASE_MODULE_AFU_JSON ${RESULT_VAR})
endfunction(ase_module_set_afu_json name RESULT_VAR)

# ase_module_get_simulator(RESULT_VAR name)
# Return EDA simulation tool name
# ASE_MODULE_SIMULATOR
function(ase_module_get_simulator RESULT_VAR name)
  if(NOT TARGET ${name})
    message(FATAL_ERROR "\"${name}\" is not really a target.")
  endif(NOT TARGET ${name})
  get_property(ase_module_type TARGET ${name} PROPERTY ASE_MODULE_TYPE)
  if(NOT ase_module_type)
    message(FATAL_ERROR "\"${name}\" is not really a target for ASE module.")
  endif(NOT ase_module_type)
  get_property(module_simulator TARGET ${name} PROPERTY ASE_MODULE_SIMULATOR)
  set(${RESULT_VAR} ${module_simulator} PARENT_SCOPE)
endfunction(ase_module_get_simulator RESULT_VAR module)

############################################################################
## Helpers to add some ASE module properties ###############################
############################################################################

#  ase_module_add_definitions (flags)
# Specify additional flags for compile ASE module.
function(ase_module_add_definitions target_name defs)
  get_property(current_flags TARGET ${target_name} PROPERTY ASE_MODULE_COMPILE_DEFINITIONS)
  list(APPEND current_flags ${defs})
  set_property(TARGET ${target_name} PROPERTY ASE_MODULE_COMPILE_DEFINITIONS "${current_flags}")
endfunction(ase_module_add_definitions target_name defs)

#  ase_modules_include_directories(dirs ...)
# Add include directories for compile ASE module.
function(ase_module_include_directories target_name dir)
  get_property(current_flags TARGET ${target_name} PROPERTY ASE_MODULE_INCLUDE_DIRECTORIES)
  list(APPEND current_flags ${dir})
  set_property(TARGET ${target_name} PROPERTY ASE_MODULE_INCLUDE_DIRECTORIES "${current_flags}")
endfunction(ase_module_include_directories target_name dir)

#  ase_modules_include_bbbs(bbbs ...)
# Add include BBBs for compile ASE module.
function(ase_module_include_bbbs target_name bbbs)
  get_property(current_flags TARGET ${target_name} PROPERTY ASE_MODULE_INCLUDE_BBBS)
  list(APPEND current_flags ${bbbs})
  set_property(TARGET ${target_name} PROPERTY ASE_MODULE_INCLUDE_BBBS "${current_flags}")
endfunction(ase_module_include_bbbs target_name defs)

############################################################################
## Helper utilities ########################################################
############################################################################

# _declare_per_build_vars(<variable> <doc-pattern>)
#
# [INTERNAL] Per-build type definitions for given <variable>.
#
# Create CACHE STRING variables <variable>_{DEBUG|RELEASE|RELWITHDEBINFO|MINSIZEREL}
# with documentation string <doc-pattern> where %build% is replaced
# with corresponded build type description.
#
# Default value for variable <variable>_<type> is taken from <variable>_<type>_INIT.
macro(_declare_per_build_vars variable doc_pattern)
  set(_build_type)
  foreach(t
      COVERAGE "coverage"
      RELEASE "release"
      DEBUG "debug"
      RELWITHDEBINFO "Release With Debug Info"
      MINSIZEREL "release minsize")
    if(_build_type)
      string(REPLACE "%build%" "${t}" _docstring ${doc_pattern})
      set("${variable}_${_build_type}" "${${variable}_${_build_type}_INIT}"
        CACHE STRING "${_docstring}")
      set(_build_type)
    else(_build_type)
      set(_build_type "${t}")
    endif(_build_type)
  endforeach(t)
endmacro(_declare_per_build_vars variable doc_pattern)

#  _get_per_build_var(RESULT_VAR variable)
#
# Return value of per-build variable.
macro(_get_per_build_var RESULT_VAR variable)
  if(CMAKE_BUILD_TYPE)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" _build_type_uppercase)
    set(${RESULT_VAR} "${${variable}_${_build_type_uppercase}}")
  else(CMAKE_BUILD_TYPE)
    set(${RESULT_VAR})
  endif(CMAKE_BUILD_TYPE)
endmacro(_get_per_build_var RESULT_VAR variable)

#  _string_join(sep RESULT_VAR str1 str2)
#
# Join strings <str1> and <str2> using <sep> as glue.
#
# Note, that precisely 2 string are joined, not a list of strings.
# This prevents automatic replacing of ';' inside strings while parsing arguments.
macro(_string_join sep RESULT_VAR str1 str2)
  if("${str1}" STREQUAL "")
    set("${RESULT_VAR}" "${str2}")
  elseif("${str2}" STREQUAL "")
    set("${RESULT_VAR}" "${str1}")
  else("${str1}" STREQUAL "")
    set("${RESULT_VAR}" "${str1}${sep}${str2}")
  endif("${str1}" STREQUAL "")
endmacro(_string_join sep RESULT_VAR str1 str2)

#  _build_get_directory_property_chained(RESULT_VAR <propert_name> [<separator>])
#
# Return list of all values for given property in the current directory
# and all parent directories.
#
# If <separator> is given, it is used as glue for join values.
# By default, cmake list separator (';') is used.
function(_get_directory_property_chained RESULT_VAR property_name)
  set(sep ";")
  foreach(arg ${ARGN})
    set(sep "${arg}")
  endforeach(arg ${ARGN})
  set(result "")
  set(d "${CMAKE_CURRENT_SOURCE_DIR}")
  while(NOT "${d}" STREQUAL "")
    get_property(p DIRECTORY "${d}" PROPERTY "${property_name}")
    # debug
    # message("Property ${property_name} for directory ${d}: '${p}'")
    _string_join("${sep}" result "${p}" "${result}")
    get_property(d DIRECTORY "${d}" PROPERTY PARENT_DIRECTORY)
  endwhile(NOT "${d}" STREQUAL "")
  set("${RESULT_VAR}" "${result}" PARENT_SCOPE)
endfunction(_get_directory_property_chained RESULT_VAR property_name)
