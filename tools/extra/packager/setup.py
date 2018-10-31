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
        'console_scripts': ['packager = opae.tools.packager:main',
			     'afu_json_mgr = opae.tools.packager.afu_json_mgr:main',
		],
    },
    install_requires=['jsonschema>=2.3.0'],
    description="packager tool ",
    license="BSD3",
    keywords="OPAE tools extra packager",
    url="https://01.org/OPAE",
)
