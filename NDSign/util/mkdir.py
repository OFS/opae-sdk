#!/usr/bin/env python
import errno
import os

class Mkdir(object):

    def mkdir_p(self,path):
        if path is None or path == "":
            return
        try:
            os.makedirs(path)
        except OSError as exc:
            if exc.errno == errno.EEXIST and os.path.isdir(path):
                pass
            else:
                raise
