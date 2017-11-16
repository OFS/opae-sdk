# Copyright(c) 2017, Intel Corporation
##
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
##
# * Redistributions of  source code  must retain the  above copyright notice,
# this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
# may be used to  endorse or promote  products derived  from this  software
# without specific prior written permission.
##
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
# CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
import glob
import os
import re
import fpga_common
import uuid

pattern = ('.*pci\d+:\w+/\d+:(?P<bus>\w{2}):'
           '(?P<device>\d{2})\.(?P<function>\d).*')
bdf_pattern = re.compile(pattern)


def read_bdf(path):
    symlink = os.readlink(path)
    m = bdf_pattern.match(symlink)
    return m.groupdict() if m else {}


class sysfs_resource(object):
    def __init__(self, path, socket_id, **kwargs):
        self._path = path
        self._socket_id = socket_id
        self._bus = kwargs.get('bus')
        self._device = kwargs.get('device')
        self._function = kwargs.get('function')
        self._errors_rev = int(self.read_sysfs("errors/revision"))

    def read_sysfs(self, *paths):
        filepath = os.path.join(self._path, *paths)
        with open(filepath, 'r') as fd:
            return fd.read().strip()

    @property
    def socket_id(self):
        return self._socket_id

    @property
    def bus(self):
        return self._bus

    @property
    def device(self):
        return self._device

    @property
    def function(self):
        return self._function

    @property
    def errors_revision(self):
        return self._errors_rev


class fme_info(sysfs_resource):
    def __init__(self, path, socket_id, **kwargs):
        super(fme_info, self).__init__(path, socket_id, **kwargs)

    @property
    def pr_interface_id(self):
        return uuid.UUID(self.read_sysfs("pr/interface_id"))


class port_info(sysfs_resource):
    def __init__(self, path, socket_id, **kwargs):
        super(port_info, self).__init__(path, socket_id, **kwargs)

    @property
    def afu_id(self):
        return uuid.UUID(self.read_sysfs("afu_id"))


class sysfsinfo(object):
    def __init__(self):
        self._fmelist = []
        self._portlist = []
        sysfs_paths = glob.glob(fpga_common.FPGA_DEVICE.format(socket_id='*'))
        if not sysfs_paths:
            print "WARNING: No sysfs paths found"
        for path in sysfs_paths:
            bdf = read_bdf(path)
            # strip {socket_id} from the template FPGA_DEVICE
            # socket id is what comes after this in the real path
            socket_id = path.strip(fpga_common.FPGA_DEVICE
                                   .strip('{socket_id}'))
            sysfs_fme = fpga_common.FME_DEVICE.format(socket_id=socket_id)
            sysfs_port = fpga_common.PORT_DEVICE.format(socket_id=socket_id)
            self._fmelist.append(fme_info(sysfs_fme, socket_id, **bdf))
            self._portlist.append(port_info(sysfs_port, socket_id, **bdf))

    def fme(self):
        return self._fmelist

    def port(self):
        return self._portlist
