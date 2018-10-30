import os
from setuptools import setup, find_packages

setup(
    name="opae.packager",
    version="@INTEL_FPGA_API_VERSION@",
    packages=find_packages(),
    package_data={
        'opae.tools.packager' : ['schema/*.json'],
    },
    entry_points={
        'console_scripts': ['packager = opae.tools.packager:main'],
    },
    install_requires=['jsonschema'],
    description="packager tool ",
    license="BSD3",
    keywords="OPAE tools extra packager",
    url="https://01.org/OPAE",
)
