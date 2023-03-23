#!/bin/bash
# Copyright(c) 2023, Intel Corporation
#
# Redistribution  and  use  in source  and  binary  forms,  with  or  without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of  source code  must retain the  above copyright notice,
#   this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
# * Neither the name  of Intel Corporation  nor the names of its contributors
#   may be used to  endorse or promote  products derived  from this  software
#   without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
# IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
# LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
# CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
# SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
# INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
# CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

shopt -o -s nounset

source fuzz-fpgaconf.sh
source fuzz-fpgainfo.sh
source fuzz-fpgad.sh
source fuzz-mmlink.sh
source fuzz-hello_fpga.sh
source fuzz-hello_events.sh
source fuzz-userclk.sh
source fuzz-fpgametrics.sh
source fuzz-hello_cxxcore.sh
source fuzz-n5010-ctl.sh
source fuzz-n5010-test.sh
source fuzz-host_exerciser.sh
source fuzz-opaeuiotest.sh
source fuzz-bist_app.sh
source fuzz-dummy_afu.sh
source fuzz-fpgabist.sh
source fuzz-nlb3.sh
source fuzz-fpgadiag.sh
source fuzz-fpga_dma_N3000_test.sh
source fuzz-fpga_dma_test.sh
source fuzz-mem_tg.sh
source fuzz-opae.io.sh
source fuzz-hps.sh
source fuzz-hssi.sh

fuzz_all() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_all <ITERS>\n"
    exit 1
  fi
  local -i iters=$1

  fuzz_fpgaconf ${iters}
  fuzz_fpgainfo ${iters}
  fuzz_fpgad ${iters}
  fuzz_mmlink ${iters}
  fuzz_hello_fpga ${iters}
  fuzz_hello_events ${iters}
  fuzz_userclk ${iters}
  fuzz_fpgametrics ${iters}
  fuzz_hello_cxxcore ${iters}
  fuzz_n5010_ctl ${iters}
  fuzz_n5010_test ${iters}
  fuzz_opaeuiotest ${iters}
  fuzz_bist_app ${iters}
  fuzz_dummy_afu ${iters}
  fuzz_host_exerciser ${iters}
  fuzz_opaeuiotest ${iters}  
  fuzz_bist_app ${iters}  
  fuzz_nlb3 ${iters}
  fuzz_fpgadiag ${iters}
  fuzz_fpgabist ${iters}
  fuzz_fpga_dma_N3000_test ${iters}
  fuzz_fpga_dma_test ${iters}
  fuzz_mem_tg ${iters}
  fuzz_opae_io ${iters}
  fuzz_hps ${iters}  
  fuzz_hssi ${iters}
}

declare -i iters=1
if [ $# -gt 0 ]; then
  iters=$1
fi
fuzz_all ${iters}