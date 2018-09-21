import os
import stat
import zipfile
from io import BytesIO

# Write to a buffer so that the shebang can be prepended easily
wr_buf = BytesIO()
wr_buf.write('#!/usr/bin/env python' + os.linesep)

z = zipfile.PyZipFile(wr_buf, 'w')
z.write('/home/workspace/jelonanx/opae-sdk/unittests/tools/extra/packager/packager.py', 'packager.py')
z.write('afu.py')
z.write('gbs.py')
z.write('utils.py')
z.write('metadata/constants.py')
z.write('metadata/__init__.py')
z.write('metadata/metadata.py')
z.write('schema/afu_schema_v01.json')
z.write('README')
z.write('/home/workspace/jelonanx/opae-sdk/unittests/tools/extra/packager/packager_main/__main__.py', '__main__.py')
z.close()

# Write out the buffer
with open('/home/workspace/jelonanx/opae-sdk/unittests/bin/packager', 'wb') as f:
  f.write(wr_buf.getvalue())
  # Mark the file executable
  mode = os.fstat(f.fileno()).st_mode
  mode |= stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
  os.fchmod(f.fileno(), stat.S_IMODE(mode))

f.close()
