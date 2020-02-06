## Copyright(c) 2017-2020, Intel Corporation
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

project(safe_string)

set(SRC
  abort_handler_s.c
  ignore_handler_s.c
  safeclib_private.h
  safe_mem_constraint.c
  safe_mem_constraint.h
  safe_str_constraint.c
  safe_str_constraint.h
  memcmp16_s.c
  memcmp32_s.c
  memcmp_s.c
  memcpy16_s.c
  memcpy32_s.c
  memcpy_s.c
  memmove16_s.c
  memmove32_s.c
  memmove_s.c
  mem_primitives_lib.c
  mem_primitives_lib.h
  memset16_s.c
  memset32_s.c
  memset_s.c
  memzero16_s.c
  memzero32_s.c
  memzero_s.c
  snprintf_support.c
  stpcpy_s.c
  stpncpy_s.c
  strcasecmp_s.c
  strcasestr_s.c
  strcat_s.c
  strcmpfld_s.c
  strcmp_s.c
  strcpyfldin_s.c
  strcpyfldout_s.c
  strcpyfld_s.c
  strcpy_s.c
  strcspn_s.c
  strfirstchar_s.c
  strfirstdiff_s.c
  strfirstsame_s.c
  strisalphanumeric_s.c
  strisascii_s.c
  strisdigit_s.c
  strishex_s.c
  strislowercase_s.c
  strismixedcase_s.c
  strispassword_s.c
  strisuppercase_s.c
  strlastchar_s.c
  strlastdiff_s.c
  strlastsame_s.c
  strljustify_s.c
  strcat_s.c
  strncpy_s.c
  strnlen_s.c
  strnterminate_s.c
  strpbrk_s.c
  strprefix_s.c
  strremovews_s.c
  strspn_s.c
  strstr_s.c
  strtok_s.c
  strtolowercase_s.c
  strtouppercase_s.c
  strzero_s.c
  wcpcpy_s.c
  wcscat_s.c
  wcscpy_s.c
  wcsncat_s.c
  wcsncpy_s.c
  wcsnlen_s.c
  wmemcmp_s.c
  wmemcpy_s.c
  wmemmove_s.c
  wmemset_s.c
)

opae_add_static_library(TARGET safestr SOURCE ${SRC})
set_property(TARGET safestr PROPERTY POSITION_INDEPENDENT_CODE ON)
target_compile_definitions(safestr PRIVATE PIC=1)
target_compile_options(safestr PRIVATE -O3)

# mem_primitives_lib.c is optimized through the use of fallthrough switch statements.
set_source_files_properties(mem_primitives_lib.c PROPERTIES COMPILE_FLAGS -Wno-implicit-fallthrough)

install(TARGETS safestr
    ARCHIVE DESTINATION ${OPAE_LIB_INSTALL_DIR}
    COMPONENT safestrlib)
