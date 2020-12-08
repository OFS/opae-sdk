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
//****************************************************************************
/// @file mm_debug_link_interface.h
/// @brief Basic AFU interaction.
/// @ingroup SigTap
/// @verbatim
//****************************************************************************
#ifndef MM_DEBUG_LINK_INTERFACE_H
#define MM_DEBUG_LINK_INTERFACE_H

#include <unistd.h>


class mm_debug_link_interface
{
public:
	virtual int open(unsigned char* stpAddr) = 0;
	virtual ssize_t read() = 0;
	virtual ssize_t write(const void *buf, size_t count) = 0;
	virtual void close(void) = 0;
	virtual void ident(int id[4]) = 0;
	virtual void write_ident(int val) = 0;
	virtual void reset(bool val) = 0;
	virtual void enable(int channel, bool state) = 0;
	virtual int get_fd(void) = 0;
	virtual bool can_read_data() = 0;
	virtual size_t buf_end(void) = 0;
	virtual void buf_end(int index) = 0;
	virtual char *buf(void) = 0;
	virtual bool is_empty(void) = 0;
	virtual bool flush_request(void) = 0;
};

// Concrete classes must implement this routine.
mm_debug_link_interface *get_mm_debug_link(void);

#define UNUSED_PARAM(x) (void)x

#endif

