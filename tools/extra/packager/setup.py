import os
from setuptools import setup, find_packages

setup(
    name="opae.fpga.packager",
    version="@INTEL_FPGA_API_VERSION@",
    packages=find_packages(),
    entry_points={
        'console_scripts': ['packager = opae.fpga.packager:main'],
    },
    install_requires=['jsonschema'],
    description="packager tool ",
    license="BSD3",
    keywords="OPAE tools extra packager",
    url="https://01.org/OPAE",
)
