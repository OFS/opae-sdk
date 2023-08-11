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
source fuzz-nlb0.sh
source fuzz-hps.sh
source fuzz-nlb7.sh
source fuzz-object_api.sh
source fuzz-opaevfiotest.sh


fuzz_all() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_all <ITERS> [QUIET]\n"
    exit 1
  fi
  local -i iters=$1

  local -i quiet=0
  if [ $# -gt 1 ]; then
    quiet=$2
  fi

  fuzz_fpgaconf ${iters} ${quiet}
  fuzz_fpgainfo ${iters} ${quiet}
  fuzz_fpgad ${iters} ${quiet}
  fuzz_mmlink ${iters} ${quiet}
  fuzz_hello_fpga ${iters} ${quiet}
  fuzz_hello_events ${iters} ${quiet}
  fuzz_userclk ${iters} ${quiet}
  fuzz_fpgametrics ${iters} ${quiet}
  fuzz_hello_cxxcore ${iters} ${quiet}
  fuzz_n5010_ctl ${iters} ${quiet}
  fuzz_n5010_test ${iters} ${quiet}
  fuzz_host_exerciser ${iters} ${quiet}
  fuzz_opaeuiotest ${iters} ${quiet}
  fuzz_bist_app ${iters} ${quiet}
  fuzz_dummy_afu ${iters} ${quiet}
  fuzz_fpgabist ${iters} ${quiet}
  fuzz_nlb3 ${iters} ${quiet}
  fuzz_fpgadiag ${iters} ${quiet}
  fuzz_fpga_dma_N3000_test ${iters} ${quiet}
  fuzz_fpga_dma_test ${iters} ${quiet}
  fuzz_mem_tg ${iters} ${quiet}
  fuzz_opae_io ${iters} ${quiet}
  fuzz_hps ${iters} ${quiet}
  fuzz_hssi ${iters} ${quiet}
  fuzz_hps ${iters} ${quiet}
  fuzz_nlb0 ${iters} ${quiet}
  fuzz_nlb7 ${iters} ${quiet}
  fuzz_object_api ${iters} ${quiet}
  fuzz_opaevfiotest ${iters} ${quiet}

}

declare -i iters=1
if [ $# -gt 0 ]; then
  iters=$1
fi

declare -i quiet=0
if [ $# -gt 1 ] && [ $2 = '-q' ]; then
  quiet=1
fi

fuzz_all ${iters} ${quiet}
printf "\nAll fuzz tests PASSED\n"
