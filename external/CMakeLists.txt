cmake_minimum_required(VERSION 2.8)

# projectname is the same as the main-executable
project(external)
# Download and unpack dependencies at configure time

configure_file(opae-test-CMakeLists.txt.in
               ${CMAKE_CURRENT_BINARY_DIR}/opae-test/download/CMakeLists.txt)

execute_process(
    COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/opae-test/download)

if(result)
  message(FATAL_ERROR "CMake step for opae-test failed: ${result}")
endif()
execute_process(
    COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/opae-test/download)
if(result)
    message(FATAL_ERROR "Build step for opae-test failed:${result}")
endif()

# Add googletest directly to our build. This defines
# the opae-test targets
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/opae-test
                 ${CMAKE_CURRENT_BINARY_DIR}/opae-test
                 EXCLUDE_FROM_ALL)
