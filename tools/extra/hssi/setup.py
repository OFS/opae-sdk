# Copyright(c) 2021, Intel Corporation
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
from setuptools import setup, find_packages
from setuptools.command.build_ext import build_ext
from distutils.ccompiler import new_compiler
from distutils.extension import Extension
from distutils.errors import CompileError, DistutilsExecError

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
            Extension("dummy",
                      sources=['dummy/dummy.cpp'],
                      language="c++",
                      extra_compile_args=["-std=c++11"],
                      extra_link_args=["-std=c++11"]
                      )
]

class pybind_include_dirs(object):
    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)


setup(
    name='hssi',
    version="2.0",
    package_dir={'hssi': '.'},
    packages=['hssi', 'hssi.'],
    platforms='linux-x86_x64',
    include_dirs=[pybind_include_dirs()],
    entry_points={
        'console_scripts': [
            'hssistats = hssi.hssistats:main',
            'hssimac = hssi.hssimac:main',
            'hssiloopback = hssi.hssiloopback:main',
            'hssicommon = hssi.hssicommon:main',
        ]
    },
    description="hssi eth tools"
    "for ethernet UIO",
    license="BSD3",
    ext_modules=extensions,
    keywords="OPAE hssi tools ",
    url="https://01.org/OPAE",
    include_package_data=True,
)
