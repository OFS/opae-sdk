from setuptools import setup, find_packages
from setuptools.command.build_ext import build_ext
from distutils.extension import Extension

original_build_extensions = build_ext.build_extensions
def override_build_extensions(self):
    self.compiler.compiler_so.remove('-Wstrict-prototypes')
    original_build_extensions(self)
build_ext.build_extensions = override_build_extensions

extensions = [
    Extension("opae._opae",
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
    packages=find_packages(),
    entry_points={
        'console_scripts': [
            'accel = opae.accel:main',
        ]
    },
    ext_modules=extensions,
    description="pyopae provides Python bindings around the "
                "OPAE C API",
    license="BSD3",
    keywords="OPAE accelerator fpga bindings",
    url="https://01.org/OPAE",
)
