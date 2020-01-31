#cmake_minimum_required (VERSION 2.8)

# projectname is the same as the main-executable
#project(plugins)

add_subdirectory(xfpga)
#add_subdirectory(xfpga/metrics/bmc)

if(BUILD_ASE)
  add_subdirectory(ase)
endif()
