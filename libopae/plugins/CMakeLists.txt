cmake_minimum_required (VERSION 2.8)

# projectname is the same as the main-executable
project(plugins)

add_subdirectory(xfpga)
add_subdirectory(xfpga/metrics/bmc)
add_subdirectory(xfpga/feature_plugins/dma_sav/intel-dma) # intel-dma is a place holder for the real DMA library
