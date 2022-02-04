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
#ifndef STI_NOSYS_PROT_SERVER_H_INCLUDED
#define STI_NOSYS_PROT_SERVER_H_INCLUDED

#include "sockets.h"
#include "packet.h"
#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS
typedef const char * const printf_format_arg;
#else
typedef const char * printf_format_arg;
#endif

// Enumerations
typedef enum {
    SINGLE_CLIENT,    // Server will only ever service one client (useful for unit test)
    MULTIPLE_CLIENTS  // Server will serve an unlimited number of clients, one at a time
} SERVER_LIFESPAN;

// Structure Definitions
typedef struct {
    char *ctrl_rx_buff;
    size_t ctrl_rx_buff_sz;
    char *ctrl_tx_buff;
    size_t ctrl_tx_buff_sz;

    char h2t_header_buff[SIZEOF_PACKET_GUARDBAND + SIZEOF_H2T_PACKET_HEADER];
    char t2h_header_buff[SIZEOF_PACKET_GUARDBAND + SIZEOF_H2T_PACKET_HEADER];
    char mgmt_header_buff[SIZEOF_PACKET_GUARDBAND + SIZEOF_MGMT_PACKET_HEADER];
    char mgmt_rsp_header_buff[SIZEOF_PACKET_GUARDBAND + SIZEOF_MGMT_PACKET_HEADER];

    // Used to indicate if the 4 data stream buffers (H2T, T2H, MGMT, MGMT_RSP) are implemented
    // as circular buffers.  If set to '1' the server needs to calculate and account for when
    // the wrap occurs.
    char use_wrapping_data_buffers;

    char *h2t_rx_buff;
    size_t h2t_rx_buff_sz;

    char *mgmt_rx_buff;
    size_t mgmt_rx_buff_sz;

    char *t2h_tx_buff;
    size_t t2h_tx_buff_sz;

    char *mgmt_rsp_tx_buff;
    size_t mgmt_rsp_tx_buff_sz;
} SERVER_BUFFERS;

typedef struct {
    // Optional driver initialization, invoked by the server for each new client connection.
    // This is the first callback to be invoked by the server. A return value of < 0 indicates an error condition.
    int(*init_driver)();

    // Will return NULL if a buffer of size 'sz' is unavailable
    char *(*get_h2t_buffer)(size_t sz);

    // A return value of < 0 indicates an error condition
    int(*h2t_data_received)(H2T_PACKET_HEADER *header, unsigned char *payload);

    // Will return NULL if a buffer of size 'sz' is unavailable
    char *(*get_mgmt_buffer)(size_t sz);

    // A return value of < 0 indicates an error condition
    int(*mgmt_data_received)(MGMT_PACKET_HEADER *header, unsigned char *payload);

    // 'payload' & 'header' are outputs to be filled -- a header->DATA_LEN_BYTES equal to 0 implies no data.
    // A return value of < 0 indicates an error condition
    int(*acquire_t2h_data)(H2T_PACKET_HEADER *header, unsigned char **payload);

    // Used to indicate the most recently acquired T2H data has been fully processed and is ready to have
    // any associated resources (e.g. payload / header memory) freed
    void(*t2h_data_complete)();

    // 'payload' & 'header' are outputs to be filled -- a header->DATA_LEN_BYTES equal to 0 implies no data.
    // A return value of < 0 indicates an error condition
    int(*acquire_mgmt_rsp_data)(MGMT_PACKET_HEADER *header, unsigned char **payload);

    // Used to indicate the most recently acquired MGMT RSP data has been fully processed and is ready to have
    // any associated resources (e.g. payload / header memory) freed
    void(*mgmt_rsp_data_complete)();

    // Optional callback, if left NULL it implies no MGMT support.
    // Used to declare whether or not the driver has support for MGMT + MGMT RSP channels.
    // A return value of '1' indicates MGMT support, anything else indicates no MGMT support.
    int (*has_mgmt_support)();

    // Optional callback to set a driver parameter.  A return value of < 0 indicates an error condition.
    int (*set_param)(const char *param, const char *val);

    // Optional callback to get a driver parameter.  Returns NULL if param is undefined.
    char *(*get_param)(const char *param);

    // Print a message using printf style formatting + varargs
    int(*server_printf)(printf_format_arg, ...);
} SERVER_HW_CALLBACKS;

typedef struct {
    size_t h2t_cnt;
    size_t t2h_cnt;
    size_t mgmt_cnt;
    size_t mgmt_rsp_cnt;
} SERVER_PKT_STATS;

typedef struct {
    // Buffers
    SERVER_BUFFERS *buff;
    char h2t_waiting;
    char mgmt_waiting;

    // Callbacks
    SERVER_HW_CALLBACKS hw_callbacks;
    char loopback_mode; // 1 enabled, 0 disabled (default)

    // Connection info
    SOCKET server_fd;
    struct sockaddr_in server_addr;
    char t2h_nagle;
    char mgmt_rsp_nagle;

    // Misc
    SERVER_PKT_STATS pkt_stats;
} SERVER_CONN;

typedef struct {
    SOCKET ctrl_fd;
    SOCKET mgmt_fd;
    SOCKET mgmt_rsp_fd;
    SOCKET h2t_data_fd;
    SOCKET t2h_data_fd;
} CLIENT_CONN;

extern const SERVER_BUFFERS SERVER_BUFFERS_default;
extern const SERVER_CONN SERVER_CONN_default;
extern const SERVER_HW_CALLBACKS SERVER_HW_CALLBACKS_default;
extern const SERVER_PKT_STATS SERVER_PKT_STATS_default;
extern const CLIENT_CONN CLIENT_CONN_default;

// Server code
RETURN_CODE initialize_server(unsigned short port, SERVER_CONN *server_conn, const char *port_filename);
void server_main(SERVER_LIFESPAN lifespan, SERVER_CONN *server_conn);
void server_terminate();
void reject_client(SERVER_CONN *server_conn);
void handle_client(SERVER_CONN *server_conn, CLIENT_CONN *client_conn);
RETURN_CODE bind_server_socket(SERVER_CONN *server_conn);
RETURN_CODE connect_client_socket(SERVER_CONN *server_conn, int handle_id, SOCKET *client_fd, const char *sock_name, char use_nagle);
RETURN_CODE close_client_conn(CLIENT_CONN *client_conn, SERVER_CONN *server_conn);

// Message handling
const char *get_parameter(char *cmd, SERVER_CONN *server_conn);
const char *set_parameter(char *cmd, SERVER_CONN *server_conn, CLIENT_CONN *client_conn);
const char *get_driver_parameter(char *cmd, SERVER_CONN *server_conn);
const char *set_driver_parameter(char *cmd, SERVER_CONN *server_conn);
RETURN_CODE process_control_message(CLIENT_CONN *client_conn, SERVER_CONN *server_conn, char *disconnect_client);
unsigned long buff_len_to_wrap_boundary(char *buff_sa, size_t buff_sz, char *buff, size_t payload_sz);
RETURN_CODE update_curr_h2t_header(CLIENT_CONN *client_conn, SERVER_CONN *server_conn);
RETURN_CODE process_h2t_data(CLIENT_CONN *client_conn, SERVER_CONN *server_conn);
RETURN_CODE update_curr_mgmt_header(CLIENT_CONN *client_conn, SERVER_CONN *server_conn);
RETURN_CODE process_mgmt_data(CLIENT_CONN *client_conn, SERVER_CONN *server_conn);
RETURN_CODE process_t2h_data(CLIENT_CONN *client_conn, SERVER_CONN *server_conn);
RETURN_CODE process_mgmt_rsp_data(CLIENT_CONN *client_conn, SERVER_CONN *server_conn);

// Misc helper
void reset_buffers(SERVER_CONN *server_conn);
void generate_server_welcome_message(char *buff, size_t buff_size, int mgmt_support, SERVER_BUFFERS *serv_buff, int handle);
void print_last_socket_error(const char *context_msg, int(*printf_fp)(printf_format_arg, ...));
void print_last_socket_error_b(const char *context_msg, ssize_t bytes_transferred, int(*printf_fp)(printf_format_arg, ...));

#ifdef __cplusplus    
}
#endif

#endif //STI_NOSYS_PROT_SERVER_H_INCLUDED
