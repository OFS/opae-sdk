#ifndef STI_NOSYS_PROT_CONSTANTS_H_INCLUDED
#define STI_NOSYS_PROT_CONSTANTS_H_INCLUDED

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Socket names
extern const char *SERVER_SOCK_NAME;
extern const char *CONTROL_SOCK_NAME;
extern const char *MANAGEMENT_SOCK_NAME;
extern const char *MANAGEMENT_RSP_SOCK_NAME;
extern const char *H2T_SOCK_NAME;
extern const char *T2H_SOCK_NAME;

// Server control messages
extern const char *READY_MSG;
extern const size_t READY_MSG_LEN;
extern const char *NOT_READY_MSG;
extern const size_t NOT_READY_MSG_LEN;
extern const char *REJECT_MSG;
extern const size_t REJECT_MSG_LEN;

// Control commands
extern const char *PING_CMD;
extern const size_t PING_CMD_LEN;
extern const char *GET_PARAM_CMD;
extern const size_t GET_PARAM_CMD_LEN;
extern const char *DISCONNECT_CMD;
extern const size_t DISCONNECT_CMD_LEN;
extern const char *SET_PARAM_CMD;
extern const size_t SET_PARAM_CMD_LEN;
extern const size_t MAX_DRIVER_PARAM_VALUE_LEN;
extern const char *SET_DRIVER_PARAM_CMD;
extern const size_t SET_DRIVER_PARAM_CMD_LEN;
extern const char *GET_DRIVER_PARAM_CMD;
extern const size_t GET_DRIVER_PARAM_CMD_LEN;

// Control command responses
extern const char *UNRECOGNIZED_CMD_RSP;
extern const size_t UNRECOGNIZED_CMD_RSP_LEN;
extern const char *PING_CMD_RSP;
extern const size_t PING_CMD_RSP_LEN;
extern const char *SET_PARAM_CMD_RSP;
extern const size_t SET_PARAM_CMD_RSP_LEN;
extern const char *SET_PARAM_CMD_FAIL_RSP;
extern const size_t SET_PARAM_CMD_FAIL_RSP_LEN;
extern const char *GET_PARAM_CMD_FAIL_RSP;
extern const size_t GET_PARAM_CMD_FAIL_RSP_LEN;
extern const char *DISCONNECT_CMD_RSP;
extern const size_t DISCONNECT_CMD_RSP_LEN;

// Server control params
extern const size_t MAX_SERVER_PARAM_VALUE_LEN;
extern const char *SERVER_LOOPBACK_MODE_PARAM;
extern const size_t SERVER_LOOPBACK_MODE_PARAM_LEN;
extern const char *H2T_RX_BUFFER_SIZE_PARAM;
extern const size_t H2T_RX_BUFFER_SIZE_PARAM_LEN;
extern const char *MGMT_RX_BUFFER_SIZE_PARAM;
extern const size_t MGMT_RX_BUFFER_SIZE_PARAM_LEN;
extern const char *CTRL_RX_BUFFER_SIZE_PARAM;
extern const size_t CTRL_RX_BUFFER_SIZE_PARAM_LEN;
extern const char *T2H_NAGLE_PARAM;
extern const size_t T2H_NAGLE_PARAM_LEN;
extern const char *MGMT_RSP_NAGLE_PARAM;
extern const size_t MGMT_RSP_NAGLE_PARAM_LEN;
extern const char *MGMT_SUPPORT_PARAM;
extern const size_t MGMT_SUPPORT_PARAM_LEN;

// Global ST Host params
#define HOSTNAMES_PARAM "hostnames"
#define HOSTNAMES_PARAM_DEFAULT ""

#define ADDRESSES_PARAM "server_addresses"
#define ADDRESSES_PARAM_DEFAULT ""

#define CONNECT_TIMEOUT_SECONDS_PARM "conn_timeout_s"
#define CONNECT_TIMEOUT_SECONDS_PARM_DEFAULT "5"

// Instance ST Host params
#define ENABLE_H2T_NAGLE "enable_h2t_nagle"
#define ENABLE_H2T_NAGLE_DEFAULT "0"

#define ENABLE_MGMT_NAGLE "enable_mgmt_nagle"
#define ENABLE_MGMT_NAGLE_DEFAULT "0"

#ifdef __cplusplus    
}
#endif

#endif //STI_NOSYS_PROT_CONSTANTS_H_INCLUDED
