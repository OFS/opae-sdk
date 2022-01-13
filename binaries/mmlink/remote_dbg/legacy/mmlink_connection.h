// Copyright(c) 2017-2020, Intel Corporation
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
/// @file  mmlink_connection.h
/// @brief Basic AFU interaction.
/// @ingroup SigTap
/// @verbatim
//****************************************************************************

#ifndef MMLINK_CONNECTION_H
#define MMLINK_CONNECTION_H

#include <sys/socket.h>
#include <unistd.h>

#include "mm_debug_link_interface.h"
#include "mmlink_server.h"

class mmlink_connection
{
public:
	// m_bufsize is the size of the buffer for h2t data
	mmlink_connection(mmlink_server*);
	~mmlink_connection() { close_connection(); delete[] m_buf; }
	bool is_open() { return m_fd >= 0; }
	bool is_data() { return m_is_data; }
	bool is_bound() { return m_is_bound; }
	void set_is_data(void) { m_is_data = true; }

	size_t send(const char *msg, const size_t len);
	void close_connection() { if (is_open()) ::close(m_fd); init(); }
	void bind() { m_is_bound = true; }
	void socket(int socket) { m_fd = socket; }
	int getsocket() { return m_fd; }
	int handle_receive();
	int handle_management(void);

	char *buf(void) { return m_buf; }
	void buf_end(size_t index) { m_buf_end = index; }
	size_t buf_end(void) { return m_buf_end; }

	static const char *UNKNOWN;
	static const char *OK;

protected:
	int m_fd;
	bool m_is_bound;
	bool m_is_data;
	const int m_bufsize;
	mmlink_server *m_server;

	char *m_buf;
	volatile size_t m_buf_end;

	void init(mmlink_server *server) { m_server = server; init(); }

	mmlink_connection(const mmlink_connection& mm_conn):m_bufsize( mm_conn.m_bufsize)
		{
			m_fd             = mm_conn.m_fd;
			m_buf_end        = mm_conn.m_buf_end;
			m_is_bound       = mm_conn.m_is_bound;
			m_is_data        = mm_conn.m_is_data;
			m_server         = mm_conn.m_server;

			m_buf= new char[m_bufsize];
			strncpy(m_buf, mm_conn.m_buf, m_bufsize);
		}

		mmlink_connection& operator=(const mmlink_connection& mm_conn)
		{
			if( this != &mm_conn) {
				m_fd             = mm_conn.m_fd;
				m_buf_end        = mm_conn.m_buf_end;
				m_is_bound       = mm_conn.m_is_bound;
				m_is_data        = mm_conn.m_is_data;
				m_server         = mm_conn.m_server;

				if(m_buf) delete m_buf;

				m_buf = new char[m_bufsize];
				strncpy(m_buf, mm_conn.m_buf, m_bufsize);
			}
			return *this;
		}

private:
	int handle_data(void);
	int handle_management_command(char *cmd);
	int handle_unbound_command(char *cmd);
	int handle_bound_command(char *cmd);
	int get_server_id(void) { return m_server->get_server_id(); }
	mm_debug_link_interface *driver(void) {
		return m_server->get_driver_fd();
	}
	void init(void) { m_fd = -1; m_is_bound = false;
				m_is_data = false; m_buf_end = 0; }
};

#endif
