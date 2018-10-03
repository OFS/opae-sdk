cmake_minimum_required (VERSION 2.8)

# projectname is the same as the main-executable
project(testing)
find_package(GTest)

If(${GTest_FOUND})
  message(STATUS Found GTest)
else()
    include(test_config)
    Build_GTEST()
endif()

add_subdirectory(mock)

add_library(opae-c-static
    ${OPAE_SDK_SOURCE}/libopae/api-shell.c
    ${OPAE_SDK_SOURCE}/libopae/init.c
    ${OPAE_SDK_SOURCE}/libopae/pluginmgr.c
    ${OPAE_SDK_SOURCE}/libopae/props.c
    )
target_include_directories(opae-c-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga
  PRIVATE ${OPAE_SDK_SOURCE}/libopae)

add_library(opae-cxx-core-static
    ${OPAE_SDK_SOURCE}/libopaecxx/src/errors.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/events.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/except.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/handle.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/properties.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/shared_buffer.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/token.cpp
#    ${OPAE_SDK_SOURCE}/libopaecxx/src/version.cpp
    )
target_include_directories(opae-cxx-core-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE ${OPAE_SDK_SOURCE}/libopaecxx)

add_library(xfpga-static STATIC
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/bitstream.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/buffer.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/close.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/common.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/enum.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/error.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/event.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/hostif.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/init.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/manage.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/mmap.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/mmio.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/open.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/properties.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/reconf.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/reset.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/sysfs.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/sysobject.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/token_list.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/umsg.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/userclk.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/version.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/wsid_list.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/usrclk/user_clk_pgm_uclock.c)

target_include_directories(xfpga-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga
  PRIVATE ${OPAE_SDK_SOURCE}/libopae)

add_library(fpgad-static
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/ap6.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/ap_event.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/daemonize.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/errtable.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/evt.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/log.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/srv.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/fpgad.c
    )

target_compile_definitions(fpgad-static PRIVATE main=fpgad_main)

target_include_directories(fpgad-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>)

set(CMAKE_C_FLAGS "-DSTATIC='' -DHAVE_CONFIG_H ${CMAKE_C_FLAGS} -pthread")

add_custom_target(test_unit)

macro(add_mock_test test_name static_lib)
    add_executable(${test_name}
        mock/mock.c
        ${ARGN})
    target_link_libraries(${test_name}
        ${static_lib}
        opae-c
        safestr
        test_system
        ${libjson-c_LIBRARIES}
        ${libuuid_LIBRARIES}
        ${GTEST_BOTH_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT})
    add_test(
        NAME ${test_name}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND $<TARGET_FILE:${test_name}>
        )
    add_dependencies(test_unit ${test_name})

endmacro(add_mock_test test_name)


############################################################################
############################################################################
############################################################################


include_directories(
    ${OPAE_SDK_SOURCE}/common/include
    ${PROJECT_BINARY_DIR}/include
    ${OPAE_SDK_SOURCE}/libopae
    ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga
    ${OPAE_SDK_SOURCE}/tools/base
    mock
    ${GTEST_INCLUDE_DIRS}
    {CMAKE_CURRENT_SOURCE_DIR}
    {CMAKE_CURRENT_SOURCE_DIR}/mock
    )

############################################################################
# opae-c tests #############################################################
############################################################################

add_mock_test(test_opae_enum_c opae-c-static
    opae-c/test_enum_c.cpp
)

add_mock_test(test_opae_open_c opae-c-static
    opae-c/test_open_c.cpp
)

add_mock_test(test_opae_props_c opae-c-static
    opae-c/test_props_c.cpp
)

add_mock_test(test_opae_reset_c opae-c-static
    opae-c/test_reset_c.cpp
)

add_mock_test(test_opae_mmio_c opae-c-static
    opae-c/test_mmio_c.cpp
)

add_mock_test(test_opae_umsg_c opae-c-static
    opae-c/test_umsg_c.cpp
)

add_mock_test(test_opae_buffer_c opae-c-static
    opae-c/test_buffer_c.cpp
)

add_mock_test(test_opae_version_c opae-c-static
    opae-c/test_version_c.cpp
)

add_mock_test(test_opae_error_c opae-c-static
    opae-c/test_error_c.cpp
)

add_mock_test(test_opae_event_c opae-c-static
    opae-c/test_event_c.cpp
)
target_link_libraries(test_opae_event_c fpgad-static safestr)

add_mock_test(test_opae_hostif_c opae-c-static
    opae-c/test_hostif_c.cpp
)

add_mock_test(test_opae_reconf_c opae-c-static
    opae-c/test_reconf_c.cpp
)

add_mock_test(test_opae_object_c opae-c-static
    opae-c/test_object_c.cpp
)

add_mock_test(test_opae_usrclk_c opae-c-static
    opae-c/test_usrclk_c.cpp
)

add_mock_test(test_opae_init_c opae-c-static
    opae-c/test_init_c.cpp
)

add_mock_test(test_opae_pluginmgr_c opae-c-static
    opae-c/test_pluginmgr_c.cpp
)

############################################################################
# cxx core tests ###########################################################
############################################################################
add_mock_test(test_opae_handle_cxx_core opae-cxx-core-static
    opae-cxx/test_handle_cxx_core.cpp
)

############################################################################
# xfpga tests ##############################################################
############################################################################

add_mock_test(test_xfpga_token_list_c xfpga-static
    xfpga/test_token_list_c.cpp
)

add_mock_test(test_xfpga_error_c xfpga-static
    xfpga/test_error_c.cpp
)

add_mock_test(test_xfpga_mock_errinj_c xfpga-static
    xfpga/test_mock_errinj_c.cpp
)

add_mock_test(test_xfpga_common_c xfpga-static
    xfpga/test_common_c.cpp
)

add_mock_test(test_xfpga_properties_c xfpga-static
    xfpga/test_properties_c.cpp
)

add_mock_test(test_xfpga_object_c xfpga-static
    xfpga/test_object_c.cpp
)

add_mock_test(test_xfpga_enum_c xfpga-static
    xfpga/test_enum_c.cpp
)

add_mock_test(test_xfpga_buffer_c xfpga-static
    xfpga/test_buffer_c.cpp
)

add_mock_test(test_xfpga_wsid_list_c xfpga-static
    xfpga/test_wsid_list_c.cpp
)

add_mock_test(test_xfpga_sysfs_c xfpga-static
    xfpga/test_sysfs_c.cpp
)

add_mock_test(test_xfpga_mmio_c xfpga-static
    xfpga/test_mmio_c.cpp
)

add_mock_test(test_xfpga_metadata_c xfpga-static
    xfpga/test_metadata_c.cpp
)

add_mock_test(test_xfpga_reconf_c xfpga-static
    xfpga/test_reconf_c.cpp
)

add_mock_test(test_xfpga_version_c xfpga-static
    xfpga/test_version_c.cpp
)

add_mock_test(test_xfpga_open_close_c xfpga-static
    xfpga/test_open_close_c.cpp
)

add_mock_test(test_xfpga_usrclk_c xfpga-static
    xfpga/test_usrclk_c.cpp
)

add_mock_test(test_xfpga_reset_c xfpga-static
    xfpga/test_reset_c.cpp
)

add_mock_test(test_xfpga_umsg_c xfpga-static
    xfpga/test_umsg_c.cpp
)

add_mock_test(test_xfpga_manage_c xfpga-static
    xfpga/test_manage_c.cpp
)

add_mock_test(test_xfpga_events_c xfpga-static
    xfpga/test_events_c.cpp
)
target_link_libraries(test_xfpga_events_c fpgad-static safestr 
        ${libjson-c_LIBRARIES})

############################################################################
# fpgad tests ##############################################################
############################################################################

add_mock_test(test_fpgad_log_c fpgad-static
    fpgad/test_log_c.cpp
)

add_mock_test(test_fpgad_srv_c fpgad-static
    fpgad/test_srv_c.cpp
)

add_mock_test(test_fpgad_evt_c fpgad-static
    fpgad/test_evt_c.cpp
)

add_mock_test(test_fpgad_errtable_c fpgad-static
    fpgad/test_errtable_c.cpp
)

add_mock_test(test_fpgad_ap_event_c fpgad-static
    fpgad/test_ap_event_c.cpp
)

add_mock_test(test_fpgad_daemonize_c fpgad-static
    fpgad/test_daemonize_c.cpp
)

add_mock_test(test_fpgad_fpgad_c fpgad-static
    fpgad/test_fpgad_c.cpp
)

add_mock_test(test_fpgad_ap6_c fpgad-static
    fpgad/test_ap6_c.cpp
)

add_custom_command(TARGET test_unit
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-1socket-nlb0.tar.gz
    ${CMAKE_BINARY_DIR})
