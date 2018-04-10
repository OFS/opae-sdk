Building the OPAE C Library
===========================

Steps
-----

1.  Fetch the Intel FPGA API repository
2.  Configure the Intel FPGA API CMake project
3.  Build the Intel FPGA API project

Fetch the Intel FPGA API repository
--------------------------------------

``` {.bash}

git clone ssh://«Place your username here»@git-amr-1.devtools.intel.com:29418/cpt_sys_sw-fpga-sw
cd cpt_sys_sw-fpga-sw
git config user.name "«Place your name here»"
git config user.email "«Place your email here»"
curl -k https://git-amr-1.devtools.intel.com/gerrit/tools/hooks/commit-msg -o .git/hooks/commit-msg
git checkout feature/cmake
git pull

```

Configure the Intel FPGA API CMake project
------------------------------------------

``` {.bash}

cd cpt_sys_sw-fpga-sw
mkdir mybuild
cd mybuild
cmake .. «user configuration flags»

```

Valid «user configuration flags» are:


```
|----------------------------|-----------------------|-------------------------------------|---------------------------------------|----------------|
| cmake flag                 | Optional or Mandatory | Purpose                             | Valid values                          | Default value  |
|----------------------------|-----------------------|-------------------------------------|---------------------------------------|----------------|
| -DINTEL_FPGA_API_VER_MAJOR | Optional              | OPAE major version                  | Integer                               | 0              |
| -DINTEL_FPGA_API_VER_MINOR | Optional              | OPAE minor version                  | Integer                               | 13             |
| -DINTEL_FPGA_API_VER_REV   | Optional              | OPAE revision version               | Integer                               | 0              |
| -DCMAKE_BUILD_TYPE         | Optional              | Set compiler flags                  | Debug/Release/Coverage/RelWithDebInfo | RelWithDebInfo |
| -DBUILD_TESTS              | Optional              | Enable/disable building gtests      | ON/OFF                                | OFF            |
| -DBUILD_ASE                | Optional              | Enable/disable building ASE         | ON/OFF                                | ON             |
| -DPACK_ASE                 | Optional              | Include or not ASE in final package | ON/OFF                                | OFF            |
| -DBUILD_SPHINX_DOC         | Optional              | Enable/disable building Sphinx docs | ON/OFF                                | OFF            |

```


Building Sphinx documentation site requires previous installation of Python,
Sphinx and Sphinx-related Python packages.


```eval_rst

.. warning::
   Required Python packages to generate OPAE documentation can be installed with:

   $ pip install --user -r ${CMAKE_SRC_DIR}/doc/sphinx/requirements.txt
```

Sphinx documentation website is generated under
`${CMAKE_BINARY_DIR}/sphinx/${INTEL_FPGA_API_VER_MAJOR}.{INTEL_FPGA_API_VER_MINOR}.{INTEL_FPGA_API_VER_REV}/html/index.html`

Build the Intel FPGA API project
--------------------------------

``` {.bash}

cd cpt_sys_sw-fpga-sw
cd mybuild # (created during previous step)
make «user target»

```

Valid «user targets» are:

  make target                Purpose
  -------------------------- --------------------------------------------------------------------------------------
  make                       Compiles the Intel FPGA API libraries, sample applications, utilities
  make dist                  Creates distributable tarball package intel-fpga_0.1.0.tar.gz
  make package               Create Redhat installer package: intel-fpga_0.1.0_1.x86_64.rpm
  make docs                  Generate doxygen documentation
  make install               Install headers, libraries, sample applications and utilities under installation directory (typically /usr/local)


```eval_rst

.. note::

   For information on how to build and link applications using the Intel FPGA API, please refer to the Intel FPGA Library Programming Guide.

```
