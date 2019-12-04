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
| -DBUILD_TESTS              | Optional              | Enable/disable building gtests      | ON/OFF                                | OFF            |
| -DBUILD_ASE                | Optional              | Enable/disable building ASE         | ON/OFF                                | ON             |

```


