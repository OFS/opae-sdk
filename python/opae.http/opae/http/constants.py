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

