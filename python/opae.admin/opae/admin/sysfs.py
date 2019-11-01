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
from subprocess import CalledProcessError
from opae.admin.utils.process import call_process, DRY_RUN
from opae.admin.utils.log import loggable, LOG


PCI_ADDRESS_PATTERN = (r'(?P<pci_address>'
                       r'(?:(?P<segment>[\da-f]{4}):)?'
                       r'(?P<bdf>(?P<bus>[\da-f]{2}):'
                       r'(?P<device>[\da-f]{2})\.(?P<function>\d)))')
PCI_ADDRESS_RE = re.compile(PCI_ADDRESS_PATTERN, re.IGNORECASE)


class sysfs_node(loggable):
    """sysfs_node is a base class representing a sysfs object in sysfs """

    def __init__(self, sysfs_path):
        """__init__ Initializes a new sysfs_node object with a sysfs path.

        Args:
            sysfs_path: sysfs path to a 'file' or 'directory' object.
        """
        super(sysfs_node, self).__init__()
        self._sysfs_path = sysfs_path

    def node(self, *nodes):
        """node Gets a new sysfs_node object using the given paths.

        Args:
            *nodes: A list of paths to join with the first one being relative
            to the path of this node.

        Returns:
            A sysfs_node object representing the final path derived from
            joining the path of this node and the paths given in *nodes.

        Raises:
            NameError: If the final path does not exist.
        """
        path = os.path.join(self._sysfs_path, *nodes)
        if not os.path.exists(path):
            raise NameError("Could not find sysfs node: {}".format(path))
        return sysfs_node(path)

    def have_node(self, name):
        """have_node Determine if child node exists.

        Args:
            name: Name of child node to look for.

        Returns:
            True if node with given name exists, False otherwise.
        """
        return os.path.exists(os.path.join(self._sysfs_path, name))

    def find(self, pattern):
        """find Find sysfs_node objects using a given glob pattern.

        Args:
            pattern(str): A glob pattern relative to the path of this node.

        Returns:
            A generator object that yields a sysfs_node object found when
            iterated.
        """
        for p in glob.glob(os.path.join(self._sysfs_path, pattern)):
            yield sysfs_node(p)

    def find_all(self, pattern):
        """find_all Find sysfs_node objects using a given glob pattern.

        Args:
            pattern(str): A glob pattern relative to the path of this node.

        Returns:
            A list of all sysfs_node objects using the given pattern.
        """
        return [item for item in self.find(pattern)]

    def find_one(self, pattern):
        """find_one Find one sysfs_node object using the given glob pattern.

        Args:
            pattern(str): A glob pattern relative to the path of this node.

        Returns:
            None if no sysfs_node object are found with the given pattern.
            One sysfs_node object if it finds one or more sysfs_node objects.

        Notes:
            This will return the first object found if more than one is found.
            It will log a warning message if zero or more than one is found.
        """
        items = self.find_all(pattern)
        if not items:
            self.log.debug('could not find: "%s"', pattern)
            return None
        if len(items) > 1:
            self.log.warn('found more than one: "%s"', pattern)
        return items[0]

    @contextmanager
    def _open(self, mode):
        try:
            with open(self._sysfs_path, mode) as fd:
                yield fd
        except IOError as ioerr:
            self.log.warn(ioerr)
            raise

    @property
    def value(self):
        """value Read the value of a sysfs attribute

        Returns: The string returned (trimmed of whitespace) from reading the
                 attribute.

        Raises:
            IOError: If an IOError is caught while attempting to open the path
                     object represented by this node.

        Note:
            Attempting to get a value from a sysfs path that is a directory
            will result in an IOError being raised.
        """
        try:
            with self._open('r') as fd:
                return fd.read().strip()
        except IOError as err:
            self.log.exception('error opening: %s - %s', self.sysfs_path, err)
            raise

    @value.setter
    def value(self, val):
        if DRY_RUN:
            print('echo {} > {}'.format(val, self._sysfs_path))
        else:
            with self._open('w') as fd:
                fd.write(str(val))

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

        Args:
            pci_address(dict): The pci address as a dictionary that contains
                               the following keys:
                               segment, bus, device, function.
            parent(pci_node): Another pci_node object that is the parent of
                              this node in the PCIe tree.
        """
        node_path = os.path.join(self.PCI_BUS_SYSFS,
                                 pci_address['pci_address'])
        super(pci_node, self).__init__(node_path)
        self._pci_address = pci_address
        self._parent = parent
        self._branch = []
        self._children = []
        self._endpoints = []
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

    @staticmethod
    def parse_address(inpstr):
        """parse_address Parse a pci_address from the input string

        Args:
            inpstr: A string to parse
        Returns:
            A dictionary of the pci_address fields (segment, bus, device, etc.)
            If the parsing failed, an empty dictionary is returned.
        Note:
            If the input string does not contain a segment number, this will
            set it to 0000 in the returned dictionary.

        """
        data = {}
        try:
            m = PCI_ADDRESS_RE.match(inpstr)
        except TypeError as err:
            LOG('parse_address').warn(err)
        else:
            if m:
                data = m.groupdict()
                if data['segment'] is None:
                    data['segment'] = '0000'
                    pci_address = '{segment}:{pci_address}'.format(**data)
                    data['pci_address'] = pci_address
        return data

    def tree(self, level=0):
        """tree Gets a tree representation of this node and any child nodes.

        Args:
            level: The level of this node. Default is 0. This is used to pad
            spaces to the left.

        Notes:
            This function is recursively called with any children found in
            this node, incrementing the level every recursive call.
        """
        text = '{}{}\n'.format(' ' * level*4, self)
        for n in self.children:
            text += n.tree(level+1)
        return text

    @property
    def root(self):
        """root Gets the root pci_node object.

        Returns:
            None if this node does not have a parent.
            A pci_node object representing the root node by following the
            parent up the tree until no parent is found.

        Notes:
            This relies on the parent given during initialization. This means
            that it is possible for a root not be found even though the device
            represented by this object is not a root port.
        """
        if self.parent is None:
            return self
        return self.parent.root

    @property
    def branch(self):
        """branch Gets the list of nodes from the root to this node.

           Returns:
               A list of nodes starting from the root port to this node.

           Notes:
               This relies on the parent given during initialization of this
               node and its ancestors.

        """
        if not self._branch:
            node = self
            while node:
                self._branch.insert(0, node)
                node = node.parent
        return self._branch

    @property
    def endpoints(self):
        """endpoints Gets a list of endpoints (or leaf nodes) in a PCIe tree
           rooted at this node.

        Returns:
            A list of endpoints with this node as the root.

        Notes:
            If no children are found, this node is the list of endpoints.
        """
        if not self._endpoints:
            self._endpoints = self._find_endpoints()
        return self._endpoints

    def _find_endpoints(self):
        if not self.children:
            return [self]
        endpoints = []
        for child in self.children:
            endpoints.extend(child._find_endpoints())
        return endpoints

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

        Args:
            value: set the parent (pci_node) to this value
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
        """vendor_id Gets the vendor id of the PCIe device represented by this
                     pci_node object.
            Returns:
                The vendor id of the device as an integer.
        """
        return int(self.node('vendor').value, 16)

    @property
    def device_id(self):
        """device_id Gets the device id of the PCIe device represented by this
                     pci_node object.
            Returns:
                The device id of the device as an integer.
        """
        return int(self.node('device').value, 16)

    @property
    def pci_id(self):
        """pci_id Gets the vendor and device id of the PCIe device
                  represented by this pci_node object.

        Returns:
            A two-element tuple made up of the vendor and device id,
            as integers.
        """
        return (self.vendor_id, self.device_id)

    @property
    def pci_bus(self):
        """pci_bus Gets the pci_bus node for the root port segment and bus.
                   pci bus nodes are rooted at /sys/devices/pci<segment>:<bus>.

        Returns: a sysfs_node representing the pci bus
        """
        path = ('/sys/devices/pci{segment}:{bus}/pci_bus'
                '/{segment}:{bus}').format(segment=self.root.segment,
                                           bus=self.root.bus)
        if os.path.exists(path):
            return sysfs_node(path)
        self.log.warn('pci_bus not found at %s', path)

    def remove(self):
        """remove Perform a hot-remove of the PCIe device represented by
           this pci_node object.
        """
        self.log.debug('removing device at %s', self.pci_address)
        self.node('remove').value = '1'

    def rescan(self):
        """remove Perform a pci rescan of the PCIe device represented by
                  this pci_node object.
        """
        self.log.debug('rescanning device at %s', self.pci_address)
        self.node('rescan').value = '1'

    def rescan_bus(self, bus, power_on=True):
        """rescan_bus Perform a pci rescan of the bus under this device
                      represented by this pci_node object.

        Args:
            bus(str): The bus (including the segment or domain) to rescan.
                      Example: 0000:5e
            power_on(boolean): A boolean flag indicating whether or not power
                               control should be set to 'on'
        """
        if power_on:
            power = self.node('power', 'control')
            if power.value != 'on':
                power.value = 'on'
        self.log.debug('rescanning bus %s under %s', bus, self.pci_address)
        self.node('pci_bus', bus, 'rescan').value = '1'
        self._children = []

    @property
    def aer(self):
        """aer Gets the current AER settings of the device represented by this
               pci_node object.

        Returns:
            A two-element tuple (of integers) of the current AER mask
            values.

        Notes:
            This relies on calling 'setpci' and will log an exception if an
            error was encountered while calling 'setpci'.
        """
        try:
            return (int(call_process(self._aer_cmd1), 16),
                    int(call_process(self._aer_cmd2), 16))
        except CalledProcessError as err:
            self.log.exception('error getting aer: %s', err)

    @aer.setter
    def aer(self, values):
        """aer Sets the AER settings of the device represented by this pci_node
               object.

        Args:
            values(tuple:int): A two-element tuple of integers.

        Notes:
            This relies on calling 'setpci' and will log an exception if an
            error was encountered while calling 'setpci'.
        """
        try:
            call_process('{}={:#08x}'.format(self._aer_cmd1, values[0]))
            call_process('{}={:#08x}'.format(self._aer_cmd2, values[1]))
        except CalledProcessError as err:
            self.log.exception('error setting aer: %s', err)

    @property
    def sriov_totalvfs(self):
        """sriov_totalvfs Get total number of VFs supported"""
        name = 'sriov_totalvfs'
        if self.have_node(name):
            return int(self.node(name).value)

    @property
    def sriov_numvfs(self):
        """sriov_numvfs Get the current number of VFs created"""
        name = 'sriov_numvfs'
        if self.have_node(name):
            return int(self.node(name).value)

    @sriov_numvfs.setter
    def sriov_numvfs(self, value):
        """sriov_numvfs Create a number of VFs as indicated by the argument.

        Args:
            value(int): The number of VFs to create

        Raises:
            ValueError: If the number of VFs to create is greater than the
                        total VFs supported.
            AttributeError: If this pci_node does not support SR-IOV (e.g. VFs)
        """
        if not self.supports_sriov:
            msg = 'do not support SR-IOV'
            self.log.error(msg)
            raise AttributeError(msg)
        if value > self.sriov_totalvfs:
            msg = 'does not support VFs creater than: {}'.format(
                self.sriov_totalvfs)
            self.log.warn(msg)
            raise ValueError(msg)
        self.node('sriov_numvfs').value = value

    @property
    def supports_sriov(self):
        """supports_sriov Determine if pci_node supports SR-IOV

        Returns:
            True if this pci_node supports SR-IOV functions.
        """
        return bool(self.sriov_totalvfs)


class class_node(sysfs_node):
    """class_node A class_node object represents a sysfs object directly
                  under '/sys/class' directory.
    """
    def __init__(self, path):
        """__init__ Initializes a new class_node object found in /sys/class/..

        Args:
            path(str): Path to the sysfs class object represented by this path.

        Notes:
            Linux Kernel pci drivers typically place symbolic links to a sysfs
            path of a pci device in /sys/class/<class name>/*.These links
            typically point to paths found with this pattern:
                '/sys/devices/pci*:*'
            This initialization will resolve these links and parse the resolved
            paths to determine the PCIe branch from the root port to the
            device represented by this class_node object. Once the ancestors
            are determined, the whole PCIe tree topology can be derived from
            the root port.

            This class is designed to be inherited from and contains a lot
            of the boiler-plate code that sub-classes get for free.
        """
        super(class_node, self).__init__(path)
        self._pci_node = self._parse_class_path(path)

    @property
    def pci_node(self):
        """pci_node Gets the pci_node object this class_node points to.


        Returns:
            A pci_node object representing the PCIe device pointed to by the
            symbolic link under /sys/class/<class name>
        """
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
    def enum_class(cls, sysfs_class_name, sysfs_class=None):
        """enum_class Discover class_node objects under a given "class".

        Args:
            sysfs_class_name: The name of the class under /sys/class to look
                              at.
            sysfs_class(class_node): The class_node or deriving class to use
                                     when creating class_node objects.

        Notes:
            If 'sysfs_class' is not specified, it will use this class.
            This is meant so that classes deriving from this can omit this.
        """
        sysfs_class = sysfs_class or cls
        log = LOG(cls.__name__)
        nodes = []
        class_paths = glob.glob(
            os.path.join(
                '/sys/class',
                sysfs_class_name,
                '*'))
        log.debug('found %s objects: %s', sysfs_class_name, class_paths)
        for path in class_paths:
            nodes.append(sysfs_class(path))
        return nodes

    @classmethod
    def enum(cls, filt=[]):
        """enum Discover and return a list of class_node-derived objects given
                a filter.

        Args:
            filt(list): A list of dictionaries that will be used as a filter.
                        If an object matches all items in any one of these
                        dictionaries, it will be added to the list of objects
                        returned.

        Notes:
            The dictionary keys should contain a dot seperated list of
            attributes where the first item is applied to the class discovered
            and the following attributes are applied to the result of the
            preceding result.
            Examples:
                class fpga(class_node):
                    pass

                fpga.enum([
                    { 'pci_node.vendor_id': 0x8086 }
                ])

                This example finds all devices pointed to by symbolic links
                under '/sys/class/fpga/*'

                While finding these nodes, the root port will also be found by
                processing the path under: /sys/devices/pci0000:XX/...

        """
        log = LOG(cls.__name__)

        def func(obj):
            for f in filt:
                for k, v in f.items():
                    cur_obj = obj
                    attrs = k.split('.')
                    while len(attrs) > 1:
                        try:
                            attr = attrs.pop(0)
                            cur_obj = getattr(cur_obj, attr)
                        except AttributeError:
                            log.debug('"%s" is not an attribute of "%s"',
                                      attr, obj)
                            attrs = []
                            break
                    if attrs and hasattr(cur_obj, attrs[0]):
                        attr_value = getattr(cur_obj, attrs[0])
                        if isinstance(attr_value, str):
                            attr_value = attr_value.lower()
                            v = v.lower()
                        if attr_value != v:
                            return False
            return True
        return list(filter(func, cls.enum_class(cls.__name__, cls)))
