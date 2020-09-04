## Copyright(c) 2020, Intel Corporation
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
## POSSIBILITY OF SUCH DAMAGE

cmake_minimum_required(VERSION 2.8.12)

opae_external_project_add(PROJECT_NAME CLI11
                          GIT_URL https://github.com/CLIUtils/CLI11.git
                          GIT_TAG v1.9.1
                          PRESERVE_REPOS ${OPAE_PRESERVE_REPOS}
                          NO_ADD_SUBDIRECTORY
)

opae_external_project_add(PROJECT_NAME spdlog
                          GIT_URL https://github.com/gabime/spdlog.git
                          GIT_TAG v1.7.0
                          PRESERVE_REPOS ${OPAE_PRESERVE_REPOS}
                          NO_ADD_SUBDIRECTORY
)

if(OPAE_BUILD_LEGACY)
    opae_external_project_add(PROJECT_NAME opae-legacy
                              GIT_URL https://github.com/OPAE/opae-legacy.git
                              GIT_TAG ${OPAE_LEGACY_TAG}
                              PRESERVE_REPOS ${OPAE_PRESERVE_REPOS})

endif(OPAE_BUILD_LEGACY)
