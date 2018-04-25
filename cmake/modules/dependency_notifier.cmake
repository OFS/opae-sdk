## Copyright(c) 2017, Intel Corporation
##
## Redistribution  and  use  in source  and  binary  forms,  with  or  without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of  source code  must retain the  above copyright notice,
##   this list of conditions and the following disclaimer.
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
## * Neither the name  of Intel Corporation  nor the names of its contributors
##   may be used to  endorse or promote  products derived  from this  software
##   without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
## IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
## LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
## CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
## SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
## INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
## CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.

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
    RPM: yum install json-c-devel
   If you have already installed this package in a non-standard location 
   please specify the location by defining the variable LIBJSON-C_ROOT in 
   your cmake command as follows: cmake <path to clone dir> -DLIBJSON-C_ROOT=<path to json-c install location>")
   set(REQUIRED_DEPENDENCIES "libjson-c ${REQUIRED_DEPENDENCIES}")
endif()

if(NOT libuuid_FOUND)
    message("-- uuid not found. Please install uuid package for your respective distribution:
    DEB: apt-get install uuid-dev
    RPM: yum install libuuid-devel
   If you have already installed this package in a non-standard location 
   please specify the location by defining the variable LIBUUID_ROOT in 
   your cmake command as follows: cmake <path to clone dir> -DLIBUUID_ROOT=<path to uuid install location>")
   set(REQUIRED_DEPENDENCIES "libuuid ${REQUIRED_DEPENDENCIES}")
endif()

find_package(PythonInterp 2.7)
if(NOT PYTHONINTERP_FOUND)
    message("-- No suitable Python interpreter found. Some Python based tools will not function. Please install Python >= 2.7.")
endif()

if(NOT DOXYGEN_FOUND)
    message("-- Doxygen not found. Documentation will not be built. If you want documentation to be built, please install doxygen:
    DEB: apt-get install doxygen
    RPM: yum install doxygen
   If you have already installed this package in a non-standard location 
   please specify the location by defining the variable DOXYGEN_ROOT in 
   your cmake command as follows: cmake <path to clone dir> -DDOXYGEN_ROOT=<path to doxygen install location>")
endif()

if(NOT SPHINX_FOUND)
    message("-- Sphinx not found. HTML documentation will not be built. If you want HTML documentation to be built, 
   please install python-sphinx:
    DEB: apt-get install python-sphinx
    RPM: Please follow the official documentation to install Sphinx: http://www.sphinx-doc.org/en/stable/install.html
   If you have already installed this package in a non-standard location 
   please specify the location by defining the variable SPHINX_ROOT in 
   your cmake command as follows: cmake <path to clone dir> -DSPHINX_ROOT=<path to sphinx install location>")
endif()

if(REQUIRED_DEPENDENCIES)
    message(FATAL_ERROR "The following dependencies are required; libopae-c will not be built unless they are satisfied. 
   ---- ${REQUIRED_DEPENDENCIES}----")
endif()