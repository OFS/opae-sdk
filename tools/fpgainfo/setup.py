from setuptools import setup, find_packages
setup(
    name="fpgainfo",
    version="@INTEL_FPGA_API_VERSION@",
    packages=find_packages(),
    entry_points={
        'console_scripts': [
            'fpgainfo = fpgainfo:main',
        ],
    },
    description="fpgainfo is an OPAE utility to query information on "
                "OPAE accelerators on the system",
    license="BSD3",
    keywords="OPAE accelerator fpga",
    url="https://01.org/OPAE",
    project_urls={
        "Bug Tracker": "https://github.com/OPAE/opae-sdk/issues",
        "Documentation": "https://opae.github.io",
        "Source Code": "https://github.com/OPAE/opae-sdk"
    }
)
