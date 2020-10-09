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