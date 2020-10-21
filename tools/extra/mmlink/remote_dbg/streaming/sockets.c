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
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "sockets.h"
#include "common.h"
#include "constants.h"

const struct timeval ZERO_TIMEOUT = { 0, 0 };

enum { SW_SOCKET_BUFF_SZ = 0x1000 };
static char g_socket_recv_buff[SW_SOCKET_BUFF_SZ+64];
static char g_socket_send_buff[SW_SOCKET_BUFF_SZ+64];

SOCKET max_of(SOCKET *array, int size) {
    SOCKET result = 0;
    int i;
    for (i = 0; i < size; ++i) {
        result = MAX_MACRO(array[i], result);
    }
    return result;
}

#if STI_NOSYS_PROT_PLATFORM!=STI_PLATFORM_NIOS_INICHE
RETURN_CODE connect_with_timeout(SOCKET fd, const struct sockaddr *serv_addr, const struct timeval timeout) {
    // First set the socket to non-blocking
    int set_nonblk_result = set_socket_non_blocking(fd, 1);
    RETURN_CODE result = (set_nonblk_result >= 0) ? OK : FAILURE;

    // If connect fails to immediately succeed (we expect it to)
    if ((result == OK) && connect(fd, serv_addr, sizeof(*serv_addr)) < 0) {
        int sockerr = get_last_socket_error();
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS
        if ((sockerr == EINPROGRESS) || (sockerr == WSAEWOULDBLOCK)) {
#else
        if ((sockerr == EINPROGRESS) || (sockerr == EWOULDBLOCK)) {
#endif
            // Loop is to account for possibility of 'select' being interrupted
            char again = 0;
            do {
                again = 0;
                fd_set writefds;
                FD_ZERO(&writefds);
                FD_SET(fd, &writefds);
                struct timeval non_const_timeout = timeout; // Select may modify timeout
                int select_res = select((int)(fd + 1), NULL, &writefds, NULL, &non_const_timeout);
                if (select_res < 0) {
                    sockerr = get_last_socket_error();
                    if (sockerr != EINTR) {
                        // Select failed for a reason other than interruption
                        result = FAILURE;
                    } else {
                        // Try again, select was interrupted.  This edge case has a weakness of resetting the requested timeout.
                        again = 1;
                    }
                } else if (select_res > 0) {
                    // Socket has a write (connection event occurred!)
                    socklen_t lon = sizeof(int);
                    int valopt;
                    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) {
                        // 'getsockopt' failed
                        result = FAILURE;
                    } else {
                        // Check the value returned... 
                        if (valopt) {
                            // An error is present
                            result = FAILURE;
                        }
                    }
                } else {
                    // Timeout occurred...
                    result = FAILURE;
                }
            } while (again == 1);
        } else {
            // Error was something other than "in progress"
            result = FAILURE;
        }
    }

    // Restore the socket back to blocking
    if (set_nonblk_result >= 0) {
        int set_blk_result = set_socket_non_blocking(fd, 0);
        if (result == OK) {
            result = (set_blk_result >= 0) ? OK : FAILURE;
        }
    }

    return result;
}
#endif

RETURN_CODE socket_send_all(SOCKET fd, const char *buff, const size_t len, int flags, ssize_t *bytes_sent) {
    ssize_t curr_bytes_sent;
    size_t bytes_remaining = len;

    while (bytes_remaining > 0) {
        if ((curr_bytes_sent = send(fd, buff + (len - bytes_remaining), bytes_remaining, flags)) <= 0) {
            if (bytes_sent != NULL) {
                *bytes_sent = curr_bytes_sent;
            }
            return FAILURE;
        }
        bytes_remaining -= curr_bytes_sent;
    }
    if (bytes_sent != NULL) {
        *bytes_sent = len;
    }
    return OK;
}


RETURN_CODE socket_send_all_t2h_data(SOCKET fd, const char *buff, const size_t len, int flags, ssize_t *bytes_sent) {
    // First copy the mmio ptr into local memory domain
    volatile uint64_t *mmio_ptr = (uint64_t *)buff;
    uint64_t *buff64 = (uint64_t *)g_socket_send_buff;
    size_t transfers = (len + 7) / 8;
    size_t i;
    for (i = 0; i < transfers; ++i) {
        *buff64++ = *mmio_ptr++;      
    }
    
    RETURN_CODE ret = socket_send_all(fd, g_socket_send_buff, len, flags, bytes_sent);

    return ret;
}

RETURN_CODE socket_recv_until_null_reached(SOCKET sock_fd, char *buff, const size_t max_len, int flags, ssize_t *bytes_recvd) {
    ssize_t curr_bytes_recvd;
    size_t bytes_remaining = max_len;

    while (bytes_remaining > 0) {
        if ((curr_bytes_recvd = recv(sock_fd, buff, bytes_remaining, flags)) <= 0) {
            if (bytes_recvd != NULL) {
                *bytes_recvd = curr_bytes_recvd; // Return the error
            }
            return FAILURE;
        }

        bytes_remaining -= curr_bytes_recvd;
        void *pos = memchr(buff, 0, curr_bytes_recvd);
        if (pos != NULL) {
            if (bytes_recvd != NULL) {
                *bytes_recvd = max_len - bytes_remaining;
            }
            return OK;
        }
        buff += curr_bytes_recvd;
    }

    // No null character was found within the bounded max length
    if (bytes_recvd != NULL) {
        *bytes_recvd = max_len;
    }
    return FAILURE;
}

RETURN_CODE socket_recv_accumulate(SOCKET sock_fd, char *buff, const size_t len, int flags, ssize_t *bytes_recvd) {
    ssize_t curr_bytes_recvd;
    size_t bytes_remaining = len;

    while (bytes_remaining > 0) {
        if ((curr_bytes_recvd = recv(sock_fd, buff, bytes_remaining, flags)) <= 0) {
            if (bytes_recvd != NULL) {
                *bytes_recvd = curr_bytes_recvd; // Return the error
            }
            return FAILURE;
        }
        bytes_remaining -= curr_bytes_recvd;
        buff += curr_bytes_recvd;
    }
    
    if (bytes_recvd != NULL) {
        *bytes_recvd = len;
    }
    
    return OK;
}


RETURN_CODE socket_recv_accumulate_h2t_data(SOCKET sock_fd, char *buff, const size_t len, int flags, ssize_t *bytes_recvd) {
    RETURN_CODE rc = socket_recv_accumulate(sock_fd, g_socket_recv_buff, len, flags, bytes_recvd);
    
    if (rc != FAILURE) {
        // Copy the local memory ptr into the mmio domain
        volatile uint64_t *mmio_ptr = (uint64_t *)buff;
        uint64_t *buff64 = (uint64_t *)g_socket_recv_buff;
        size_t transfers = (len +7) / 8;
        size_t i;
        for (i = 0; i < transfers; ++i) {
            *mmio_ptr++ = *buff64++;
        }
    }
    
    return OK;
}

RETURN_CODE initialize_sockets_library() {
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    wVersionRequested = MAKEWORD(1, 1);
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0) {
        // We couldn't find a useable winsock.dll
        printf("Failed to initialize Windows socket library component \"winsock.dll\"\n");
        return FAILURE;
    }
#endif
    return OK;
}

int set_boolean_socket_option(SOCKET socket_fd, int option, int option_val) {
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS
    return setsockopt(socket_fd, SOL_SOCKET, option, (const char *)&option_val, sizeof(option_val));
#else
    return setsockopt(socket_fd, SOL_SOCKET, option, &option_val, sizeof(option_val));
#endif
}

int set_tcp_no_delay(SOCKET socket_fd, int no_delay) {
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS
    return setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, (const char *)&no_delay, sizeof(no_delay));
#else
    return setsockopt(socket_fd, IPPROTO_TCP, TCP_NODELAY, &no_delay, sizeof(no_delay));
#endif
}

int set_linger_socket_option(SOCKET socket_fd, int l_onoff, int l_linger) {
    struct linger linger_opt_val;
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS
    linger_opt_val.l_linger = (u_short)l_linger;
    linger_opt_val.l_onoff = (u_short)l_onoff;
    return setsockopt(socket_fd, SOL_SOCKET, SO_LINGER, (const char *)&linger_opt_val, sizeof(linger_opt_val));
#else
    linger_opt_val.l_linger = l_linger;
    linger_opt_val.l_onoff = l_onoff;
    return setsockopt(socket_fd, SOL_SOCKET, SO_LINGER, &linger_opt_val, sizeof(linger_opt_val));
#endif
}

int set_socket_non_blocking(SOCKET socket_fd, int non_blocking) {
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS
    u_long mode = non_blocking;  // 1 to enable non-blocking socket
    return ioctlsocket(socket_fd, FIONBIO, &mode);
#else
    int flags;
    if ((flags = fcntl(socket_fd, F_GETFL, 0)) < 0)
        flags = 0;
    int val = non_blocking ? (flags | O_NONBLOCK) : (flags & !O_NONBLOCK);
    return fcntl(socket_fd, F_SETFL, val);
#endif
}

char is_last_socket_error_would_block() {
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS
    return (get_last_socket_error() == WSAEWOULDBLOCK) ? 1 : 0;
#else
    return (get_last_socket_error() == EAGAIN) ? 1 : 0;
#endif
}

int close_socket_fd(SOCKET socket_fd) {
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS
    return closesocket(socket_fd);
#else
    return close(socket_fd);
#endif
}

void wait_for_read_event(SOCKET socket_fd, long seconds, long useconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = useconds;
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socket_fd, &readfds);
    select((int)(socket_fd + 1), &readfds, NULL, NULL, &timeout);
}

int get_last_socket_error() {
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS
    return WSAGetLastError();
#else
    return errno;
#endif
}

const char *get_last_socket_error_msg(char *buff, size_t buff_sz) {
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS
    int err = WSAGetLastError();
    if (err > 0) {
        FormatMessage
        (
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
            (LPTSTR)buff,
            buff_sz,
            NULL
        );
        return buff;
    } else {
        return NULL;
    }
#else
    USE_ARG_MACRO(buff);
    USE_ARG_MACRO(buff_sz);
    if (errno > 0) {
        return strerror(errno);
    } else {
        return NULL;
    }
#endif
}
