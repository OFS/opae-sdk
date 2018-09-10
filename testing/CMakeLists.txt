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


add_library(opae-c-static STATIC
              ${CMAKE_SOURCE_DIR}/libopae/src/bitstream.c
              ${CMAKE_SOURCE_DIR}/libopae/src/buffer.c
              ${CMAKE_SOURCE_DIR}/libopae/src/close.c
              ${CMAKE_SOURCE_DIR}/libopae/src/common.c
              ${CMAKE_SOURCE_DIR}/libopae/src/enum.c
              ${CMAKE_SOURCE_DIR}/libopae/src/error.c
              ${CMAKE_SOURCE_DIR}/libopae/src/hostif.c
              ${CMAKE_SOURCE_DIR}/libopae/src/init.c
              ${CMAKE_SOURCE_DIR}/libopae/src/manage.c
              ${CMAKE_SOURCE_DIR}/libopae/src/mmap.c
              ${CMAKE_SOURCE_DIR}/libopae/src/mmio.c
              ${CMAKE_SOURCE_DIR}/libopae/src/open.c
              ${CMAKE_SOURCE_DIR}/libopae/src/properties.c
              ${CMAKE_SOURCE_DIR}/libopae/src/reconf.c
              ${CMAKE_SOURCE_DIR}/libopae/src/reset.c
              ${CMAKE_SOURCE_DIR}/libopae/src/sysfs.c
              #${CMAKE_SOURCE_DIR}/libopae/src/sysobject.c
              ${CMAKE_SOURCE_DIR}/libopae/src/token_list.c
              ${CMAKE_SOURCE_DIR}/libopae/src/umsg.c
              ${CMAKE_SOURCE_DIR}/libopae/src/version.c
              ${CMAKE_SOURCE_DIR}/libopae/src/wsid_list.c
              ${CMAKE_SOURCE_DIR}/libopae/src/usrclk/user_clk_pgm_uclock.c)

add_custom_target(test_c
    DEPENDS opae-c-static)

macro(add_mock_test test_name)
    add_executable(${test_name}
        mock/mock.c
        ${ARGN})
    target_link_libraries(${test_name}
        opae-c-static
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
    add_dependencies(test_c ${test_name})

endmacro(add_mock_test test_name)


############################################################################
## Create config_int.h #########################################################
############################################################################

configure_file ("${CMAKE_SOURCE_DIR}/cmake/config/config.h.in"
                "${PROJECT_BINARY_DIR}/include/config_int.h" )

include_directories(
    ${CMAKE_SOURCE_DIR}/common/include
    ${PROJECT_BINARY_DIR}/include
    ${CMAKE_SOURCE_DIR}/libopae/src
    ${CMAKE_SOURCE_DIR}/tools/base
    mock
    ${GTEST_INCLUDE_DIRS}
    {CMAKE_CURRENT_SOURCE_DIR}
    {CMAKE_CURRENT_SOURCE_DIR}/mock
    )


add_mock_test(test_properties_c
    unit/test_properties_c.cpp
)

add_mock_test(test_enum_c
    unit/test_enum_c.cpp
)

add_mock_test(test_buffer_c
    unit/test_buffer_c.cpp
)

add_mock_test(test_wsid_list_c
    unit/test_wsid_list_c.cpp
)

add_mock_test(test_sysfs_c
    unit/test_sysfs_c.cpp
)

add_mock_test(test_mmio_c
    unit/test_mmio_c.cpp
)

add_mock_test(test_metadata_c
    unit/test_metadata_c.cpp
)

add_mock_test(test_reconf_c
    unit/test_reconf_c.cpp
)

add_mock_test(test_version_c
    unit/test_version_c.cpp
)

add_mock_test(test_open_close_c
    unit/test_open_close_c.cpp
)

add_mock_test(test_usrclk_c
    unit/test_usrclk_c.cpp
)

add_mock_test(test_events_c
    unit/test_events_c.cpp
    ${CMAKE_SOURCE_DIR}/libopae/src/event.c
    ${CMAKE_SOURCE_DIR}/tools/base/fpgad/srv.c
    ${CMAKE_SOURCE_DIR}/tools/base/fpgad/log.c
)

add_custom_command(TARGET opae-c-static
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-1socket-nlb0.tar.gz
    ${CMAKE_BINARY_DIR})
