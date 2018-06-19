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
include(ase_variables_config)
include(ase_add_modelsim)

############################################################################
## Global ASE variables ####################################################
############################################################################

function(ase_module_process_json name)

  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include)
  set(afu_json)
  ase_module_get_afu_json(afu_json ${m})
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/afu_json_info.vh"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${AFU_JSON_MGR}
    json-info
    --afu-json       ${CMAKE_CURRENT_BINARY_DIR}/${afu_json}
    --verilog-hdr    ${CMAKE_CURRENT_BINARY_DIR}/include/afu_json_info.vh
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/include/afu_json_info.vh")

  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/afu_json_info.h"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${AFU_JSON_MGR}
    json-info
    --afu-json       ${CMAKE_CURRENT_BINARY_DIR}/${afu_json}
    --c-hdr    ${CMAKE_CURRENT_BINARY_DIR}/include/afu_json_info.h
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/include/afu_json_info.h")

  # afu_platform_config --sim --tgt=rtl --src ccip_st6d_afu.json  intg_xeon
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/platform_includes)
  set(ase_platform)
  ase_module_get_platform_name(ase_platform ${m})
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/platform_includes/platform_afu_top_config.vh"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${AFU_PLATFORM_CONFIG}
    --sim
    --tgt=platform_includes
    --src ${CMAKE_CURRENT_BINARY_DIR}/${afu_json}
    ${ase_platform}
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/platform_includes/platform_afu_top_config.vh"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/include/afu_json_info.vh")

endfunction(ase_module_process_json name)

############################################################################
#  ase_add_afu_module(<name> [EXCLUDE_FROM_ALL] [MODULE_NAME <module_name>] [<sources> ...])
############################################################################
#
# Build ASE module from <sources>, analogue of add_library().
#
# Source files are divided into two categories:
# -Object sources
# -Other sources
#
# Object sources are those sources, which may be used in building ASE
# module externally.
# Follow types of object sources are supported now:
# .txt: AFU project containing synthesizable files
# .json: AFU specification file
# .sv: SystemVerilog files
#
# Other sources are treated as only prerequisite of building process.
# In either case, if MODULE_NAME option is given, it determine name
# of the ASE module. Otherwise <name> itself is used.

function(ase_add_afu_module name)

  cmake_parse_arguments(ase_add_afu_module "IMPORTED;EXCLUDE_FROM_ALL" "MODULE_NAME" "" ${ARGN})
  if(ase_add_afu_module_MODULE_NAME)
    set(module_name "${ase_add_afu_module_MODULE_NAME}")
  else(ase_add_afu_module_MODULE_NAME)
    set(module_name "${name}")
  endif(ase_add_afu_module_MODULE_NAME)

  # List of all source files, which are given.
  set(sources ${ase_add_afu_module_UNPARSED_ARGUMENTS})

  # Calculate sources absolute paths
  to_abs_path(ase_module_sources_abs ${sources})

  # List of files from which module building is depended
  set(ase_modules_sources)

  # Modelsim project files (.txt)
  set(prj_ase_module_sources_abs)
  # AFU metadata files (.json)
  set(json_ase_module_sources_abs)
  # SystemVerilog source files (.sv)
  set(sverilog_ase_module_sources_abs)
  # SystemVerilog header files (.svh)
  set(sverilog_ase_module_headers_abs)
  # Quartus Qsys files (.qsys)
  set(qsys_ase_module_sources_abs)
  # Quartus .ip files (.ip)
  set(ip_ase_module_sources_abs)

  ############################################################################
  ## Categorize sources
  ############################################################################
  foreach(file_i ${ase_module_sources_abs})
    get_filename_component(ext ${file_i} EXT)
    get_filename_component(source_noext "${file_i}" NAME_WE)
    get_filename_component(source_dir "${file_i}" PATH)
    get_filename_component(source_filename "${file_i}" NAME)

    # Calculate module locations
    if(ext STREQUAL ".txt"
        # JSON metadata
        OR ext STREQUAL ".json"
        # Verilog files
        OR ext STREQUAL ".sv"
        OR ext STREQUAL ".svh"
        OR ext STREQUAL ".v"
        OR ext STREQUAL ".vh"
        # Quartus files
        OR ext MATCHES "\\.ip$"
        OR ext MATCHES "\\.qsys$"
        OR ext MATCHES "\\.tcl$")

      # Categorize sources
      set(source_abs "${CMAKE_CURRENT_BINARY_DIR}/${source_noext}")
      if(ext STREQUAL ".txt")
        # Project source file
        list(APPEND prj_ase_module_sources_abs ${source_abs}.txt)
      elseif(ext STREQUAL ".json")
        # JSON source file
        list(APPEND json_ase_module_sources_abs ${source_abs}.json)
      elseif(ext STREQUAL ".sv")
        # SystemVerilog source file
        list(APPEND sverilog_ase_module_sources_abs ${source_abs}.sv)
      elseif(ext STREQUAL ".svh")
        # SystemVerilog source file
        list(APPEND sverilog_ase_module_headers_abs ${source_abs}.svh)
      endif()
    endif()

    # Add file to depend list
    list(APPEND ase_modules_sources ${file_i})
  endforeach(file_i ${ase_module_sources_abs})

  # ASE module sources relative to current binary dir
  set(ase_module_sources_rel)
  foreach(file_i
      ${prj_ase_module_sources_abs}
      ${json_ase_module_sources_abs}
      ${sverilog_ase_module_sources_abs}
      ${sverilog_ase_module_headers_abs})
    file(RELATIVE_PATH ase_module_source_rel
      ${CMAKE_CURRENT_BINARY_DIR} ${file_i})
    list(APPEND ase_module_sources_rel ${ase_module_source_rel})
  endforeach(file_i)

  if(NOT ase_module_sources_rel)
    message(FATAL_ERROR "List of object files for building ASE module ${name} is empty.")
  endif(NOT ase_module_sources_rel)

  ############################################################################
  ## Create main target ######################################################
  ############################################################################

  # Target for create C and SVerilog headers from JSON metadata.
  add_custom_target(${name}_verilog_hdr ALL
    DEPENDS
    "${CMAKE_CURRENT_BINARY_DIR}/include/afu_json_info.vh")

  # Target for create module platform configuration.
  add_custom_target(${name}_platform_config ALL
    DEPENDS
    intel-fpga-bbb
    ${ase_modules_sources}
    "${CMAKE_CURRENT_BINARY_DIR}/platform_includes/platform_afu_top_config.vh")

  # Target to build the ASE module
  add_custom_target (${name} ALL
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/include/platform_dpi.h")
  add_dependencies(${name} ${name}_platform_config)
  add_dependencies(${name} ${name}_verilog_hdr)

  ############################################################################
  ## Initialize ASE module properties ########################################
  ############################################################################

  # Initial set of properties for the module
  set_property(TARGET ${name} PROPERTY ASE_MODULE_NAME                ${name})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_TYPE                ${ASE_MODULE_TYPE})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_MODULE_LOCATION     ${CMAKE_CURRENT_BINARY_DIR}/work)

  # Initialize target properties using default values
  set_property(TARGET ${name} PROPERTY ASE_MODULE_PLATFORM_FULLNAME   ${ASE_PLATFORM})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_PLATFORM_NAME       ${ASE_PLATFORM_NAME})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_PLATFORM_IF         ${ASE_PLATFORM_IF})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_SIMULATOR           ${ASE_SIMULATOR})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_TIMESCALE           ${ASE_TIMESCALE})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_USR_CLOCK_MHZ       ${ASE_USR_CLK_MHZ})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_ASE_MODE            ${ASE_MODE})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_ASE_TIMEOUT         ${ASE_TIMEOUT})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_AFU_JSON            ${ASE_AFU_JSON})

  # Target specific: Compilation/linker flags + sources
  set_property(TARGET ${name} PROPERTY ASE_MODULE_VLOG_FLAGS)
  set_property(TARGET ${name} PROPERTY ASE_MODULE_COMPILE_DEFINITIONS)
  set_property(TARGET ${name} PROPERTY ASE_MODULE_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/include
    ${CMAKE_CURRENT_BINARY_DIR}/platform_includes
    ${CMAKE_CURRENT_BINARY_DIR}/rtl
    ${OPAE_BASE_DIR}/${PLATFORM_IF_RTL}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/rtl)
  set_property(TARGET ${name} PROPERTY ASE_MODULE_BUILD_DIR           ${CMAKE_CURRENT_BINARY_DIR})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_SOURCES             ${ase_module_sources})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_SOURCES_ABS         ${ase_module_ase_module_sources_abs})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_SOURCES_REL         ${ase_module_sources_rel})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_SOURCE_DIR          ${CMAKE_CURRENT_SOURCE_DIR})

  # Target specific properties
  set(vlog_flags_local)
  get_property(vlog_definitions_local TARGET ${name} PROPERTY ASE_MODULE_COMPILE_DEFINITIONS)
  foreach(definition ${definitions_local})
    list(APPEND vlog_flags_local +define+${definition})
  endforeach(definition ${definitions_local})

  get_property(include_dirs_local TARGET ${name} PROPERTY ASE_MODULE_INCLUDE_DIRECTORIES)
  foreach(dir ${include_dirs_local})
    list(APPEND vlog_flags_local +incdir+${dir})
  endforeach(dir ${include_dirs_local})

  # Create target specific cache entries
  set(ASE_MODULE_${name}_DIRECTORY_FLAGS                ${vlog_flags_dir}           CACHE STRING "Directory-specific compiler flags used to build ASE module '${name}'.")
  set(ASE_MODULE_${name}_TARGET_FLAGS                   ${vlog_flags_local}         CACHE STRING "Target-specific compiler flags used to build ASE module '${name}'.")
  set(ASE_MODULE_${name}_SOURCES                        ${ase_module_sources}       CACHE STRING "List of source files used to build ASE module '${name}'.")
  set(ASE_MODULE_${name}_SOURCES_ABS                    ${ase_module_sources_abs}   CACHE STRING "List of source files used to build ASE module '${name}'.")
  set(ASE_MODULE_${name}_SOURCES_REL                    ${ase_module_sources_rel}}  CACHE STRING "List of source files used to build ASE module '${name}'.")

  mark_as_advanced(
    ASE_MODULE_${name}_DIRECTORY_FLAGS
    ASE_MODULE_${name}_TARGET_FLAGS
    ASE_MODULE_${name}_SOURCES
    ASE_MODULE_${name}_SOURCES_ABS
    ASE_MODULE_${name}_SOURCES_REL)

  # Cached properties
  set_property(CACHE ASE_MODULE_${name}_DIRECTORY_FLAGS    PROPERTY VALUE ${vlog_flags})
  set_property(CACHE ASE_MODULE_${name}_TARGET_FLAGS       PROPERTY VALUE ${vlog_flags_local})
  set_property(CACHE ASE_MODULE_${name}_SOURCES            PROPERTY VALUE ${ase_module_sources})
  set_property(CACHE ASE_MODULE_${name}_SOURCES_ABS        PROPERTY VALUE ${ase_module_sources_abs})
  set_property(CACHE ASE_MODULE_${name}_SOURCES_REL        PROPERTY VALUE ${ase_module_sources_rel})

  # Add target to the global list of modules for built.
  set_property(GLOBAL APPEND PROPERTY ASE_MODULE_TARGETS "${name}")

  # Keep track of which modules will be built from same directory
  set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    APPEND PROPERTY ASE_MODULE_TARGETS "${name}")

  ############################################################################
  ## The rule to clean files #################################################
  ############################################################################

  # ase_module_clean_files(${name}
  #   SV_SOURCE       ${sverilog_ase_module_sources_abs}
  #   JSON_SOURCE     ${json_ase_module_sources_abs}
  #   PRJ_SOURCE      ${prj_ase_module_sources_abs}
  #   SHIPPED_SOURCE  ${shipped_ase_module_sources_abs})

endfunction(ase_add_afu_module name)
