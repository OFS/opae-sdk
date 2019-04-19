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

# Enable checking compiler flags
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# Export compile commands
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# Set the default build type to release with debug info
if (CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE RelWithDebInfo
    CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel Coverage."
    FORCE)
endif (CMAKE_BUILD_TYPE STREQUAL "")

# Helper function to set cached variables
function(SET_CACHED_VARIABLE var)
  set(${var} ${ARGN} CACHE INTERNAL "")
  list(APPEND _cached_vars ${var})
  list(REMOVE_DUPLICATES _cached_vars)
  set(_cached_vars ${_cached_vars} CACHE INTERNAL "")
endfunction(SET_CACHED_VARIABLE)

# Default flags to compiler when build user-space programs.
# Should come before enabling language.

CHECK_C_COMPILER_FLAG(-Werror=implicit-fallthrough=3 COMPILER_SUPPORTS_IMPLICIT_FALLTHROUGH3)

set(CMAKE_C_FLAGS_DEBUG            "-g -O0 -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_DEBUG          "-g -O0 -Wall -Wextra -Werror")

set(CMAKE_C_FLAGS_RELEASE          "-O2 -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_RELEASE        "-O2 -Wall -Wextra -Werror")

set(CMAKE_C_FLAGS_RELWITHDEBINFO   "-g -O2 -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-g -O2 -Wall -Wextra -Werror")

set(CMAKE_C_FLAGS_MINSIZEREL       "-Os -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -Wall -Wextra -Werror")

if(COMPILER_SUPPORTS_IMPLICIT_FALLTHROUGH3)
  set(CMAKE_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS_DEBUG} -Werror=implicit-fallthrough=3")
  set(CMAKE_C_FLAGS_RELEASE        "${CMAKE_C_FLAGS_RELEASE} -Werror=implicit-fallthrough=3")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO} -Werror=implicit-fallthrough=3")
  set(CMAKE_C_FLAGS_MINSIZEREL     "${CMAKE_C_FLAGS_MINSIZEREL} -Werror=implicit-fallthrough=3")
endif()

# Check if support for C++ 11 is available
check_cxx_compiler_flag("-std=c++14" COMPILER_SUPPORTS_CXX14)
check_cxx_compiler_flag("-std=c++11" COMPILER_SUPPORTS_CXX11)
check_cxx_compiler_flag("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX14)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
  set(CMAKE_CXX_STANDARD 14)
elseif(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  set(CMAKE_CXX_STANDARD 11)
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

# Disable some warnings that fire in system libraries
check_cxx_compiler_flag("-Wno-unused-local-typedefs"
  CXX_SUPPORTS_NO_LOCAL_TYPEDEFS)
if (CXX_SUPPORTS_NO_LOCAL_TYPEDEFS)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")
endif()

# Disable some warnings that fire during gtest compilation
check_cxx_compiler_flag("-Wno-sign-compare"
  CXX_SUPPORTS_NO_SIGN_COMPARE)
if (CXX_SUPPORTS_NO_SIGN_COMPARE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
endif()

# If building on a 32-bit system, make sure off_t can store offsets > 2GB
if(CMAKE_COMPILER_IS_GNUCC)
  if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_definitions(-D_LARGEFILE_SOURCE)
    add_definitions(-D_FILE_OFFSET_BITS=64)
  endif()
endif(CMAKE_COMPILER_IS_GNUCC)

# Set debug flags, if required
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_definitions(-DLIBOPAE_DEBUG)
endif(CMAKE_BUILD_TYPE STREQUAL "Debug")

############################################################################
## Defensive compilation for Release #######################################
############################################################################
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  ## C options
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat -Wformat-security")
  set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -D_FORTIFY_SOURCE=2")
  if (GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9)
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -z noexecstack -z relro -z now")
  else()
      set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-all")
  endif()

  ## C++ options
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat -Wformat-security")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -D_FORTIFY_SOURCE=2")
  if (GCC_VERSION VERSION_GREATER 4.9 OR GCC_VERSION VERSION_EQUAL 4.9)
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-strong")
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -z noexecstack -z relro -z now")
  else()
      set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstack-protector-all")
  endif()

  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pie")

  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")
endif(CMAKE_BUILD_TYPE STREQUAL "Release")
