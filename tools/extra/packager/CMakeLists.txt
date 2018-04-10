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

cmake_minimum_required(VERSION 2.8.12)
include(packaging)

## Install target for jsonschema files
set(JSONSCHEMA_SHARE_DIR share/opae/python)
set(JSONSCHEMA_BIN_DIR ${PROJECT_BINARY_DIR}/share/opae/jsonschema-2.3.0)
set(JSONSCHEMA_DIR ${OPAE_SDK_SOURCE}/tools/extra/packager/jsonschema-2.3.0)

################# Add version to packager.py ########################
configure_file(packager.py ${CMAKE_CURRENT_BINARY_DIR}/packager.py @ONLY)

set(PKG_PATH_MODULE ${CMAKE_CURRENT_BINARY_DIR}/packager.py packager.py)

################# generate executable python zip files ########################
set(METADATA_FILES metadata/constants.py metadata/__init__.py metadata/metadata.py)
set(SCHEMA_FILES schema/afu_schema_v01.json)

set(PKG_COMMON_FILES afu.py gbs.py utils.py ${METADATA_FILES} ${SCHEMA_FILES})

set(PKG_PYTHON_FILES PKG_PATH_MODULE ${PKG_COMMON_FILES} README)
CREATE_PYTHON_EXE(packager packager ${PKG_PYTHON_FILES})
set(PACKAGER_BIN_LIST ${PACKAGER_BIN})

set(AFU_JSON_MGR_FILES afu_json_mgr.py ${PKG_COMMON_FILES} schema/afu_template.json)
CREATE_PYTHON_EXE(afu_json_mgr afu_json_mgr ${AFU_JSON_MGR_FILES})
list(APPEND PACKAGER_BIN_LIST ${PACKAGER_BIN})

file(COPY ${OPAE_SDK_SOURCE}/tools/extra/packager/jsonschema-2.3.0/ DESTINATION ${JSONSCHEMA_BIN_DIR})

################# install executable python zip files  ######################## 
install(PROGRAMS ${PACKAGER_BIN_LIST}
        DESTINATION bin
        COMPONENT toolpackager)

################# install jsonschema files  ######################## 
install(DIRECTORY ${JSONSCHEMA_DIR}
        DESTINATION ${JSONSCHEMA_SHARE_DIR}
        COMPONENT jsonschema)

