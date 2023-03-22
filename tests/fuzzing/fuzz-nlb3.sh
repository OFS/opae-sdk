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

# $ nlb3 --help
#
#Usage: fpgadiag --mode {read,write,trput} [options]:
#
#    --help, -h. Show help
#    --version, -v. Show version
#    --config, -c. Path to test config file Default=nlb3.json
#    --target, -t. one of {fpga, ase} Default=fpga
#    --mode, -m. mode {read, write, trput} Default=read
#    --begin, -b. where 1 <= <value> <= 65535 Default=1
#    --end, -e. where 1 <= <value> <= 65535 Default=1
#    --multi-cl, -u. one of {1, 2, 4} Default=1
#    --strided-access, -a. where 1 <= <value> <= 64 Default=1
#    --cont, -L. Continuous mode
#    --warm-fpga-cache, -H. Attempt to prime the cache with hits
#    --cool-fpga-cache, -M. Attempt to prime the cache with misses
#    --cool-cpu-cache, -C. Attempt to prime the cpu cache with misses
#    --cache-policy, -p. one of {wrline-M, wrline-I, wrpush-I} Default=wrline-M
#    --cache-hint, -i. one of {rdline-I, rdline-S} Default=rdline-I
#    --read-vc, -r. one of {auto, vl0, vh0, vh1, random} Default=auto
#    --write-vc, -w. one of {auto, vl0, vh0, vh1, random} Default=auto
#    --wrfence-vc, -f. one of {auto, vl0, vh0, vh1} Default=auto
#    --alt-wr-pattern, -l. use alt wr pattern
#    --dsm-timeout-usec Timeout for test completion Default=1000000
#    --timeout-usec Timeout for continuous mode (microseconds portion)
#    --timeout-msec Timeout for continuous mode (milliseconds portion)
#    --timeout-sec Timeout for continuous mode (seconds portion) Default=1
#    --timeout-min Timeout for continuous mode (minutes portion)
#    --timeout-hour Timeout for continuous mode (hours portion)
#    --socket-id, -S. Socket id encoded in BBS
#    --bus, -B. Bus number of PCIe device
#    --device, -D. Device number of PCIe device
#    --function, -F. Function number of PCIe device
#    --guid, -G. accelerator id to enumerate Default=F7DF405C-BD7A-CF72-22F1-44B0B93ACD18
#    --freq, -T. Clock frequency (used for bw measurements) Default=400000000
#    --suppress-hdr Suppress column headers
#    --csv, -V. Comma separated value format
#    --suppress-stats Show stas at end
fuzz_nlb3() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_nlb3 <ITERS>\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

  local -a short_parms=(\
'-h' \
'-v' \
'-c nlb3.json' \
'-t fpga' \
'-t ase' \
'-m read' \
'-m write' \
'-m trput' \
'-b 1' \
'-b 65535' \
'-e 1' \
'-e 65535' \
'-u 1' \
'-u 2' \
'-u 4' \
'-a 1' \
'-a 64' \
'-L' \
'-H' \
'-M' \
'-C' \
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
'-l' \
'-S 0' \
'-B 0x0' \
'-B 0' \
'-D 0x0' \
'-D 0' \
'-F 0x0' \
'-F 0' \
'-G F7DF405C-BD7A-CF72-22F1-44B0B93ACD18' \
'-T 400000000' \
'-V'\
)

  local -a long_parms=(\
'--help' \
'--version' \
'--config nlb3.json' \
'--target fpga' \
'--target ase' \
'--mode read' \
'--mode write' \
'--mode trput' \
'--begin 1' \
'--begin 65535' \
'--end 1' \
'--end 65535' \
'--multi-cl 1' \
'--multi-cl 2' \
'--multi-cl 4' \
'--strided-access 1' \
'--strided-access 64' \
'--cont' \
'--warm-fpga-cache' \
'--cool-fpga-cache' \
'--cool-cpu-cache' \
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
'--wrfence-vc auto' \
'--wrfence-vc vl0' \
'--wrfence-vc vh0' \
'--wrfence-vc vh1' \
'--wrfence-vc random' \
'--alt-wr-pattern' \
'--dsm-timeout-usec 1000000' \
'--timeout-usec 0' \
'--timeout-msec 0' \
'--timeout-sec 0' \
'--timeout-min 0' \
'--timeout-hour 0' \
'--socket-id 0' \
'--bus 0x0' \
'--bus 0' \
'--device 0x0' \
'--device 0' \
'--function 0x0' \
'--function 0' \
'--guid F7DF405C-BD7A-CF72-22F1-44B0B93ACD18' \
'-T 400000000' \
'--suppress-hdr' \
'--csv' \
'--suppress-stats'\
)

  local cmd=''
  local -i num_parms
  local parm=''

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "Fuzz Iteration: %d\n" $i

    cmd='nlb3 '
    let "num_parms = ${RANDOM} % ${#short_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = 1 + ${RANDOM} % ${#short_parms[@]}"
      parm="${short_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    printf "%s\n" "${cmd}"
    ${cmd}

    cmd='nlb3 '
    let "num_parms = ${RANDOM} % ${#long_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = 1 + ${RANDOM} % ${#long_parms[@]}"
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
#fuzz_nlb3 ${iters}
