# Copyright(c) 2019-2020, Intel Corporation
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
from opae.admin.version import pretty_version

setup(
    name="opae.admin",
    version=pretty_version(),
    packages=find_packages(include=['opae.*']),
    entry_points={
        'console_scripts': [
            'fpgasupdate = opae.admin.tools.fpgasupdate:main',
            'rsu = opae.admin.tools.rsu:main',
            'super-rsu = opae.admin.tools.super_rsu:main',
            'fpgaflash = opae.admin.tools.fpgaflash:main',
            'fpgaotsu = opae.admin.tools.fpgaotsu:main',
            'fpgaport = opae.admin.tools.fpgaport:main',
            'bitstreaminfo = opae.admin.tools.bitstream_info:main',
            'opaevfio = opae.admin.tools.opaevfio:main',
            'opaeuio = opae.admin.tools.opaeuio:main',
            'pci_device = opae.admin.tools.pci_device:main',
            'regmap-debugfs = opae.admin.tools.regmap_debugfs:main',
        ]
    },
    install_requires=[],
    description="opae.admin provides Python classes for interfacing with"
                "OPAE kernel drivers",
    license="BSD3",
    keywords="OPAE accelerator fpga kernel sysfs",
    data_files=[('share/doc/opae.admin',
                 ['LICENSE'])],
    url="https://01.org/OPAE",
    namespace_packages=['opae']
)
