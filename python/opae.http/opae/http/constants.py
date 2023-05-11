#!/usr/bin/env python3
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

# fpga_result
FPGA_OK = 0
FPGA_INVALID_PARAM = 1
FPGA_BUSY = 2
FPGA_EXCEPTION = 3
FPGA_NOT_FOUND = 4
FPGA_NO_MEMORY = 5
FPGA_NOT_SUPPORTED = 6
FPGA_NO_DRIVER = 7
FPGA_NO_DAEMON = 8
FPGA_NO_ACCESS = 9
FPGA_RECONF_ERROR = 10

def fpga_result_to_str(r):
    d = {FPGA_OK: 'FPGA_OK',
         FPGA_INVALID_PARAM: 'FPGA_INVALID_PARAM',
         FPGA_BUSY: 'FPGA_BUSY',
         FPGA_EXCEPTION: 'FPGA_EXCEPTION',
         FPGA_NOT_FOUND: 'FPGA_NOT_FOUND',
         FPGA_NO_MEMORY: 'FPGA_NO_MEMORY',
         FPGA_NOT_SUPPORTED: 'FPGA_NOT_SUPPORTED',
         FPGA_NO_DRIVER: 'FPGA_NO_DRIVER',
         FPGA_NO_DAEMON: 'FPGA_NO_DAEMON',
         FPGA_NO_ACCESS: 'FPGA_NO_ACCESS',
         FPGA_RECONF_ERROR: 'FPGA_RECONF_ERROR'}
    return d[r]

def fpga_result_from_str(s):
    d = {'FPGA_OK': FPGA_OK,
         'FPGA_INVALID_PARAM': FPGA_INVALID_PARAM,
         'FPGA_BUSY': FPGA_BUSY,
         'FPGA_EXCEPTION': FPGA_EXCEPTION,
         'FPGA_NOT_FOUND': FPGA_NOT_FOUND,
         'FPGA_NO_MEMORY': FPGA_NO_MEMORY,
         'FPGA_NOT_SUPPORTED': FPGA_NOT_SUPPORTED,
         'FPGA_NO_DRIVER': FPGA_NO_DRIVER,
         'FPGA_NO_DAEMON': FPGA_NO_DAEMON,
         'FPGA_NO_ACCESS': FPGA_NO_ACCESS,
         'FPGA_RECONF_ERROR': FPGA_RECONF_ERROR}
    return d[s]

def raise_for_error(result, msg):
    res = fpga_result_from_str(result) if type(result) is str else result
    s = fpga_result_to_str(result) if type(result) is int else result
    if res != FPGA_OK:
        raise RuntimeError(f'{msg} {s}')

# fpga_objtype
FPGA_DEVICE = 0
FPGA_ACCELERATOR = 1

def fpga_objtype_to_str(t):
    d = {FPGA_DEVICE: 'FPGA_DEVICE',
         FPGA_ACCELERATOR: 'FPGA_ACCELERATOR'}
    return d[t] 

def fpga_objtype_from_str(s):
    d = {'FPGA_DEVICE': FPGA_DEVICE,
         'FPGA_ACCELERATOR': FPGA_ACCELERATOR}
    return d[s]

# fpga_interface
FPGA_IFC_DFL = 0
FPGA_IFC_VFIO = 1
FPGA_IFC_SIM_DFL = 2
FPGA_IFC_SIM_VFIO = 3

def fpga_interface_to_str(i):
    d = {FPGA_IFC_DFL: 'FPGA_IFC_DFL',
         FPGA_IFC_VFIO: 'FPGA_IFC_VFIO',
         FPGA_IFC_SIM_DFL: 'FPGA_IFC_SIM_DFL',
         FPGA_IFC_SIM_VFIO: 'FPGA_IFC_SIM_VFIO'}
    return d[i] 

def fpga_interface_from_str(s):
    d = {'FPGA_IFC_DFL': FPGA_IFC_DFL,
         'FPGA_IFC_VFIO': FPGA_IFC_VFIO,
         'FPGA_IFC_SIM_DFL': FPGA_IFC_SIM_DFL,
         'FPGA_IFC_SIM_VFIO': FPGA_IFC_SIM_VFIO}
    return d[s]

# fpga_accelerator_state
FPGA_ACCELERATOR_ASSIGNED = 0
FPGA_ACCELERATOR_UNASSIGNED = 1

def fpga_accelerator_state_to_str(st):
    d = {FPGA_ACCELERATOR_ASSIGNED: 'FPGA_ACCELERATOR_ASSIGNED',
         FPGA_ACCELERATOR_UNASSIGNED: 'FPGA_ACCELERATOR_UNASSIGNED'}
    return d[st] 

def fpga_accelerator_state_from_str(s):
    d = {'FPGA_ACCELERATOR_ASSIGNED': FPGA_ACCELERATOR_ASSIGNED,
         'FPGA_ACCELERATOR_UNASSIGNED': FPGA_ACCELERATOR_UNASSIGNED}
    return d[s]

# fpga_open_flags
FPGA_OPEN_SHARED = 1

# fpga_buffer_flags
FPGA_BUF_PREALLOCATED = 1
FPGA_BUF_QUIET = 2
FPGA_BUF_READ_ONLY = 4
