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

add_subdirectory(platform)
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

add_library(bitstream-static
    ${OPAE_SDK_SOURCE}/libbitstream/bitstream.c
    )
target_include_directories(bitstream-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_link_libraries(bitstream-static
    safestr
    ${libjson-c_LIBRARIES}
    ${libuuid_LIBRARIES}
    )

add_library(opae-cxx-core-static
    ${OPAE_SDK_SOURCE}/libopaecxx/src/errors.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/events.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/except.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/handle.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/properties.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/shared_buffer.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/token.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/sysobject.cpp
    ${OPAE_SDK_SOURCE}/libopaecxx/src/version.cpp
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
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/opae_drv.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/properties.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/plugin.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/reconf.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/reset.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/sysfs.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/sysobject.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/token_list.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/umsg.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/userclk.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/version.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/wsid_list.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/usrclk/user_clk_pgm_uclock.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/metrics/metrics_max10.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/metrics/metrics_utils.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/metrics/afu_metrics.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/metrics/metrics.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/metrics/vector.c
              ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/metrics/threshold.c)

target_include_directories(xfpga-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga
  PRIVATE ${OPAE_SDK_SOURCE}/libopae)

add_library(bmc-static STATIC 
    ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/metrics/bmc/bmc.c
    ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/metrics/bmc/bmc_ioctl.c
    ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/metrics/bmc/bmcdata.c
    ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/metrics/bmc/bmcinfo.c
    )
  
add_library(fpgad-static
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/command_line.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/config_file.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/daemonize.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/event_dispatcher_thread.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/events_api_thread.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/fpgad.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/monitored_device.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/monitor_thread.c
    )

target_compile_definitions(fpgad-static PRIVATE main=fpgad_main)

target_include_directories(fpgad-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  PRIVATE ${OPAE_SDK_SOURCE}/libbitstream
  $<INSTALL_INTERFACE:include>)

target_link_libraries(fpgad-static
    safestr
    bitstream-static
    ${libjson-c_LIBRARIES}
    ${libuuid_LIBRARIES}
    )

add_library(fpgad-api-static
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/api/logging.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/api/opae_events_api.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/api/device_monitoring.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/api/sysfs.c
    ) 

target_include_directories(fpgad-api-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE ${OPAE_SDK_SOURCE}/libopae
  PRIVATE ${OPAE_SDK_SOURCE}/libbitstream
  PRIVATE ${OPAE_SDK_SOURCE}/tools/base
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  )

target_link_libraries(fpgad-api-static
    safestr
    ${CMAKE_THREAD_LIBS_INIT}
    ${libjson-c_LIBRARIES}
    )

add_library(fpgad-xfpga-static
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/plugins/fpgad-xfpga/fpgad-xfpga.c
    ) 

target_include_directories(fpgad-xfpga-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE ${OPAE_SDK_SOURCE}/libopae
  PRIVATE ${OPAE_SDK_SOURCE}/libbitstream
  PRIVATE ${OPAE_SDK_SOURCE}/tools/base
  )

target_link_libraries(fpgad-xfpga-static
    fpgad-api-static
    ${CMAKE_THREAD_LIBS_INIT}
    )

add_library(fpgad-vc-static
    ${OPAE_SDK_SOURCE}/tools/base/fpgad/plugins/fpgad-vc/fpgad-vc.c
    ) 

target_include_directories(fpgad-vc-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
  PRIVATE ${OPAE_SDK_SOURCE}/libopae
  PRIVATE ${OPAE_SDK_SOURCE}/libbitstream
  PRIVATE ${OPAE_SDK_SOURCE}/tools/base
  )

target_link_libraries(fpgad-vc-static
    ${CMAKE_THREAD_LIBS_INIT}
    )

add_library(fpgaconf-static
    ${OPAE_SDK_SOURCE}/tools/base/fpgaconf/fpgaconf.c
    )

target_compile_definitions(fpgaconf-static PRIVATE main=fpgaconf_main)

target_include_directories(fpgaconf-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>)

add_library(ras-static
    ${OPAE_SDK_SOURCE}/tools/extra/ras/main.c)

target_compile_definitions(ras-static PRIVATE main=ras_main)

target_include_directories(ras-static PUBLIC
    $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>)

add_library(coreidle-static
    ${OPAE_SDK_SOURCE}/tools/extra/coreidle/coreidle.c
    ${OPAE_SDK_SOURCE}/tools/extra/coreidle/main.c
    )

target_compile_definitions(coreidle-static PRIVATE main=coreidle_main)

target_include_directories(coreidle-static PUBLIC
    $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>)

add_library(userclk-static
    ${OPAE_SDK_SOURCE}/tools/extra/userclk/main.c
    )

target_compile_definitions(userclk-static PRIVATE main=userclk_main)

target_include_directories(userclk-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>)

add_library(argsfilter-static
    ${OPAE_SDK_SOURCE}/tools/base/argsfilter/argsfilter.c
    )

target_include_directories(argsfilter-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>)

add_library(fpgainfo-static
    ${OPAE_SDK_SOURCE}/tools/base/fpgainfo/bmcdata.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgainfo/bmcinfo.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgainfo/errors.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgainfo/fmeinfo.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgainfo/fpgainfo.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgainfo/portinfo.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgainfo/powerinfo.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgainfo/tempinfo.c
    ${OPAE_SDK_SOURCE}/tools/base/fpgainfo/main.c
    )

target_compile_definitions(fpgainfo-static PRIVATE main=fpgainfo_main)

target_include_directories(fpgainfo-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  PRIVATE ${OPAE_SDK_SOURCE}/tools/base/argsfilter
  $<INSTALL_INTERFACE:include>)

add_library(hello_fpga-static
    ${OPAE_SDK_SOURCE}/samples/hello_fpga.c
    )

target_compile_definitions(hello_fpga-static PRIVATE main=hello_fpga_main)

target_include_directories(hello_fpga-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>)

add_library(hello_events-static
    ${OPAE_SDK_SOURCE}/samples/hello_events.c
    )

target_compile_definitions(hello_events-static PRIVATE main=hello_events_main)

target_include_directories(hello_events-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>)

add_library(object_api-static
    ${OPAE_SDK_SOURCE}/samples/object_api.c
    )

target_compile_definitions(object_api-static PRIVATE main=object_api_main)

target_include_directories(object_api-static PUBLIC
  $<BUILD_INTERFACE:${OPAE_INCLUDE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>)



set(CMAKE_C_FLAGS "-DSTATIC='' -DHAVE_CONFIG_H ${CMAKE_C_FLAGS} -pthread")

add_custom_target(test_unit)
add_dependencies(test_unit xfpga)
if (ENABLE_MOCK)
    set(MOCK_C mock/mock.c)
endif(ENABLE_MOCK)

macro(add_unit_test test_name static_lib)
    add_executable(${test_name}
        ${MOCK_C}
        ${ARGN})
    target_include_directories(${test_name} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        PRIVATE ${OPAE_SDK_SOURCE}/libbitstream
        )
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

endmacro(add_unit_test test_name)


############################################################################
############################################################################
############################################################################


include_directories(
    ${OPAE_SDK_SOURCE}/common/include
    ${PROJECT_BINARY_DIR}/include
    ${OPAE_SDK_SOURCE}/libopae
    ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga
    ${OPAE_SDK_SOURCE}/libopae/plugins/xfpga/metrics/bmc
    ${OPAE_SDK_SOURCE}/tools/base
    mock
    ${GTEST_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/mock
    )

############################################################################
# opae-c tests #############################################################
############################################################################

add_unit_test(test_opae_enum_c opae-c-static
    opae-c/test_enum_c.cpp
)

add_unit_test(test_opae_open_c opae-c-static
    opae-c/test_open_c.cpp
)

add_unit_test(test_opae_props_c opae-c-static
    opae-c/test_props_c.cpp
)

add_unit_test(test_opae_reset_c opae-c-static
    opae-c/test_reset_c.cpp
)

add_unit_test(test_opae_mmio_c opae-c-static
    opae-c/test_mmio_c.cpp
)

add_unit_test(test_opae_umsg_c opae-c-static
    opae-c/test_umsg_c.cpp
)

add_unit_test(test_opae_buffer_c opae-c-static
    opae-c/test_buffer_c.cpp
)

add_unit_test(test_opae_version_c opae-c-static
    opae-c/test_version_c.cpp
)

add_unit_test(test_opae_error_c opae-c-static
    opae-c/test_error_c.cpp
)

add_unit_test(test_opae_event_c opae-c-static
    opae-c/test_event_c.cpp
)
target_link_libraries(test_opae_event_c
    safestr
    fpgad-static
    fpgad-api-static
    )

add_unit_test(test_opae_hostif_c opae-c-static
    opae-c/test_hostif_c.cpp
)

add_unit_test(test_opae_reconf_c opae-c-static
    opae-c/test_reconf_c.cpp
)

add_unit_test(test_opae_object_c opae-c-static
    opae-c/test_object_c.cpp
)

add_unit_test(test_opae_usrclk_c opae-c-static
    opae-c/test_usrclk_c.cpp
)

add_unit_test(test_opae_init_c opae-c-static
    opae-c/test_init_c.cpp
)

add_unit_test(test_opae_pluginmgr_c opae-c-static
    opae-c/test_pluginmgr_c.cpp
)
set_tests_properties(test_opae_pluginmgr_c
    PROPERTIES ENVIRONMENT "LD_LIBRARY_PATH=${LIBRARY_OUTPUT_PATH}")
add_library(dummy_plugin MODULE opae-c/dummy_plugin.c)
target_link_libraries(dummy_plugin ${libjson-c_LIBRARIES})
add_dependencies(test_opae_pluginmgr_c dummy_plugin)

############################################################################
# bitstream tests ##########################################################
############################################################################

add_unit_test(test_bitstream_bitstream_c bitstream-static
    bitstream/test_bitstream_c.cpp
)

############################################################################
# cxx core tests ###########################################################
############################################################################

add_unit_test(test_opae_handle_cxx_core opae-cxx-core-static
    opae-cxx/test_handle_cxx_core.cpp
)

add_unit_test(test_opae_buffer_cxx_core opae-cxx-core-static
    opae-cxx/test_buffer_cxx_core.cpp
)

add_unit_test(test_opae_errors_cxx_core opae-cxx-core-static
    opae-cxx/test_errors_cxx_core.cpp
)

add_unit_test(test_opae_events_cxx_core opae-cxx-core-static
    opae-cxx/test_events_cxx_core.cpp
)
target_link_libraries(test_opae_events_cxx_core
    safestr
    fpgad-static
    fpgad-api-static
    ${libjson-c_LIBRARIES})

add_unit_test(test_opae_properties_cxx_core opae-cxx-core-static
    opae-cxx/test_properties_cxx_core.cpp
)

add_unit_test(test_opae_version_cxx_core opae-cxx-core-static
    opae-cxx/test_version_cxx_core.cpp
)

add_unit_test(test_opae_except_cxx_core opae-cxx-core-static
    opae-cxx/test_except_cxx_core.cpp
)

add_unit_test(test_opae_object_cxx_core opae-cxx-core-static
    opae-cxx/test_object_cxx_core.cpp
)

############################################################################
# xfpga tests ##############################################################
############################################################################

add_unit_test(test_xfpga_token_list_c xfpga-static
    xfpga/test_token_list_c.cpp
)

add_unit_test(test_xfpga_error_c xfpga-static
    xfpga/test_error_c.cpp
)

set_tests_properties(test_xfpga_error_c
    PROPERTIES ENVIRONMENT "LIBOPAE_LOG=1")
                     
add_unit_test(test_xfpga_mock_errinj_c xfpga-static
    xfpga/test_mock_errinj_c.cpp
)

add_unit_test(test_xfpga_common_c xfpga-static
    xfpga/test_common_c.cpp
)

add_unit_test(test_xfpga_properties_c xfpga-static
    xfpga/test_properties_c.cpp
)

add_unit_test(test_xfpga_object_c xfpga-static
    xfpga/test_object_c.cpp
)

add_unit_test(test_xfpga_enum_c xfpga-static
    xfpga/test_enum_c.cpp
)

add_unit_test(test_xfpga_buffer_c xfpga-static
    xfpga/test_buffer_c.cpp
)

add_unit_test(test_xfpga_wsid_list_c xfpga-static
    xfpga/test_wsid_list_c.cpp
)

add_unit_test(test_xfpga_sysfs_c xfpga-static
    xfpga/test_sysfs_c.cpp
)

add_unit_test(test_xfpga_mmio_c xfpga-static
    xfpga/test_mmio_c.cpp
)

add_unit_test(test_xfpga_metadata_c xfpga-static
    xfpga/test_metadata_c.cpp
)

add_unit_test(test_xfpga_reconf_c xfpga-static
    xfpga/test_reconf_c.cpp
)

add_unit_test(test_xfpga_version_c xfpga-static
    xfpga/test_version_c.cpp
)

add_unit_test(test_xfpga_open_close_c xfpga-static
    xfpga/test_open_close_c.cpp
)

add_unit_test(test_xfpga_usrclk_c xfpga-static
    xfpga/test_usrclk_c.cpp
)

add_unit_test(test_xfpga_reset_c xfpga-static
    xfpga/test_reset_c.cpp
)

add_unit_test(test_xfpga_umsg_c xfpga-static
    xfpga/test_umsg_c.cpp
)

add_unit_test(test_xfpga_manage_c xfpga-static
    xfpga/test_manage_c.cpp
)

add_unit_test(test_xfpga_events_c xfpga-static
    xfpga/test_events_c.cpp
)
target_link_libraries(test_xfpga_events_c
    safestr
    fpgad-static
    fpgad-api-static
    bmc
    ${libjson-c_LIBRARIES})

add_unit_test(test_xfpga_metrics_vector_c xfpga-static
    xfpga/test_metrics_vector_c.cpp
)

add_unit_test(test_xfpga_metrics_utils_c xfpga-static
    xfpga/test_metrics_utils_c.cpp
)

add_unit_test(test_xfpga_afu_metrics_c xfpga-static
    xfpga/test_afu_metrics_c.cpp
)

add_unit_test(test_xfpga_metrics_c xfpga-static
    xfpga/test_metrics_c.cpp
)

add_unit_test(test_xfpga_bmc_c xfpga-static
    xfpga/test_bmc_c.cpp
)

add_unit_test(test_xfpga_max10_c xfpga-static
    xfpga/test_metrics_max10_c.cpp
)

add_unit_test(test_xfpga_threshold_c xfpga-static
    xfpga/test_thresold_c.cpp
)


target_link_libraries(test_xfpga_bmc_c
    safestr
    bmc-static
    ${libjson-c_LIBRARIES})

############################################################################
# fpgad tests ##############################################################
############################################################################

add_unit_test(test_fpgad_command_line_c fpgad-static
    fpgad/test_command_line_c.cpp
)
target_link_libraries(test_fpgad_command_line_c
    fpgad-api-static
    )

add_unit_test(test_fpgad_config_file_c fpgad-static
    fpgad/test_config_file_c.cpp
)
target_link_libraries(test_fpgad_config_file_c
    fpgad-api-static
    )

add_unit_test(test_fpgad_fpgad_c fpgad-static
    fpgad/test_fpgad_c.cpp
)
target_link_libraries(test_fpgad_fpgad_c
    fpgad-api-static
    )

add_unit_test(test_fpgad_daemonize_c fpgad-static
    fpgad/test_daemonize_c.cpp
)

add_unit_test(test_fpgad_event_dispatcher_thread_c fpgad-static
    fpgad/test_event_dispatcher_thread_c.cpp
)
target_link_libraries(test_fpgad_event_dispatcher_thread_c
    fpgad-api-static
    )

add_unit_test(test_fpgad_events_api_thread_c fpgad-static
    fpgad/test_events_api_thread_c.cpp
)
target_link_libraries(test_fpgad_events_api_thread_c
    fpgad-api-static
    )

add_unit_test(test_fpgad_monitor_thread_c fpgad-static
    fpgad/test_monitor_thread_c.cpp
)
target_link_libraries(test_fpgad_monitor_thread_c
    fpgad-api-static
    )

add_unit_test(test_fpgad_monitored_device_c fpgad-static
    fpgad/test_monitored_device_c.cpp
)
target_link_libraries(test_fpgad_monitored_device_c
    fpgad-api-static
    )

############################################################################
# fpgad-api tests ##########################################################
############################################################################

add_unit_test(test_fpgad_api_logging_c fpgad-static
    fpgad/test_api_logging_c.cpp
)
target_link_libraries(test_fpgad_api_logging_c
    safestr
    fpgad-api-static
    )

add_unit_test(test_fpgad_api_device_monitoring_c fpgad-static
    fpgad/test_api_device_monitoring_c.cpp
)
target_link_libraries(test_fpgad_api_device_monitoring_c
    safestr
    fpgad-api-static
    )

add_unit_test(test_fpgad_api_opae_events_api_c fpgad-static
    fpgad/test_api_opae_events_api_c.cpp
)
target_link_libraries(test_fpgad_api_opae_events_api_c
    safestr
    fpgad-api-static
    )

add_unit_test(test_fpgad_api_sysfs_c fpgad-static
    fpgad/test_api_sysfs_c.cpp
)
target_link_libraries(test_fpgad_api_sysfs_c
    safestr
    fpgad-api-static
    )

############################################################################
# fpgad-xfpga tests ########################################################
############################################################################

add_unit_test(test_fpgad_plugin_fpgad_xfpga_c fpgad-static
    fpgad/test_plugin_fpgad_xfpga_c.cpp
)
target_link_libraries(test_fpgad_plugin_fpgad_xfpga_c
    safestr
    fpgad-api-static
    fpgad-xfpga-static
    )

############################################################################
# RAS tests ##############################################################
############################################################################

add_unit_test(test_ras_c ras-static
    ras/test_main_c.cpp
)

############################################################################
# fpgaconf tests ###########################################################
############################################################################

add_unit_test(test_fpgaconf_fpgaconf_c fpgaconf-static
    fpgaconf/test_fpgaconf_c.cpp
)

############################################################################
# coreidle tests ###########################################################
############################################################################

add_unit_test(test_coreidle_coreidle_c coreidle-static
    coreidle/test_coreidle_c.cpp
)

add_unit_test(test_coreidle_main_c coreidle-static
    coreidle/test_main_c.cpp
)

############################################################################
# userclk tests ############################################################
############################################################################

add_unit_test(test_userclk_c userclk-static
    userclk/test_userclk_c.cpp
)

############################################################################
# fpgainfo tests ############################################################
############################################################################

add_unit_test(test_fpgainfo_c fpgainfo-static
    fpgainfo/test_fpgainfo_c.cpp
)
target_link_libraries(test_fpgainfo_c argsfilter-static)


############################################################################
# hello_fpga tests #########################################################
############################################################################

add_unit_test(test_hello_fpga_c hello_fpga-static
    hello_fpga/test_hello_fpga_c.cpp
)

############################################################################
# hello_events tests #######################################################
############################################################################

add_unit_test(test_hello_events_c hello_events-static
    hello_events/test_hello_events_c.cpp
)
target_link_libraries(test_hello_events_c
    safestr
    fpgad-static
    fpgad-api-static
    )

############################################################################
# object_api tests #######################################################
############################################################################

add_unit_test(test_object_api_c object_api-static
    object_api/test_object_api_c.cpp
)

add_custom_command(TARGET test_unit
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-1socket-nlb0.tar.gz
    ${CMAKE_BINARY_DIR})

add_custom_command(TARGET test_unit
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-1socket-nlb0-vf.tar.gz
    ${CMAKE_BINARY_DIR})

add_custom_command(TARGET test_unit
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dcp-rc-nlb3.tar.gz
    ${CMAKE_BINARY_DIR}
    )

add_custom_command(TARGET test_unit
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dfl0-nlb0.tar.gz
    ${CMAKE_BINARY_DIR}
    )

add_custom_command(TARGET test_unit
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-dcp-vc.tar.gz
    ${CMAKE_BINARY_DIR}
    )

############################################################################
# pyopae tests #############################################################
############################################################################
try_compile(SUPPORTS_EMBEDDED_PYTHON
    ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/pyopae/test_embed.cpp
    CMAKE_FLAGS
    "-DINCLUDE_DIRECTORIES=${PYTHON_INCLUDE_DIRS};${CMAKE_SOURCE_DIR}/pyopae/pybind11/include"
    LINK_LIBRARIES ${PYTHON_LIBRARIES}
    OUTPUT_VARIABLE TRY_COMPILE_OUTPUT
    )

if (SUPPORTS_EMBEDDED_PYTHON)
    add_executable(test_pyopae
         ${MOCK_C}
         pyopae/test_pyopae.cpp
         ${OPAE_SDK_SOURCE}/pyopae/opae.cpp
         ${OPAE_SDK_SOURCE}/pyopae/pycontext.h
         ${OPAE_SDK_SOURCE}/pyopae/pycontext.cpp
         ${OPAE_SDK_SOURCE}/pyopae/pyproperties.h
         ${OPAE_SDK_SOURCE}/pyopae/pyproperties.cpp
         ${OPAE_SDK_SOURCE}/pyopae/pyhandle.h
         ${OPAE_SDK_SOURCE}/pyopae/pyhandle.cpp
         ${OPAE_SDK_SOURCE}/pyopae/pytoken.h
         ${OPAE_SDK_SOURCE}/pyopae/pytoken.cpp
         ${OPAE_SDK_SOURCE}/pyopae/pyshared_buffer.h
         ${OPAE_SDK_SOURCE}/pyopae/pyshared_buffer.cpp
         ${OPAE_SDK_SOURCE}/pyopae/pyevents.h
         ${OPAE_SDK_SOURCE}/pyopae/pyevents.cpp
         ${OPAE_SDK_SOURCE}/pyopae/pyerrors.h
         ${OPAE_SDK_SOURCE}/pyopae/pyerrors.cpp
         ${OPAE_SDK_SOURCE}/pyopae/pysysobject.h
         ${OPAE_SDK_SOURCE}/pyopae/pysysobject.cpp)
    target_compile_definitions(test_pyopae PRIVATE
        OPAE_EMBEDDED)
    target_include_directories(test_pyopae
         PRIVATE ${CMAKE_SOURCE_DIR}/pyopae/pybind11/include
         PRIVATE ${PYTHON_INCLUDE_DIRS}
         PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    target_link_libraries(test_pyopae PUBLIC opae-c opae-cxx-core
         test_system ${PYTHON_LIBRARIES})
    add_dependencies(test_unit test_pyopae)

    macro(add_pyopae_test pytest)
        add_custom_command(TARGET test_pyopae
           POST_BUILD
           COMMAND ${CMAKE_COMMAND} -E copy
           ${CMAKE_CURRENT_SOURCE_DIR}/pyopae/${pytest}
           ${CMAKE_BINARY_DIR}
           )
        add_test(
            NAME ${pytest}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND $<TARGET_FILE:test_pyopae> test ${pytest}
        )
    endmacro(add_pyopae_test pytest)

    add_pyopae_test(test_properties.py)
    add_pyopae_test(test_shared_buffers.py)
    add_pyopae_test(test_sysobject.py)
else(SUPPORTS_EMBEDDED_PYTHON)
    message(WARNING
        "Could not compile embedded Python. See errors in embed_errors.txt")
    file(WRITE ${CMAKE_BINARY_DIR}/embed_errors.txt ${TRY_COMPILE_OUTPUT})
endif (SUPPORTS_EMBEDDED_PYTHON)
