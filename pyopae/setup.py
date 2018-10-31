import os
from setuptools import setup, find_packages
from setuptools.command.build_ext import build_ext
from distutils.extension import Extension




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


class pybind_include_dirs(object):
    def __init__(self, user=False):
        self.user = user

    def __str__(self):
        import pybind11
        return pybind11.get_include(self.user)


extensions = [
    Extension("opae.fpga._opae",
              sources=["pyproperties.cpp",
                       "pycontext.cpp",
                       "pyhandle.cpp",
                       "pytoken.cpp",
                       "pyshared_buffer.cpp",
                       "pyevents.cpp",
                       "pyerrors.cpp",
                       "pysysobject.cpp",
                       "opae.cpp"],
              language="c++",
              extra_compile_args=["-std=c++11"],
              extra_link_args=["-std=c++11"],
              include_dirs=[
                  "@CMAKE_INSTALL_PREFIX@/include",
                  os.environ.get("OPAE_INCLUDE_DIR", ""),
                  pybind_include_dirs(),
                  pybind_include_dirs(True)
              ],
              libraries=["opae-c", "opae-cxx-core", "uuid"],
              library_dirs=[os.environ.get("OPAE_LIBRARY_DIR", ""),
                            "@CMAKE_INSTALL_PREFIX@/lib",
                            "@CMAKE_INSTALL_PREFIX@/lib64"])
]

setup(
    name="opae.fpga",
    version="@INTEL_FPGA_API_VERSION@",
    packages=find_packages(),
    entry_points={
        'console_scripts': [
        ]
    },
    ext_modules=extensions,
    install_requires=['pybind11>=@PYOPAE_PYBIND11_VERSION@'],
    description="pyopae provides Python bindings around the "
                "OPAE C API",
    license="BSD3",
    keywords="OPAE accelerator fpga bindings",
    url="https://01.org/OPAE",
)
