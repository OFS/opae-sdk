// Copyright(c) 2020-2023, Intel Corporation
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
#include <csignal>
#include "hssi_afu.h"
#include "hssi_10g_cmd.h"
#include "hssi_100g_cmd.h"
#include "hssi_200g_400g_cmd.h"
#include "hssi_pkt_filt_10g_cmd.h"
#include "hssi_pkt_filt_100g_cmd.h"

hssi_afu app;

void sig_handler(int signum)
{
  opae::afu_test::command::ptr_t c = app.current_command();
  switch (signum) {
  case SIGINT: // Handling signal interrupt(SIGINT) ctrl+C
    std::cerr << "caught SIGINT" << std::endl;
    if (c)
      c->stop();
    break;

  case SIGTSTP: // Handling signal terminal stop(SIGSTP) ctrl+Z
    std::cerr << "caught SIGTSTP" << std::endl;
    if (c)
      c->stop();
    break;
  }
}

int main(int argc, char *argv[])
{
  int res = 1;
  signal(SIGINT, sig_handler);
  signal(SIGTSTP, sig_handler);
  app.register_command<hssi_10g_cmd>();
  app.register_command<hssi_100g_cmd>();
  app.register_command<hssi_200g_400g_cmd>();
  app.register_command<hssi_pkt_filt_10g_cmd>();
  app.register_command<hssi_pkt_filt_100g_cmd>();
  try {
    res = app.main(argc, argv);
  } catch (std::runtime_error &e) {
    std::cerr << e.what() << std::endl;
  }
  return res;
}
