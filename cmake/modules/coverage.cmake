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

# Check prereqs
find_program(GCOV_EXECUTABLE gcov)
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

if(NOT GCOV_EXECUTABLE)
  message(FATAL_ERROR "gcov not found! Aborting...")
endif()

set(GCOV_COMPILE_FLAGS "-g -O0 --coverage -fprofile-arcs -ftest-coverage -Wall -Wextra -Werror")
set(GCOV_LINK_FLAGS "-lgcov")

set(CMAKE_CXX_FLAGS_COVERAGE
  ${GCOV_COMPILE_FLAGS}
  CACHE STRING "Flags used by the C++ compiler during coverage builds."
  FORCE)
set(CMAKE_C_FLAGS_COVERAGE
  ${GCOV_COMPILE_FLAGS}
  CACHE STRING "Flags used by the C compiler during coverage builds."
  FORCE)
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE
  ${GCOV_LINK_FLAGS}
  CACHE STRING "Flags used for linking binaries during coverage builds."
  FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_COVERAGE
  ${GCOV_LINK_FLAGS}
  CACHE STRING "Flags used by the shared libraries linker during coverage builds."
  FORCE)
mark_as_advanced(
  CMAKE_C_FLAGS_COVERAGE
  CMAKE_CXX_FLAGS_COVERAGE
  CMAKE_EXE_LINKER_FLAGS_COVERAGE
  CMAKE_SHARED_LINKER_FLAGS_COVERAGE)

# targetname     The name of original target from which the current target depends on
# testrunner     The name of the target which runs the tests.
# Optional third parameter is passed as arguments to testrunner
# Pass them in list form, e.g.: "-k;1" for -k 1
function(set_target_for_coverage target_name testrunner)

  if(NOT LCOV_EXECUTABLE)
    message(FATAL_ERROR "lcov not found! Aborting...")
  endif()

  if(NOT GENHTML_EXECUTABLE)
    message(FATAL_ERROR "genhtml not found! Aborting...")
  endif()

  message("-- Setting ${target_name} for coverage run.")
  set(outputfile ${target_name})
  set(coverage_info "${CMAKE_BINARY_DIR}/coverage_${target_name}/${outputfile}.info")
  set(coverage_cleaned "${coverage_info}.cleaned")
  set(coverage_runtest_script "coverage_${target_name}.sh")

  separate_arguments(test_command UNIX_COMMAND "${testrunner}")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/config/run_coverage_test.sh.in
    ${CMAKE_BINARY_DIR}/${coverage_runtest_script})

  # Setup target
  set(name "coverage_${target_name}")
  add_custom_target(${name}

    # Cleanup lcov
    COMMAND ${LCOV_EXECUTABLE} --directory . --zerocounters

    # Wrap test on script, so coverage files generate even if tests return 1
    # CMake will stop if this step returns 1
    COMMAND chmod 755 ${coverage_runtest_script}
    COMMAND ${CMAKE_BINARY_DIR}/${coverage_runtest_script}

    # Capturing lcov counters and generating report
    COMMAND ${LCOV_EXECUTABLE} -t ${target_name} -o ${coverage_info} -c -d ${CMAKE_BINARY_DIR}/coverage_${target_name}

    # Clean coverage file
    COMMAND ${LCOV_EXECUTABLE} --remove ${coverage_info} '/usr/**' 'tests/**' '*/**/*CMakefiles*' ${LCOV_REMOVE_EXTRA} --output-file ${coverage_cleaned}
    COMMAND ${GENHTML_EXECUTABLE} --branch-coverage --function-coverage ${coverage_info} -o coverage_${target_name} ${coverage_cleaned}
    COMMAND ${CMAKE_COMMAND} -E remove ${coverage_info} ${coverage_cleaned}

    # Add dependencies
    DEPENDS ${target_name}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Run coverage tests.")

endfunction()

# targetname     The name of original target from which the current target depends on
# testrunner     The name of the target which runs the tests.
# Optional third parameter is passed as arguments to testrunner
# Pass them in list form, e.g.: "-k;1" for -k 1
function(set_target_for_coverage_local target_name)
  if(BUILD_ASE_INTR)
    cmake_parse_arguments(set_target_for_coverage_local "" "TESTRUNNER" "TESTRUNNER_ARGS;COVERAGE_EXTRA_COMPONENTS;COVERAGE_EXTRA_COMPONENTS2" ${ARGN})
  else()
    cmake_parse_arguments(set_target_for_coverage_local "" "TESTRUNNER" "TESTRUNNER_ARGS;COVERAGE_EXTRA_COMPONENTS" ${ARGN})
  endif()
  set(testrunner ${set_target_for_coverage_local_TESTRUNNER})
  set(COVERAGE_EXTRA 0)
  set(COVERAGE_EXTRA_COMPONENTS "")
  foreach(comp_i ${set_target_for_coverage_local_COVERAGE_EXTRA_COMPONENTS})
	set(COVERAGE_EXTRA_COMPONENTS "${COVERAGE_EXTRA_COMPONENTS} ${comp_i}")
	set(COVERAGE_EXTRA 1)
  endforeach()

  set(COVERAGE_EXTRA2 0)
  set(COVERAGE_EXTRA_COMPONENTS2 "")
  if(BUILD_ASE_INTR)
    foreach(comp_i ${set_target_for_coverage_local_COVERAGE_EXTRA_COMPONENTS2})
	  set(COVERAGE_EXTRA_COMPONENTS2 "${COVERAGE_EXTRA_COMPONENTS2} ${comp_i}")
	  set(COVERAGE_EXTRA2 1)
    endforeach()
  endif()

  set(TESTRUNNER_ARGS "")
  foreach(arg_i ${set_target_for_coverage_local_TESTRUNNER_ARGS})
    set(TESTRUNNER_ARGS "${TESTRUNNER_ARGS} ${arg_i}")
  endforeach()

  if(NOT LCOV_EXECUTABLE)
    message(FATAL_ERROR "lcov not found! Aborting...")
  endif()

  if(NOT GENHTML_EXECUTABLE)
    message(FATAL_ERROR "genhtml not found! Aborting...")
  endif()

  message("-- Setting ${target_name} for coverage run.")
  set(outputfile ${target_name})
  set(coverage_info "${CMAKE_BINARY_DIR}/coverage_${target_name}/${outputfile}.info")
  set(coverage_cleaned "${coverage_info}.cleaned")
  set(coverage_runtest_script "coverage_${target_name}.sh")

  separate_arguments(test_command UNIX_COMMAND "${testrunner}")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/config/run_coverage_test_local.sh.in
    ${CMAKE_BINARY_DIR}/${coverage_runtest_script})

  # Setup target
  set(name "coverage_${target_name}")
  if(NOT BUILD_ASE_TESTS)
		add_custom_target(${name} 

		COMMAND ${LCOV_EXECUTABLE} --directory . --zerocounters
		# Wrap test on script, so coverage files generate even if tests return 1
		# CMake will stop if this step returns 1
		COMMAND chmod 755 ${coverage_runtest_script}
		COMMAND ${CMAKE_BINARY_DIR}/${coverage_runtest_script}

		# Capturing lcov counters and generating report
		COMMAND ${LCOV_EXECUTABLE} -t ${target_name} -o ${coverage_info} -c -d ${CMAKE_BINARY_DIR}/coverage_${target_name}

		# Clean coverage file
		COMMAND ${LCOV_EXECUTABLE} --remove ${coverage_info} '/usr/**' 'tests/**' '*/**/*CMakefiles*' ${LCOV_REMOVE_EXTRA} --output-file ${coverage_cleaned}
		COMMAND ${GENHTML_EXECUTABLE} --branch-coverage --function-coverage ${coverage_info} -o coverage_${target_name} ${coverage_cleaned}
		COMMAND ${CMAKE_COMMAND} -E remove ${coverage_info} ${coverage_cleaned}

		# Add dependencies
		DEPENDS ${target_name}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMENT "Run coverage tests.")
  else()
 		add_custom_target(${name} 

		# Wrap test on script, so coverage files generate even if tests return 1
		# CMake will stop if this step returns 1
		COMMAND chmod 755 ${coverage_runtest_script}
		COMMAND ${CMAKE_BINARY_DIR}/${coverage_runtest_script}	

		# Capturing lcov counters from ASE client, NLB test and interrupt test
		COMMAND ${LCOV_EXECUTABLE} -o ase_client.info -c -d ${CMAKE_BINARY_DIR}/coverage_${target_name}
		COMMAND ${LCOV_EXECUTABLE} -o ase_nlb.info 	-c -d ${CMAKE_BINARY_DIR}/coverage_${target_name}/nlb
		COMMAND ${LCOV_EXECUTABLE} -o ase_intr.info  -c -d ${CMAKE_BINARY_DIR}/coverage_${target_name}/intr

		# Capturing lcov counters and generating report
		COMMAND ${LCOV_EXECUTABLE} -a ase_client.info -a ase_nlb.info -a ase_intr.info -t ${target_name} -o ${coverage_info}

		# Clean coverage file
		COMMAND ${LCOV_EXECUTABLE} --remove ${coverage_info} '/usr/**' 'tests/**' '*/**/*CMakefiles*' ${LCOV_REMOVE_EXTRA} --output-file ${coverage_cleaned}
		COMMAND ${GENHTML_EXECUTABLE} --branch-coverage --function-coverage ${coverage_info} -o coverage_${target_name} ${coverage_cleaned}
		COMMAND ${CMAKE_COMMAND} -E remove ${coverage_info} ${coverage_cleaned}

		# Add dependencies
		DEPENDS ${target_name}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMENT "Run coverage tests.")
  endif()


endfunction()
