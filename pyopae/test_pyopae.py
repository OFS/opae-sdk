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
import select
import subprocess
import threading
import time
import unittest
import uuid

import opae.fpga

NLB0 = "d8424dc4-a4a3-c413-f89e-433683f9040b"

MOCK_PORT_ERROR = "/tmp/class/fpga/intel-fpga-dev.0/intel-fpga-port.0/errors/errors"

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
                                        bbs_version=(0,1,2))
        assert props.bbs_version == (0,1,2)
        props.bbs_version = (1,2,3)
        assert props.bbs_version == (1,2,3)

    def test_set_vendor_id(self):
        props = opae.fpga.properties(vendor_id=0xfafa)
        assert props.vendor_id == 0xfafa
        props.vendor_id = 0xdada
        assert props.vendor_id == 0xdada

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
        props = opae.fpga.properties(type=opae.fpga.ACCELERATOR,
                                        accelerator_state=opae.fpga.ACCELERATOR_ASSIGNED)
        assert props.accelerator_state == opae.fpga.ACCELERATOR_ASSIGNED
        props.accelerator_state = opae.fpga.ACCELERATOR_UNASSIGNED
        assert props.accelerator_state == opae.fpga.ACCELERATOR_UNASSIGNED


class TestToken(unittest.TestCase):
    def test_enumerate(self):
        props = opae.fpga.properties(guid=NLB0)
        toks = opae.fpga.enumerate([props])
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
        assert buff1.iova() != 0
        mv = memoryview(buff1)
        assert mv
        assert not buff1.compare(buff2, 4096)
        buff1.fill(0xAA)
        buff2.fill(0xEE)
        assert buff1.compare(buff2, 4096)
        assert mv[0] == '\xaa'
        assert mv[-1] == '\xaa'
        ba = bytearray(buff1)
        assert ba[0] == 0xaa

def trigger_port_error(value=1):
    with open(MOCK_PORT_ERROR, 'w') as fd:
        fd.write('0\n')
    if value:
        print "Writing {} to {}".format(value, MOCK_PORT_ERROR)
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
        assert type(os_object) is int
        assert os_object > -1
        print "os_object: {}".format(os_object)
        epoll = select.epoll()
        epoll.register(os_object, select.EPOLLIN)
        received_event = False
        count = 0
        trigger_error_timer = threading.Timer(1, trigger_port_error)
        trigger_error_timer.start()
        for _ in range(10):
            for fileno, ev in epoll.poll(1):
                print "fileno: {}".format(fileno)
                if fileno == os_object:
                    received_event = True
                    break
            if received_event:
                break
            time.sleep(1)

        trigger_error_timer.cancel()
        assert received_event



if __name__ == "__main__":
    test = TestEvent('test_events')
    test.run()

