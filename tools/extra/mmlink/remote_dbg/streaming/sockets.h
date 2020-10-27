// Copyright(c) 2020, Intel Corporation
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
#ifndef STI_NOSYS_PROT_SOCKETS_H_INCLUDED
#define STI_NOSYS_PROT_SOCKETS_H_INCLUDED

#include "common.h"
#include "platform.h"

// Platform specific includes are best kept here -- otherwise
// one can easily get in trouble if the order of inclusions
// isn't correct to setup the 'STI_NOSYS_PROT_PLATFORM'
// variable first...
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <WS2tcpip.h>
    #define snprintf _snprintf
    #define inet_pton InetPton
    #define poll WSAPoll
#else
    #if STI_NOSYS_PROT_PLATFORM == STI_PLATFORM_NIOS_INICHE
        #include "ipport.h"
        #include "tcpport.h"
    #else
        #include <sys/socket.h>
        #include <netinet/in.h>
        #include <netinet/tcp.h>
        #include <arpa/inet.h>
        #include <poll.h>
    #endif
    #include <fcntl.h>
    #include <unistd.h> // close
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Typedefs
#if STI_NOSYS_PROT_PLATFORM!=STI_PLATFORM_WINDOWS
typedef int SOCKET;
#define INVALID_SOCKET -1
#else
typedef int ssize_t;
#endif

extern const struct timeval ZERO_TIMEOUT;

SOCKET max_of(SOCKET *array, int size);

#define BOOL int
#define TRUE 1
#define FALSE 0

// Nichestack doesn't support 'getsockopt' which is part of connect with timeout.
// All good since the server doesn't need this function.
#if STI_NOSYS_PROT_PLATFORM!=STI_PLATFORM_NIOS_INICHE
RETURN_CODE connect_with_timeout(SOCKET fd, const struct sockaddr *serv_addr, const struct timeval timeout);
#endif
RETURN_CODE socket_send_all(SOCKET fd, const char *buff, const size_t len, int flags, ssize_t *bytes_sent);
RETURN_CODE socket_send_all_t2h_data(SOCKET fd, const char *buff, const size_t len, int flags, ssize_t *bytes_sent);
RETURN_CODE socket_recv_until_null_reached(SOCKET sock_fd, char *buff, const size_t max_len, int flags, ssize_t *bytes_recvd);
RETURN_CODE socket_recv_accumulate(SOCKET sock_fd, char *buff, const size_t len, int flags, ssize_t *bytes_recvd);
RETURN_CODE socket_recv_accumulate_h2t_data(SOCKET sock_fd, char *buff, const size_t len, int flags, ssize_t *bytes_recvd);
RETURN_CODE initialize_sockets_library();
int set_boolean_socket_option(SOCKET socket_fd, int option, int option_val);
int set_tcp_no_delay(SOCKET socket_fd, int no_delay);
int set_linger_socket_option(SOCKET socket_fd, int l_onoff, int l_linger);
int set_socket_non_blocking(SOCKET socket_fd, int non_blocking);
char is_last_socket_error_would_block();
int close_socket_fd(SOCKET socket_fd);
void wait_for_read_event(SOCKET socket_fd, long seconds, long useconds);
int get_last_socket_error();
const char *get_last_socket_error_msg(char *buff, size_t buff_sz);

#ifdef __cplusplus    
}
#endif

#endif //STI_NOSYS_PROT_SOCKETS_H_INCLUDED
