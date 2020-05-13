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

find_package(Perl)
set(PACKAGE_VERSION            "${CMAKE_VERSION_MAJOR}.${CMAKE_VERSION_MINOR}.${CMAKE_VERSION_PATCH}")
set(PACKAGE                    ${CMAKE_PROJECT})
set(ABS_SRCDIR                 ${OPAE_SDK_SOURCE})
set(DOX_EXTRACT_ALL            YES)
set(DOX_OUTPUT_DIRECTORY       ${CMAKE_BINARY_DIR}/doc)
set(DOX_STRIP_FROM_PATH        ${OPAE_SDK_SOURCE})
set(DOX_STRIP_FROM_INC_PATH    ${OPAE_INCLUDE_PATH})
set(DOX_GENERATETODOLIST       NO)
set(DOX_GENERATETESTLIST       YES)
set(DOX_GENERATEBUGLIST        NO)
set(DOX_GENERATEDEPRECATEDLIST YES)
set(DOX_LAYOUT_FILE            ${CMAKE_CURRENT_SOURCE_DIR}/DoxygenLayout.xml )
set(DOX_WARNINGS               YES)
set(DOX_GENERATE_HTML          YES)
set(DOX_GENERATE_LATEX         YES)
set(DOX_USE_PDFLATEX           NO)
set(DOX_GENERATE_RTF           NO)
set(DOX_GENERATE_MAN           YES)
set(DOX_GENERATE_XML           YES)
set(DOX_GENERATE_TAGFILE       ${CMAKE_BINARY_DIR}/doc/doxygen_sdk.tag)

if(PERL_FOUND)
    set(PERL ${PERL_EXECUTABLE})
endif(PERL_FOUND)

if(DOXYGEN_DOT_FOUND)
    set(HAVE_DOT YES)
else(DOXYGEN_DOT_FOUND)
    set(HAVE_DOT NO)
endif(DOXYGEN_DOT_FOUND)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
               ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

add_custom_target(doxygen
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API docs" VERBATIM
    SOURCES ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

                   
if(DOX_GENERATE_HTML)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc/html)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/doc/html DESTINATION share/opae COMPONENT dochtml)
endif()
if(DOX_GENERATE_LATEX)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc/latex)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/doc/latex DESTINATION share/opae COMPONENT doclatex)
endif()
if(DOX_GENERATE_RTF)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc/rtf)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/doc/rtf DESTINATION share/opae COMPONENT docrtf)
endif()
if(DOX_GENERATE_MAN)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc/man)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/doc/man DESTINATION share/opae COMPONENT docman)
endif()
if(DOX_GENERATE_XML)
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/doc/xml)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/doc/xml DESTINATION share/opae COMPONENT docxml)
endif()
