cmake_minimum_required (VERSION 2.8)

if (ENABLE_MOCK)
  add_definitions(-DENABLE_MOCK)
endif (ENABLE_MOCK)

# Enable checking compiler flags
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# projectname is the same as the main-executable
project(fpga_db)
include_directories(
    ${CMAKE_SOURCE_DIR}/testing/mock
    ${OPAE_INCLUDE_DIR}
    )
add_library(fpga_db SHARED
    fpga_hw.cpp
)

add_executable(discover_fpgas discover.cpp)
target_link_libraries(discover_fpgas fpga_db)
