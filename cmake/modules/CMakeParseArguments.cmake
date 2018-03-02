# Analogue for standard CMake module with same name, appeared in CMake 2.8.3.
#
# Used for compatibility with older versions of cmake.

function(cmake_parse_arguments prefix options one_value_keywords multi_value_keywords)
    set(all_keywords ${options} ${one_value_keywords} ${multi_value_keywords})
    # Clear variables before parsing.
    foreach(arg ${one_value_keywords} ${multie_value_keywords})
        set(cpa_${arg})
    endforeach()

    foreach(arg ${options})
        set(cpa_${arg} FALSE)
    endforeach()

    set(cpa_UNPARSED_ARGUMENTS)

    set(current_keyword)
    # Classification for current_keyword:
    # 'ONE' or 'MULTY'.
    set(current_keyword_type)

    # now iterate over all arguments and fill the result variables
    foreach(arg ${ARGN})
        list(FIND all_keywords ${arg} keyword_index)

        if(keyword_index EQUAL -1)
            if(current_keyword)
                if(current_keyword_type STREQUAL "ONE")
                    set(cpa_${current_keyword} ${arg})
                    set(current_keyword)
                else(current_keyword_type STREQUAL "ONE")
                    list(APPEND cpa_${current_keyword} ${arg})
                endif(current_keyword_type STREQUAL "ONE")
            else(current_keyword)
                list(APPEND cpa_UNPARSED_ARGUMENTS ${arg})
            endif(current_keyword)
        else(keyword_index EQUAL -1)
            if(current_keyword AND current_keyword_type STREQUAL "ONE")
                message(SEND_ERROR "Value is expected for one-value-keyword ${current_keyword}")
            endif(current_keyword AND current_keyword_type STREQUAL "ONE")
            list(FIND options ${arg} option_index)
            if(option_index EQUAL -1)
                set(current_keyword ${arg})
                list(FIND one_value_keywords ${arg} one_value_index)
                if(one_value_index EQUAL -1)
                    set(current_keyword_type "MULTI")
                else(one_value_index EQUAL -1)
                    set(current_keyword_type "ONE")
                endif(one_value_index EQUAL -1)
            else(option_index EQUAL -1)
                set(current_keyword)
                set(cpa_${arg} TRUE)
            endif(option_index EQUAL -1)
        endif(keyword_index EQUAL -1)
    endforeach(arg ${ARGN})

    if(current_keyword AND current_keyword_type STREQUAL "ONE")
        message(SEND_ERROR "Value is expected for one-value-keyword ${current_keyword}")
    endif(current_keyword AND current_keyword_type STREQUAL "ONE")

    # propagate the result variables to the caller:
    foreach(keyword ${all_keywords} UNPARSED_ARGUMENTS)
        set(${prefix}_${keyword}  ${cpa_${keyword}} PARENT_SCOPE)
    endforeach(keyword ${all_keywords})
endfunction(cmake_parse_arguments prefix options one_value_keywords multi_value_keywords)
