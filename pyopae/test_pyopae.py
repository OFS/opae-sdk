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

import uuid
import random
import opae

NLB0 = "d8424dc4-a4a3-c413-f89e-433683f9040b"


def test_guid():
    """test_guid test guid property"""
    props = opae.properties()
    props.guid = NLB0
    guid_str = props.guid
    guid = uuid.UUID(guid_str)
    assert str(guid).lower() == NLB0


def test_enumerate():
    """test_enumerate test if we can enumerate for NLB0 using GUID"""
    props = opae.properties()
    props.guid = NLB0
    toks = opae.token.enumerate([props])
    assert toks


def test_open():
    """test_open test if we can enumerate/open NLB0 using GUID"""
    props = opae.properties()
    props.guid = NLB0
    toks = opae.token.enumerate([props])
    assert toks
    resource = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert resource is not None


def test_mmio():
    """test_mmio sweep through CSRs writing random values and reading from
    them. NOTE: This should NOT be run on real hardware.
    """
    props = opae.properties()
    props.guid = NLB0
    toks = opae.token.enumerate([props])
    assert toks
    resource = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert resource is not None
    dummy_values = dict([(offset, random.randint(0, 1000))
                         for offset in range(24, 4096, 8)])

    for key, value in dummy_values.iteritems():
        resource.write_csr64(key, value)
        assert resource.read_csr64(key) == value
        resource.write_csr64(key, 0)
        assert resource.read_csr64(key) == 0


def test_buffer():
    """test_buffer test shared buffer allocation"""
    props = opae.properties()
    props.guid = NLB0
    toks = opae.token.enumerate([props])
    assert toks
    resource = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert resource is not None
    buf = opae.shared_buffer.allocate(resource, 1024)


def test_close():
    """test_close test if we can enumerate/open and then close an accelerator
    handle"""
    props = opae.properties()
    props.guid = NLB0
    toks = opae.token.enumerate([props])
    assert toks
    resource = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert resource is not None
    result = resource.close()
    assert result == opae.CLOSED

def test_version():
    """test_version test the version API"""
    ver = opae.version()
    assert ver == (0, 13, 1)

def test_register_event():
    """test_event test that we can register for an event"""
    props = opae.properties()
    props.guid = NLB0
    toks = opae.token.enumerate([props])
    assert toks
    resource = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert resource is not None
    event = opae.event.register_event(resource, opae.fpga_event_type.ERROR, 0)
    assert event is not None
    fd = event.os_object()
    assert fd >= 0

if __name__ == "__main__":
    test_mmio()
    test_buffer()
