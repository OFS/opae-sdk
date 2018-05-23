## Copyright(c) 2017, 2018, Intel Corporation
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
include(ase_add_dpic)
include(intel_fpga_bbb)

############################################################################
## Fetch tool and script locations #########################################
############################################################################

find_package(Quartus)
find_package(Questa)
set(ALTERA_MEGAFUNCTIONS "${QUARTUS_DIR}/../modelsim_ae/altera/verilog/altera_mf")

############################################################################
## Setup Questa global flags ###############################################
############################################################################

set(questa_flags)
list(APPEND questa_flags VENDOR_ALTERA)
list(APPEND questa_flags TOOL_QUARTUS)
list(APPEND questa_flags RTL_SIMULATION)
list(APPEND questa_flags PLATFORM_IF_AVAIL)
list(APPEND questa_flags FPGA_BBB_CCI_SRC=${FPGA_BBB_CCI_SRC})
set(QUESTA_VLOG_GLOBAL_COMPILE_DEFINITIONS ${questa_flags}
  CACHE STRING "Modelsim/Questa global define flags" FORCE)

set(questa_flags)
list(APPEND questa_flags .)
list(APPEND questa_flags work)
list(APPEND questa_flags ${ASE_SERVER_RTL})
list(APPEND questa_flags ${PLATFORM_IF_RTL})
set(QUESTA_VLOG_GLOBAL_INCLUDE_DIRECTORIES ${questa_flags}
  CACHE STRING "Modelsim/Questa global include flags" FORCE)

set(questa_flags)
list(APPEND questa_flags -nologo)
list(APPEND questa_flags -sv)
list(APPEND questa_flags -dpicpppath ${CMAKE_C_COMPILER})
list(APPEND questa_flags +librescan)
list(APPEND questa_flags -timescale ${ASE_TIMESCALE})
list(APPEND questa_flags -work work)
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  list(APPEND questa_flags +cover=bcfst)
endif()
_declare_per_build_vars(QUESTA_VLOG_FLAGS "Compiler flags used by Modelsim/Questa during %build% builds.")
set(QUESTA_VLOG_FLAGS_DEBUG          ${questa_flags} CACHE STRING "Modelsim/Questa global compiler flags" FORCE)
set(QUESTA_VLOG_FLAGS_RELWITHDEBINFO ${questa_flags} CACHE STRING "Modelsim/Questa global compiler flags" FORCE)
set(QUESTA_VLOG_FLAGS_RELEASE        ${questa_flags} CACHE STRING "Modelsim/Questa global compiler flags" FORCE)
set(QUESTA_VLOG_FLAGS_MINSIZEREL     ${questa_flags} CACHE STRING "Modelsim/Questa global compiler flags" FORCE)
set(QUESTA_VLOG_FLAGS_COVERAGE       ${questa_flags} CACHE STRING "Modelsim/Questa global compiler flags" FORCE)

# VSIM flags
set(questa_flags)
list(APPEND questa_flags -c)
list(APPEND questa_flags -dpioutoftheblue 1)
list(APPEND questa_flags -dpicpppath /usr/bin/gcc)
list(APPEND questa_flags -cpppath ${CMAKE_C_COMPILER})
list(APPEND questa_flags -do vsim_run.tcl)
list(APPEND questa_flags -sv_seed 1234)
# list(APPEND questa_flags -L ${ALTERA_MEGAFUNCTIONS})
if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
  list(APPEND questa_flags -coverage)
  list(APPEND questa_flags -voptargs='+cover=bcfst')
endif()
_declare_per_build_vars(QUESTA_VSIM_FLAGS "Compiler flags used by Modelsim/Questa during %build% builds.")
set(QUESTA_VSIM_FLAGS_DEBUG          ${questa_flags} CACHE STRING "Modelsim/Questa simulator flags" FORCE)
set(QUESTA_VSIM_FLAGS_RELWITHDEBINFO ${questa_flags} CACHE STRING "Modelsim/Questa simulator flags" FORCE)
set(QUESTA_VSIM_FLAGS_RELEASE        ${questa_flags} CACHE STRING "Modelsim/Questa simulator flags" FORCE)
set(QUESTA_VSIM_FLAGS_MINSIZEREL     ${questa_flags} CACHE STRING "Modelsim/Questa simulator flags" FORCE)
set(QUESTA_VSIM_FLAGS_COVERAGE       ${questa_flags} CACHE STRING "Modelsim/Questa simulator flags" FORCE)

############################################################################
## Setup Questa directory-specific flags ###################################
############################################################################

# Per-directory tracking for questa_vlog compiler flags.
define_property(DIRECTORY PROPERTY QUESTA_VLOG_COMPILE_DEFINITIONS
  BRIEF_DOCS "Compiler flags used by Questa_vlog system added in this directory."
  FULL_DOCS "Compiler flags used by Questa_vlog system added in this directory.")

# Per-directory tracking for questa_vlog include directories.
define_property(DIRECTORY PROPERTY QUESTA_VLOG_INCLUDE_DIRECTORIES
  BRIEF_DOCS "Include directories used by Questa_vlog system."
  FULL_DOCS "Include directories used by Questa_vlog system.")

#  questa_vlog_include_directories(dirs ...)
# Add include directories for questa_vlog process.
macro(questa_vlog_include_directories)
  set_property(DIRECTORY APPEND PROPERTY QUESTA_VLOG_INCLUDE_DIRECTORIES "${ARGN}")
endmacro(questa_vlog_include_directories)

#  questa_vlog_add_definitions (flags)
# Specify additional flags for questa_vlog process.
function(questa_vlog_add_definitions)
  set_property(DIRECTORY APPEND PROPERTY QUESTA_VLOG_COMPILE_DEFINITIONS "${ARGN}")
endfunction(questa_vlog_add_definitions flags)

############################################################################
#  ase_add_modelsim_module(<name> [EXCLUDE_FROM_ALL] [MODULE_NAME <module_name>] [<sources> ...])
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
# .txt: Modelsim project
# .json: AFU specification file
# .sv: SystemVerilog files
#
# Other sources are treated as only prerequisite of building process.
# In either case, if MODULE_NAME option is given, it determine name
# of the ASE module. Otherwise <name> itself is used.

function(ase_add_modelsim_module name)

  cmake_parse_arguments(ase_add_modelsim_module "IMPORTED;EXCLUDE_FROM_ALL" "MODULE_NAME" "" ${ARGN})
  if(ase_add_modelsim_module_MODULE_NAME)
    set(module_name "${ase_add_modelsim_module_MODULE_NAME}")
  else(ase_add_modelsim_module_MODULE_NAME)
    set(module_name "${name}")
  endif(ase_add_modelsim_module_MODULE_NAME)

  # List of all source files, which are given.
  set(sources ${ase_add_modelsim_module_UNPARSED_ARGUMENTS})

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
  set(sverilog_headers_abs)

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
        OR ext STREQUAL ".sv"
        OR ext STREQUAL ".svh"
        OR ext STREQUAL ".v"
        OR ext STREQUAL ".vh"
        OR ext STREQUAL ".json")

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

  ############################################################################
  ## Target to build DPI C library
  ############################################################################

  ase_add_dpic_module(${name})

  # Assure compilation occurs after generation of DPI header file
  add_dependencies(opae-c-ase-server-${name} ${name})

  ############################################################################
  ## Initialize ASE module properties ########################################
  ############################################################################

  # Initial set of properties for the module
  set_property(TARGET ${name} PROPERTY ASE_MODULE_TYPE                "verilog")
  set_property(TARGET ${name} PROPERTY ASE_MODULE_NAME                ${name})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_MODULE_LOCATION     ${CMAKE_CURRENT_BINARY_DIR}/work)

  # Fill properties for the target using default values
  set_property(TARGET ${name} PROPERTY ASE_MODULE_PLATFORM_FULLNAME   ${ASE_PLATFORM})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_PLATFORM_NAME       "intg_xeon")
  set_property(TARGET ${name} PROPERTY ASE_MODULE_PLATFORM_IF         "ccip_std_afu")
  set_property(TARGET ${name} PROPERTY ASE_MODULE_SIMULATOR           ${ASE_SIMULATOR})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_TIMESCALE           ${ASE_TIMESCALE})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_USR_CLOCK_MHZ       ${ASE_USR_CLK_MHZ})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_ASE_MODE            ${ASE_MODE})
  set_property(TARGET ${name} PROPERTY ASE_MODULE_ASE_TIMEOUT         ${ASE_TIMEOUT})

  # Target specific: Compilation/linker flags + sources
  set_property(TARGET ${name} PROPERTY ASE_MODULE_VLOG_FLAGS)
  set_property(TARGET ${name} PROPERTY ASE_MODULE_COMPILE_DEFINITIONS)
  set_property(TARGET ${name} PROPERTY ASE_MODULE_INCLUDE_DIRECTORIES
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/platform_includes
    ${CMAKE_CURRENT_BINARY_DIR}/hw/rtl
    ${OPAE_BASE_DIR}/${PLATFORM_IF_RTL}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/hw/rtl)
  set_property(TARGET ${name} PROPERTY ASE_MODULE_BINARY_DIR          ${CMAKE_CURRENT_BINARY_DIR})
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
  set(ASE_MODULE_${name}_DIRECTORY_FLAGS                ${vlog_flags_dir}           CACHE STRING "Compiler flags used by Questa to build ASE module '${name}'.")
  set(ASE_MODULE_${name}_TARGET_FLAGS                   ${vlog_flags_local}         CACHE STRING "Compiler flags used by Questa to build ASE module '${name}'.")
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

  _ase_module_clean_files(${name}
    SV_SOURCE       ${sverilog_ase_module_sources_abs}
    JSON_SOURCE     ${json_ase_module_sources_abs}
    PRJ_SOURCE      ${prj_ase_module_sources_abs}
    SHIPPED_SOURCE  ${shipped_ase_module_sources_abs})

endfunction(ase_add_modelsim_module name)

#  _ase_module_clean_files(module_name ...)
#
# Tell CMake that intermediate files, created by kbuild system,
# should be cleaned with 'make clean'.
function(_ase_module_clean_files module_name)
  cmake_parse_arguments(ase_module_clean "" "" "SV_SOURCE;JSON_SOURCE;PRJ_SOURCE;SHIPPED_SOURCE" ${ARGN})
  if(ase_module_clean_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unparsed arguments")
  endif(ase_module_clean_UNPARSED_ARGUMENTS)

  # List common files (names only) for cleaning
  set(common_files_names
    "_vmake"
    "_info"
    "_lib1_0.qdb"
    "_lib1_0.qpg"
    "_lib1_0.qtl"
    "_lib.qdb"
    "_dpi" # Directory
    "work" # Directory
    "vlog.log"
    "platform_dpi.h")

  # List module name-depending files (extensions only) for cleaning
  set(name_files_ext
    ".o"
    ".sv"
    ".svh"
    ".v"
    ".vh")

  # List source name-depending files (extensions only) for cleaning
  set(source_name_files_ext
    ".sv"
    ".svh"
    ".v"
    ".vh"
    ".o")

  # Now collect all sort of files into list
  set(files_list)
  foreach(name ${common_files_names})
    list(APPEND files_list "${CMAKE_CURRENT_BINARY_DIR}/${name}")
  endforeach(name ${common_files_names})

  foreach(ext ${name_files_ext})
    list(APPEND files_list
      "${CMAKE_CURRENT_BINARY_DIR}/${module_name}${ext}")
  endforeach(ext ${name_files_ext})

  # All the types of sources are processed in a similar way
  foreach(obj_source_noext_abs
      ${ase_module_clean_SV_SOURCE}
      ${ase_module_clean_JSON_SOURCE}
      ${ase_module_clean_PRJ_SOURCE}
      ${ase_module_clean_SHIPPED_SOURCE})

    get_filename_component(dir ${obj_source_noext_abs} PATH)
    get_filename_component(name ${obj_source_noext_abs} NAME)
    foreach(ext ${source_name_files_ext})
      list(APPEND files_list "${dir}/${name}${ext}")
    endforeach(ext ${source_name_files_ext})
  endforeach(obj_source_noext_abs)

  # Tell CMake that given files should be cleaned.
  set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${files_list}")

endfunction(_ase_module_clean_files module_name)

############################################################################
## ase_finalize_modelsim_linking(m) ########################################
############################################################################
#
# Should be called after ASE module `m` is defined.
#

function(ase_finalize_modelsim_module_linking m)

  _get_per_build_var(vlog_flags QUESTA_VLOG_FLAGS)
  list(APPEND vlog_flags -f sources_simulation.txt)
  foreach(variable ${QUESTA_VLOG_GLOBAL_COMPILE_DEFINITIONS})
    list(APPEND vlog_flags +define+${variable})
  endforeach(variable ${QUESTA_VLOG_COMPILE_DEFINITIONS})
  foreach(dir ${QUESTA_VLOG_GLOBAL_INCLUDE_DIRECTORIES})
    list(APPEND vlog_flags +incdir+${dir})
  endforeach(dir ${QUESTA_VLOG_GLOBAL_INCLUDE_DIRECTORIES})

  # Concatenate directory specific flags (definitions and include directories)
  _get_directory_property_chained(compile_definitions QUESTA_VLOG_COMPILE_DEFINITIONS " ")
  foreach(variable ${compile_definitions})
    list(APPEND vlog_flags +define+${variable})
  endforeach(variable ${compile_definitions})

  _get_directory_property_chained(include_dirs QUESTA_VLOG_INCLUDE_DIRECTORIES " ")
  foreach(dir ${include_dirs})
    list(APPEND vlog_flags +incdir+${dir})
  endforeach(dir ${include_dirs})

  # Get object properties
  get_property(module_binary_dir TARGET ${m} PROPERTY ASE_MODULE_BINARY_DIR)
  get_property(module_source_dir TARGET ${m} PROPERTY ASE_MODULE_SOURCE_DIR)
  get_property(module_name       TARGET ${m} PROPERTY ASE_MODULE_NAME)

  # Get object sources
  get_property(ase_module_sources_rel
    CACHE ASE_MODULE_${m}_SOURCES_REL
    PROPERTY VALUE)
  get_property(ase_module_sources_abs
    CACHE ASE_MODULE_${m}_SOURCES_ABS
    PROPERTY VALUE)

  # Get object compilation flags
  get_property(vlog_flags_local
    CACHE ASE_MODULE_${m}_TARGET_FLAGS
    PROPERTY VALUE)

  get_property(definitions_local              TARGET ${m} PROPERTY ASE_MODULE_COMPILE_DEFINITIONS)
  get_property(include_dirs_local             TARGET ${m} PROPERTY ASE_MODULE_INCLUDE_DIRECTORIES)

  # Target specific properties
  set(vlog_flags_local)
  get_property(vlog_definitions_local TARGET ${m} PROPERTY ASE_MODULE_COMPILE_DEFINITIONS)
  foreach(definition ${definitions_local})
    list(APPEND vlog_flags_local +define+${definition})
  endforeach(definition ${definitions_local})

  get_property(include_dirs_local TARGET ${m} PROPERTY ASE_MODULE_INCLUDE_DIRECTORIES)
  foreach(dir ${include_dirs_local})
    list(APPEND vlog_flags_local +incdir+${dir})
  endforeach(dir ${include_dirs_local})

  ############################################################################
  ## copy and configure sources files ########################################
  ############################################################################

  foreach(file_i ${ase_module_sources_abs})
    get_filename_component(ext             ${file_i}   EXT)
    get_filename_component(source_noext    "${file_i}" NAME_WE)
    get_filename_component(source_dir      "${file_i}" PATH)
    get_filename_component(source_filename "${file_i}" NAME)

    # Copy source files for the module
    if(ext STREQUAL ".txt"
        OR ext STREQUAL ".sv"
        OR ext STREQUAL ".svh"
        OR ext STREQUAL ".v"
        OR ext STREQUAL ".vh"
        OR ext STREQUAL ".json")

      # Copy source into binary tree, if needed
      # copy_source_to_binary_dir("${file_i}" file_i)
      if(ext STREQUAL ".txt")
        configure_file("${file_i}" ${CMAKE_CURRENT_BINARY_DIR}/${source_filename} @ONLY)
      elseif(ext STREQUAL ".json")
        configure_file("${file_i}" ${CMAKE_CURRENT_BINARY_DIR}/ccip_std_afu.json @ONLY)
      else()
        configure_file("${file_i}" ${CMAKE_CURRENT_BINARY_DIR}/${source_filename} COPYONLY)
      endif()
    endif()

    # Add simulation project
    configure_file(${ASE_SHARE_DIR}/in/sources_simulation.txt.in
       ${CMAKE_CURRENT_BINARY_DIR}/sources_simulation.txt @ONLY)
  endforeach(file_i ${ase_module_sources_abs})

  ############################################################################
  ## copy remaining sources files (ASE scripts) ##############################
  ############################################################################
  get_property(ase_module_ase_mode      TARGET ${m} PROPERTY ASE_MODULE_ASE_MODE)
  get_property(ase_module_ase_timeout   TARGET ${m} PROPERTY ASE_MODULE_ASE_TIMEOUT)
  get_property(ase_module_usr_clock_mhz TARGET ${m} PROPERTY ASE_MODULE_USR_CLOCK_MHZ)

  set(ASE_WORKDIR             "${PROJECT_BINARY_DIR}")
  set(ASE_CONFIG              "${PROJECT_BINARY_DIR}/ase.cfg")
  set(ASE_REGRESS_SCRIPT      "${PROJECT_BINARY_DIR}/ase_regress.sh")
  set(ASE_SERVER_SCRIPT       "${PROJECT_BINARY_DIR}/ase_server.sh")
  set(ASE_SIMULATION_SCRIPT   "${PROJECT_BINARY_DIR}/vsim_run.tcl")

  if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    set(vsim_coverage_header "coverage save -onexit ase.ucdb")
  endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

  # Create ASE scripts
  configure_file(${CMAKE_BINARY_DIR}/ase/rtl/sources_ase_server.txt
    ${ASE_WORKDIR}/sources_ase_server.txt)
  configure_file(${CMAKE_BINARY_DIR}/ase/rtl/sources_quartus_libs.txt
    ${ASE_WORKDIR}/sources_quartus_libs.txt)
  configure_file(${ASE_SCRIPTS_IN}/ase.cfg.in
    ${ASE_CONFIG})
  configure_file(${ASE_SCRIPTS_IN}/ase_regress.sh.in
    ${ASE_REGRESS_SCRIPT})
  configure_file(${ASE_SCRIPTS_IN}/vsim_run.tcl.in
    ${ASE_SIMULATION_SCRIPT})

  # Set vsim flags in server script
  _get_per_build_var(vsim_flags_list QUESTA_VSIM_FLAGS)
  # TODO: Replace 'lib' with ${OPAE_LIB_INSTALL_DIR} after directory naming update PR is merged
  list(APPEND vsim_flags_list -sv_lib ${CMAKE_BINARY_DIR}/lib/${ASE_SHOBJ_NAME}-${m})
  list(APPEND vsim_flags_list -l vlog_run.log)

  set(vsim_flags)
  foreach(flag ${vsim_flags_list})
    set(vsim_flags "${vsim_flags} ${flag}")
  endforeach(flag ${vsim_flags_list})

  configure_file(${ASE_SCRIPTS_IN}/ase_server.in
    ${ASE_WORKDIR}/tmp/ase_server.sh)

  # Create simulation application
  file(COPY ${PROJECT_BINARY_DIR}/tmp/ase_server.sh
    DESTINATION ${PROJECT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

  # Fetch modules sharing same directory
  get_property(ase_module_targets_per_directory
    DIRECTORY "${module_source_dir}"
    PROPERTY ASE_MODULE_TARGETS)

  ############################################################################
  ## Finally set commands to create targets ##################################
  ############################################################################

  # afu_platform_config --sim --tgt=rtl --src ccip_st6d_afu.json  intg_xeon
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/platform_includes)
  set(ase_platform)
  ase_module_get_platform_name(ase_platform ${m})
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/platform_includes/platform_afu_top_config.vh"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${AFU_PLATFORM_CONFIG}
    --sim
    --tgt=platform_includes
    --src ${CMAKE_CURRENT_BINARY_DIR}/ccip_std_afu.json
    ${ase_platform}
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/platform_includes/platform_afu_top_config.vh")

  # Define DPI header file generation rule
  file(MAKE_DIRECTORY ${module_binary_dir}/include)
  add_custom_command(
    OUTPUT "${module_binary_dir}/include/platform_dpi.h"
    WORKING_DIRECTORY ${module_binary_dir}
    COMMAND ${QUESTA_VLIB_EXECUTABLE} work
    COMMAND ${QUESTA_VLOG_EXECUTABLE}
    -dpiheader ${CMAKE_CURRENT_BINARY_DIR}/include/platform_dpi.h
    ${vlog_flags}
    ${vlog_flags_local}
    -l vlog.log
    DEPENDS "${module_binary_dir}/platform_includes/platform_afu_top_config.vh"
    COMMENT "Building ASE module ${module_name}"
    VERBATIM)

endfunction(ase_finalize_modelsim_module_linking m)
