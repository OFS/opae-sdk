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

if(NOT DEFINED CPACK_GENERATOR)
  set(CPACK_GENERATOR "RPM")
endif()

function(DEFINE_PKG name)
  set(_components "COMPONENTS")

  # Parse all these entries
  set(_entries "GROUP;DISPLAY_NAME;DESCRIPTION;DEPENDS;RPM_REQUIRES")

  # Only valid options for a component
  set(_component_entries "GROUP;DISPLAY_NAME;DESCRIPTION;DEPENDS")

  # Define parsing order
  cmake_parse_arguments(DEFINE_PKG
    ""
    "${_entries}"
    "${_components}"
    ${ARGN})

  # Iterate over 2-valued entries
  foreach(_component ${DEFINE_PKG_COMPONENTS})
    string(TOUPPER "${_component}" _component_upper)

    # Assume all entrys refer to component variables
    foreach(_entry ${_component_entries})
      if(DEFINE_PKG_${_entry})
        set_cached_variable(CPACK_COMPONENT_${_component_upper}_${_entry}
	  "${DEFINE_PKG_${_entry}}")
      endif()
    endforeach()
  endforeach()

  # Set RPM filename according to Redhat convention
  if(HASH_ARCHIVES)
    set_cached_variable(
      CPACK_RPM_${DEFINE_PKG_GROUP}_FILE_NAME
      "${CMAKE_PROJECT_NAME}-${DEFINE_PKG_GROUP}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.x86_64_git${GIT_COMMIT_HASH}.rpm")
  else()
    set_cached_variable(
      CPACK_RPM_${DEFINE_PKG_GROUP}_FILE_NAME
      "${CMAKE_PROJECT_NAME}-${DEFINE_PKG_GROUP}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-${CPACK_PACKAGE_RELEASE}.x86_64.rpm")
  endif()

  # Set RPM requires
  if(DEFINE_PKG_RPM_REQUIRES)
    set_cached_variable(
      CPACK_RPM_${DEFINE_PKG_GROUP}_PACKAGE_REQUIRES
      ${DEFINE_PKG_RPM_REQUIRES})
  endif()

endfunction(DEFINE_PKG)
