if(NOT CMAKE_C_COMPILER)
    message("-- You don't have gcc installed. Please install the gcc package for your distribution.") 
endif()

if(NOT CMAKE_CXX_COMPILER)
    message("-- You don't have g++ installed. Please install the g++ package for your distribution.") 
endif()

if(NOT libjson-c_FOUND)
    message("-- Please install libjson-c package.
   If you have already installed this package in a nonstandard location 
   please specify the location by defining the variable LIBJSON-C_ROOT in 
   your cmake command as follows: cmake .. -DLIBJSON-C_ROOT=<path to install location>")
endif()

if(NOT libuuid_FOUND)
    message("-- Please install liuuid package.
   If you have already installed this package in a nonstandard location 
   please specify the location by defining the variable LIBJSON-C_ROOT in 
   your cmake command as follows: cmake .. -DUUID_ROOT=<path to install location>")
endif()

find_package(PythonInterp 2.7 REQUIRED)
if(NOT PYTHONINTERP_FOUND )
    message("-- No suitable python interpreter found. Please install python > 2.7 to satisfy dependency for building tools.")
endif()

if(NOT DOXYGEN_FOUND)
    message("-- Doxygen not found. Documentation will not be built. If you want documentation to be built, please install doxygen.")
endif()
if(NOT SPHINX_FOUND)
    message("-- Sphinx not found. HTML documentation will not be built. If you want HTML documentation to be built, 
   please install python-sphinx.")
endif()

if(NOT CURSES_FOUND)
    message("-- Curses not found. Please install Curses for you distribution.")
endif()

if((NOT libjson-c_FOUND) OR (NOT libuuis_FOUND))
    if(NOT PYTHONINTERP_FOUND)
        message(FATAL_ERROR "libjson-c, libuuid and python were not found. libopae and base tools will not be built unless they are satisfied. Please install the necessary packages as mentioned above.")
    else()
        message(FATAL_ERROR "libjson-c and libuuid were not found. libopae will not be built unless they are satisfied. Please install the necessary packages as mentioned above.")
    endif()
endif()


