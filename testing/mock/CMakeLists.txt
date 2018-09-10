cmake_minimum_required (VERSION 2.8)

# projectname is the same as the main-executable
project(test_system)

add_library(test_system SHARED EXCLUDE_FROM_ALL
    test_system.cpp
    ioctl_handlers.cpp)
target_link_libraries(test_system dl)
target_include_directories(test_system PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/libopae/src>)
