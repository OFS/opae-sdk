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
include(cmake_useful)
include(CMakeParseArguments)

# find afu_sim_setup
find_program(AFU_PLATFORM_CONFIG
  NAMES
  afu_platform_config
  PATHS
  ${CMAKE_SOURCE_DIR}/platforms/scripts
  /usr/bin
  /usr/local/bin
  /opt/local/bin
  DOC "AFU platform configuration utility")

# Find Quartus and Questa
find_package(Quartus)
find_package(Questa)
set(ALTERA_MEGAFUNCTIONS "${QUARTUS_DIR}/../modelsim_ae/altera/verilog/altera_mf" )

# SW library name
set(ASE_SHOBJ_NAME "libopae-c-ase-server")
set(ASE_SHOBJ_SO  ${ASE_SHOBJ_NAME}.so)

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

# VLOG flags
set(questa_flags "")
list(APPEND questa_flags +define+VENDOR_ALTERA)
list(APPEND questa_flags +define+TOOL_QUARTUS)
list(APPEND questa_flags +define+${ASE_SIMULATOR})
list(APPEND questa_flags +define+${ASE_PLATFORM})
_declare_per_build_vars(QUESTA_VLOG_DEFINES "Define flags used by Modelsim/Questa during %build% builds.")
set(QUESTA_VLOG_DEFINES "${questa_flags}"
  CACHE STRING "Modelsim/Questa global define flags" FORCE)

set(questa_flags "")
list(APPEND questa_flags +incdir+.+work+${ASE_SERVER_RTL}+${PLATFORM_IF_RTL})
_declare_per_build_vars(QUESTA_VLOG_DEFINES "Include flags used by Modelsim/Questa during %build% builds.")
set(QUESTA_VLOG_INCLUDES "${questa_flags}"
  CACHE STRING "Modelsim/Questa global include flags" FORCE)

set(questa_flags "")
list(APPEND questa_flags -nologo -sv +librescan)
list(APPEND questa_flags -timescale ${ASE_TIMESCALE})
list(APPEND questa_flags -work work)
list(APPEND questa_flags -novopt)
_declare_per_build_vars(QUESTA_VLOG_FLAGS "Compiler flags used by Modelsim/Questa during %build% builds.")
set(QUESTA_VLOG_FLAGS "${questa_flags}"
  CACHE STRING "Modelsim/Questa global compiler flags" FORCE)

# Per-directory tracking for questa_vlog compiler flags.
define_property(DIRECTORY PROPERTY QUESTA_VLOG_COMPILE_DEFINITIONS
  BRIEF_DOCS "Compiler flags used by Questa_vlog system added in this directory."
  FULL_DOCS "Compiler flags used by Questa_vlog system added in this directory.")

# Per-directory tracking for questa_vlog include directories.
define_property(DIRECTORY PROPERTY QUESTA_VLOG_INCLUDE_DIRECTORIES
  BRIEF_DOCS "Include directories used by Questa_vlog system."
  FULL_DOCS "Include directories used by Questa_vlog system.")

#  questa_vlog_include_directories(dirs ...)
# Add include directories for Questa_Vlog process.
macro(questa_vlog_include_directories)
  get_property(current_flags DIRECTORY PROPERTY QUESTA_VLOG_COMPILE_DEFINITIONS)
  _string_join(" " current_flags "${current_flags}" "+incdir+${flags}")
  set_property(DIRECTORY APPEND PROPERTY QUESTA_VLOG_INCLUDE_DIRECTORIES ${current_flags})
endmacro(questa_vlog_include_directories)

#  questa_vlog_add_definitions (flags)
# Specify additional flags for Questa_Vlog process.
function(questa_vlog_add_definitions flags)
  get_property(current_flags DIRECTORY PROPERTY QUESTA_VLOG_COMPILE_DEFINITIONS)
  _string_join(" " current_flags "${current_flags}" "+define+${flags}")
  set_property(DIRECTORY PROPERTY QUESTA_VLOG_COMPILE_DEFINITIONS "${current_flags}")
endfunction(questa_vlog_add_definitions flags)

# VSIM flags
set(questa_flags "")
list(APPEND questa_flags -novopt)
list(APPEND questa_flags -c)
list(APPEND questa_flags -dpioutoftheblue 1)
list(APPEND questa_flags -sv_lib ${ASE_SHOBJ_NAME})
list(APPEND questa_flags -do vsim_run.tcl)
list(APPEND questa_flags -sv_seed 1234)
list(APPEND questa_flags -L ${ALTERA_MEGAFUNCTIONS})
list(APPEND questa_flags -l vlog_run.log)
string(REPLACE ";" " " questa_flags "${questa_flags}")
_declare_per_build_vars(QUESTA_VSIM_FLAGS "Compiler flags used by Modelsim/Questa during %build% builds.")
set(QUESTA_VSIM_FLAGS "${questa_flags}"
  CACHE STRING "Modelsim/Questa simulator flags" FORCE)

mark_as_advanced(
  QUESTA_VLOG_DEFINES
  QUESTA_VLOG_INCLUDES
  QUESTA_VLOG_FLAGS
  QUESTA_VSIM_FLAGS)

# Helpers for simple extract some ASE module properties.

#  ase_module_get_module_location(RESULT_VAR name)
#
# Return location of the module work library, determined by the property
# ASE_MODULE_WORK_LIB_LOCATION
function(ase_module_get_module_location RESULT_VAR name)
  if(NOT TARGET ${name})
    message(FATAL_ERROR "\"${name}\" is not really a target.")
  endif(NOT TARGET ${name})
  get_property(ase_module_type TARGET ${name} PROPERTY ASE_MODULE_TYPE)
  if(NOT ase_module_type)
    message(FATAL_ERROR "\"${name}\" is not really a target for ASE module.")
  endif(NOT ase_module_type)
  get_property(module_location TARGET ${name} PROPERTY ASE_MODULE_WORK_LIB_LOCATION)
  set(${RESULT_VAR} ${module_location} PARENT_SCOPE)
endfunction(ase_module_get_module_location RESULT_VAR module)

#  ase_module_get_libopae_location(RESULT_VAR name)
#
# Return location of the OPAE server shared library, determined by the property
# ASE_MODULE_LIBOPAE_LOCATION
function(ase_module_get_libopae_location RESULT_VAR name)
  if(NOT TARGET ${name})
    message(FATAL_ERROR "\"${name}\" is not really a target.")
  endif(NOT TARGET ${name})
  get_property(ase_module_type TARGET ${name} PROPERTY ASE_MODULE_TYPE)
  if(NOT ase_module_type)
    message(FATAL_ERROR "\"${name}\" is not really a target for ASE module.")
  endif(NOT ase_module_type)
  get_property(libopae_location TARGET ${name} PROPERTY ASE_MODULE_LIBOPAE_LOCATION)
  set(${RESULT_VAR} ${module_location} PARENT_SCOPE)
endfunction(ase_module_get_libopae_location RESULT_VAR module)

#  ase_module_add_definitions (flags)
# Specify additional flags for compile ASE module.
function(ase_module_add_definitions target_name flags)
  get_property(current_flags TARGET ${target_name} PROPERTY ASE_MODULE_COMPILE_DEFINITIONS)
#  string(CONCAT current_flags "${current_flags}" " " "${flags}")
  set(current_flags "${current_flags} ${flags}")
  set_property(TARGET ${target_name} PROPERTY ASE_MODULE_COMPILE_DEFINITIONS "${current_flags}")
endfunction(ase_module_add_definitions flags)

#  ase_modules_include_directories(dirs ...)
# Add include directories for compile ASE module.
function(ase_module_include_directories target_name dir)
  set_property(TARGET ${target_name} APPEND PROPERTY ASE_MODULE_INCLUDE_DIRECTORIES ${dir})
endfunction(ase_module_include_directories target name_dir)

# Property prefixed with 'ASE_MODULE_' is readonly for outer use,
# unless explicitely noted in its description.

# Absolute location of the work library for the AFU.
define_property(TARGET PROPERTY ASE_MODULE_WORK_LIB_LOCATION
  BRIEF_DOCS "Location of the built ASE module work library."
  FULL_DOCS "Location of the built ASE module work library.")

# Name of the given module, like "intg_xeon_nlb".
define_property(TARGET PROPERTY ASE_MODULE_NAME
  BRIEF_DOCS "Name of the ASE module."
  FULL_DOCS "Name of the ASE module.")

# Type of ASE module, either platform or AFU
define_property(TARGET PROPERTY ASE_MODULE_TYPE
  BRIEF_DOCS "Type of the ASE module."
  FULL_DOCS "Type of the ASE module.")

# Absolute filename of libopae-c-ase-server.so file which is built.
define_property(TARGET PROPERTY ASE_MODULE_LIBOPAE_LOCATION
  BRIEF_DOCS "Location of the libopae-c-ase-server library of the AFU module."
  FULL_DOCS "Location of the libopae-c-ase-server library of the AFU module.")

#  ase_add_modelsim_module(<name> [EXCLUDE_FROM_ALL] [MODULE_NAME <module_name>] [<sources> ...])
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
#
#
# ase_add_modelsim_module(<name> [MODULE_NAME <module_name>] IMPORTED)
#
# In either case, if MODULE_NAME option is given, it determine name
# of the ASE module. Otherwise <name> itself is used.
function(ase_add_modelsim_module name)

  cmake_parse_arguments(ase_add_modelsim_project "IMPORTED;EXCLUDE_FROM_ALL" "MODULE_NAME" "" ${ARGN})
  if(ase_add_modelsim_module_MODULE_NAME)
    set(module_name "${ase_add_modelsim_module_MODULE_NAME}")
  else(ase_add_modelsim_module_MODULE_NAME)
    set(module_name "${name}")
  endif(ase_add_modelsim_module_MODULE_NAME)

  set(ASE_WORKDIR             "${PROJECT_BINARY_DIR}")
  set(ASE_CONFIG              "${PROJECT_BINARY_DIR}/ase.cfg")
  set(ASE_REGRESS_SCRIPT      "${PROJECT_BINARY_DIR}/ase_regress.sh")
  set(ASE_SERVER_SCRIPT       "${PROJECT_BINARY_DIR}/ase_server.sh")
  set(ASE_SIMULATION_SCRIPT   "${PROJECT_BINARY_DIR}/vsim_run.tcl")

  # Create ASE scripts
  configure_file(${ASE_SCRIPTS_IN}/ase.cfg.in
    ${ASE_CONFIG})
  configure_file(${ASE_SCRIPTS_IN}/ase_regress.sh.in
    ${ASE_REGRESS_SCRIPT})
  configure_file(${ASE_SCRIPTS_IN}/vsim_run.tcl.in
    ${ASE_SIMULATION_SCRIPT})
  configure_file(${ASE_SCRIPTS_IN}/ase_server.in
    ${PROJECT_BINARY_DIR}/tmp/ase_server.sh)

  # List of all source files, which are given.
  set(sources ${ase_add_modelsim_module_UNPARSED_ARGUMENTS})

  # Sources with absolute paths
  to_abs_path(sources_abs ${sources})

  # list of files from which module building is depended
  set(source_files)

  # Sources of "txt" type, but without extension.
  # Used for Modelsim project files
  set(prj_sources_noext_abs)
  # The sources with the code in json
  set(json_sources_noext_abs)
  # Sources of SystemVerilog type, but without extension
  set(sverilog_sources_noext_abs)
  # Sources to be configured ('in' extension)
  set(in_sources_noext_abs)

  # Categorize sources
  foreach(file_i ${sources_abs})
    get_filename_component(ext ${file_i} EXT)
    get_filename_component(source_noext "${file_i}" NAME_WE)
    get_filename_component(source_dir "${file_i}" PATH)
    get_filename_component(source_filename "${file_i}" NAME)

    # In files
    if(ext STREQUAL ".txt.in" OR ext STREQUAL ".sh.in" OR ext STREQUAL ".tcl.in" OR ext STREQUAL ".cfg.in" OR ext STREQUAL ".in" OR ext STREQUAL ".txt" OR ext STREQUAL ".sv" OR ext STREQUAL ".v" OR ext STREQUAL ".json")
      # Copy source into binary tree, if needed
      # copy_source_to_binary_dir("${file_i}" file_i)
      if(ext STREQUAL ".txt.in")
        configure_file("${file_i}" ${CMAKE_CURRENT_BINARY_DIR}/${source_noext}.txt)
      elseif(ext STREQUAL ".sh.in")
        configure_file("${file_i}" ${CMAKE_CURRENT_BINARY_DIR}/${source_noext}.sh)
      elseif(ext STREQUAL ".tcl.in")
        configure_file("${file_i}" ${CMAKE_CURRENT_BINARY_DIR}/${source_noext}.tcl)
      elseif(ext STREQUAL ".cfg.in")
        configure_file("${file_i}" ${CMAKE_CURRENT_BINARY_DIR}/${source_noext}.cfg)
      elseif(ext STREQUAL ".in")
        configure_file("${file_i}" ${CMAKE_CURRENT_BINARY_DIR}/${source_noext})
      else()
        configure_file("${file_i}" ${CMAKE_CURRENT_BINARY_DIR}/${source_filename} COPYONLY)
      endif()

      # Categorize sources
      set(source_noext_abs "${CMAKE_CURRENT_BINARY_DIR}/${source_noext}")
      if(ext STREQUAL ".txt.in" OR ext STREQUAL ".txt")
        # project source (source.txt)
        list(APPEND prj_sources_noext_abs ${source_noext_abs})
      elseif(ext STREQUAL ".json")
        # JSON source
        list(APPEND json_sources_noext_abs ${source_noext_abs})
      elseif(ext STREQUAL ".sv")
        # SystemVerilog source
        list(APPEND sverilog_sources_noext_abs ${source_noext_abs})
      endif()
    endif()

    # In any case, add file to depend list
    list(APPEND source_files ${file_i})
  endforeach(file_i ${sources_abs})

  # ASE module sources relative to current binary dir
  set(obj_sources_noext_rel)
  foreach(obj_sources_noext_abs
      ${prj_sources_noext_abs} ${json_sources_noext_abs} ${sverilog_sources_noext_abs})
    file(RELATIVE_PATH obj_source_noext_rel
      ${CMAKE_CURRENT_BINARY_DIR} ${obj_sources_noext_abs})
    list(APPEND obj_sources_noext_rel ${obj_source_noext_rel})
  endforeach(obj_sources_noext_abs)

  if(NOT obj_sources_noext_rel)
    message(FATAL_ERROR "List of object files for building ASE module ${name} is empty.")
  endif(NOT obj_sources_noext_rel)

  # Internal properties
  _get_directory_property_chained(ccflags KBUILD_COMPILE_DEFINITIONS " ")
  _get_directory_property_chained(include_dirs KBUILD_INCLUDE_DIRECTORIES " ")
  foreach(dir ${include_dirs})
    set(ccflags "${ccflags} -I${dir}")
  endforeach(dir ${include_dirs})

  # afu_platform_config --sim --tgt=rtl --src ccip_std_afu.json  intg_xeon
  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/platform_includes)
  add_custom_command(OUTPUT "platform_includes/platform_afu_top_config.vh"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND ${AFU_PLATFORM_CONFIG} --sim --tgt=platform_includes --src ${PROJECT_BINARY_DIR}/ccip_std_afu.json ${ASE_PLATFORM_ABBREV}
    COMMAND {CMAKE_COMMAND} -E touch "platform_includes/platform_afu_top_config.vh")

  # Build DPI header file for ASE
  set(questa_flags ${QUESTA_VLOG_FLAGS})
  list(APPEND questa_flags -f ${CMAKE_BINARY_DIR}/${ASE_PROJECT_SOURCES})
  list(APPEND questa_flags -l vlog_if.log)
  file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include)
  add_custom_command(OUTPUT "include/platform_dpi.h"
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND ${QUESTA_VLIB_EXECUTABLE} work
    COMMAND ${QUESTA_VLOG_EXECUTABLE} -dpiheader ${PROJECT_BINARY_DIR}/include/platform_dpi.h ${questa_flags}
    DEPENDS "platform_includes/platform_afu_top_config.vh")

  # Compile SystemVerilog code for AFU, keep reuse questa_flags
  list(APPEND questa_flags -f ${CMAKE_BINARY_DIR}/${ASE_PROJECT_SOURCES})
  list(APPEND questa_flags -l vlog_afu.log)
  add_custom_command (OUTPUT vlog
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND ${QUESTA_VLOG_EXECUTABLE} ${questa_flags}
    DEPENDS "include/platform_dpi.h")

  # Define SystemVerilog compilation target
  add_custom_target (vlog_compile ALL
    DEPENDS vlog)

  # Create simulation application
  file(COPY ${PROJECT_BINARY_DIR}/tmp/ase_server.sh
    DESTINATION ${PROJECT_BINARY_DIR}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

endfunction(ase_add_modelsim_module name)
