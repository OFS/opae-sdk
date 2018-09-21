import os
import stat
import zipfile
from io import BytesIO

# Write to a buffer so that the shebang can be prepended easily
wr_buf = BytesIO()
wr_buf.write('#!/usr/bin/env python' + os.linesep)

z = zipfile.PyZipFile(wr_buf, 'w')
z.write('/home/workspace/jelonanx/opae-sdk/unittests/platforms/scripts/bin/afu_platform_config.py', 'afu_platform_config.py')
z.write('platmgr/__init__.py')
z.write('platmgr/jsondb.py')
z.write('platmgr/emitcfg.py')
z.write('/home/workspace/jelonanx/opae-sdk/unittests/platforms/scripts/afu_platform_config_main/__main__.py', '__main__.py')
z.close()

# Write out the buffer
with open('/home/workspace/jelonanx/opae-sdk/unittests/bin/afu_platform_config', 'wb') as f:
  f.write(wr_buf.getvalue())
  # Mark the file executable
  mode = os.fstat(f.fileno()).st_mode
  mode |= stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
  os.fchmod(f.fileno(), stat.S_IMODE(mode))

f.close()
