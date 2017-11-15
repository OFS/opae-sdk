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

  cmake flag                     Optional or mandatory   Purpose                            Valid values                                 Default value
  ----------------------------- ----------------------- ----------------------------------- -------------------------------------------- --------------
  -DINTEL\_FPGA\_API\_VER\_MAJOR    Optional                Driver major version                 Integer                                        0
  -DINTEL\_FPGA\_API\_VER\_MINOR    Optional                Driver minor version                 Integer                                        1
  -DINTEL\_FPGA\_API\_VER\_REV      Optional                Driver revision version              Integer                                        0
  -DCMAKE\_BUILD\_TYPE            Optional                Set compiler and linker flags        Debug | Release | Coverage | RelWithDebInfo   RelWithDebInfo
  -DBUILD\_TESTS                 Optional                Enable | disable building gtests     ON | OFF                                      OFF
  -DBUILD\_ASE                   Optional                Enable | disable building ASE        ON | OFF                                      OFF
  -DPACK\_ASE                    Optional                Include or not ASE in final package  ON | OFF                                      OFF

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
  make dist                  Creates distributable tarball package intel-fpga\_0.1.0.tar.gz
  make package               Create Redhat installer package: intel-fpga\_0.1.0\_1.x86\_64.rpm
  make docs                  Generate doxygen documentation
  make install               Install headers, libraries, sample applications and utilities under installation directory (typically /usr/local)

.. note::

```
   For information on how to build and link applications using the Intel FPGA API, please refer to the Intel FPGA Library Programming Guide.

```
