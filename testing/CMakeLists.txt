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

set(MUNIT_SRC mock/mock.c
              unit/munit-enum-c.cpp
              unit/munit-buffer-c.cpp
              unit/properties_test.cpp
              unit/munit-wsid_list.cpp
              unit/munit-sysfs.cpp
              unit/munit-mmio-c.cpp
              unit/munit-reconf.cpp
              unit/munit-version.cpp
              unit/munit-open-close.cpp
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
              ${CMAKE_SOURCE_DIR}/libopae/src/usrclk/user_clk_pgm_uclock.c
              )

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

add_executable(munit-opae-c ${MUNIT_SRC})
add_executable(munit-opae-c-events
    mock/mock.c
    unit/events_test.cpp
    ${CMAKE_SOURCE_DIR}/libopae/src/event.c
    ${CMAKE_SOURCE_DIR}/tools/base/fpgad/srv.c
    ${CMAKE_SOURCE_DIR}/tools/base/fpgad/log.c
    )

target_link_libraries(munit-opae-c
    safestr
    test_system
    ${libjson-c_LIBRARIES}
    ${libuuid_LIBRARIES}
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(munit-opae-c-events
    safestr
    opae-c
    test_system
    ${libjson-c_LIBRARIES}
    ${libuuid_LIBRARIES}
    ${GTEST_BOTH_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT})

## TODO: Add tests to Cmake once they're stable

# add_test(
#     NAME munit-opae-c
#     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
#     COMMAND $<TARGET_FILE:munit-opae-c>
#     )

add_custom_command(TARGET munit-opae-c
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/mock_sys_tmp-1socket-nlb0.tar.gz
    ${CMAKE_BINARY_DIR})
