pefile: Python module to read and work with PE (Portable Executable) files
https://github.com/erocarrera/pefile

To create:

- unzip pefile-2018.8.8.zip
- edit pefile.py to remove the "from builtins import XYZ"
  - Comment lines 23-28
- edit pefile.py to not parse version struct. It has problems with unicode in the version data.
  - Comment lines 3127-3128
- Build distribution:
  >>> c:\python27\python setup.py dist
- Copy library in build/lib/... to be along side get-cmf_sign.py.

Thanks!
 -> Fred Hsueh (fred.hsueh@intel.com)
