import os
from setuptools import setup, find_packages
from setuptools.command.build_ext import build_ext
from distutils.extension import Extension

# get the original build_extensions method
original_build_extensions = build_ext.build_extensions


def override_build_extensions(self):
    if '-Wstrict-prototypes' in self.compiler.compiler_so:
        self.compiler.compiler_so.remove('-Wstrict-prototypes')
    # call the original build_extensions
    original_build_extensions(self)


# replace build_extensions with our custom version
build_ext.build_extensions = override_build_extensions


def get_include_dirs(*args):
    include_dirs = list(set(args))
    try:
        import pybind11
    except ImportError:
        print "Couldn't import pybind11"
        return include_dirs
    else:
        pybind_include = pybind11.get_include()
        if not os.path.exists(os.path.join(pybind_include, 'pybind11')):
            pybind_include = pybind11.get_include(True)
        include_dirs.append(pybind_include)
    if not any([os.path.exists(os.path.join(inc, 'opae'))
                for inc in include_dirs]):
        print "Could not find OPAE in any include paths"
    return include_dirs


extensions = [
    Extension("opae.fpga._opae",
              sources=["pyproperties.cpp",
                       "pyhandle.cpp",
                       "pytoken.cpp",
                       "pyshared_buffer.cpp",
                       "pyevents.cpp",
                       "opae.cpp"],
              language="c++",
              extra_compile_args=["-std=c++11"],
              extra_link_args=["-std=c++11"],
              include_dirs=get_include_dirs("@CMAKE_INSTALL_PREFIX@/include"),
              libraries=["opae-c", "opae-cxx-core"],
              library_dirs=["@CMAKE_INSTALL_PREFIX@/lib"])
]

setup(
    name="pyopae",
    version="@INTEL_FPGA_API_VERSION@",
    packages=find_packages(),
    entry_points={
        'console_scripts': [
        ]
    },
    ext_modules=extensions,
    description="pyopae provides Python bindings around the "
                "OPAE C API",
    license="BSD3",
    keywords="OPAE accelerator fpga bindings",
    url="https://01.org/OPAE",
)
