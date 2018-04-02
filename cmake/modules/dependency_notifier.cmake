if(NOT CMAKE_C_COMPILER)
    message("-- No C compiler was found. Please install the gcc package for your distribution:
    DEB: apt-get install gcc
    RPM: yum install gcc") 
endif()

if(NOT CMAKE_CXX_COMPILER)
    message("-- No C++ compiler was found. Please install the g++ package for your distribution:
    DEB: apt-get install g++
    RPM: yum install gcc-c++") 
endif()

if(NOT libjson-c_FOUND)
    message("-- json-c not found. Please install json-c package for you respective distribution:
    DEB: apt-get install libjson0-dev
    RPM: yum install json-c
   If you have already installed this package in a nonstandard location 
   please specify the location by defining the variable LIBJSON-C_ROOT in 
   your cmake command as follows: cmake .. -DLIBJSON-C_ROOT=<path to install location>")
endif()

if(NOT libuuid_FOUND)
    message("-- uuid not found. Please install uuid package for your respective distribution:
    DEB: apt-get install uuid-dev
    RPM: yum install libuuid
   If you have already installed this package in a nonstandard location 
   please specify the location by defining the variable LIBJSON-C_ROOT in 
   your cmake command as follows: cmake .. -DUUID_ROOT=<path to install location>")
endif()

find_package(PythonInterp 2.7 REQUIRED)
if(NOT PYTHONINTERP_FOUND )
    message("-- No suitable python interpreter found. Please install python > 2.7 to satisfy dependency for building tools.")
endif()

if(NOT DOXYGEN_FOUND)
    message("-- Doxygen not found. Documentation will not be built. If you want documentation to be built, please install doxygen:
    DEB: apt-get install doxygen
    RPM: yum install doxygen")
endif()
if(NOT SPHINX_FOUND)
    message("-- Sphinx not found. HTML documentation will not be built. If you want HTML documentation to be built, 
   please install python-sphinx:
    DEB: apt-get install python-sphinx
    RPM: Please follow the official documentation to install Sphinx: http://www.sphinx-doc.org/en/stable/install.html")
endif()

if(NOT CURSES_FOUND)
    message("-- Curses not found. Please install Curses for you distribution:
    DEB: apt-get install libncurses5-dev
    RPM: yum install ncurses ")
endif()

if((NOT libjson-c_FOUND) AND (NOT libuuid_FOUND))
    if(NOT PYTHONINTERP_FOUND)
        message(FATAL_ERROR "libjson-c, libuuid and python were not found. libopae and base tools will not be built unless they are satisfied. Please install the necessary packages as mentioned above.")
    else()
        message(FATAL_ERROR "libjson-c and libuuid were not found. libopae-c will not be built unless they are satisfied. Please install the necessary packages as mentioned above.")
    endif()
elseif((NOT libjson-c_FOUND) AND (libuuid_FOUND))
    if(NOT PYTHONINTERP_FOUND)
        message(FATAL_ERROR "libjson-c and python were not found. libopae and base tools will not be built unless they are satisfied. Please install the necessary packages as mentioned above.")
    else()
        message(FATAL_ERROR "libjson-c was not found. libopae-c will not be built unless they are satisfied. Please install the necessary packages as mentioned above.")
    endif()
elseif((libjson-c_FOUND) AND (NOT libuuid_FOUND))
    if(NOT PYTHONINTERP_FOUND)
        message(FATAL_ERROR "libuuid and python were not found. libopae and base tools will not be built unless they are satisfied. Please install the necessary packages as mentioned above.")
    else()
        message(FATAL_ERROR "libuuid was not found. libopae-c will not be built unless they are satisfied. Please install the necessary packages as mentioned above.")
    endif()
endif()


