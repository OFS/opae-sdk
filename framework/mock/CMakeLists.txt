cmake_minimum_required (VERSION 2.8)

# Enable checking compiler flags
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# projectname is the same as the main-executable
project(test_system)
set(CMAKE_CXX_STANDARD 11)
# Disable some warnings that fire in system libraries
check_cxx_compiler_flag("-Wno-error=frame-address"
  CXX_SUPPORTS_NO_ERROR_FRAME_ADDRESS)
add_library(test_system SHARED EXCLUDE_FROM_ALL
    test_system.cpp
    ioctl_handlers.cpp)
if (CXX_SUPPORTS_NO_ERROR_FRAME_ADDRESS)
  set_target_properties(test_system PROPERTIES COMPILE_FLAGS "-Wno-error=frame-address")
endif()
target_link_libraries(test_system fpga_db dl)
target_include_directories(test_system PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/testing>
          $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libopae>
          $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libopae/plugins/xfpga>)
