# Copyright(c) 2018-2023, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
#   may be used to  endorse or promote  products derived  from this  software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
# CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

from setuptools import Extension, setup, find_namespace_packages
from setuptools.command.build_ext import build_ext

# get the original build_extensions method
original_build_extensions = build_ext.build_extensions


def override_build_extensions(self):
    if '-Wstrict-prototypes' in self.compiler.compiler_so:
        self.compiler.compiler_so.remove('-Wstrict-prototypes')
    self.compiler.compiler_so.append('-fvisibility=hidden')
    # call the original build_extensions
    original_build_extensions(self)


# replace build_extensions with our custom version
build_ext.build_extensions = override_build_extensions


extensions = [
    Extension("opae.fpga._opae",
              sources=["pyproperties.cpp",
                       "pycontext.cpp",
                       "pyhandle.cpp",
                       "pytoken.cpp",
                       "pyshared_buffer.cpp",
                       "pyevents.cpp",
                       "pyerrors.cpp",
                       "pysysobject.cpp",
                       "opae.cpp"],
              language="c++",
              extra_compile_args=["-std=c++11"],
              extra_link_args=["-std=c++11"],
              include_dirs=[
                  "@CMAKE_INSTALL_PREFIX@/include",
                  "@OPAE_INCLUDE_PATH@",
                  "@pybind11_ROOT@/include"
              ],
              libraries=["opae-c", "opae-cxx-core", "uuid"],
              library_dirs=["@LIBRARY_OUTPUT_PATH@",
                            "@CMAKE_INSTALL_PREFIX@/lib",
                            "@CMAKE_INSTALL_PREFIX@/lib64"])
]

setup(
    name='opae.fpga',
    version='@OPAE_VERSION@',
    packages=find_namespace_packages(include=['opae.*']),
    ext_modules=extensions,
    entry_points={
        'console_scripts': [
            'opae-mem = opae.fpga.tools.opae_mem:main',
        ]
    },
)
