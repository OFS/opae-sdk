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
#include "constants.h"

// Socket names
const char *SERVER_SOCK_NAME = "Server";
const char *CONTROL_SOCK_NAME = "Control";
const char *MANAGEMENT_SOCK_NAME = "Management";
const char *MANAGEMENT_RSP_SOCK_NAME = "Management Response";
const char *H2T_SOCK_NAME = "H2T";
const char *T2H_SOCK_NAME = "T2H";

/**
    Note: all string lengths include the NULL terminator.
    Since this is the most common usage might as well save
    some cycles while the program is running and avoid the
    "+ 1" operation all over the place
*/

// Server control messages
const char *READY_MSG = "READY";
const size_t READY_MSG_LEN = 6;
const char *NOT_READY_MSG = "NOT_READY";
const size_t NOT_READY_MSG_LEN = 10;
const char *REJECT_MSG = "SERVER_BUSY";
const size_t REJECT_MSG_LEN = 12;

// Control commands
const char *PING_CMD = "PING";
const size_t PING_CMD_LEN = 5;
const char *GET_PARAM_CMD = "GET_PARAM";
const size_t GET_PARAM_CMD_LEN = 10;
const char *DISCONNECT_CMD = "DISCONNECT";
const size_t DISCONNECT_CMD_LEN = 11;
const char *SET_PARAM_CMD = "SET_PARAM";
const size_t SET_PARAM_CMD_LEN = 10;
const size_t MAX_DRIVER_PARAM_VALUE_LEN = 256;
const char *SET_DRIVER_PARAM_CMD = "SET_DRIVER_PARAM";
const size_t SET_DRIVER_PARAM_CMD_LEN = 17;
const char *GET_DRIVER_PARAM_CMD = "GET_DRIVER_PARAM";
const size_t GET_DRIVER_PARAM_CMD_LEN = 17;

// Control command responses
const char *UNRECOGNIZED_CMD_RSP = "UNRECOGNIZED_COMMAND";
const size_t UNRECOGNIZED_CMD_RSP_LEN = 21;
const char *PING_CMD_RSP = "PONG";
const size_t PING_CMD_RSP_LEN = 5;
const char *SET_PARAM_CMD_RSP = "SET_PARAM_ACK";
const size_t SET_PARAM_CMD_RSP_LEN = 14;
const char *SET_PARAM_CMD_FAIL_RSP = "SET_PARAM_FAIL_ACK";
const size_t SET_PARAM_CMD_FAIL_RSP_LEN = 19;
const char *GET_PARAM_CMD_FAIL_RSP = "GET_PARAM_FAILURE";
const size_t GET_PARAM_CMD_FAIL_RSP_LEN = 18;
const char *DISCONNECT_CMD_RSP = "DISCONNECT_ACK";
const size_t DISCONNECT_CMD_RSP_LEN = 15;

// Server control params
const size_t MAX_SERVER_PARAM_VALUE_LEN = 256;
const char *SERVER_LOOPBACK_MODE_PARAM = "SERVER_LOOPBACK";
const size_t SERVER_LOOPBACK_MODE_PARAM_LEN = 16;
const char *H2T_RX_BUFFER_SIZE_PARAM = "H2T_RX_BUFF_SZ";
const size_t H2T_RX_BUFFER_SIZE_PARAM_LEN = 15;
const char *MGMT_RX_BUFFER_SIZE_PARAM = "MGMT_RX_BUFF_SZ";
const size_t MGMT_RX_BUFFER_SIZE_PARAM_LEN = 16;
const char *CTRL_RX_BUFFER_SIZE_PARAM = "CTRL_RX_BUFF_SZ";
const size_t CTRL_RX_BUFFER_SIZE_PARAM_LEN = 16;
const char *T2H_NAGLE_PARAM = "T2H_NAGLE";
const size_t T2H_NAGLE_PARAM_LEN = 10;
const char *MGMT_RSP_NAGLE_PARAM = "MGMT_RSP_NAGLE";
const size_t MGMT_RSP_NAGLE_PARAM_LEN = 15;
const char *MGMT_SUPPORT_PARAM = "MGMT_SUPPORT";
const size_t MGMT_SUPPORT_PARAM_LEN = 13;
