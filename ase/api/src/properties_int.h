// Copyright(c) 2017, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef __FPGA_PROPERTIES_INT_H__
#define __FPGA_PROPERTIES_INT_H__

/** Fields common across all object types */

#define FPGA_PROPERTY_PARENT         0
#define FPGA_PROPERTY_OBJTYPE        1
#define FPGA_PROPERTY_SEGMENT        2
#define FPGA_PROPERTY_BUS            3
#define FPGA_PROPERTY_DEVICE         4
#define FPGA_PROPERTY_FUNCTION       5
#define FPGA_PROPERTY_SOCKETID       6
#define FPGA_PROPERTY_VENDORID       7
#define FPGA_PROPERTY_DEVICEID       8
#define FPGA_PROPERTY_GUID           9
#define FPGA_PROPERTY_OBJECTID       10
#define FPGA_PROPERTY_NUM_ERRORS     11

/** Fields for FPGA objects */
#define FPGA_PROPERTY_NUM_SLOTS     32
#define FPGA_PROPERTY_BBSID         33
#define FPGA_PROPERTY_BBSVERSION    34
#define FPGA_PROPERTY_MODEL         36
#define FPGA_PROPERTY_LOCAL_MEMORY  37
#define FPGA_PROPERTY_CAPABILITIES  38


/** Fields for accelerator objects */
#define FPGA_PROPERTY_ACCELERATOR_STATE 32
#define FPGA_PROPERTY_NUM_MMIO          33
#define FPGA_PROPERTY_NUM_INTERRUPTS    34


#define FIELD_VALID(P, F) (((P)->valid_fields >> (F)) & 1)

#define SET_FIELD_VALID(P, F)\
	((P)->valid_fields = (P)->valid_fields | ((uint64_t)1 << (F)))

#define CLEAR_FIELD_VALID(P, F)\
	((P)->valid_fields = (P)->valid_fields & ~((uint64_t)1 << (F)))

#endif // __FPGA_PROPERTIES_INT_H__

