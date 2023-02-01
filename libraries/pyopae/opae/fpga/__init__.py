# Copyright(c) 2020-2023, Intel Corporation
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
from ._opae import (
    properties,
    token,
    handle,
    shared_buffer,
    event,
    enumerate,
    open,
    allocate_shared_buffer,
    register_event,
    error,
    errors,
    sysobject,
    version,
    build,
    memory_barrier)
from ._opae import (DEVICE, ACCELERATOR, OPEN_SHARED, EVENT_ERROR,
                    EVENT_INTERRUPT, EVENT_POWER_THERMAL, ACCELERATOR_ASSIGNED,
                    ACCELERATOR_UNASSIGNED, RECONF_FORCE, SYSOBJECT_GLOB,
                    IFC_DFL, IFC_VFIO, IFC_SIM_DFL, IFC_SIM_VFIO)
__all__ = ['properties',
           'token',
           'handle',
           'shared_buffer',
           'event',
           'enumerate',
           'open',
           'allocate_shared_buffer',
           'register_event',
           'error',
           'errors',
           'sysobject',
           'version',
           'build',
           'memory_barrier',
           'DEVICE',
           'ACCELERATOR',
           'OPEN_SHARED',
           'EVENT_ERROR',
           'EVENT_INTERRUPT',
           'EVENT_POWER_THERMAL',
           'ACCELERATOR_ASSIGNED',
           'ACCELERATOR_UNASSIGNED',
           'RECONF_FORCE',
           'SYSOBJECT_GLOB',
           'IFC_DFL',
           'IFC_VFIO',
           'IFC_SIM_DFL',
           'IFC_SIM_VFIO'
           ]
