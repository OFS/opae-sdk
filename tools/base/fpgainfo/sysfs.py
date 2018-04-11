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
import inspect
import json
import logging
import os
import re
import uuid

pattern = (r'.*/\d+:(?P<bus>\w{2}):'
           r'(?P<device>\d{2})\.(?P<function>\d).*')
bdf_pattern = re.compile(pattern)

ROOT_PATH = '/sys/class/fpga'
FPGA_DEVICE = os.path.join(ROOT_PATH, 'intel-fpga-dev.{instance_id}')
FME_DEVICE = os.path.join(FPGA_DEVICE, 'intel-fpga-fme.{instance_id}')
PORT_DEVICE = os.path.join(FPGA_DEVICE, 'intel-fpga-port.{instance_id}')

MAJOR_VER = -15
MINOR_VER = -14
PATCH_VER = -13

DCP_ID = 0x09c4


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


def add_dynamic_property(obj, property_name, sysfs_path=None):
    sysfs_path = sysfs_path or property_name

    def getter(self_):
        return self_.parse_sysfs(sysfs_path)

    def setter(self_, v_):
        self_.write_sysfs(v_, sysfs_path)

    if obj.sysfs_path_exists(sysfs_path):
        setattr(obj, property_name,
                property(getter, setter))


class sysfs_node(object):
    def __init__(self, path, instance_id, device_id=None, **kwargs):
        self._path = path
        self._instance_id = instance_id
        self._device_id = device_id
        self._bus = kwargs.get('bus')
        self._device = kwargs.get('device')
        self._function = kwargs.get('function')

    def enum_props(self, as_string=False):
        def pred(xxx_todo_changeme):
            (k, v) = xxx_todo_changeme
            if k in dir(sysfs_node):
                return False
            if inspect.ismethod(v):
                return False
            if k.startswith('_'):
                return False
            return True

        for k, v in filter(pred, inspect.getmembers(self)):
            k_, v_ = k, v
            if isinstance(v, property):
                k_, v_ = k, v.fget(self)
            elif not as_string and hasattr(v, 'to_dict') and k != '__class__':
                k_, v_ = k, v.to_dict()
            if as_string:
                yield k_, unicode(v_)
            else:
                yield k_, v_

    def to_dict(self):
        data = {}
        for k, v in self.enum_props():
            data[k] = v
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
        print('{:22} : 0x{:04X}'.format('Device Id', self.device_id))

        prop_values = [(k, v) for k, v in self.enum_props(as_string=True)]
        if props:
            def get_value(key):
                namespaces = key.split('.')
                obj = self
                try:
                    while len(namespaces) > 1:
                        obj = getattr(obj, namespaces.pop(0))
                    p = getattr(obj, namespaces[0])
                    if isinstance(p, property):
                        p = p.fget(self)
                    return p
                except AttributeError:
                    pass

            prop_values = [(k, get_value(k)) for k in props]

        for k, v in prop_values:
            if v is not None:
                value = kwargs.get(k)(v) if k in kwargs else v
                subbed = re.sub(r'[_\.]', ' ', k)
                label = ' '.join([_.capitalize() for _ in subbed.split()])
                print(u'{:22} : {}'.format(label, value))

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

    @property
    def device_id(self):
        return self._device_id


class pr_feature(sysfs_node):
    @property
    def interface_id(self):
        return str(uuid.UUID(self.read_sysfs("interface_id")))


class sysfs_device(sysfs_node):
    @property
    def device_id(self):
        return self.parse_sysfs("device")


class sysfs_resource(sysfs_node):
    @property
    def object_id(self):
        value = self.read_sysfs("dev")
        valueList = value.split(":")
        major = valueList[0]
        minor = valueList[1]
        obj_id = ((long(major) & 0xFFF) << 20) | (long(minor) & 0xFFFFF)
        return obj_id

    def to_dict(self):
        data = super(sysfs_resource, self).to_dict()
        root_data = {}
        root_data["class_path"] = self.sysfs_path
        root_data["dev_path"] = os.path.realpath(self.sysfs_path)
        root_data["bus"] = self.bus
        root_data["device"] = self.device
        root_data["function"] = self.function
        root_data["object_id"] = self.object_id
        data["pcie_info"] = root_data

        return data


class power_mgmt_feature(sysfs_node):
    def __init__(self, path, instance_id, device_id, **kwargs):
        super(power_mgmt_feature, self).__init__(path, instance_id, device_id,
                                                 **kwargs)
        if device_id != DCP_ID:
            self.consumed = add_static_property("consumed")


class thermal_feature(sysfs_node):
    def __init__(self, path, instance_id, device_id, **kwargs):
        super(thermal_feature, self).__init__(path, instance_id, device_id,
                                              **kwargs)
        self.temperature = add_static_property("temperature")
        if device_id != DCP_ID:
            self.threshold1 = add_static_property("threshold1")
            self.threshold1_policy = add_static_property("threshold1_policy")
            self.threshold1_reached = add_static_property("threshold1_reached")
            self.threshold2 = add_static_property("threshold2")
            self.threshold2_reached = add_static_property("threshold2_reached")
            self.threshold_trip = add_static_property("threshold_trip")


class errors_feature(sysfs_node):
    revision = add_static_property("revision")

    def __init__(self, path, instance_id, device_id, **kwargs):
        super(errors_feature, self).__init__(path, instance_id, device_id,
                                             **kwargs)
        self._errors_files = []
        self._name = "errors"

    def name(self):
        return self._name

    def clear(self):
        success = True
        for (err, clr) in self._errors_files:
            value = self.parse_sysfs(err)
            try:
                if value:
                    self.write_sysfs(hex(value), clr)
            except IOError:
                success = False
                logging.warn(
                    "Could not clear errors: {}."
                    "Are you running as root?".format(clr))
        return success


class fme_errors(errors_feature):
    def __init__(self, path, instance_id, device_id, **kwargs):
        super(fme_errors, self).__init__(path, instance_id, device_id,
                                         **kwargs)
        self._name = "fme errors"
        self._errors_files = [("fme-errors/errors", "fme-errors/clear"),
                              ("inject_error", "inject_error"),
                              ("pcie0_errors", "pcie0_errors"),
                              ("pcie1_errors", "pcie1_errors")]
        add_dynamic_property(self, "errors", "fme-errors/errors")
        add_dynamic_property(self, "first_error", "fme-errors/first_error")
        add_dynamic_property(self, "next_error", "fme-errors/next_error")
        add_dynamic_property(self, "pcie0_errors")
        if device_id != DCP_ID:
            add_dynamic_property(self, "pcie1_errors")
            add_dynamic_property(self, "bbs_errors")
            add_dynamic_property(self, "gbs_errors")
        add_dynamic_property(self, "catfatal_errors")
        add_dynamic_property(self, "nonfatal_errors")


class port_errors(errors_feature):
    def __init__(self, path, instance_id, device_id, **kwargs):
        super(port_errors, self).__init__(path, instance_id, device_id,
                                          **kwargs)
        self._name = "port errors"
        self._errors_files = [("errors", "clear")]
        add_dynamic_property(self, "errors")
        add_dynamic_property(self, "first_error")


class fme_info(sysfs_resource):
    def __init__(self, path, instance_id, device_id, **kwargs):
        super(fme_info, self).__init__(path, instance_id, device_id, **kwargs)
        self.pr = pr_feature(os.path.join(path, 'pr'), instance_id, device_id,
                             **kwargs)
        self.power_mgmt = power_mgmt_feature(os.path.join(path, 'power_mgmt'),
                                             instance_id,
                                             device_id,
                                             **kwargs)
        self.thermal_mgmt = thermal_feature(os.path.join(path, "thermal_mgmt"),
                                            instance_id,
                                            device_id,
                                            **kwargs)
        self.errors = fme_errors(os.path.join(path, "errors"),
                                 instance_id,
                                 device_id,
                                 **kwargs)

    @property
    def version(self):
        if self.device_id != DCP_ID:
            return self.read_sysfs("version")

    @property
    def fim_version(self):
        """This function formats bitstream_id majorver, minorver, patchver
           to fim version format
        bitstream_id format contains 0x followed by a number string
        this is not 0 filled so index needs to be right aligned
        >>> obj.fim_version
        '1.1.2'
        """
        bitstr_id = self.read_sysfs("bitstream_id")
        vers = '.'.join([bitstr_id[MAJOR_VER], bitstr_id[MINOR_VER],
                        bitstr_id[PATCH_VER]])
        return vers

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


class port_info(sysfs_resource):
    def __init__(self, path, instance_id, device_id, **kwargs):
        super(port_info, self).__init__(path, instance_id, device_id, **kwargs)
        self.errors = port_errors(os.path.join(path, "errors"),
                                  instance_id,
                                  device_id,
                                  **kwargs)

    @property
    def afu_id(self):
        return str(uuid.UUID(self.read_sysfs("afu_id")))


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
            device_id = sysfs_device(os.path.join(path, 'device'),
                                     instance_id, None,
                                     **bdf).device_id
            sysfs_fme = FME_DEVICE.format(instance_id=instance_id)
            sysfs_port = PORT_DEVICE.format(instance_id=instance_id)
            self._fmelist.append(fme_info(sysfs_fme, instance_id,
                                 device_id, **bdf))
            self._portlist.append(port_info(sysfs_port, instance_id,
                                  device_id, **bdf))

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
