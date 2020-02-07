## Copyright(c) 2017-2018, Intel Corporation
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


# board lib
add_subdirectory(base/libboard/board_rc)
add_subdirectory(base/libboard/board_vc)

# base
add_subdirectory(base/fpgaconf)
#add_subdirectory(base/fpgad)
add_subdirectory(base/argsfilter)
add_subdirectory(base/fpgainfo)
#add_subdirectory(base/fpgaport)
add_subdirectory(base/fpgametrics)

# extra
#add_subdirectory(extra/coreidle)
add_subdirectory(extra/userclk)
#add_subdirectory(extra/packager)
#if (BUILD_PYTHON_DIST)
#    add_subdirectory(extra/pypackager)
#	add_subdirectory(extra/pyfpgadiag)
#endif()
add_subdirectory(extra/c++utils)
add_subdirectory(extra/fpgadiag)
add_subdirectory(extra/mmlink)
#add_subdirectory(extra/ras)

# integrated
#add_subdirectory(extra/integrated/hssi)

# pac
#add_subdirectory(extra/pac/fpgaflash)
#if (BUILD_PYTHON_DIST)
#    add_subdirectory(extra/pac/pyfpgaflash)
#endif()
#add_subdirectory(extra/pac/pac_hssi_config)
add_subdirectory(extra/pac/fpgabist)
