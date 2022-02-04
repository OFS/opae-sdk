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
