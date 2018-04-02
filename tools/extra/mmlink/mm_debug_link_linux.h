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
/// @file  mm_debug_link_linux.h
/// @brief Basic AFU interaction.
/// @ingroup SigTap
/// @verbatim
//****************************************************************************

#ifndef MM_DEBUG_LINK_LINUX_H
#define MM_DEBUG_LINK_LINUX_H

#include <cstdlib>
#include <cstdint>

#include <unistd.h>
#include "time.h"

#include "mm_debug_link_interface.h"

// size of buffer for t2h data
#define BUFFERSIZE_T2H 1073741824

class mm_debug_link_linux: public mm_debug_link_interface
{
private:
	int m_fd;
	static const size_t BUFSIZE = BUFFERSIZE_T2H;
	char m_buf[BUFSIZE];
	volatile size_t m_buf_end;
	int m_write_fifo_capacity;
	volatile unsigned char* map_base;
	bool m_write_before_any_read_rfifo_level;
	clock_t m_last_read_rfifo_level_empty_time;
	clock_t m_read_rfifo_level_empty_interval;

public:
	mm_debug_link_linux();
	int open(unsigned char* stpAddr);

	template <typename T, typename U>
	T read_mmr(U offset)
	{
		return *reinterpret_cast<volatile T *>(map_base + offset);
	}

	void write_mmr(off_t target, int access_type, uint64_t write_val);
	ssize_t read();
	ssize_t write( const void *buf, size_t count);
	void close(void);
	void ident(int id[4]);
	void write_ident(int val);
	void reset(bool val);
	void enable(int channel, bool state);
	int get_fd(void) { return m_fd; }
	bool can_read_data(void);
	char *buf(void) { return m_buf; }
	bool is_empty(void) { return m_buf_end == 0; }
	bool flush_request(void);
	size_t buf_end(void) { return m_buf_end; }
	void buf_end(int index) { m_buf_end = index; }
};

#endif
