#include <sys/types.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h> // offsetof

#include "server.h"
#include "packet.h"
#include "constants.h"

const SERVER_BUFFERS SERVER_BUFFERS_default = {
    .ctrl_rx_buff = NULL,
    .ctrl_rx_buff_sz = 0,
    .ctrl_tx_buff = NULL,
    .ctrl_tx_buff_sz = 0,

    .h2t_header_buff = { 0 },
    .t2h_header_buff = { 0 },
    .mgmt_header_buff = { 0 },
    .mgmt_rsp_header_buff = { 0 },

    .use_wrapping_data_buffers = 0,

    .h2t_rx_buff = NULL,
    .h2t_rx_buff_sz = 0,

    .mgmt_rx_buff = NULL,
    .mgmt_rx_buff_sz = 0,

    .t2h_tx_buff = NULL,
    .t2h_tx_buff_sz = 0,

    .mgmt_rsp_tx_buff = NULL,
    .mgmt_rsp_tx_buff_sz = 0
};
const SERVER_CONN SERVER_CONN_default = {
    .buff = NULL,
    .h2t_waiting = 0,
    .mgmt_waiting = 0,
    .hw_callbacks = {
        .init_driver = NULL,
        .get_h2t_buffer = NULL,
        .h2t_data_received = NULL,
        .get_mgmt_buffer = NULL,
        .mgmt_data_received = NULL,
        .acquire_t2h_data = NULL,
        .t2h_data_complete = NULL,
        .acquire_mgmt_rsp_data = NULL,
        .mgmt_rsp_data_complete = NULL,
        .has_mgmt_support = NULL,
        .set_param = NULL,
        .get_param = NULL,
        .server_printf = printf
    },
    .loopback_mode = 0,
    .server_fd = INVALID_SOCKET,
    .t2h_nagle = 0,
    .mgmt_rsp_nagle = 0,
    .pkt_stats = { 0, 0, 0, 0 }
};
const SERVER_HW_CALLBACKS SERVER_HW_CALLBACKS_default = {
    .init_driver = NULL,
    .get_h2t_buffer = NULL,
    .h2t_data_received = NULL,
    .get_mgmt_buffer = NULL,
    .mgmt_data_received = NULL,
    .acquire_t2h_data = NULL,
    .t2h_data_complete = NULL,
    .acquire_mgmt_rsp_data = NULL,
    .mgmt_rsp_data_complete = NULL,
    .has_mgmt_support = NULL,
    .set_param = NULL,
    .get_param = NULL,
    .server_printf = printf
};
const SERVER_PKT_STATS SERVER_PKT_STATS_default = { 0, 0, 0, 0 };
const CLIENT_CONN CLIENT_CONN_default = { INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET, INVALID_SOCKET };

// Global variables
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS || STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_NIOS_INICHE
int sizeof_addr = -1;
#else
uint32_t sizeof_addr = 0;
#endif

void reset_buffers(SERVER_CONN *conn) {
    zero_mem(conn->buff->ctrl_rx_buff, conn->buff->ctrl_rx_buff_sz);
    zero_mem(conn->buff->ctrl_tx_buff, conn->buff->ctrl_tx_buff_sz);
}

void print_last_socket_error(const char *context_msg, int(*printf_fp)(printf_format_arg, ...)) {
    enum { ERR_MSG_BUFF_SZ = 256 };
    char err_buff[ERR_MSG_BUFF_SZ] = { 0 };
    const char *system_msg = get_last_socket_error_msg(err_buff, ERR_MSG_BUFF_SZ);
    if (system_msg != NULL) {
        printf_fp("%s: %s\n", context_msg, system_msg);
    } else {
        printf_fp("%s\n", context_msg);
    }
}

void print_last_socket_error_b(const char *context_msg, ssize_t bytes_transferred, int(*printf_fp)(printf_format_arg, ...)) {
    if (bytes_transferred < 0) {
        print_last_socket_error(context_msg, printf_fp);
    } else {
        printf_fp("%s\n", context_msg);
    }
}

void generate_server_welcome_message(char *buff, size_t buff_size, int mgmt_support, SERVER_BUFFERS *serv_buff, int handle) {
    snprintf(buff, buff_size, "Welcome to INTEL_ST_HOST_EP_SERVER: %s=%d %s=%ld %s=%ld %s=%ld HANDLE=%d",
        MGMT_SUPPORT_PARAM,
        mgmt_support,
        H2T_RX_BUFFER_SIZE_PARAM,
        serv_buff->h2t_rx_buff_sz,
        MGMT_RX_BUFFER_SIZE_PARAM,
        serv_buff->mgmt_rx_buff_sz,
        CTRL_RX_BUFFER_SIZE_PARAM,
        serv_buff->ctrl_rx_buff_sz,
        handle
    );
}

RETURN_CODE bind_server_socket(SERVER_CONN *server_conn) {
    const int MAX_LISTEN = 8;
    unsigned char errors = 0;
    
    // Create the socket
    if ((server_conn->server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        print_last_socket_error("Failed to create socket", server_conn->hw_callbacks.server_printf);
        ++errors;
    }

    // Set some options
    if ((errors == 0) && (set_boolean_socket_option(server_conn->server_fd, SO_REUSEADDR, 1) < 0)) {
        print_last_socket_error("Failed to set socket options", server_conn->hw_callbacks.server_printf);
        ++errors;
    }

    // Bind it to PORT + Protocol
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_NIOS_INICHE
    if ((errors == 0) && (bind(server_conn->server_fd, (struct sockaddr *)(&(server_conn->server_addr)), sizeof_addr) < 0)) {
        print_last_socket_error("Failed to bind socket", server_conn->hw_callbacks.server_printf);
        ++errors;
    }
#else
    if ((errors == 0) && (bind(server_conn->server_fd, (const struct sockaddr *)(&(server_conn->server_addr)), sizeof_addr) < 0)) {
        print_last_socket_error("Failed to bind socket", server_conn->hw_callbacks.server_printf);
        ++errors;
    }
#endif

    // Tell the socket to listen for incoming connections
    if ((errors == 0) && (listen(server_conn->server_fd, MAX_LISTEN) < 0)) {
        print_last_socket_error("Failed to listen on server socket", server_conn->hw_callbacks.server_printf);
        ++errors;
    }
    
    if (errors > 0) {
        if (server_conn->server_fd != INVALID_SOCKET) {
            close_socket_fd(server_conn->server_fd);
            server_conn->server_fd = INVALID_SOCKET;
        }
        return FAILURE;
    } else {
        return OK;
    }
}

RETURN_CODE connect_client_socket(SERVER_CONN *server_conn, int handle_id, SOCKET *client_fd, const char *sock_name, char use_nagle) {
    enum { FAIL_MSG_SIZE = 80, MAX_HANDLE_RSP = 64 };
    char socket_fail_msg[FAIL_MSG_SIZE];
    if((*client_fd = accept(server_conn->server_fd, (struct sockaddr *)(&(server_conn->server_addr)), &sizeof_addr)) == INVALID_SOCKET) {
        snprintf(socket_fail_msg, FAIL_MSG_SIZE, "Failed to accept %s socket", sock_name);
        print_last_socket_error(socket_fail_msg, server_conn->hw_callbacks.server_printf);
    } else {
        if (set_tcp_no_delay(*client_fd, (use_nagle == 0) ? 1 : 0) == 0) {
            ssize_t bytes_transferred;
            if (socket_recv_until_null_reached(*client_fd, server_conn->buff->ctrl_rx_buff, MAX_HANDLE_RSP, 0, &bytes_transferred) == OK) {
                // Client needs to send a null terminated handle ack message
                generate_expected_handle_message(server_conn->buff->ctrl_tx_buff, server_conn->buff->ctrl_tx_buff_sz, sock_name, handle_id);
                if (strncmp(server_conn->buff->ctrl_rx_buff, server_conn->buff->ctrl_tx_buff, MAX_HANDLE_RSP) == 0) {
                    RETURN_CODE result;
                    if ((result = socket_send_all(*client_fd, READY_MSG, READY_MSG_LEN, 0, &bytes_transferred)) != OK) {
                        snprintf(socket_fail_msg, FAIL_MSG_SIZE, "Failed to send handle ready message for %s socket", sock_name);
                        print_last_socket_error_b(socket_fail_msg, bytes_transferred, server_conn->hw_callbacks.server_printf);
                    }
                    return result;
                } else {
                    socket_send_all(*client_fd, NOT_READY_MSG, NOT_READY_MSG_LEN, 0, NULL);
                    server_conn->hw_callbacks.server_printf("Got unexpected handle ack message: %s\n\tExpected: %s\n", server_conn->buff->ctrl_rx_buff, server_conn->buff->ctrl_tx_buff);
                }
            } else {
                snprintf(socket_fail_msg, FAIL_MSG_SIZE, "Failed to recv handle ack message for %s socket", sock_name);
                print_last_socket_error_b(socket_fail_msg, bytes_transferred, server_conn->hw_callbacks.server_printf);
            }
        }
    }
    return FAILURE;
}

RETURN_CODE connect_client(SERVER_CONN *server_conn, CLIENT_CONN *client_conn) {
    enum { MAX_HANDLE_RSP = 64 };
    RETURN_CODE result = OK;
    int handle = get_random_id();
    ssize_t bytes_transferred;

    server_conn->pkt_stats = SERVER_PKT_STATS_default;

    // Initialize the driver if required.  Initialization occurs here since it is the first thing run per spec,
    // and the welcome message requires querying the driver for MGMT support.
    if (server_conn->hw_callbacks.init_driver != NULL) {
        int init_driver_rc;
        if ((init_driver_rc = server_conn->hw_callbacks.init_driver(server_conn->buff->h2t_rx_buff, server_conn->buff->h2t_rx_buff_sz, server_conn->buff->mgmt_rx_buff, server_conn->buff->mgmt_rx_buff_sz)) != 0) {
            server_conn->hw_callbacks.server_printf("Failed to initialize driver: %d\n", init_driver_rc);
            return FAILURE; // Early return if driver fails to initialize, client is rejected.
        }
    }
    
    // Connect CTRL socket
    if((client_conn->ctrl_fd = accept(server_conn->server_fd, (struct sockaddr *)(&(server_conn->server_addr)), &sizeof_addr)) == INVALID_SOCKET) {
        print_last_socket_error("Failed to accept CTRL socket", server_conn->hw_callbacks.server_printf);
        result = FAILURE;
    } else {
        // Send out the welcome message
        int mgmt_support = server_conn->hw_callbacks.has_mgmt_support != NULL ? server_conn->hw_callbacks.has_mgmt_support() : 0;
        generate_server_welcome_message(server_conn->buff->ctrl_tx_buff, server_conn->buff->ctrl_tx_buff_sz, mgmt_support, server_conn->buff, handle);
        if (socket_send_all(client_conn->ctrl_fd, server_conn->buff->ctrl_tx_buff, strnlen(server_conn->buff->ctrl_tx_buff, server_conn->buff->ctrl_tx_buff_sz) + 1, 0, &bytes_transferred) == FAILURE) {
            print_last_socket_error_b("Failed to send welcome message to CTRL socket", bytes_transferred, server_conn->hw_callbacks.server_printf);
            result = FAILURE;
        }
    }
    
    // Verify CTRL handle response
    if (result == OK) {
        if ((result = socket_recv_until_null_reached(client_conn->ctrl_fd, server_conn->buff->ctrl_rx_buff, MAX_HANDLE_RSP, 0, &bytes_transferred)) == OK) {
            generate_expected_handle_message(server_conn->buff->ctrl_tx_buff, server_conn->buff->ctrl_tx_buff_sz, CONTROL_SOCK_NAME, handle);
            if (strncmp(server_conn->buff->ctrl_rx_buff, server_conn->buff->ctrl_tx_buff, MAX_HANDLE_RSP) != 0) {
                socket_send_all(client_conn->ctrl_fd, NOT_READY_MSG, NOT_READY_MSG_LEN, 0, NULL);
                server_conn->hw_callbacks.server_printf("Got unexpected handle ack message: %s\n\tExpected: %s\n", server_conn->buff->ctrl_rx_buff, server_conn->buff->ctrl_tx_buff);
                result = FAILURE;
            } else {
                if ((result = socket_send_all(client_conn->ctrl_fd, READY_MSG, READY_MSG_LEN, 0, &bytes_transferred)) != OK) {
                    print_last_socket_error_b("Failed to send handle ready message for CTRL socket", bytes_transferred, server_conn->hw_callbacks.server_printf);
                }
            }
        } else {
            print_last_socket_error_b("Failed to recv handle ack message for CTRL socket", bytes_transferred, server_conn->hw_callbacks.server_printf);
        }
    }

    if (result == OK) {
        result = connect_client_socket(server_conn, handle, &(client_conn->mgmt_fd), MANAGEMENT_SOCK_NAME, 0);
    }
    if (result == OK) {
        result = connect_client_socket(server_conn, handle, &(client_conn->mgmt_rsp_fd), MANAGEMENT_RSP_SOCK_NAME, server_conn->mgmt_rsp_nagle);
    }
    if (result == OK) {
        result = connect_client_socket(server_conn, handle, &(client_conn->h2t_data_fd), H2T_SOCK_NAME, 0);
    }
    if (result == OK) {
        result = connect_client_socket(server_conn, handle, &(client_conn->t2h_data_fd), T2H_SOCK_NAME, server_conn->t2h_nagle);
    }
    
    if (result != OK) {
        send(client_conn->ctrl_fd, NOT_READY_MSG, NOT_READY_MSG_LEN, 0);
        return FAILURE;
    } else {
        if (socket_send_all(client_conn->ctrl_fd, READY_MSG, READY_MSG_LEN, 0, &bytes_transferred) == OK) {
            return OK;
        } else {
            print_last_socket_error_b("Failed to send ready message to CTRL socket", bytes_transferred, server_conn->hw_callbacks.server_printf);
            return FAILURE;
        }
    }
}

RETURN_CODE close_client_conn(CLIENT_CONN *client_conn, SERVER_CONN *server_conn) {
    unsigned char errors = 0;

    if (client_conn->ctrl_fd != INVALID_SOCKET) {
        set_linger_socket_option(client_conn->ctrl_fd, 1, 0);
        if (close_socket_fd(client_conn->ctrl_fd) != 0) {
            print_last_socket_error("Failed to close CONTROL socket", server_conn->hw_callbacks.server_printf);
            ++errors;
        }
    }
    if (client_conn->mgmt_fd != INVALID_SOCKET) {
        set_linger_socket_option(client_conn->mgmt_fd, 1, 0);
        if (close_socket_fd(client_conn->mgmt_fd) != 0) {
            print_last_socket_error("Failed to close MGMT socket", server_conn->hw_callbacks.server_printf);
            ++errors;
        }
    }
    if (client_conn->mgmt_rsp_fd != INVALID_SOCKET) {
        set_linger_socket_option(client_conn->mgmt_rsp_fd, 1, 0);
        if (close_socket_fd(client_conn->mgmt_rsp_fd) != 0) {
            print_last_socket_error("Failed to close MGMT RSP socket", server_conn->hw_callbacks.server_printf);
            ++errors;
        }
    }
    if (client_conn->h2t_data_fd != INVALID_SOCKET) {
        set_linger_socket_option(client_conn->h2t_data_fd, 1, 0);
        if (close_socket_fd(client_conn->h2t_data_fd) != 0) {
            print_last_socket_error("Failed to close H2T_DATA socket", server_conn->hw_callbacks.server_printf);
            ++errors;
        }
    }
    if (client_conn->t2h_data_fd != INVALID_SOCKET) {
        set_linger_socket_option(client_conn->t2h_data_fd, 1, 0);
        if (close_socket_fd(client_conn->t2h_data_fd) != 0) {
            print_last_socket_error("Failed to close T2H_DATA socket", server_conn->hw_callbacks.server_printf);
            ++errors;
        }
    }

    if (errors > 0) {
        return FAILURE;
    } else {
        return OK;
    }
}

const char *get_parameter(char *cmd, SERVER_CONN *server_conn) {
    const char *param_name = strstr(cmd, GET_PARAM_CMD) + GET_PARAM_CMD_LEN;
    if (strncmp(param_name, SERVER_LOOPBACK_MODE_PARAM, SERVER_LOOPBACK_MODE_PARAM_LEN) == 0) {
        return server_conn->loopback_mode == 1 ? "1" : "0";
    } else if (strncmp(param_name, H2T_RX_BUFFER_SIZE_PARAM, H2T_RX_BUFFER_SIZE_PARAM_LEN) == 0) {
        snprintf(server_conn->buff->ctrl_tx_buff, server_conn->buff->ctrl_tx_buff_sz, "%ld", server_conn->buff->h2t_rx_buff_sz);
        return server_conn->buff->ctrl_tx_buff;
    } else if (strncmp(param_name, MGMT_RX_BUFFER_SIZE_PARAM, MGMT_RX_BUFFER_SIZE_PARAM_LEN) == 0) {
        snprintf(server_conn->buff->ctrl_tx_buff, server_conn->buff->ctrl_tx_buff_sz, "%ld", server_conn->buff->mgmt_rx_buff_sz);
        return server_conn->buff->ctrl_tx_buff;
    } else if (strncmp(param_name, CTRL_RX_BUFFER_SIZE_PARAM, CTRL_RX_BUFFER_SIZE_PARAM_LEN) == 0) {
        snprintf(server_conn->buff->ctrl_tx_buff, server_conn->buff->ctrl_tx_buff_sz, "%ld", server_conn->buff->ctrl_rx_buff_sz);
        return server_conn->buff->ctrl_tx_buff;
    } else if (strncmp(param_name, T2H_NAGLE_PARAM, T2H_NAGLE_PARAM_LEN) == 0) {
        snprintf(server_conn->buff->ctrl_tx_buff, server_conn->buff->ctrl_tx_buff_sz, "%d", (int)(server_conn->t2h_nagle));
        return server_conn->buff->ctrl_tx_buff;
    } else if (strncmp(param_name, MGMT_RSP_NAGLE_PARAM, MGMT_RSP_NAGLE_PARAM_LEN) == 0) {
        snprintf(server_conn->buff->ctrl_tx_buff, server_conn->buff->ctrl_tx_buff_sz, "%d", (int)(server_conn->mgmt_rsp_nagle));
        return server_conn->buff->ctrl_tx_buff;
    } else {
        return GET_PARAM_CMD_FAIL_RSP;
    }
}

const char *set_parameter(char *cmd, SERVER_CONN *server_conn, CLIENT_CONN *client_conn) {
    const char *param_name = strstr(cmd, SET_PARAM_CMD) + SET_PARAM_CMD_LEN;
    const char *param_value;
    if (strstr(param_name, SERVER_LOOPBACK_MODE_PARAM) == param_name) {
        param_value = param_name + SERVER_LOOPBACK_MODE_PARAM_LEN;
        if (strnlen(param_value, 1) == 1) {
            server_conn->loopback_mode = (*param_value == '1' ? 1 : 0);
            return SET_PARAM_CMD_RSP;
        }
    } else if (strstr(param_name, T2H_NAGLE_PARAM) == param_name) {
        param_value = param_name + T2H_NAGLE_PARAM_LEN;
        if (strnlen(param_value, 1) == 1) {
            const char t2h_nagle = (*param_value == '1' ? 1 : 0);
            if (set_tcp_no_delay(client_conn->t2h_data_fd, t2h_nagle ? 0 : 1) == 0) {
                server_conn->t2h_nagle = t2h_nagle;
                return SET_PARAM_CMD_RSP;
            }
        }
    } else if (strstr(param_name, MGMT_RSP_NAGLE_PARAM) == param_name) {
        param_value = param_name + MGMT_RSP_NAGLE_PARAM_LEN;
        if (strnlen(param_value, 1) == 1) {
            const char mgmt_rsp_nagle = (*param_value == '1' ? 1 : 0);
            if (set_tcp_no_delay(client_conn->mgmt_rsp_fd, mgmt_rsp_nagle ? 0 : 1) == 0) {
                server_conn->mgmt_rsp_nagle = mgmt_rsp_nagle;
                return SET_PARAM_CMD_RSP;
            }
        }
    }
    return SET_PARAM_CMD_FAIL_RSP;
}

const char *get_driver_parameter(char *cmd, SERVER_CONN *server_conn) {
    if (server_conn->hw_callbacks.get_param != NULL) {
        const char *param_name = strstr(cmd, GET_DRIVER_PARAM_CMD) + GET_DRIVER_PARAM_CMD_LEN;
        const char *result = server_conn->hw_callbacks.get_param(param_name);
        if (result != NULL) {
            return result;
        }
    }
    return GET_PARAM_CMD_FAIL_RSP;
}

const char *set_driver_parameter(char *cmd, SERVER_CONN *server_conn) {
    enum { MAX_DRIVER_PARAM_NAME_LEN = 32 };
    char param_name_buff[MAX_DRIVER_PARAM_NAME_LEN + 1] = { 0 };

    if (server_conn->hw_callbacks.set_param != NULL) {
        const char *param_name = strstr(cmd, SET_DRIVER_PARAM_CMD) + SET_DRIVER_PARAM_CMD_LEN;
        const char *param_value = strstr(param_name, " ") + 1;
        size_t param_name_len = (size_t)(param_value - param_name) - 1;
        for (size_t i = 0; i < MIN_MACRO(param_name_len, MAX_DRIVER_PARAM_NAME_LEN); ++i) {
            param_name_buff[i] = param_name[i];
        }
        if (server_conn->hw_callbacks.set_param(param_name_buff, param_value) >= 0) {
            return SET_PARAM_CMD_RSP;
        }
    }

    return SET_PARAM_CMD_FAIL_RSP;
}

RETURN_CODE process_control_message(CLIENT_CONN *client_conn, SERVER_CONN *server_conn, char *disconnect_client) {
    ssize_t bytes_transferred;
    RETURN_CODE result;
    if (socket_recv_until_null_reached(client_conn->ctrl_fd, server_conn->buff->ctrl_rx_buff, server_conn->buff->ctrl_rx_buff_sz, 0, &bytes_transferred) == OK) {
        if (strstr(server_conn->buff->ctrl_rx_buff, GET_PARAM_CMD) == server_conn->buff->ctrl_rx_buff) {
            const char *resp = get_parameter(server_conn->buff->ctrl_rx_buff, server_conn);
            result = socket_send_all(client_conn->ctrl_fd, resp, strnlen(resp, MAX_SERVER_PARAM_VALUE_LEN) + 1, 0, &bytes_transferred);
        } else if (strncmp(server_conn->buff->ctrl_rx_buff, PING_CMD, PING_CMD_LEN) == 0) {
            result = socket_send_all(client_conn->ctrl_fd, PING_CMD_RSP, PING_CMD_RSP_LEN, 0, &bytes_transferred);
        } else if (strncmp(server_conn->buff->ctrl_rx_buff, DISCONNECT_CMD, DISCONNECT_CMD_LEN) == 0) {
            socket_send_all(client_conn->ctrl_fd, DISCONNECT_CMD_RSP, DISCONNECT_CMD_RSP_LEN, 0, NULL);
            wait_for_read_event(client_conn->ctrl_fd, 10, 0); // Wait 10 seconds at most for the client to close first
            *disconnect_client = 1;
            result = OK;
        } else if (strstr(server_conn->buff->ctrl_rx_buff, SET_PARAM_CMD) == server_conn->buff->ctrl_rx_buff) {
            const char *resp = set_parameter(server_conn->buff->ctrl_rx_buff, server_conn, client_conn);
            result = socket_send_all(client_conn->ctrl_fd, resp, strnlen(resp, 128) + 1, 0, &bytes_transferred);
        } else if (strstr(server_conn->buff->ctrl_rx_buff, SET_DRIVER_PARAM_CMD) == server_conn->buff->ctrl_rx_buff) {
            const char *resp = set_driver_parameter(server_conn->buff->ctrl_rx_buff, server_conn);
            result = socket_send_all(client_conn->ctrl_fd, resp, strnlen(resp, 256) + 1, 0, &bytes_transferred);
        } else if (strstr(server_conn->buff->ctrl_rx_buff, GET_DRIVER_PARAM_CMD) == server_conn->buff->ctrl_rx_buff) {
            const char *resp = get_driver_parameter(server_conn->buff->ctrl_rx_buff, server_conn);
            result = socket_send_all(client_conn->ctrl_fd, resp, strnlen(resp, MAX_SERVER_PARAM_VALUE_LEN) + 1, 0, &bytes_transferred);
        } else {
            result = socket_send_all(client_conn->ctrl_fd, UNRECOGNIZED_CMD_RSP, UNRECOGNIZED_CMD_RSP_LEN, 0, &bytes_transferred);
        }
        if (result != OK) {
            print_last_socket_error_b("Failed to send CTRL message response", bytes_transferred, server_conn->hw_callbacks.server_printf);
        }
        return result;
    } else {
        print_last_socket_error_b("Failed to recv CTRL message", bytes_transferred, server_conn->hw_callbacks.server_printf);
        return FAILURE;
    }
}

unsigned long buff_len_to_wrap_boundary(char *buff_sa, size_t buff_sz, char *buff, size_t payload_sz) {
    const unsigned long mem_base = (unsigned long)((uintptr_t)buff_sa);
    const unsigned long end_addr = mem_base + buff_sz;
    if (((unsigned long)((uintptr_t)buff) + payload_sz) > end_addr) {
        return (end_addr - (unsigned long)((uintptr_t)buff));
    } else {
        return 0;
    }
}

RETURN_CODE update_curr_h2t_header(CLIENT_CONN *client_conn, SERVER_CONN *server_conn) {
    if (server_conn->h2t_waiting == 0) {
        ssize_t bytes_recvd;
        RETURN_CODE result = socket_recv_accumulate(client_conn->h2t_data_fd, server_conn->buff->h2t_header_buff, SIZEOF_PACKET_GUARDBAND + SIZEOF_H2T_PACKET_HEADER, 0, &bytes_recvd);
        if (result != OK) {
            print_last_socket_error_b("Failed to recv H2T header", bytes_recvd, server_conn->hw_callbacks.server_printf);
        }
        return result;
    }
    return OK;
}

RETURN_CODE process_h2t_data(CLIENT_CONN *client_conn, SERVER_CONN *server_conn) {
    ssize_t bytes_recvd;
    RETURN_CODE has_error = OK;

    if ((has_error = update_curr_h2t_header(client_conn, server_conn)) == OK) {
        H2T_PACKET_HEADER *header = (H2T_PACKET_HEADER *)(server_conn->buff->h2t_header_buff + SIZEOF_PACKET_GUARDBAND);
        size_t bytes_to_transfer = header->DATA_LEN_BYTES;

        // Polls to see if there is room for the packet
        char *h2t_buff = ((server_conn->hw_callbacks.get_h2t_buffer != NULL) && (server_conn->loopback_mode == 0)) ? server_conn->hw_callbacks.get_h2t_buffer(bytes_to_transfer) : server_conn->buff->h2t_rx_buff;

        // Recv H2T payload
        if (h2t_buff != NULL) {
            server_conn->pkt_stats.h2t_cnt++;
            server_conn->h2t_waiting = 0;
            size_t first_len;
            if (server_conn->buff->use_wrapping_data_buffers && ((first_len = buff_len_to_wrap_boundary(server_conn->buff->h2t_rx_buff, server_conn->buff->h2t_rx_buff_sz, h2t_buff, header->DATA_LEN_BYTES)) != 0)) {
                // Wrap, 2 recv necessary
                size_t second_len = header->DATA_LEN_BYTES - first_len;
                has_error = socket_recv_accumulate_h2t_data(client_conn->h2t_data_fd, h2t_buff, first_len, 0, &bytes_recvd);
                if (has_error == OK) {
                    has_error = socket_recv_accumulate_h2t_data(client_conn->h2t_data_fd, server_conn->buff->h2t_rx_buff, second_len, 0, &bytes_recvd);
                }
            } else {
                // No wrap
            	has_error = socket_recv_accumulate_h2t_data(client_conn->h2t_data_fd, h2t_buff, bytes_to_transfer, 0, &bytes_recvd);
            }

            // Push to driver or loopback
            if (has_error == OK) {
                if (server_conn->loopback_mode == 0) {
                    // Normal operation, push the transaction to HW
                    has_error = (server_conn->hw_callbacks.h2t_data_received != NULL) ? server_conn->hw_callbacks.h2t_data_received(header, (unsigned char *)h2t_buff) : OK;
                } else {
                    // Send the header
                    if ((has_error = socket_send_all(client_conn->t2h_data_fd, server_conn->buff->h2t_header_buff, SIZEOF_PACKET_GUARDBAND + SIZEOF_H2T_PACKET_HEADER, 0, &bytes_recvd)) == OK) {
                        // Send the payload
                        if ((has_error = socket_send_all_t2h_data(client_conn->t2h_data_fd, h2t_buff, bytes_to_transfer, 0, &bytes_recvd)) != OK) {
                            print_last_socket_error_b("Failed to send loopback T2H data", bytes_recvd, server_conn->hw_callbacks.server_printf);
                        }
                    } else {
                        print_last_socket_error_b("Failed to send loopback T2H header", bytes_recvd, server_conn->hw_callbacks.server_printf);
                    }
                }
            } else {
                print_last_socket_error_b("Failed to recv H2T data", bytes_recvd, server_conn->hw_callbacks.server_printf);
            }
        } else {
            // Wait for buffer to be available!
            server_conn->h2t_waiting = 1;
        }
    }

    return has_error;
}

RETURN_CODE update_curr_mgmt_header(CLIENT_CONN *client_conn, SERVER_CONN *server_conn) {
    if (server_conn->mgmt_waiting == 0) {
        ssize_t bytes_recvd;
        RETURN_CODE result = socket_recv_accumulate(client_conn->mgmt_fd, server_conn->buff->mgmt_header_buff, SIZEOF_PACKET_GUARDBAND + SIZEOF_MGMT_PACKET_HEADER, 0, &bytes_recvd);
        if (result != OK) {
            print_last_socket_error_b("Failed to recv MGMT header", bytes_recvd, server_conn->hw_callbacks.server_printf);
        }
        return result;
    }
    return OK;
}

RETURN_CODE process_mgmt_data(CLIENT_CONN *client_conn, SERVER_CONN *server_conn) {
    ssize_t bytes_recvd;
    RETURN_CODE has_error = OK;

    if ((has_error = update_curr_mgmt_header(client_conn, server_conn)) == OK) {
        MGMT_PACKET_HEADER *header = (MGMT_PACKET_HEADER *)(server_conn->buff->mgmt_header_buff + SIZEOF_PACKET_GUARDBAND);
        size_t bytes_to_transfer = header->DATA_LEN_BYTES;

        // Polls to see if there is room for the packet
        char *mgmt_buff = ((server_conn->hw_callbacks.get_mgmt_buffer != NULL) && (server_conn->loopback_mode == 0)) ? server_conn->hw_callbacks.get_mgmt_buffer(bytes_to_transfer) : server_conn->buff->mgmt_rx_buff;

        // Recv MGMT payload
        if (mgmt_buff != NULL) {
            server_conn->pkt_stats.mgmt_cnt++;
            server_conn->mgmt_waiting = 0;
            size_t first_len;
            if (server_conn->buff->use_wrapping_data_buffers && ((first_len = buff_len_to_wrap_boundary(server_conn->buff->mgmt_rx_buff, server_conn->buff->mgmt_rx_buff_sz, mgmt_buff, header->DATA_LEN_BYTES)) != 0)) {
                // Wrap, 2 recv necessary
                size_t second_len = header->DATA_LEN_BYTES - first_len;
                has_error = socket_recv_accumulate(client_conn->mgmt_fd, mgmt_buff, first_len, 0, &bytes_recvd);
                if (has_error == OK) {
                    has_error = socket_recv_accumulate(client_conn->mgmt_fd, server_conn->buff->mgmt_rx_buff, second_len, 0, &bytes_recvd);
                }
            } else {
                // No wrap
                has_error = socket_recv_accumulate(client_conn->mgmt_fd, mgmt_buff, bytes_to_transfer, 0, &bytes_recvd);
            }

            // Push to driver or loopback
            if (has_error == OK) {
                if (server_conn->loopback_mode == 0) {
                    // Normal operation, push the transaction to HW
                    has_error = (server_conn->hw_callbacks.mgmt_data_received != NULL) ? server_conn->hw_callbacks.mgmt_data_received(header, (unsigned char *)mgmt_buff) : OK;
                } else {
                    // Send the header
                    if ((has_error = socket_send_all(client_conn->mgmt_rsp_fd, server_conn->buff->mgmt_header_buff, SIZEOF_PACKET_GUARDBAND + SIZEOF_MGMT_PACKET_HEADER, 0, &bytes_recvd)) == OK) {
                        // Send the payload
                        if ((has_error = socket_send_all(client_conn->mgmt_rsp_fd, mgmt_buff, bytes_to_transfer, 0, &bytes_recvd)) != OK) {
                            print_last_socket_error_b("Failed to send loopback MGMT RSP data", bytes_recvd, server_conn->hw_callbacks.server_printf);
                        }
                    } else {
                        print_last_socket_error_b("Failed to send loopback MGMT RSP header", bytes_recvd, server_conn->hw_callbacks.server_printf);
                    }
                }
            } else {
                print_last_socket_error_b("Failed to recv MGMT data", bytes_recvd, server_conn->hw_callbacks.server_printf);
            }
        } else {
            // Wait for buffer to be available!
            server_conn->mgmt_waiting = 1;
        }
    }

    return has_error;
}

RETURN_CODE process_t2h_data(CLIENT_CONN *client_conn, SERVER_CONN *server_conn) {
    ssize_t bytes_sent;
    RETURN_CODE has_error = OK;

    H2T_PACKET_HEADER *header = (H2T_PACKET_HEADER *)(server_conn->buff->t2h_header_buff + SIZEOF_PACKET_GUARDBAND);
    unsigned char *t2h_buff;
    if ((has_error = (server_conn->hw_callbacks.acquire_t2h_data(header, &t2h_buff) == 0) ? OK : FAILURE) == OK) {
        unsigned short curr_payload_bytes;
        if ((curr_payload_bytes = header->DATA_LEN_BYTES) == 0) {
            return has_error;
        }
        server_conn->pkt_stats.t2h_cnt++;
        if ((has_error = socket_send_all(client_conn->t2h_data_fd, (const char *)server_conn->buff->t2h_header_buff, SIZEOF_PACKET_GUARDBAND + SIZEOF_H2T_PACKET_HEADER, 0, &bytes_sent)) == OK) {
            size_t first_len;
            if (server_conn->buff->use_wrapping_data_buffers && ((first_len = buff_len_to_wrap_boundary(server_conn->buff->t2h_tx_buff, server_conn->buff->t2h_tx_buff_sz, (char *)t2h_buff, header->DATA_LEN_BYTES)) != 0)) {
                // Wrap, 2 sends necessary
            	size_t second_len = header->DATA_LEN_BYTES - first_len;
            	if ((has_error = socket_send_all_t2h_data(client_conn->t2h_data_fd, (const char *)t2h_buff, first_len, 0, &bytes_sent)) == OK) {
                    has_error = socket_send_all_t2h_data(client_conn->t2h_data_fd, (const char *)server_conn->buff->t2h_tx_buff, second_len, 0, &bytes_sent);
            	}
            } else {
                // No wrap
                has_error = socket_send_all_t2h_data(client_conn->t2h_data_fd, (const char *)t2h_buff, curr_payload_bytes, 0, &bytes_sent);
            }
            if (has_error == OK) {
                if (server_conn->hw_callbacks.t2h_data_complete != NULL) {
                    server_conn->hw_callbacks.t2h_data_complete();
                }
            }
        }
        if (has_error != OK) {
            print_last_socket_error_b("An error occurred sending T2H data", bytes_sent, server_conn->hw_callbacks.server_printf);
        }
    }

    return has_error;
}

RETURN_CODE process_mgmt_rsp_data(CLIENT_CONN *client_conn, SERVER_CONN *server_conn) {
    ssize_t bytes_sent;
    RETURN_CODE has_error = OK;

    MGMT_PACKET_HEADER *header = (MGMT_PACKET_HEADER *)(server_conn->buff->mgmt_rsp_header_buff + SIZEOF_PACKET_GUARDBAND);
    unsigned char *mgmt_rsp_buff;
    if ((has_error = (server_conn->hw_callbacks.acquire_mgmt_rsp_data(header, &mgmt_rsp_buff) == 0) ? OK : FAILURE) == OK) {
        unsigned short curr_payload_bytes;
        if ((curr_payload_bytes = header->DATA_LEN_BYTES) == 0) {
            return has_error;
        }
        server_conn->pkt_stats.mgmt_rsp_cnt++;
        if ((has_error = socket_send_all(client_conn->mgmt_rsp_fd, (const char *)server_conn->buff->mgmt_rsp_header_buff, SIZEOF_PACKET_GUARDBAND + SIZEOF_MGMT_PACKET_HEADER, 0, &bytes_sent)) == OK) {
            size_t first_len;
            if (server_conn->buff->use_wrapping_data_buffers && ((first_len = buff_len_to_wrap_boundary(server_conn->buff->mgmt_rsp_tx_buff, server_conn->buff->mgmt_rsp_tx_buff_sz, (char *)mgmt_rsp_buff, header->DATA_LEN_BYTES)) != 0)) {
                // Wrap, 2 sends necessary
                size_t second_len = header->DATA_LEN_BYTES - first_len;
                if ((has_error = socket_send_all(client_conn->mgmt_rsp_fd, (const char *)mgmt_rsp_buff, first_len, 0, &bytes_sent)) == OK) {
                    has_error = socket_send_all(client_conn->mgmt_rsp_fd, (const char *)server_conn->buff->mgmt_rsp_tx_buff, second_len, 0, &bytes_sent);
                }
            } else {
                // No wrap
                has_error = socket_send_all(client_conn->mgmt_rsp_fd, (const char *)mgmt_rsp_buff, curr_payload_bytes, 0, &bytes_sent);
            }
            if (has_error == OK) {
                if (server_conn->hw_callbacks.mgmt_rsp_data_complete != NULL) {
                    server_conn->hw_callbacks.mgmt_rsp_data_complete();
                }
            }
        }
        if (has_error != OK) {
            print_last_socket_error_b("An error occurred sending MGMT RSP data", bytes_sent, server_conn->hw_callbacks.server_printf);
        }
    }

    return has_error;
}

void reject_client(SERVER_CONN *server_conn) {
    SOCKET sock_fd = INVALID_SOCKET;
    if((sock_fd = accept(server_conn->server_fd, (struct sockaddr *)(&(server_conn->server_addr)), &sizeof_addr)) == INVALID_SOCKET) {
        print_last_socket_error("Failed to accept additional client", server_conn->hw_callbacks.server_printf);
    } else {
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS
        int flags = 0;
#else
        int flags = MSG_DONTWAIT;
#endif
        size_t reject_msg_len = 12;
        if (send(sock_fd, REJECT_MSG, reject_msg_len, flags) < 0) {
            print_last_socket_error("Failed to send rejection message to additional client", server_conn->hw_callbacks.server_printf);
        }

        // Prevent TIME_WAIT
        set_linger_socket_option(sock_fd, 1, 0);
        close_socket_fd(sock_fd);
    }
}

void handle_client(SERVER_CONN *server_conn, CLIENT_CONN *client_conn) {
    fd_set read_fds;
    fd_set write_fds;
    fd_set except_fds;
    enum { NUM_FDS = 6 };
    SOCKET all_fds[NUM_FDS];
    all_fds[0] = server_conn->server_fd;
    all_fds[1] = client_conn->ctrl_fd;
    all_fds[2] = client_conn->mgmt_fd;
    all_fds[3] = client_conn->mgmt_rsp_fd;
    all_fds[4] = client_conn->h2t_data_fd;
    all_fds[5] = client_conn->t2h_data_fd;
    const char *all_fd_names[NUM_FDS];
    all_fd_names[0] = SERVER_SOCK_NAME;
    all_fd_names[1] = CONTROL_SOCK_NAME;
    all_fd_names[2] = MANAGEMENT_SOCK_NAME;
    all_fd_names[3] = MANAGEMENT_RSP_SOCK_NAME;
    all_fd_names[4] = H2T_SOCK_NAME;
    all_fd_names[5] = T2H_SOCK_NAME;

    SOCKET max_fd = max_of(all_fds, NUM_FDS) + 1;

    while (1) {
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);
        FD_ZERO(&except_fds);
        
        // H2T, MGMT, and server listening socket are read-only
        FD_SET(client_conn->h2t_data_fd, &read_fds);
        FD_SET(client_conn->mgmt_fd, &read_fds);
        FD_SET(server_conn->server_fd, &read_fds);
        
        // T2H & MGMT_RSP are write-only
        FD_SET(client_conn->t2h_data_fd, &write_fds);
        FD_SET(client_conn->mgmt_rsp_fd, &write_fds);
        
        // Ctrl is R/W
        FD_SET(client_conn->ctrl_fd, &read_fds);
        FD_SET(client_conn->ctrl_fd, &write_fds);
        
        // Any socket can have an exception
        FD_SET(client_conn->ctrl_fd, &except_fds);
        FD_SET(client_conn->mgmt_fd, &except_fds);
        FD_SET(client_conn->mgmt_rsp_fd, &except_fds);
        FD_SET(client_conn->h2t_data_fd, &except_fds);
        FD_SET(client_conn->t2h_data_fd, &except_fds);
        
        struct timeval to;
        to.tv_sec = 1;
        to.tv_usec = 0;
        if (select((int)max_fd, &read_fds, &write_fds, &except_fds, &to) < 0) {
            print_last_socket_error("Select failure", server_conn->hw_callbacks.server_printf);
            break;
        }
        
        // First handle exceptional conditions
        char disconnect_client = 0;
        for (int i = 0; i < NUM_FDS; ++i) {
            if (FD_ISSET(all_fds[i], &except_fds)) {
                server_conn->hw_callbacks.server_printf("Exception found on socket: %s\n", all_fd_names[i]);
                disconnect_client = 1;
                break;
            }
        }
        if (disconnect_client) {
            break;
        }
        
        // Check for additional clients attempting to connect,
        // if so, politely tell them to get lost.
        if (FD_ISSET(server_conn->server_fd, &read_fds)) {
            reject_client(server_conn);
        }

        // See if any incoming control messages are present
        if (FD_ISSET(client_conn->ctrl_fd, &read_fds)) {
            if (process_control_message(client_conn, server_conn, &disconnect_client) == FAILURE) {
                break;
            }

            if (disconnect_client) {
                break;
            }
        }
        
        // See if any incoming management commands are present
        if (FD_ISSET(client_conn->mgmt_fd, &read_fds)) {
            if (process_mgmt_data(client_conn, server_conn) == FAILURE) {
                break;
            }
        }

        // Lastly handle incoming H2T data
        if (FD_ISSET(client_conn->h2t_data_fd, &read_fds)) {
            if (process_h2t_data(client_conn, server_conn) == FAILURE) {
                break;
            }
        }

        // See if any outbound management data is present, if so send it out
        if (server_conn->loopback_mode == 0) {
            if (FD_ISSET(client_conn->mgmt_rsp_fd, &write_fds)) {
                if (server_conn->hw_callbacks.acquire_mgmt_rsp_data != NULL) {
                    if (process_mgmt_rsp_data(client_conn, server_conn) == FAILURE) {
                        break;
                    }
                }
            }

            // See if any outbound t2h data is present, if so send it out
            if (FD_ISSET(client_conn->t2h_data_fd, &write_fds)) {
                if (server_conn->hw_callbacks.acquire_t2h_data != NULL) {
                    if (process_t2h_data(client_conn, server_conn) == FAILURE) {
                        break;
                    }
                }
            }
        }
    }
}

RETURN_CODE initialize_server(unsigned short port, SERVER_CONN *server_conn, const char *port_filename) {
    if (initialize_sockets_library() == FAILURE) {
        return FAILURE;
    }

    // Fill the guardband preamble just once
    populate_guardband((unsigned char *)server_conn->buff->mgmt_rsp_header_buff);
    populate_guardband((unsigned char *)server_conn->buff->t2h_header_buff);

    server_conn->server_addr.sin_family = AF_INET;
    server_conn->server_addr.sin_addr.s_addr = INADDR_ANY;
    server_conn->server_addr.sin_port = htons(port);
    sizeof_addr = sizeof(server_conn->server_addr);

    if (bind_server_socket(server_conn) != OK) {
        server_conn->hw_callbacks.server_printf("Failed to bind server socket!\n");
        return FAILURE;
    }

    // If a request to bind to any available port, take note of which port was assigned by the kernel
    if (port == 0) {
        if (getsockname(server_conn->server_fd, (struct sockaddr *)(&(server_conn->server_addr)), &sizeof_addr) < 0) {
            print_last_socket_error("getsockname failed", server_conn->hw_callbacks.server_printf);
        }
    }

    unsigned short port_used = ntohs(server_conn->server_addr.sin_port);
    server_conn->hw_callbacks.server_printf("Server socket is listening on port: %d\n", port_used);

    // Write out the port used.  This is especially useful when an ephermal port is used.
#if STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_WINDOWS || STI_NOSYS_PROT_PLATFORM==STI_PLATFORM_LINUX
    if (port_filename != NULL) {
        FILE *port_fp = fopen(port_filename, "w");
        if (port_fp != NULL) {
            fprintf(port_fp, "%d", port_used);
            fclose(port_fp);
        } else {
            server_conn->hw_callbacks.server_printf("Failed to save out port file: %s\n", port_filename);
        }
    }
#endif

    fflush(stdout);
    return OK;
}

void server_main(SERVER_LIFESPAN lifespan, SERVER_CONN *server_conn) {    
    // Main loop of server app
    do {
        reset_buffers(server_conn);
        CLIENT_CONN client_conn = CLIENT_CONN_default;
        if (connect_client(server_conn, &client_conn) == OK) {
            handle_client(server_conn, &client_conn);
        } else {
            server_conn->hw_callbacks.server_printf("Rejected remote client.\n");
        }
        
        close_client_conn(&client_conn, server_conn);
    } while (lifespan == MULTIPLE_CLIENTS);

    // Close the listening socket
    set_linger_socket_option(server_conn->server_fd, 1, 0);
    close_socket_fd(server_conn->server_fd);
}
