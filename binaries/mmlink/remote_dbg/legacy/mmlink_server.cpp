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
/// @file  mmlink_server.cpp
/// @brief Basic AFU interaction.
/// @ingroup SigTap
/// @verbatim
//****************************************************************************

#include <cerrno>
#include <cstdarg>
#include <cstring>
#include <string>
#include <iostream>

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include "mm_debug_link_interface.h"
#include "mmlink_connection.h"
#include "mmlink_server.h"

using namespace std;

mmlink_server::mmlink_server(struct sockaddr_in *sock, mm_debug_link_interface *driver)
{
	m_addr = *sock;

	m_num_bound_connections = 0;
	m_num_connections = 0;

	m_t2h_pending = false;
	m_h2t_pending = false;

	m_conn = new mmlink_connection*[MAX_CONNECTIONS];
	for (size_t i = 0; i < MAX_CONNECTIONS; ++i)
		m_conn[i] = new mmlink_connection(this);

	m_running = false;
	m_driver = driver;
	m_server_id = 0;

	m_listen = -1;

	m_h2t_stats = NULL;
	m_t2h_stats = NULL;
#ifdef ENABLE_MMLINK_STATS
	m_h2t_stats = new mmlink_stats("h2t");
	m_t2h_stats = new mmlink_stats("t2h");
#endif

}

mmlink_server::~mmlink_server()
{
	if (m_conn)
		for (size_t i = 0; i < MAX_CONNECTIONS; ++i)
		{
			delete m_conn[i]; m_conn[i] = NULL;
		}
	delete[] m_conn; m_conn = NULL;
	m_driver->close();

	if ( -1 != m_listen ) {
		close(m_listen);
	}

#ifdef ENABLE_MMLINK_STATS
	delete m_h2t_stats; m_h2t_stats = NULL;
	delete m_t2h_stats; m_t2h_stats = NULL;
#endif
}

int mmlink_server::setup_listen_socket(void)
{
	m_listen = socket(AF_INET, SOCK_STREAM, 0);

	if (m_listen < 0)
	{
		cerr << "Socket creation failed: " <<  errno << endl;
		return errno;
	}
	printf("m_listen: %d\n", m_listen);

	// Allow reconnect sooner, after server exit.
	int optval = 1;
	int err =
		setsockopt(m_listen, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	if (err < 0)
	{
		fprintf(stderr, "setsockopt failed: %d\n", errno);
		return errno;
	}
	return 0;
}

int mmlink_server::run(unsigned char* stpAddr)
{
	int err = 0;
	m_running = true;

	if (m_driver->open(stpAddr))
	{
		fprintf(stderr, "failed to init driver (%d).\n", err);
		return err;
	}

	// Todo: modulate timeout based on number of connections, expectation of data.
	struct timeval tv;
	tv.tv_sec  = 0;
	tv.tv_usec = 1000;

	if (setup_listen_socket())
	{
		fprintf(stderr, "setup_listen_socket() failed\n");
		return -1;
	}

	if (bind(m_listen, (struct sockaddr *)&m_addr, sizeof(m_addr)) != 0)
	{
		fprintf(stderr, "bind() failed: %d (%s)\n", errno, strerror(errno));
		return errno;
	}

	if (listen(m_listen, 5) < 0)
	{
		fprintf(stderr, "listen() failed: %d (%s)\n", errno, strerror(errno));
		return errno;
	}

	printf("listening on ip: %s; port: %d\n", inet_ntoa(m_addr.sin_addr),
	       htons(m_addr.sin_port));

	while (m_running)
	{
		fd_set readfds, writefds;
		FD_ZERO(&readfds);
		FD_ZERO(&writefds);

		int max_fd = -1;
		// Listen for more connections, if needed.
		if ((size_t)m_num_connections < MAX_CONNECTIONS)
		{
			FD_SET(m_listen, &readfds);
			max_fd = MAX(m_listen, max_fd);
		}

		// Listen for read on all connections.
		for (size_t i = 0; i < MAX_CONNECTIONS; ++i)
		{
			mmlink_connection *pc = *(m_conn + i);
			if (pc->is_open())
			{
				int fd = pc->getsocket();
				FD_SET(fd, &readfds);

				max_fd = MAX(fd, max_fd);
			}
		}

		// If we have a data socket, listen for read and write on the driver fd.
		mmlink_connection *data_conn = get_data_connection();
		if (data_conn)
		{
			int host_fd = data_conn->getsocket();

			// Listen for write on the host
			// Data from the driver are written here.
			FD_SET(host_fd, &writefds);
			max_fd = MAX(host_fd, max_fd);
		}

		tv.tv_sec  = 0;
		tv.tv_usec = 1000;
		if (select(max_fd + 1, &readfds, &writefds, NULL, &tv) < 0)
		{
			fprintf(stderr, "select error: %d (%s)\n", errno, strerror(errno));
			break;
		}

		// Handle new connection attempts.
		if (FD_ISSET(m_listen, &readfds))
		{
			mmlink_connection *pc = handle_accept();
			// If a new connection was accepted, send the welcome string.
			if (pc)
			{
				char msg[256];

				get_welcome_message(msg, sizeof(msg) / sizeof(*msg));
				// to do:spin until all bytes sent.
				pc->send(msg, strnlen(msg, sizeof(msg)));
			}
		}

		// Transfer response data from the driver to the data socket.
		if (data_conn)
		{
			bool can_write_host = FD_ISSET(data_conn->getsocket(), &writefds);
			//bool can_read_driver = FD_ISSET(m_driver->get_fd(), &readfds);
			bool can_read_driver = m_driver->can_read_data();
			err = handle_t2h(data_conn, can_read_driver, can_write_host); //TODO add logic to check if driver has data to be read

			if (err)
				break;

			// Transfer command data from the data socket to the driver.
			//bool can_write_driver = FD_ISSET(m_driver->get_fd(), &writefds);
			bool can_write_driver = true;
			bool can_read_host = FD_ISSET(data_conn->getsocket(), &readfds);
			err = handle_h2t(data_conn, can_read_host, can_write_driver); //TODO add logic to check if host has data to be written to driver

			if (err < 0)
			{
				m_num_connections--;
				data_conn->close_connection();
				printf("closed data connection due to handle_h2t return value, now have %d\n", m_num_connections);
			}

			// Yield after done process the current known acitivty
			::sched_yield();
		}

		// Handle management connection commands and responses.
		for (size_t i = 0; i < MAX_CONNECTIONS; ++i)
		{
			mmlink_connection *pc = *(m_conn + i);
			if (!pc->is_open())
			{
				continue;
			}
			if (pc->is_data())
			{
				continue;
			}

			if (FD_ISSET(pc->getsocket(), &readfds))
			{
				int fail = pc->handle_receive();
				if (fail)
				{
					--m_num_connections;
					printf("%d: handle_receive() returned %d, closing connection, now have %d\n",
					       pc->getsocket(), fail, m_num_connections);
					pc->close_connection();
				}
				else
				{
					fail = pc->handle_management();
					if (fail)
					{
						--m_num_connections;
						printf("%d: handle_management() returned %d, closing connection, now have %d\n",
						       pc->getsocket(), fail, m_num_connections);
						pc->close_connection();
					}
					else if (pc->is_data())
					{
						printf("%d: converted to data\n", pc->getsocket());
						// A management connection was converted to data. There can be only one.
						close_other_data_connection(pc);
						m_h2t_pending = true;
					}
				}
			}
		}
	}
	printf("goodbye with code %d\n", err);

	return err;
}

void mmlink_server::print_stats(void)
{
#ifdef ENABLE_MMLINK_STATS
	printf("mmlink_connection::print_stats()\n");

	m_h2t_stats->print();
	m_t2h_stats->print();
#endif
}

mmlink_connection *mmlink_server::handle_accept()
{
	int socket;
	struct sockaddr_in incoming_addr;
	socklen_t len = sizeof(incoming_addr);

	// Find an mmlink_connection for this new connection,
	// or NULL if none available.
	mmlink_connection *pc = get_unused_connection();
	socket = ::accept(m_listen, (struct sockaddr *)&incoming_addr, &len);
	if (socket < 0)
	{
		fprintf(stderr, "accept failed: %d (%s)\n", errno, strerror(errno));
		pc = NULL;
	}
	else
	{
		if (pc)
		{
			++m_num_connections;
			pc->socket(socket);
			printf("I have %d connections now; latest socket is %d\n", m_num_connections, socket);
			// The 1st connection is bound upon connection.  The 2nd connection will
			// be bound if it sends the correct handle.
			if (m_num_connections == 1)
			{
				printf("%d: binding first connection\n", pc->getsocket());
				pc->bind();
			}
			printf("%d: Accepted connection request from %s\n", pc->getsocket(), inet_ntoa(incoming_addr.sin_addr));
		}
		else
		{
			// If there are no unused connections available, we shouldn't be in
			// this routine in the first place. If this happens anyway, accept
			// and close the connection.
			fprintf(stderr, "%d: Rejected connection request from %s\n", socket, inet_ntoa(incoming_addr.sin_addr));
			::close(socket);
			pc = NULL;
		}
	}

	return pc;
}

void mmlink_server::get_welcome_message(char *msg, size_t msg_len)
{
	int ident[4];

	m_driver->ident(ident);

	if (m_num_connections == 1)
	{
		++m_server_id;
		//snprintf(msg, msg_len, "SystemConsole CONFIGROM IDENT=%08X%08X%08X%08X HANDLE=%08X\r\n",
		//         ident[3], ident[2], ident[1], ident[0], m_server_id);

		snprintf(msg, msg_len, "SystemConsole CONFIGROM IDENT=0001000000007BF899BB8B9AA2D864C3 HANDLE=%08X\r\n", m_server_id);
	}
	else
	{
		strncpy(msg, "SystemConsole CONFIGROM IDENT=0001000000007BF899BB8B9AA2D864C3 HANDLE\r\n", 73);

		//snprintf(msg, msg_len, "SystemConsole CONFIGROM IDENT=0001000000007BF899BB8B9AA2D864C3 HANDLE\r\n");
		//snprintf(msg, msg_len, "SystemConsole CONFIGROM IDENT=%08X%08X%08X%08X HANDLE\r\n",
		//         ident[3], ident[2], ident[1], ident[0]);
	}
}

mmlink_connection *mmlink_server::get_unused_connection()
{
	mmlink_connection *pc = NULL;
	for (size_t i = 0; i < MAX_CONNECTIONS; ++i)
		if (!m_conn[i]->is_open())
		{

			pc = *(m_conn + i);
			break;
		}

	return pc;
}

void mmlink_server::close_other_data_connection(mmlink_connection *pc)
{
	for (size_t i = 0; i < MAX_CONNECTIONS; ++i)
	{
		mmlink_connection *other_pc = *(m_conn + i);
		if (other_pc == pc)
			continue;
		if (other_pc->is_open() && other_pc->is_data())
		{
			printf("closing old data connection in favor of new one\n");
			m_num_connections--;
			other_pc->close_connection();
		}
	}
}

// Return the data connection, or NULL if none.
// Could cache this.
mmlink_connection *mmlink_server::get_data_connection(void)
{
	for (size_t i = 0; i < MAX_CONNECTIONS; ++i)
	{
		mmlink_connection *pc = *(m_conn + i);
		if (pc->is_data())
			return pc;
	}

	return NULL;
}

int mmlink_server::handle_t2h(mmlink_connection *data_conn, bool can_read_driver, bool can_write_host)
{
	int err = 0;
	bool socket_error = false;
	bool t2h_ready = m_t2h_pending ? can_write_host : can_read_driver;


	if (!t2h_ready)
	{
		return 0;
	}

	// Try to get more data.
	if (can_read_driver)
	{
		m_driver->read();
	}
	if (m_driver->is_empty())
	{
		// Still no t2h data; done here.
		m_t2h_pending = false;
		return 0;
	}

	// Handle response data from the driver.
	if (can_write_host && data_conn && m_driver->flush_request())
	{
		// Send the data to the data socket.
		int total_sent = 0;

		while ((size_t)total_sent < m_driver->buf_end())
		{
			ssize_t sent = data_conn->send(m_driver->buf() + total_sent, m_driver->buf_end() - total_sent);
			// printf("t2h sent: %u (%d of %d)\n", sent, total_sent, m_driver->buf_end());

//      if (sent == 8 && !printed8)
//      {
//        // printed8 = true;
//        for (int i = 0; i < sent; ++i)
//        {
//          printf_RAW("0x%02X; ", m_driver->buf()[i]);
//        }
//        printf_RAW("\n");
//      }

			if (sent < 0)
			{
				if (errno == EAGAIN)
				{
					// Try again later.
					break;
				}
				else
				{
					// Socket error, disconnected?
					socket_error = true;
					break;
				}
			}
			if (sent == 0)
			{
				// Didn't send all data; Try to send the remaining data later.
				break;
			}

			total_sent += sent;
		}

		if (total_sent > 0)
			m_t2h_stats->update(total_sent, m_driver->buf());

		int rem = m_driver->buf_end() - total_sent;
		if (rem > 0)
		{
			printf("t2h rem: %d; total_sent: %d; m_h2t_pending: %d\n", rem, total_sent, m_t2h_pending);
			if (total_sent > 0)
			{
				m_t2h_pending = true;
				memmove(m_driver->buf(), m_driver->buf() + total_sent, rem);
			}
		}
		m_driver->buf_end(rem);
	}

	if (socket_error || !data_conn)
	{
		// We didn't have a data connection in the first place, or an error
		// has occurred on the data connection.
		fprintf(stderr, "hardware returned data but there's no data socket\n");
		err = -1;
	}

	return err;
}

int mmlink_server::handle_h2t(mmlink_connection *data_conn, bool can_read_host, bool can_write_driver)
{
	int err = 0;

	bool h2t_ready = m_h2t_pending ? can_write_driver : can_read_host;
	if (!h2t_ready)
	{
		return 0;
	}

	// printf("h2t_ready: m_h2t_pending: %d; can_read_host: %d; can_write_driver: %d\n", m_h2t_pending, can_read_host, can_write_driver);

	// If no stored data, try to get some.
	if (can_read_host)
	{
		err = data_conn->handle_receive();
		if (err < 0)
		{
			return err;
		}
	}

	if (data_conn->buf_end() == 0)
	{
		// No data to send.
		m_h2t_pending = false;
		return 0;
	}

	if (!can_write_driver)
		return 0;

	// Handle command data from the data socket.
	int total_sent = 0;
	while ((size_t)total_sent < data_conn->buf_end())
	{
		ssize_t sent = m_driver->write(data_conn->buf() + total_sent, data_conn->buf_end() - total_sent);
		if (sent < 0)
		{
			if (errno == EAGAIN)
			{
				// Try again later
				printf("handle_h2t(): driver returned EAGAIN\n");
				break;
			}
			else
			{
				// Not sure if this can happen.
				printf("handle_h2t(): driver returned error %d (%s)\n", errno, strerror(errno));
			}
		}
		if (sent == 0)
		{
			// Didn't send all data; Try to send the remaining data later.
			break;
		}
		total_sent += sent;
	}
//  if (total_sent > 0)
//  {
//    printf("sent on %d: %d bytes\n", data_conn->socket(), total_sent);
//    for (int i = 0; i < total_sent; ++i)
//    {
//      unsigned char the_byte = data_conn->buf()[i];
//      printf_RAW("%s\\x%02X", (the_byte == 0x7C) ? "\n" : "", the_byte);
//    }
//    printf_RAW("\n");
//  }

	if (total_sent > 0)
		m_h2t_stats->update(total_sent, data_conn->buf());

	int rem = data_conn->buf_end() - total_sent;
	if (rem > 0)
	{
		// printf("h2t rem: %d; total_sent: %d; m_h2t_pending: %d\n", rem, total_sent, m_h2t_pending);
		m_h2t_pending = true;
		if (total_sent > 0)
		{
			// memmove(data_conn->buf(), data_conn->buf() + data_conn->buf_end(), rem);
			memmove(data_conn->buf(), data_conn->buf() + total_sent, rem);
		}
	}
	data_conn->buf_end(rem);

	return err;
}
