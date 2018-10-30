from setuptools import setup, find_packages

setup(
    name="opae.diag",
    version="@INTEL_FPGA_API_VERSION@",
    packages=find_packages(),
    install_requires=['opae.fpga>=1.2'],
    entry_points={
        'console_scripts': [
            "fpgadiag=opae.tools.fpgadiag:main"
        ]
    },
    description="pydiag includes fpgadiag and related utility libraries",
    license="BSD3",
    keywords="OPAE accelerator fpga fpgadiag nlb",
    url="https://01.org/OPAE",
)
