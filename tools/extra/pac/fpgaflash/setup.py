import os
from setuptools import setup, find_packages

setup(
    name="opae.fpgaflash",
    version="@INTEL_FPGA_API_VERSION@",
    packages=find_packages(),
    entry_points={
        'console_scripts': ['fpgaflash = opae.tools.fpgaflash:main'],
    },
    description="fpgaflash tool ",
    license="BSD3",
    keywords="OPAE tools extra fpgaflash",
    url="https://01.org/OPAE",
)