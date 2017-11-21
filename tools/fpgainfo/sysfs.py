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
import json
import logging
import os
import re
import uuid

pattern = ('.*/\d+:(?P<bus>\w{2}):'
           '(?P<device>\d{2})\.(?P<function>\d).*')
bdf_pattern = re.compile(pattern)

ROOT_PATH = '/sys/class/fpga'
FPGA_DEVICE = os.path.join(ROOT_PATH, 'intel-fpga-dev.{instance_id}')
FME_DEVICE = os.path.join(FPGA_DEVICE, 'intel-fpga-fme.{instance_id}')
PORT_DEVICE = os.path.join(FPGA_DEVICE, 'intel-fpga-port.{instance_id}')


def read_bdf(path):
    symlink = os.readlink(path)
    m = bdf_pattern.match(symlink)
    data = m.groupdict() if m else {}
    return dict([(k, int(v, 16)) for (k, v) in data.iteritems()])


class sysfs_filter(object):
    def __init__(self, **kwargs):
        self._bus = None
        self._device = None
        self._function = None
        for attr in ('bus', 'device', 'function'):
            value = kwargs.get(attr)
            if isinstance(value, str) or isinstance(value, unicode):
                value = int(value, 16)
            setattr(self, '_{}'.format(attr), value)

    def __call__(self, resource):
        if self._bus is not None and self._bus != resource.bus:
            return False
        if self._device is not None and self._device != resource.device:
            return False
        if self._function is not None and self._function != resource.function:
            return False
        return True


def add_static_property(sysfs_path):
    return property(lambda s_: s_.parse_sysfs(sysfs_path),
                    lambda s_, v_: s_.write_sysfs(v_, sysfs_path))


class sysfs_resource(object):
    def __init__(self, path, instance_id, **kwargs):
        self._path = path
        self._instance_id = instance_id
        self._bus = kwargs.get('bus')
        self._device = kwargs.get('device')
        self._function = kwargs.get('function')

    def to_dict(self, include_bdf=True):
        data = {}
        for k, v in vars(self).iteritems():
            if hasattr(v.__class__, 'to_dict'):
                data[k] = v.to_dict(False)
        for k, v in vars(self.__class__).iteritems():
            if isinstance(v, property):
                data[k] = v.fget(self)
        for k, v in vars(self).iteritems():
            if isinstance(v, property):
                data[k] = v.fget(self)
        if include_bdf:
            root_data = {}
            root_data["class_path"] = self.sysfs_path
            root_data["dev_path"] = os.path.realpath(self.sysfs_path)
            root_data["bus"] = self.bus
            root_data["device"] = self.device
            root_data["function"] = self.function
            data["pcie_info"] = root_data

        return data

    def to_json(self):
        return json.dumps(self.to_dict(), indent=4, sort_keys=False)

    def sysfs_path_exists(self, *paths):
        return os.path.exists(os.path.join(self._path, *paths))

    def read_sysfs(self, *paths):
        filepath = os.path.join(self._path, *paths)
        if not os.path.exists(filepath):
            print("WARNING: {} not found".format(filepath))
            return None
        with open(filepath, 'r') as fd:
            return fd.read().strip()

    def write_sysfs(self, value, *paths):
        filepath = os.path.join(self._path, *paths)
        if not os.path.exists(filepath):
            print("WARNING: {} not found".format(filepath))
            return None
        with open(filepath, 'w') as fd:
            return fd.write(value)

    def parse_sysfs(self, *paths):
        value = self.read_sysfs(*paths)
        try:
            value = int(value)
        except ValueError:
            try:
                value = int(value, 16)
            except Exception:
                logging.warn("Could not parse value: {}".format(value))
        finally:
            return value

    def print_info(self, label, *props, **kwargs):
        print(label.upper())
        print('{:22} : {}'.format('Class Path', self.sysfs_path))
        print(
            '{:22} : {}'.format(
                'Device Path',
                os.path.realpath(
                    self.sysfs_path)))
        print('{:22} : 0x{:02X}'.format('Bus', self.bus))
        print('{:22} : 0x{:02X}'.format('Device', self.device))
        print('{:22} : 0x{:02X}'.format('Function', self.function))
        for prop in filter(
                lambda p: hasattr(
                    self.__class__, p) or hasattr(self, p),
                props):
            prop_value = getattr(self, prop)
            prop_value = kwargs.get(prop, lambda p: p)(prop_value)
            print(
                u'{:22} : {}'.format(
                    ' '.join([p.capitalize() for p in prop.split('_')]),
                    prop_value))
        print('\n')

    @property
    def sysfs_path(self):
        return self._path

    @property
    def instance_id(self):
        return self._instance_id

    @property
    def bus(self):
        return self._bus

    @property
    def device(self):
        return self._device

    @property
    def function(self):
        return self._function


class pr_feature(sysfs_resource):
    @property
    def interface_id(self):
        return str(uuid.UUID(self.read_sysfs("interface_id")))


class power_mgmt_feature(sysfs_resource):
    consumed = add_static_property("consumed")


class thermal_feature(sysfs_resource):
    temperature = add_static_property("temperature")
    threshold1 = add_static_property("threshold1")
    threshold1_policy = add_static_property("threshold1_policy")
    threshold1_reached = add_static_property("threshold1_reached")
    threshold2 = add_static_property("threshold2")
    threshold2_reached = add_static_property("threshold2_reached")
    threshold_trip = add_static_property("threshold_trip")


class errors_feature(sysfs_resource):
    error_files = {}
    revision = add_static_property("revision")

    def __init__(self, path, instance_id, **kwargs):
        super(errors_feature, self).__init__(path, instance_id, **kwargs)
        self.name = None
        self.errors_file = None
        self.clear_file = None
        self.error_classes = []
        self.valid_error_files = []
        # first check the error files for the given revision
        errfiles = self.error_files[self.revision]
        if all([self.sysfs_path_exists(f) for f in errfiles]):
            self.add_error_props(*errfiles)
        else:
            for rev, errfiles in self.error_files.iteritems():
                self.add_error_props(*errfiles)

    def print_info(self, label, *props, **kwargs):
        super(errors_feature, self).print_info(label, *self.valid_error_files)

    def add_error_props(self, *errfiles):
        for err_file in errfiles:
            base_name = os.path.basename(err_file)
            if self.sysfs_path_exists(
                    err_file) and not hasattr(self, base_name):
                self.valid_error_files.append(base_name)
                setattr(self,
                        os.path.basename(err_file),
                        property(lambda self_: self_.parse_sysfs(err_file)))

    def clear(self):
        value = self.parse_sysfs(self.errors_file)
        try:
            if value:
                self.write_sysfs(hex(value), self.clear_file)
            return True
        except IOError:
            logging.warn(
                "Could not clear errors: {}."
                "Are you running as root?".format(
                    self.clear_file))
        return False


class fme_errors(errors_feature):
    error_files = {
        0: ["fme-errors/errors",
            "fme-errors/first_error",
            "fme-errors/next_error",
            "bbs_errors",
            "gbs_errors",
            "pcie0_errors",
            "pcie1_errors"],
        1: ["fme-errors/errors",
            "fme-errors/first_error",
            "fme-errors/next_error",
            "catfatal_errors",
            "nonfatal_errors",
            "pcie0_errors",
            "pcie1_errors"]}

    def __init__(self, path, instance_id, **kwargs):
        super(fme_errors, self).__init__(path, instance_id, **kwargs)
        self.name = "fme errors"
        self.errors_file = "fme-errors/errors"
        self.clear_file = "fme-errors/clear"


class port_errors(errors_feature):
    error_files = {
        0: ["errors",
            "first_error"],
        1: ["errors",
            "first_error"]}

    def __init__(self, path, instance_id, **kwargs):
        super(port_errors, self).__init__(path, instance_id, **kwargs)
        self.name = "port errors"
        self.errors_file = "errors"
        self.clear_file = "clear"


class fme_info(sysfs_resource):
    def __init__(self, path, instance_id, **kwargs):
        super(fme_info, self).__init__(path, instance_id, **kwargs)
        self.pr = pr_feature(os.path.join(path, 'pr'), instance_id, **kwargs)
        self.power_mgmt = power_mgmt_feature(os.path.join(path, 'power_mgmt'),
                                             instance_id,
                                             **kwargs)
        self.thermal_mgmt = thermal_feature(os.path.join(path, "thermal_mgmt"),
                                            instance_id,
                                            **kwargs)
        self.errors = fme_errors(os.path.join(path, "errors"),
                                 instance_id,
                                 **kwargs)

    @property
    def version(self):
        return self.read_sysfs("version")

    @property
    def ports_num(self):
        return self.read_sysfs("ports_num")

    @property
    def socket_id(self):
        return self.read_sysfs("socket_id")

    @property
    def bitstream_id(self):
        return self.read_sysfs("bitstream_id")

    @property
    def bitstream_metadata(self):
        return self.read_sysfs("bitstream_metadata")

    @property
    def object_ID(self):
        value = self.read_sysfs("dev")
        valueList = value.split(":")
        major = valueList[0]
        minor = valueList[1]
        objID = ((int(major) & 0xFFF) << 20) | (int(minor) & 0xFFFFF)
        return hex(objID) + "   FPGA_DEVICE"


class port_info(sysfs_resource):
    def __init__(self, path, instance_id, **kwargs):
        super(port_info, self).__init__(path, instance_id, **kwargs)
        self.errors = port_errors(os.path.join(path, "errors"),
                                  instance_id,
                                  **kwargs)

    @property
    def afu_id(self):
        return str(uuid.UUID(self.read_sysfs("afu_id")))

    @property
    def object_ID(self):
        value = self.read_sysfs("dev")
        valueList = value.split(":")
        major = valueList[0]
        minor = valueList[1]
        objID = ((int(major) & 0xFFF) << 20) | (int(minor) & 0xFFFFF)
        return hex(objID) + "   FPGA_ACCELERATOR"


class sysfsinfo(object):
    def __init__(self):
        self._fmelist = []
        self._portlist = []
        sysfs_paths = glob.glob(
            FPGA_DEVICE.format(instance_id='*'))
        if not sysfs_paths:
            print "WARNING: No sysfs paths found"
        for path in sysfs_paths:
            bdf = read_bdf(os.path.join(path, 'device'))
            # strip {instance_id} from the template FPGA_DEVICE
            # socket id is what comes after this in the real path
            instance_id = path.strip(FPGA_DEVICE.strip('{instance_id}'))
            sysfs_fme = FME_DEVICE.format(instance_id=instance_id)
            sysfs_port = PORT_DEVICE.format(instance_id=instance_id)
            self._fmelist.append(fme_info(sysfs_fme, instance_id, **bdf))
            self._portlist.append(port_info(sysfs_port, instance_id, **bdf))

    def fme(self, **kwargs):
        return filter(sysfs_filter(**kwargs), self._fmelist)

    def port(self, **kwargs):
        return filter(sysfs_filter(**kwargs), self._portlist)


if __name__ == "__main__":
    import argparse
    import pprint
    parser = argparse.ArgumentParser()
    parser.add_argument('-b', '--bus')
    parser.add_argument('-d', '--device')
    parser.add_argument('-f', '--function')

    args = parser.parse_args()
    info = sysfsinfo()
    fmelist = info.fme(**vars(args))
    portlist = info.port(**vars(args))
    pprint.pprint([f.to_dict() for f in fmelist])
