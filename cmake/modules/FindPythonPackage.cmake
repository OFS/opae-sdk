# - Find python package installation status

macro(FIND_PYTHON_PKG PKG_NAME )
    set(STATUS_FLAG ${PKG_NAME}_NOT_FOUND)
    set(EXECUTE_COMMAND python -c "import ${PKG_NAME}" ERROR_QUIET RESULT_VARIABLE  ${STATUS_FLAG})

    execute_process( COMMAND ${EXECUTE_COMMAND})

    if(STATUS_FLAG)
        message(WARNING "PACKAGER runtime depenency python ${PKG_NAME} package is missing!!!")
    endif(STATUS_FLAG)
endmacro(FIND_PYTHON_PKG)
