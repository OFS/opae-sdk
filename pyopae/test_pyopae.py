# Copyright(c) 2018, Intel Corporation
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
# POSSIBILITY OF SUCH DAMAGE.
from __future__ import absolute_import
import json
import select
import struct
import subprocess
import threading
import time
import unittest
import uuid
import sys
import opae.fpga

NLB0 = "d8424dc4-a4a3-c413-f89e-433683f9040b"

MOCK_PORT_ERROR = "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-port.0/errors/errors"

NLB0_MDATA = {"version": 640,
              "afu-image": {"clock-frequency-high": 312,
                            "clock-frequency-low": 156,
                            "power": 50,
                            "interface-uuid": "1a422218-6dba-448e-b302-425cbcde1406",
                            "magic-no": 488605312,
                            "accelerator-clusters": [{"total-contexts": 1,
                                                      "name": "nlb_400",
                                                      "accelerator-type-uuid": "d8424dc4-a4a3-c413-f89e-433683f9040b"}]},
              "platform-name": "MCP"}

class TestProperties(unittest.TestCase):
    def test_set_parent(self):
        props = opae.fpga.properties(type=opae.fpga.DEVICE)
        toks = opae.fpga.enumerate([props])
        props2 = opae.fpga.properties(type=opae.fpga.ACCELERATOR,
                                      parent=toks[0])
        assert props2.parent
        props2 = opae.fpga.properties(type=opae.fpga.ACCELERATOR)
        props2.parent = toks[0]
        assert props2.parent

    def test_guid(self):
        props = opae.fpga.properties(guid=NLB0)
        guid_str = props.guid
        guid = uuid.UUID(guid_str)
        assert str(guid).lower() == NLB0
        props = opae.fpga.properties()
        props.guid = NLB0
        guid_str = props.guid
        guid = uuid.UUID(guid_str)
        assert str(guid).lower() == NLB0

    def test_set_objtype_accelerator(self):
        props = opae.fpga.properties(type=opae.fpga.ACCELERATOR)
        assert props.type == opae.fpga.ACCELERATOR
        props = opae.fpga.properties(type=opae.fpga.DEVICE)
        props.type = opae.fpga.ACCELERATOR
        assert props.type == opae.fpga.ACCELERATOR

    def test_set_objtype_device(self):
        props = opae.fpga.properties(type=opae.fpga.DEVICE)
        assert props.type == opae.fpga.DEVICE
        props = opae.fpga.properties(type=opae.fpga.ACCELERATOR)
        props.type = opae.fpga.DEVICE
        assert props.type == opae.fpga.DEVICE

    def test_set_segment(self):
        props = opae.fpga.properties(segment=0x9090)
        assert props.segment == 0x9090
        props.segment = 0xA1A1
        assert props.segment == 0xA1A1

    def test_set_bus(self):
        props = opae.fpga.properties(bus=0x5e)
        assert props.bus == 0x5e
        props.bus = 0xbe
        assert props.bus == 0xbe

    def test_set_device(self):
        props = opae.fpga.properties(device=0xe)
        assert props.device == 0xe
        props.device = 0xf
        assert props.device == 0xf

    def test_set_function(self):
        props = opae.fpga.properties(function=0x7)
        assert props.function == 0x7
        props.function = 0x6
        assert props.function == 0x6

    def test_set_socket_id(self):
        props = opae.fpga.properties(socket_id=1)
        assert props.socket_id == 1
        props.socket_id = 0
        assert props.socket_id == 0

    def test_set_object_id(self):
        props = opae.fpga.properties(object_id=0xcafe)
        assert props.object_id == 0xcafe
        props.object_id = 0xfade
        assert props.object_id == 0xfade

    def test_set_num_errors(self):
        props = opae.fpga.properties(num_errors=8)
        assert props.num_errors == 8
        props.num_errors = 4
        assert props.num_errors == 4

    def test_set_num_slots(self):
        props = opae.fpga.properties(type=opae.fpga.DEVICE,
                                     num_slots=3)
        assert props.num_slots == 3
        props.num_slots = 2
        assert props.num_slots == 2

    def test_set_bbs_id(self):
        props = opae.fpga.properties(type=opae.fpga.DEVICE,
                                     bbs_id=0xc0c0cafe)
        assert props.bbs_id == 0xc0c0cafe
        props.bbs_id = 0xb0b0fade
        assert props.bbs_id == 0xb0b0fade

    def test_set_bbs_version(self):
        props = opae.fpga.properties(type=opae.fpga.DEVICE,
                                     bbs_version=(0, 1, 2))
        assert props.bbs_version == (0, 1, 2)
        props.bbs_version = (1, 2, 3)
        assert props.bbs_version == (1, 2, 3)

    def test_set_vendor_id(self):
        props = opae.fpga.properties(vendor_id=0xfafa)
        assert props.vendor_id == 0xfafa
        props.vendor_id = 0xdada
        assert props.vendor_id == 0xdada

    def test_set_device_id(self):
        props = opae.fpga.properties(device_id=0xfa)
        assert props.device_id == 0xfa
        props.device_id = 0xda
        assert props.device_id == 0xda

    @unittest.skip("model not implemented yet")
    def test_set_model(self):
        props = opae.fpga.properties(model="intel skxp")
        assert props.model == "intel skxp"
        props.model = "intel skxp 2"
        assert props.model == "intel skxp 2"

    @unittest.skip("local_memory_size not implemented yet")
    def test_set_local_memory_size(self):
        props = opae.fpga.properties(local_memory_size=0xffff)
        assert props.local_memory_size == 0xffff
        props.local_memory_size = 0xaaaa
        assert props.local_memory_size == 0xaaaa

    @unittest.skip("capabilities not implemented yet")
    def test_set_capabilities(self):
        props = opae.fpga.properties(capabilities=0xdeadbeef)
        assert props.capabilities == 0xdeadbeef
        props.capabilities = 0xfeebdaed
        assert props.capabilities == 0xfeebdaed

    def test_set_num_mmio(self):
        props = opae.fpga.properties(type=opae.fpga.ACCELERATOR,
                                     num_mmio=4)
        assert props.num_mmio == 4
        props.num_mmio = 5
        assert props.num_mmio == 5

    def test_set_num_interrupts(self):
        props = opae.fpga.properties(type=opae.fpga.ACCELERATOR,
                                     num_interrupts=9)
        assert props.num_interrupts == 9
        props.num_interrupts = 8
        assert props.num_interrupts == 8

    def test_set_accelerator_state(self):
        props = opae.fpga.properties(
            type=opae.fpga.ACCELERATOR,
            accelerator_state=opae.fpga.ACCELERATOR_ASSIGNED)
        assert props.accelerator_state == opae.fpga.ACCELERATOR_ASSIGNED
        props.accelerator_state = opae.fpga.ACCELERATOR_UNASSIGNED
        assert props.accelerator_state == opae.fpga.ACCELERATOR_UNASSIGNED


class TestToken(unittest.TestCase):
    def test_enumerate(self):
        props = opae.fpga.properties(guid=NLB0)
        toks = opae.fpga.enumerate([props])
        assert toks

    def test_enumerate_kwargs(self):
        toks = opae.fpga.enumerate(guid=NLB0)
        assert toks

    def test_enumerate_empty_kwargs(self):
        toks = opae.fpga.enumerate()
        assert toks

    def test_token_properties(self):
        props = opae.fpga.properties(guid=NLB0)
        toks = opae.fpga.enumerate([props])
        assert toks
        for tok in toks:
            token_props = opae.fpga.properties(tok)
            assert token_props
            assert token_props.guid and token_props.guid != ""


class TestHandle(unittest.TestCase):
    def setUp(self):
        self.props = opae.fpga.properties(type=opae.fpga.ACCELERATOR)
        self.toks = opae.fpga.enumerate([self.props])
        assert self.toks
        self.handle = opae.fpga.open(self.toks[0])
        assert self.handle
        with open('m0.gbs', 'w+b') as fd:
            if sys.version_info[0] == 3:
                fd.write(bytes("XeonFPGA\b7GBSv001\53\02\00\00", 'UTF-8'))
                fd.write(bytes(json.dumps(NLB0_MDATA), 'UTF-8'))
            else:
                fd.write("XeonFPGA\b7GBSv001\53\02\00\00")
                fd.write(json.dumps(NLB0_MDATA))


    def test_open_null_token(self):
        with self.assertRaises(ValueError):
            hndl = opae.fpga.open(None)

    def test_open(self):
        assert self.handle

    def test_reconfigure(self):
        with open('m0.gbs', 'r') as fd:
            self.handle.reconfigure(0, fd)

    def test_close(self):
        self.handle.close()
        assert not self.handle

    def test_context(self):
        self.handle.close()
        assert not self.handle
        with opae.fpga.open(self.toks[0]) as h:
            assert h
        assert not h

    def test_reset(self):
        self.handle.reset()

    def test_close_reset(self):
        self.handle.close()
        assert not self.handle
        with self.assertRaises(RuntimeError):
            self.handle.reset()

    def test_mmio(self):
        offset = 0x100
        write_value = 10
        self.handle.write_csr32(offset, write_value)
        read_value = self.handle.read_csr32(offset)
        assert read_value == write_value
        self.handle.write_csr64(offset, write_value)
        read_value = self.handle.read_csr64(offset)
        assert read_value == write_value
        write_value = 0
        self.handle.write_csr32(offset, write_value)
        read_value = self.handle.read_csr32(offset)
        assert read_value == write_value
        self.handle.write_csr64(offset, write_value)
        read_value = self.handle.read_csr64(offset)
        assert read_value == write_value

    def test_close_mmio(self):
        self.handle.close()
        assert not self.handle
        with self.assertRaises(RuntimeError):
            self.handle.write_csr32(0x100, 0xbeef)

        with self.assertRaises(RuntimeError):
            self.handle.write_csr64(0x100, 0xbeef)

        with self.assertRaises(RuntimeError):
            self.handle.read_csr32(0x100)

        with self.assertRaises(RuntimeError):
            self.handle.read_csr64(0x100)


class TestSharedBuffer(unittest.TestCase):
    def setUp(self):
        self.props = opae.fpga.properties(type=opae.fpga.ACCELERATOR)
        self.toks = opae.fpga.enumerate([self.props])
        assert self.toks
        self.handle = opae.fpga.open(self.toks[0])
        assert self.handle

    def test_allocate(self):
        buff1 = opae.fpga.allocate_shared_buffer(self.handle, 4096)
        buff2 = opae.fpga.allocate_shared_buffer(self.handle, 4096)
        assert buff1
        assert buff2
        assert buff1.size() == 4096
        assert buff1.wsid() != 0
        assert buff1.io_address() != 0
        mv = memoryview(buff1)
        assert mv
        assert not buff1.compare(buff2, 4096)
        buff1.fill(0xAA)
        buff2.fill(0xEE)
        assert buff1.compare(buff2, 4096)
        if sys.version_info[0] == 2:
            assert mv[0] == '\xaa'
            assert mv[-1] == '\xaa'
        else:
            assert mv[0] == 0xaa
            assert mv[-1] == 0xaa
        ba = bytearray(buff1)
        assert ba[0] == 0xaa
        buff1[42] = int(65536)
        assert struct.unpack('<L', (bytearray(buff1[42:46])))[0] == 65536

    def test_conext_release(self):
        assert self.handle
        self.handle.close()
        with opae.fpga.open(self.toks[0]) as h:
            buff = opae.fpga.allocate_shared_buffer(h, 4096)
            assert buff
            assert buff.size() == 4096
            assert buff.wsid() != 0
        assert not h
        assert buff.size() == 0
        assert buff.wsid() == 0

def trigger_port_error(value=1):
    with open(MOCK_PORT_ERROR, 'w') as fd:
        fd.write('0\n')
    if value:
        with open(MOCK_PORT_ERROR, 'w') as fd:
            fd.write('{}\n'.format(value))

class TestEvent(unittest.TestCase):
    def setUp(self):
        trigger_port_error(0)
        self.props = opae.fpga.properties(type=opae.fpga.ACCELERATOR,
                                          socket_id=0)
        self.toks = opae.fpga.enumerate([self.props])
        assert self.toks
        self.handle = opae.fpga.open(self.toks[0])
        assert self.handle


    def tearDown(self):
        trigger_port_error(0)

    def test_events(self):
        err_ev = opae.fpga.register_event(self.handle,
                                          opae.fpga.EVENT_ERROR)
        assert err_ev
        # FIXME: os_object returns long
        # WA is to cast it to int
        os_object = int(err_ev.os_object())
        assert isinstance(os_object, int)
        assert os_object > -1
        epoll = select.epoll()
        epoll.register(os_object, select.EPOLLIN)
        received_event = False
        count = 0
        trigger_error_timer = threading.Timer(1, trigger_port_error)
        trigger_error_timer.start()
        for _ in range(10):
            for fileno, ev in epoll.poll(1):
                if fileno == os_object:
                    received_event = True
                    break
            if received_event:
                break
            time.sleep(1)

        trigger_error_timer.cancel()
        # temporarily disalbe this assertion
        #assert received_event


class TestError(unittest.TestCase):
    def setUp(self):
        self.port_errors = {"errors": {"can_clear": True},
                            "first_error": {"can_clear": False},
                            "first_malformed_req": {"can_clear": False}}
        self.fme_errors = {"pcie0_errors": {"can_clear": True},
                           "warning_errors": {"can_clear": True},
                           "pcie1_errors": {"can_clear": True},
                           "gbs_errors": {"can_clear": False},
                           "bbs_errors": {"can_clear": False},
                           "next_error": {"can_clear": False},
                           "errors": {"can_clear": False},
                           "first_error": {"can_clear": False},
                           "inject_error": {"can_clear": True}}
        props = opae.fpga.properties(type=opae.fpga.ACCELERATOR)
        toks = opae.fpga.enumerate([props])
        assert toks
        self.acc_token = toks[0]
        props.type = opae.fpga.DEVICE
        toks = opae.fpga.enumerate([props])
        assert toks
        self.dev_token = toks[0]


    def test_port_errors(self):
        for err in opae.fpga.errors(self.acc_token):
            assert self.port_errors[err.name]["can_clear"] == err.can_clear
            self.port_errors.pop(err.name)
        assert not self.port_errors

    def test_fme_errors(self):
        for err in opae.fpga.errors(self.dev_token):
            assert self.fme_errors[err.name]["can_clear"] == err.can_clear
            self.fme_errors.pop(err.name)
        assert not self.fme_errors

if __name__ == "__main__":
    test = TestEvent('test_events')
    test.run()

