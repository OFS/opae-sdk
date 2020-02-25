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

#  _ase_module_clean_modelsim_files(module_name ...)
#
# Tell CMake that intermediate files, created by kbuild system,
# should be cleaned with 'make clean'.
function(ase_module_clean_modelsim_files module_name)
  cmake_parse_arguments(ase_module_clean_modelsim_files "" "" "SV_SOURCE;JSON_SOURCE;PRJ_SOURCE;SHIPPED_SOURCE" ${ARGN})
  if(ase_module_clean_questa_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "Unparsed arguments")
  endif(ase_module_clean_questa_UNPARSED_ARGUMENTS)

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

endfunction(ase_module_clean_modelsim_files module_name)

############################################################################
## ase_finalize_modelsim_linking(m) ########################################
############################################################################
#
# Should be called after ASE module `m` is defined.
#

function(ase_module_finalize_modelsim_linking m)

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
  get_property(module_build_dir  TARGET ${m} PROPERTY ASE_MODULE_BUILD_DIR)
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
    get_filename_component(ext              ${file_i}  EXT)
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
        configure_file("${file_i}" ${module_build_dir}/${source_filename} @ONLY)
      elseif(ext STREQUAL ".json")
        configure_file("${file_i}" ${module_build_dir}/${source_filename} @ONLY)
        ase_module_set_afu_json(${m} ${source_filename})
      else()
        configure_file("${file_i}" ${module_build_dir}/${source_filename} COPYONLY)
      endif()
    endif()

    # Generate the BBBs project section
    set(AFU_SIM_BBBS)
    get_property(include_bbbs TARGET ${m} PROPERTY ASE_MODULE_INCLUDE_BBBS)
    foreach(bbb_i ${include_bbbs})
      set(AFU_SIM_BBBS "${AFU_SIM_BBBS}# Adding BBB: ${bbb_i}\n")
      set(AFU_SIM_BBBS "${AFU_SIM_BBBS}-F ${FPGA_BBB_CCI_SRC}/BBB_${bbb_i}/hw/sim/${bbb_i}_sim_addenda.txt\n\n")
    endforeach(bbb_i ${include_bbbs})

    # Add simulation project
    configure_file(${ASE_SHARE_DIR}/in/sources_simulation.txt.in
       ${module_build_dir}/sources_simulation.txt @ONLY)
  endforeach(file_i ${ase_module_sources_abs})

  ############################################################################
  ## copy remaining sources files (ASE scripts) ##############################
  ############################################################################
  get_property(ase_module_ase_mode      TARGET ${m} PROPERTY ASE_MODULE_ASE_MODE)
  get_property(ase_module_ase_timeout   TARGET ${m} PROPERTY ASE_MODULE_ASE_TIMEOUT)
  get_property(ase_module_usr_clock_mhz TARGET ${m} PROPERTY ASE_MODULE_USR_CLOCK_MHZ)

  set(ASE_WORKDIR             "${module_build_dir}")
  set(ASE_CONFIG              "${module_build_dir}/ase.cfg")
  set(ASE_REGRESS_SCRIPT      "${module_build_dir}/ase_regress.sh")
  set(ASE_SERVER_SCRIPT       "${module_build_dir}/ase_server.sh")
  set(ASE_SIMULATION_SCRIPT   "${module_build_dir}/vsim_run.tcl")

  if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
    set(vsim_coverage_header "coverage save -onexit ase.ucdb")
  endif(CMAKE_BUILD_TYPE STREQUAL "Coverage")

  # Create ASE scripts
  configure_file(${CMAKE_BINARY_DIR}/libopae/plugins/ase/rtl/sources_ase_server.txt
    ${ASE_WORKDIR}/sources_ase_server.txt)
  configure_file(${CMAKE_BINARY_DIR}/libopae/plugins/ase/rtl/sources_quartus_libs.txt
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
  file(COPY ${module_build_dir}/tmp/ase_server.sh
    DESTINATION ${module_build_dir}
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
    GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

  # Fetch modules sharing same directory
  get_property(ase_module_targets_per_directory
    DIRECTORY "${module_source_dir}"
    PROPERTY ASE_MODULE_TARGETS)

  ############################################################################
  ## Process AFU JSON metadata file ##########################################
  ############################################################################

  ase_module_process_json(${m})

  ############################################################################
  ## Target to build DPI C library
  ############################################################################

  ase_module_add_dpic(${m})

  # Assure DPIC compilation occurs after generation of DPI header file
  add_dependencies(opae-c-ase-server-${m} ${m})

  ############################################################################
  ## Finally set commands to create targets ##################################
  ############################################################################

  # Define DPI header file generation rule
  file(MAKE_DIRECTORY ${module_build_dir}/include)
  add_custom_command(
    OUTPUT "${module_build_dir}/include/platform_dpi.h"
    WORKING_DIRECTORY ${module_build_dir}
    COMMAND ${QUESTA_VLIB_EXECUTABLE} work
    COMMAND ${QUESTA_VLOG_EXECUTABLE}
    -dpiheader ${module_build_dir}/include/platform_dpi.h
    ${vlog_flags}
    ${vlog_flags_local}
    -l vlog.log
    DEPENDS "${module_build_dir}/platform_includes/platform_afu_top_config.vh"
    COMMENT "Building ASE module ${module_name}"
    VERBATIM)

endfunction(ase_module_finalize_modelsim_linking m)
