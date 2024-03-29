## Copyright(c) 2017-2023, Intel Corporation
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

if (OPAE_WITH_HWLOC AND OPAE_WITH_TBB)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
        "${OPAE_SDK_SOURCE}/cmake/modules")

    #TODO: Edit the code dependencies here
    set(PYTHON_SRC
         bist_app.py
         bist_common.py
         bist_def.py
         bist_dma.py
         bist_nlb0.py
         bist_nlb3.py
         fpgabist)

    if(tbb_FOUND)
      add_subdirectory(dma)
      add_subdirectory(dma_N3000)
    else(tbb_FOUND)
      message(WARNING "Thread Building Blocks not found: not building fpgabist/dma.")
    endif(tbb_FOUND)

    add_subdirectory(bist)

    install(PROGRAMS ${PYTHON_SRC}
        DESTINATION bin
        COMPONENT toolfpgabist)
endif(OPAE_WITH_HWLOC AND OPAE_WITH_TBB)
