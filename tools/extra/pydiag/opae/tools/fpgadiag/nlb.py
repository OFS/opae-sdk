# Copyright(c) 2017, Intel Corporation
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
from opae.utils.csr import csr, f_enum


class CFG(csr):
    """cfg"""
    _offset_ = 0x0140
    _bits_ = [
        ("wrthru_en", (0, 0)),
        ("cont", (1, 1)),
        ("mode", (4, 2)),
        ("multiCL_len", (6, 5)),
        ("rdsel", (10, 9)),
        ("rd_chsel", (14, 12)),
        ("wrdin_msb", (15, 15)),
        ("wrpush_i", (16, 16)),
        ("wr_chsel", (19, 17)),
        ("wf_chsel", (31, 30)),
        ("test_cfg", (27, 20)),
        ("interrupt_on_error", (28, 28)),
        ("interrupt_testmode", (29, 29))
    ]

    _enums_ = [
        f_enum(name="wrthru_en", wrline_M=0, wrline_I=1),
        f_enum(name="mode", lbpk1=0, read=1, write=2, trput=3, sw=7),
        f_enum(name="multiCL_len", mcl1=0, mcl2=1, mcl4=3),
        f_enum(name="rdsel", rds=0, rdi=1),
        f_enum(name="rd_chsel", auto=0, vl0=1, vh0=2, vh1=3, random=4),
        f_enum(name="wrdin_msb", alt_wr_prn=1),
        f_enum(name="wrpush_i", wrpush_I=1),
        f_enum(name="wr_chsel", auto=0, vl0=1, vh0=2, vh1=3, random=4),
        f_enum(name="test_cfg", csr_write=64, umsg_data=128, umsg_hint=192),
        f_enum(name="wf_chsel", auto=0, vl0=1, vh0=2, vh1=3)
    ]


class CTL(csr):
    """ctl"""
    _offset_ = 0x0138
    _bits_ = [
        ("reset", (0, 0)),
        ("start", (1, 1)),
        ("stop", (2, 2))
    ]

