import os
import stat
import zipfile
from io import BytesIO

# Write to a buffer so that the shebang can be prepended easily
wr_buf = BytesIO()
wr_buf.write('#!/usr/bin/env python' + os.linesep)

z = zipfile.PyZipFile(wr_buf, 'w')
z.write('fpga_common.py')
z.write('fpga_fmeinfo.py')
z.write('fpga_portinfo.py')
z.write('fpgaerr.py')
z.write('fpgainfo.py')
z.write('fpgapwr.py')
z.write('fpgatemp.py')
z.write('sysfs.py')
z.write('/home/workspace/jelonanx/opae-sdk/unittests/tools/base/fpgainfo/fpgainfo_main/__main__.py', '__main__.py')
z.close()

# Write out the buffer
with open('/home/workspace/jelonanx/opae-sdk/unittests/bin/fpgainfo', 'wb') as f:
  f.write(wr_buf.getvalue())
  # Mark the file executable
  mode = os.fstat(f.fileno()).st_mode
  mode |= stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
  os.fchmod(f.fileno(), stat.S_IMODE(mode))

f.close()
