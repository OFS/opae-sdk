# Copyright(c) 2023, Intel Corporation
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

"""Provides a means to enumerate FPGA devices by examining their
feature ID."""

from opae import fpga
from opae.fpga import dfh
import opae.fpga.pcie.address as addr


def enumerate(access_width: int, region: int, **kwargs):
    """Enumerate FPGA regions based on the given properties.
       kwargs contains the standard FPGA properties used for
       enumeration, and supports one additional property,
       'feature_id', that when given acts as a filter for
       selecting tokens by the feature_id field of the DFH.
    """
    feature_id = None
    if 'feature_id' in kwargs:
        feature_id = kwargs.get('feature_id')
        del kwargs['feature_id']

    tokens = fpga.enumerate(**kwargs)

    access = addr.memory_access(access_width)

    if feature_id is None:
        return tokens

    result = []
    while tokens:
        tok = tokens.pop()
        remove = True
        try:
            with fpga.open(tok, fpga.OPEN_SHARED) as hndl:
                access.hndl = hndl
                d = dfh.dfh0(access.read(0, dfh.dfh0.width, region))
                if d.bits.id == int(feature_id):
                    result.append(tok)
                    remove = False
        except RuntimeError:
            pass
        if remove:
            del tok

    return result
