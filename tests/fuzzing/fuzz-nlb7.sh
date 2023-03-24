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

# $ nlb7 --help
#Usage: fpgadiag --mode sw [options]:
#
#    --help, -h. Show help
#    --version, -v. Show version
#    --config, -c. Path to test config file Default=nlb7.json
#    --target, -t. one of {fpga, ase} Default=fpga
#    --begin, -b. where 1 <= <value> <= 65535 Default=1
#    --end, -e. where 1 <= <value> <= 65535 Default=1
#    --cache-policy, -p. one of {wrline-I, wrline-M, wrpush-I} Default=wrline-M
#    --cache-hint, -i. one of {rdline-I, rdline-S} Default=rdline-I
#    --read-vc, -r. one of {auto, vl0, vh0, vh1, random} Default=auto
#    --write-vc, -w. one of {auto, vl0, vh0, vh1, random} Default=auto
#    --wrfence-vc, -f. one of {auto, vl0, vh0, vh1} Default=auto
#    --notice, -N. one of {poll, csr-write, umsg-data, umsg-hint} Default=poll
#    --dsm-timeout-usec Timeout for test completion Default=1000000
#    --bus, -B. Bus number of PCIe device
#    --device, -D. Device number of PCIe device
#    --function, -F. Function number of PCIe device
#    --socket-id, -S. Socket id encoded in BBS
#    --guid, -G. accelerator id to enumerate Default=7BAF4DEA-A57C-E91E-168A-455D9BDA88A3
#    --clock-freq, -T. Clock frequency (used for bw measurements) Default=400000000
#    --suppress-hdr Suppress column headers
#    --csv, -V. Comma separated value format
#    --suppress-stats Show stats at end


fuzz_nlb7() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_nlb7 <ITERS>\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

  local -a short_parms=(\
'-h' \
'-v' \
'-c nlb7.json' \
'-t fpga' \
'-t ase' \
'-b 1' \
'-b 65535' \
'-e 1' \
'-e 65535' \
'-p wrline-M' \
'-p wrline-I' \
'-p wrpush-I' \
'-i rdline-I' \
'-i rdline-S' \
'-r auto' \
'-r vl0' \
'-r vh0' \
'-r vh1' \
'-r random' \
'-w auto' \
'-w vl0' \
'-w vh0' \
'-w vh1' \
'-w random' \
'-f vl0' \
'-f vh0' \
'-f vh1' \
'-f random' \
'-N poll' \
'-N csr-write' \
'-N umsg-data' \
'-N umsg-hint' 
'-N ' \
'-S 0' \
'-B 0x0' \
'-B 0' \
'-D 0x0' \
'-D 0' \
'-F 0x0' \
'-F 0' \
'-G 7BAF4DEA-A57C-E91E-168A-455D9BDA88A3' \
'-T 400000000' \
'-V'\
)

  local -a long_parms=(\
'--help' \
'--version' \
'--config nlb7.json' \
'--target fpga' \
'--target ase' \
'--begin 1' \
'--begin 65535' \
'--end 1' \
'--end 65535' \
'--cont' \
'--cache-policy wrline-M' \
'--cache-policy wrline-I' \
'--cache-policy wrpush-I' \
'--cache-hint rdline-I' \
'--cache-hint rdline-S' \
'--read-vc auto' \
'--read-vc vl0' \
'--read-vc vh0' \
'--read-vc vh1' \
'--read-vc random' \
'--write-vc auto' \
'--write-vc vl0' \
'--write-vc vh0' \
'--write-vc vh1' \
'--write-vc random' \
'--wrfence-vc auto' \
'--wrfence-vc vl0' \
'--wrfence-vc vh0' \
'--wrfence-vc vh1' \
'--wrfence-vc random' \
'--dsm-timeout-usec 1000000' \
'--socket-id 0' \
'--bus 0x0' \
'--bus 0' \
'--device 0x0' \
'--device 0' \
'--function 0x0' \
'--function 0' \
'--guid 7BAF4DEA-A57C-E91E-168A-455D9BDA88A3' \
'--clock-freq 400000000' \
'--suppress-hdr' \
'--csv' \
'--suppress-stats'\
)

  local cmd=''
  local -i num_parms
  local parm=''

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "Fuzz Iteration: %d\n" $i

    cmd='nlb7 '
    let "num_parms = 1 + ${RANDOM} % ${#short_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#short_parms[@]}"
      parm="${short_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    printf "%s\n" "${cmd}"
    ${cmd}

    cmd='nlb7 '
    let "num_parms = 1 + ${RANDOM} % ${#long_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#long_parms[@]}"
      parm="${long_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    printf "%s\n" "${cmd}"
    ${cmd}

  done
}


#declare -i iters=1
#if [ $# -gt 0 ]; then
#  iters=$1
#fi
#fuzz_nlb7 ${iters}
