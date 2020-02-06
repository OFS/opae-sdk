cmake_minimum_required (VERSION 2.8)

# projectname is the same as the main-executable
project(framework)

set(CMAKE_CXX_STANDARD 11)

# Disable some warnings that fire in system libraries
check_cxx_compiler_flag("-Wno-error=frame-address"
  CXX_SUPPORTS_NO_ERROR_FRAME_ADDRESS)

opae_add_shared_library(TARGET test_system
    SOURCE mock/test_system.cpp mock/ioctl_handlers.cpp
    LIBS fpga_db dl
)

if (CXX_SUPPORTS_NO_ERROR_FRAME_ADDRESS)
  set_target_properties(test_system PROPERTIES COMPILE_FLAGS "-Wno-error=frame-address")
endif()

target_include_directories(test_system PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_PATH}>
  PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
          $<BUILD_INTERFACE:${OPAE_LIBS_ROOT}/libopae-c>
          $<BUILD_INTERFACE:${OPAE_LIBS_ROOT}/plugins/xfpga>)


opae_add_shared_library(TARGET fpga_db
    SOURCE platform/fpga_hw.cpp
)

if(OPAE_ENABLE_MOCK)
    target_compile_definitions(fpga_db PUBLIC -DENABLE_MOCK)
    
    add_custom_command(TARGET fpga_db
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-1socket-nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-1socket-nlb0-vf.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dcp-rc-nlb3.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dfl0-nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dcp-vc.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dfl0_patchset2-nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dcp-rc-dfl0_patchset2-nlb0.tar.gz
            ${CMAKE_BINARY_DIR}
    )
endif(OPAE_ENABLE_MOCK)
