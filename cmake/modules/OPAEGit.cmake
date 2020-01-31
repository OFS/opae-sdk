#!/usr/bin/cmake -P

# Exports
#
# * OPAE_GIT_EXECUTABLE
# * OPAE_GIT_COMMIT_HASH
# * OPAE_GIT_SRC_TREE_DIRTY

find_program(OPAE_GIT_EXECUTABLE git)

if(EXISTS ${OPAE_GIT_EXECUTABLE})
    # Find the abbreviated git commit hash.
    execute_process(COMMAND ${OPAE_GIT_EXECUTABLE} log -1 --format=%h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE OPAE_GIT_COMMIT_HASH
        RESULT_VARIABLE OPAE_GIT_LOG_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT ${OPAE_GIT_LOG_RESULT} EQUAL 0)
        set(OPAE_GIT_COMMIT_HASH unknown)
    endif()

    # Determine whether the working tree has changes.
    execute_process(COMMAND ${OPAE_GIT_EXECUTABLE} diff --stat
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE OPAE_GIT_DIFF_OUTPUT
	RESULT_VARIABLE OPAE_GIT_DIFF_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT ${OPAE_GIT_DIFF_RESULT} EQUAL 0)
        set(OPAE_GIT_SRC_TREE_DIRTY 0)
    else()
        if(OPAE_GIT_DIFF_OUTPUT)
            set(OPAE_GIT_SRC_TREE_DIRTY 1)
        else()
            set(OPAE_GIT_SRC_TREE_DIRTY 0)
        endif()
    endif()
else(EXISTS ${OPAE_GIT_EXECUTABLE})
    set(OPAE_GIT_COMMIT_HASH unknown)
    set(OPAE_GIT_SRC_TREE_DIRTY 0)
endif(EXISTS ${OPAE_GIT_EXECUTABLE})
