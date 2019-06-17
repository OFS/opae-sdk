# Copyright(c) 2019, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
#   may be used to  endorse or promote  products derived  from this  software
#   without specific prior written permission.
#
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
# POSSIBILITY OF SUCH DAMAGE.a
import glob
import os
import re
from contextlib import contextmanager
from opae.utils.process import call_process, DRY_RUN
from opae.utils.log import loggable, LOG


PCI_ADDRESS_PATTERN = (r'(?P<pci_address>'
                       r'(?P<segment>[\da-f]{4}):(?P<bdf>(?P<bus>[\da-f]{2}):'
                       r'(?P<device>[\da-f]{2})\.(?P<function>\d)))')
PCI_ADDRESS_RE = re.compile(PCI_ADDRESS_PATTERN, re.IGNORECASE)


class sysfs_node(loggable):
    """sysfs_node is a base class representing a sysfs object in sysfs """

    def __init__(self, sysfs_path):
        super(sysfs_node, self).__init__()
        self._sysfs_path = sysfs_path

    def node(self, *nodes):
        path = os.path.join(self._sysfs_path, *nodes)
        if not os.path.exists(path):
            raise NameError("Could not find sysfs node: {}".format(path))
        return sysfs_node(path)

    def find(self, pattern):
        for p in glob.glob(os.path.join(self._sysfs_path, pattern)):
            yield sysfs_node(p)

    @contextmanager
    def _open(self, mode):
        try:
            with open(self._sysfs_path, mode) as fd:
                yield fd
        except IOError as ioerr:
            self.log.exception(ioerr)
            raise

    @property
    def value(self):
        with self._open('r') as fd:
            return fd.read().strip()

    @value.setter
    def value(self, val):
        if DRY_RUN:
            print('echo {} > {}'.format(val, self._sysfs_path))
        else:
            with self._open('w') as fd:
                fd.write(val)

    @property
    def sysfs_path(self):
        return self._sysfs_path


class pci_node(sysfs_node):
    PCI_BUS_SYSFS = '/sys/bus/pci/devices'
    """pci_node is a class used to encapsulate a node on the pci bus
       that can be found in /sys/bus/pci/devices and can have a parent
       node and one or more children nodes
       This can be used to represent a PCIe tree or subtree"""

    def __init__(self, pci_address, parent=None, **kwargs):
        """__init__ initialize a pci_node object

        :param pci_address: The pci address of the node using following format
                            [segment:]bus:device.function
        :param parent(pci_node): Another pci_node object that is the parent of
                                 this node in the PCIe tree
        """
        node_path = os.path.join(self.PCI_BUS_SYSFS,
                                 pci_address['pci_address'])
        super(pci_node, self).__init__(node_path)
        self._pci_address = pci_address
        self._parent = parent
        self._children = []
        self._aer_cmd1 = 'setpci -s {} ECAP_AER+0x08.L'.format(
            pci_address['pci_address'])
        self._aer_cmd2 = 'setpci -s {} ECAP_AER+0x14.L'.format(
            pci_address['pci_address'])

    def __str__(self):
        return '[pci_address({}), pci_id(0x{:04x}, 0x{:04x})]'.format(
            self.pci_address, *self.pci_id)

    def __repr__(self):
        return str(self)

    def _find_children(self):
        children = []
        for f in os.listdir(self.sysfs_path):
            if not f.startswith(self.pci_address):
                m = PCI_ADDRESS_RE.match(f)
                if m:
                    children.append(pci_node(m.groupdict(), self))
        return children

    def tree(self, level=0):
        text = '{}{}\n'.format(' ' * level*4, self)
        for n in self.children:
            text += n.tree(level+1)
        return text

    @property
    def root(self):
        if self.parent is None:
            return self
        return self.parent.root

    @property
    def pci_address(self):
        """pci_address get the pci address of the node"""
        return self._pci_address['pci_address']

    @property
    def bdf(self):
        """bdf get the bus, device, function of the node"""
        return self._pci_address['bdf']

    @property
    def segment(self):
        """segment get the pci segment or domain of the node"""
        return self._pci_address['segment']

    @property
    def domain(self):
        """segment get the pci segment or domain of the node"""
        return self._pci_address['segment']

    @property
    def bus(self):
        """bus get the pci bus of the node"""
        return self._pci_address['bus']

    @property
    def device(self):
        """device get the pci device of the node"""
        return self._pci_address['device']

    @property
    def function(self):
        """function get the pci function of the node"""
        return self._pci_address['function']

    @property
    def parent(self):
        """parent get the parent (pci_node) of this node"""
        return self._parent

    @parent.setter
    def parent(self, value):
        """parent set the parent (pci_node) of this node

        :param value: set the parent (pci_node) to this value
        """
        self._parent = value

    @property
    def children(self):
        """children get the immediate children or the pci_node object"""
        if not self._children:
            self._children = self._find_children()
        return self._children

    @property
    def all_children(self):
        """all_children get all nodes under the subtree rooted at pci_node"""
        nodes = self.children
        for n in nodes:
            nodes.extend(n.all_children)
        return list(set(nodes))

    @property
    def vendor_id(self):
        return self.node('vendor').value

    @property
    def device_id(self):
        return self.node('device').value

    @property
    def pci_id(self):
        return (int(self.vendor_id, 16),
                int(self.device_id, 16))

    def remove(self):
        self.log.debug('removing device at %s', self.pci_address)
        self.node('remove').value = '1'

    def rescan(self):
        self.log.debug('rescanning device at %s', self.pci_address)
        self.node('rescan').value = '1'

    def rescan_bus(self, bus, power_on=True):
        if power_on:
            power = self.node('power', 'control')
            if power.value != 'on':
                power.value = 'on'
        self.log.debug('rescanning bus %s under %s', bus, self.pci_address)
        self.node('pci_bus', bus, 'rescan').value = '1'
        self._children = []

    @property
    def aer(self):
        return (int(call_process(self._aer_cmd1), 16),
                int(call_process(self._aer_cmd2), 16))

    @aer.setter
    def aer(self, values):
        call_process('{}={:#08x}'.format(self._aer_cmd1, values[0]))
        call_process('{}={:#08x}'.format(self._aer_cmd2, values[1]))


class class_node(sysfs_node):
    def __init__(self, path):
        super(class_node, self).__init__(self)
        self._pci_node = self._parse_class_path(path)

    @property
    def pci_node(self):
        return self._pci_node

    def _parse_class_path(self, sysfs_class_path):
        # The class paths are links to device paths typcially under pci bus.
        # example: /sys/devices/pci<segment>:<bus>/(PCI_ADDRESS_PATTERN)+
        # Read the link and parse it using the PCI_ADDRESS_PATTERN regex
        link = os.readlink(sysfs_class_path)
        match_iter = PCI_ADDRESS_RE.finditer(link)
        # After parsing it, build the path in the PCIe tree as represented
        # in the sysfs tree.
        # The first match is the root of this path and has no parent.
        # Iterate over all matches in the symlink creating a new node
        # for each match, setting the parent to the previous node.
        node = None
        path = []
        for match in match_iter:
            node = pci_node(match.groupdict(), parent=node)
            path.append(node)
        # the last node is the fpga node
        LOG('enum_class').debug('found device at %s -tree is\n %s',
                                node.pci_address,
                                path[0].tree())
        return node

    @classmethod
    def enum(cls, sysfs_class_name, sysfs_class=None):
        sysfs_class = sysfs_class or cls
        log = LOG(cls.__name__)
        log.debug(sysfs_class_name)
        nodes = []
        class_paths = glob.glob(
            os.path.join(
                '/sys/class',
                sysfs_class_name,
                '*'))
        log.debug('glob returned: %s', class_paths)
        for path in class_paths:
            nodes.append(sysfs_class(path))
        return nodes
