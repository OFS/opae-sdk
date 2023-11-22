
if(NOT "/root/mybuild_docs/numa-prefix/src/numa-stamp/numa-gitinfo.txt" IS_NEWER_THAN "/root/mybuild_docs/numa-prefix/src/numa-stamp/numa-gitclone-lastrun.txt")
  message(STATUS "Avoiding repeated git clone, stamp file is up to date: '/root/mybuild_docs/numa-prefix/src/numa-stamp/numa-gitclone-lastrun.txt'")
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "/root/mybuild_docs/numa-prefix/src/numa"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: '/root/mybuild_docs/numa-prefix/src/numa'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "/usr/bin/git"  clone --no-checkout --config "advice.detachedHead=false" "https://github.com/numactl/numactl.git" "numa"
    WORKING_DIRECTORY "/root/mybuild_docs/numa-prefix/src"
    RESULT_VARIABLE error_code
    )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once:
          ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/numactl/numactl.git'")
endif()

execute_process(
  COMMAND "/usr/bin/git"  checkout v2.0.16 --
  WORKING_DIRECTORY "/root/mybuild_docs/numa-prefix/src/numa"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: 'v2.0.16'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "/usr/bin/git"  submodule update --recursive --init 
    WORKING_DIRECTORY "/root/mybuild_docs/numa-prefix/src/numa"
    RESULT_VARIABLE error_code
    )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: '/root/mybuild_docs/numa-prefix/src/numa'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy
    "/root/mybuild_docs/numa-prefix/src/numa-stamp/numa-gitinfo.txt"
    "/root/mybuild_docs/numa-prefix/src/numa-stamp/numa-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
  )
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: '/root/mybuild_docs/numa-prefix/src/numa-stamp/numa-gitclone-lastrun.txt'")
endif()

