opae_add_shared_library(TARGET fpgaperf_counter
    SOURCE fpgaperf_counter.c
    LIBS
        ${CMAKE_THREAD_LIBS_INIT}
        opae-c
    COMPONENT opaesamplelib
)
