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

import opae
import uuid
import random
import time

NLB0 = "d8424dc4-a4a3-c413-f89e-433683f9040b"


def test_guid():
    p = opae.properties()
    p.guid = NLB0
    guid_str = p.guid
    print guid_str
    u = uuid.UUID(guid_str)
    assert str(u).lower() == NLB0


def test_enumerate():
    p = opae.properties()
    p.guid = NLB0
    toks = opae.token.enumerate([p])
    assert len(toks) > 0


def test_open():
    p = opae.properties()
    p.guid = NLB0
    toks = opae.token.enumerate([p])
    assert len(toks) > 0
    h = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert h is not None


def test_mmio():
    p = opae.properties()
    p.guid = NLB0
    toks = opae.token.enumerate([p])
    assert len(toks) > 0
    h = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert h is not None
    dummy_values = dict([(offset, random.randint(0, 1000))
                         for offset in range(24, 4096, 8)])

    for k, v in dummy_values.iteritems():
        value = h.read_csr64(k, 0)
        #assert value == 0
        h.write_csr64(k, v, 0)
        assert h.read_csr64(k, 0) == v
        h.write_csr64(k, 0, 0)
        assert h.read_csr64(k, 0) == 0


def test_buffer():
    p = opae.properties()
    p.guid = NLB0
    toks = opae.token.enumerate([p])
    assert len(toks) > 0
    h = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert h is not None
    b = opae.dma_buffer.allocate(h, 1024);
    print b.buffer()


def test_hellofpga():
    p = opae.properties()
    p.guid = NLB0
    toks = opae.token.enumerate([p])
    assert len(toks) > 0
    h = opae.handle.open(toks[0], opae.OPEN_SHARED)
    assert h is not None
    b = opae.dma_buffer.allocate(h, 1024);

# Should the Python API have a close?
# def test_close():
#     p = opae.properties()
#     p.guid = NLB0
#     toks = opae.token.enumerate([p])
#     assert len(toks) > 0
#     h = opae.handle.open(toks[0], opae.OPEN_SHARED)
#     assert h is not None
#     r = h.close()
#     assert r == opae.CLOSED


if __name__ == "__main__":
    test_mmio()
    test_buffer()
