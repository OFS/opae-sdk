from setuptools import setup, find_packages
from distutils.extension import Extension


extensions = [
    Extension("opae",
              sources=["opae.cpp"],
              language="c++",
              extra_compile_args=["-std=c++11"],
              extra_link_args=["-std=c++11"],
              include_dirs=["@CMAKE_INSTALL_PREFIX@/include"],
              libraries=["opae-c", "opae-cxx-core"],
              library_dirs=["@CMAKE_INSTALL_PREFIX@/lib"])
]

setup(
    name="pyopae",
    version="@INTEL_FPGA_API_VERSION@",
    py_modules=['accel', 'diag'],
    packages=find_packages(),
    entry_points={
        'console_scripts': [
            'accel = accel:main',
        ]
    },
    ext_modules=extensions,
    description="pyopae provides Python bindings around the "
                "OPAE C API",
    license="BSD3",
    keywords="OPAE accelerator fpga bindings",
    url="https://01.org/OPAE",
)
