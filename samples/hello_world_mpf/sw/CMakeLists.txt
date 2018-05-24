
######################################################################
# Testing application ################################################
######################################################################

include_directories(${OPAE_INCLUDE_DIR}
  ${OPAE_SDK_SOURCE}/libopae/src
  ${MPF_INCLUDE_DIRS})

add_executable(hello_world_mpf
  ${OPAE_SHARE_DIR}/samples/base/sw/opae_svc_wrapper.cpp
  hello_world_mpf.cpp)
if(BUILD_LIBOPAE_C)
  target_link_libraries(hello_world_mpf json-c uuid MPF ${CMAKE_THREAD_LIBS_INIT} opae-c)
else()
  target_link_libraries(hello_world_mpf json-c uuid MPF ${CMAKE_THREAD_LIBS_INIT} opae-c-ase)
endif()

install(FILES hello_world_mpf.cpp
  DESTINATION ${OPAE_SAMPLES}/hello_world_mpf/sw
  COMPONENT samplesrc)
