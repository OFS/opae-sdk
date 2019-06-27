Files:

cmf_sign-19.1-133.zip
 - Archive of cmf_sign.exe and all necessary DLLs from quartuskit/19.1/133/windows64.
 - Includes the msvcrt-14.13 binaries.
 - Extract these files and place them in tools\sign\cmf_sign. ndsign.py on Windows will look for
   cmf_sign\cmf_sign.exe.

msvcrt-14.13.zip
 - Archive of the Microsoft Visual Studio CRT from 14.13 release. Needed work around
   https://hsdes.intel.com/resource/1408565970. Copy these DLLs along side your cmf_sign.exe if
   you run into this problem.
 - Source:
   - Install Visual Studio 2017, including the feature "VC++ 2017 version 15.6 v14.13 toolset".
   - Copy binaries from:
     C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Redist\MSVC\14.13.26020\x64\Microsoft.VC141.CRT
