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

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCSourceCompiles)
include(CheckTypeSize)
include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CheckLibraryExists)

set(PROJECT_LIBS "${CMAKE_BINARY_DIR}/libs")

# Threads library: CMAKE_THREAD_LIBS_INIT CMAKE_USE_PTHREADS_INIT CMAKE_USE_WIN32_THREADS_INIT
find_package(Threads)
if (CMAKE_USE_PTHREADS_INIT)
  set(HAVE_PTHREAD_H 1)
  set(CMAKE_THREAD_PREFER_PTHREAD ON)
  list(APPEND PROJECT_LIBS ${CMAKE_THREAD_LIBS_INIT})
endif()

check_library_exists(pthread pthread_create "" HAVE_LIBPTHREAD)
if (HAVE_LIBPTHREAD)
  check_library_exists(pthread pthread_getspecific "" HAVE_PTHREAD_GETSPECIFIC)
  check_library_exists(pthread pthread_rwlock_init "" HAVE_PTHREAD_RWLOCK_INIT)
  check_library_exists(pthread pthread_mutex_lock "" HAVE_PTHREAD_MUTEX_LOCK)
else()
  check_library_exists(c pthread_create "" PTHREAD_IN_LIBC)
  if (PTHREAD_IN_LIBC)
    check_library_exists(c pthread_getspecific "" HAVE_PTHREAD_GETSPECIFIC)
    check_library_exists(c pthread_rwlock_init "" HAVE_PTHREAD_RWLOCK_INIT)
    check_library_exists(c pthread_mutex_lock "" HAVE_PTHREAD_MUTEX_LOCK)
  endif()
endif(HAVE_LIBPTHREAD)

# dlopen support: CMAKE_DL_LIBS (built-in cmake variable)
check_library_exists(dl dlopen "" HAVE_LIBDL)
if (HAVE_LIBDL)
  list(APPEND PROJECT_LIBS ${CMAKE_DL_LIBS})
endif()

# rt check
find_package(RT)
check_library_exists(rt clock_gettime "" HAVE_LIBRT)

# uuid check
find_package(UUID REQUIRED)

# json-c check
find_package(json-c REQUIRED)

# hwloc check
find_package(Hwloc REQUIRED)

