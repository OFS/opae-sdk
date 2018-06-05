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
import random
import unittest
import uuid

import opae.api

NLB0 = "d8424dc4-a4a3-c413-f89e-433683f9040b"

class TestProperties(unittest.TestCase):
    def test_guid(self):
        props = opae.api.properties.get(guid=NLB0)
        guid_str = props.guid
        guid = uuid.UUID(guid_str)
        assert str(guid).lower() == NLB0

    def test_set_objtype_accelerator(self):
        props = opae.api.properties.get(type=opae.api.FPGA_ACCELERATOR)
        assert props.type == opae.api.FPGA_ACCELERATOR

    def test_set_objtype_device(self):
        props = opae.api.properties.get(type=opae.api.FPGA_DEVICE)
        assert props.type == opae.api.FPGA_DEVICE

    def test_set_bus(self):
        props = opae.api.properties.get(bus=0x5e)
        assert props.bus == 0x5e

    def test_set_device(self):
        props = opae.api.properties.get(device=0xe)
        assert props.device == 0xe

    def test_set_function(self):
        props = opae.api.properties.get(function=0x7)
        assert props.function == 0x7

    def test_set_object_id(self):
        props = opae.api.properties.get(object_id=0xcafe)
        assert props.object_id == 0xcafe

    def test_set_num_slots(self):
        props = opae.api.properties.get(type=opae.api.FPGA_DEVICE,
                                        num_slots=3)
        assert props.num_slots == 3

    def test_set_bbs_id(self):
        props = opae.api.properties.get(type=opae.api.FPGA_DEVICE,
                                        bbs_id=0xc0c0cafe)
        assert props.bbs_id == 0xc0c0cafe

    def test_set_bbs_version(self):
        props = opae.api.properties.get(type=opae.api.FPGA_DEVICE,
                                        bbs_version=(0,1,2))
        assert props.bbs_version == (0,1,2)

    def test_set_vendor_id(self):
        props = opae.api.properties.get(vendor_id=0xfafa)
        assert props.vendor_id == 0xfafa

    @unittest.skip("model not implemented yet")
    def test_set_model(self):
        props = opae.api.properties.get(model="intel skxp")
        assert props.model == "intel skxp"

    @unittest.skip("local_memory_size not implemented yet")
    def test_set_local_memory_size(self):
        props = opae.api.properties.get(local_memory_size=0xffff)
        assert props.local_memory_size == 0xffff

    @unittest.skip("capabilities not implemented yet")
    def test_set_capabilities(self):
        props = opae.api.properties.get(capabilities=0xdeadbeef)
        props.capabilities == 0xdeadbeef

    def test_set_num_mmio(self):
        props = opae.api.properties.get(type=opae.api.FPGA_ACCELERATOR,
                                        num_mmio=4)
        assert props.num_mmio == 4

    def test_set_num_interrupts(self):
        props = opae.api.properties.get(type=opae.api.FPGA_ACCELERATOR,
                                        num_interrupts=9)
        assert props.num_interrupts == 9

    def test_set_accelerator_state(self):
        props = opae.api.properties.get(type=opae.api.FPGA_ACCELERATOR,
                                        accelerator_state=opae.api.FPGA_ACCELERATOR_ASSIGNED)
        assert props.accelerator_state == opae.api.FPGA_ACCELERATOR_ASSIGNED

class TestToken(unittest.TestCase):
    def test_enumerate(self):
        props = opae.api.properties.get(guid=NLB0)
        toks = opae.api.token.enumerate([props])
        assert toks

    def test_token_properties(self):
        props = opae.api.properties.get(guid=NLB0)
        toks = opae.api.token.enumerate([props])
        assert toks
        for tok in toks:
            token_props = opae.api.properties.get(tok)
            assert token_props
            assert token_props.guid and token_props.guid != ""



