# - Find python package installation status
#
# ERR_LEVEL:
#     SEND_ERROR     = CMake Error, continue processing
#     WARNING        = CMake Warning, continue processing
#     FATAL_ERROR    = CMake Error, stop processing and generation
#

if (NOT PYTHON_EXECUTABLE)
    find_package(PythonInterp 2.7 REQUIRED)
endif (NOT PYTHON_EXECUTABLE)

macro(FIND_PYTHON_PKG PKG_NAME ERR_LEVEL )
    execute_process( COMMAND ${PYTHON_EXECUTABLE} -c "import ${PKG_NAME}" ERROR_QUIET RESULT_VARIABLE  STATUS_FLAG)

    if(STATUS_FLAG)
        message(${ERR_LEVEL} "PACKAGER depenency python ${PKG_NAME} package is missing. Extra tools may not get build due to failed dependencies.")
    endif(STATUS_FLAG)
endmacro(FIND_PYTHON_PKG)
