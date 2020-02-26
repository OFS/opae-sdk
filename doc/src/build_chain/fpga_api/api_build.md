Building OPAE SDK Artifacts
===========================

Steps
-----

1.  Fetch the OPAE SDK source tree
2.  Configure the OPAE SDK CMake project
3.  Build OPAE SDK targets

The example below lists commands that can be used to fetch and build OPAE SDK.

``` {.bash}
# fetch the source
git clone https://github.com/OPAE/opae-sdk.git
cd opae-sdk
# configure CMake
cmake ..
# build
make


```

For a list of targets that can be built, type `make help` from the build
directory.

CMake options that may be set during the configuration include the following:


```
|----------------------------|-----------------------|-------------------------------------|---------------------------------------|----------------|
| cmake flag                 | Optional or Mandatory | Purpose                             | Valid values                          | Default value  |
|----------------------------|-----------------------|-------------------------------------|---------------------------------------|----------------|
| -DCMAKE_BUILD_TYPE         | Optional              | Set compiler flags                  | Debug/Release/Coverage/RelWithDebInfo | RelWithDebInfo |
| -DOPAE_BUILD_LEGACY        | Optional              | Enable/disable opae-legacy.git      | ON/OFF                                | OFF            |
| -DOPAE_BUILD_SPHINX_DOC    | Optional              | Enable/disable documentation build  | ON/OFF                                | OFF            |
| -DOPAE_BUILD_TESTS         | Optional              | Enable/disable building unit tests  | ON/OFF                                | OFF            |
| -DOPAE_INSTALL_RPATH       | Optional              | Enable/disable rpath for install    | ON/OFF                                | OFF            |
| -DOPAE_BUILD_LIBOPAE_CXX   | Optional              | Enable/disable OPAE C++ bindings    | ON/OFF                                | ON             | 
| -DOPAE_BUILD_LIBOPAE_PY    | Optional              | Enable/disable OPAE Python bindings | ON/OFF                                | ON             |
| -DOPAE_BUILD_PYTHON_DIST   | Optional              | Enable/disable Python Distribution  | ON/OFF                                | OFF            |
| -DOPAE_ENABLE_MOCK         | Optional              | Enable/disable mocks for unit tests | ON/OFF                                | OFF            |
| -DOPAE_BUILD_SIM           | Optional              | Enable/disable opae-sim.git         | ON/OFF                                | OFF            |

```


