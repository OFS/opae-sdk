// Copyright(c) 2019-2022, Intel Corporation
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include <poll.h>

extern "C" {
#include "fpgad/api/logging.h"
#include "fpgad/events_api_thread.h"

#define MAX_CLIENT_CONNECTIONS 1023
#define SRV_SOCKET             0
#define FIRST_CLIENT_SOCKET    1

extern struct pollfd pollfds[MAX_CLIENT_CONNECTIONS+1];
extern nfds_t num_fds;

void remove_client(int conn_socket);
}

#define NO_OPAE_C
#include "mock/opae_fixtures.h"

using namespace opae::testing;

class fpgad_events_api_c_p : public opae_base_p<> {
 protected:

  virtual void SetUp() override {
    opae_base_p<>::SetUp();

    log_set(stdout);
  }

  virtual void TearDown() override {
    log_close();

    opae_base_p<>::TearDown();
  }

};

/**
 * @test       remove0
 * @brief      Test: remove_client
 * @details    Test the fn's ability to remove,<br>
 *             clients from various places in the array.<br>
 */
TEST_P(fpgad_events_api_c_p, remove0) {
  int conn_socket = -1;

  // (only one client)
  pollfds[FIRST_CLIENT_SOCKET+0].fd = conn_socket;
  num_fds = 2;

  remove_client(conn_socket);
  EXPECT_EQ(num_fds, 1);

  // (client in middle)
  pollfds[FIRST_CLIENT_SOCKET+0].fd = 0;
  pollfds[FIRST_CLIENT_SOCKET+1].fd = conn_socket;
  pollfds[FIRST_CLIENT_SOCKET+2].fd = conn_socket;
  pollfds[FIRST_CLIENT_SOCKET+3].fd = 3;
  num_fds = 5;

  remove_client(conn_socket);
  EXPECT_EQ(num_fds, 3);
  EXPECT_EQ(pollfds[FIRST_CLIENT_SOCKET+0].fd, 0);
  EXPECT_EQ(pollfds[FIRST_CLIENT_SOCKET+1].fd, 3);

  // (client at end)
  pollfds[FIRST_CLIENT_SOCKET+0].fd = 0;
  pollfds[FIRST_CLIENT_SOCKET+1].fd = 1;
  pollfds[FIRST_CLIENT_SOCKET+2].fd = conn_socket;
  num_fds = 4;

  remove_client(conn_socket);
  EXPECT_EQ(num_fds, 3);
  EXPECT_EQ(pollfds[FIRST_CLIENT_SOCKET+0].fd, 0);
  EXPECT_EQ(pollfds[FIRST_CLIENT_SOCKET+1].fd, 1);

  num_fds = 1;
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(fpgad_events_api_c_p);
INSTANTIATE_TEST_SUITE_P(fpgad_events_api_c, fpgad_events_api_c_p,
                         ::testing::ValuesIn(test_platform::platforms({ "skx-p" })));
