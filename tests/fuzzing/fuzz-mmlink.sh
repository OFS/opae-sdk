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

# $ mmlink --help
# Usage:
# mmlink
# <Segment>             --segment=<SEGMENT NUMBER>   OR  -S <SEGMENT NUMBER>
# <Bus>                 --bus=<BUS NUMBER>           OR  -B <BUS NUMBER>
# <Device>              --device=<DEVICE NUMBER>     OR  -D <DEVICE NUMBER>
# <Function>            --function=<FUNCTION NUMBER> OR  -F <FUNCTION NUMBER>
# <Socket-id>           --socket-id=<SOCKET NUMBER>  OR  -s <SOCKET NUMBER>
# <TCP PORT>            --port=<PORT>                OR  -P <PORT>
# <IP ADDRESS>          --ip=<IP ADDRESS>            OR  -I <IP ADDRESS>
# <Version>             -v,--version Print version and exit
fuzz_mmlink() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_mmlink <ITERS>\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

  local -a short_parms=(\
'-h' \
'-S 0x0' \
'-S 1' \
'-B 0x0' \
'-B 2' \
'-D 0x0' \
'-D 3' \
'-F 0x0' \
'-F 4' \
'-s 0' \
'-P 0' \
'-I 192.168.0.1' \
'-v'\
)

  local -a long_parms=(\
'--help' \
'--segment 0x0' \
'--segment 1' \
'--bus 0x0' \
'--bus 2' \
'--device 0x0' \
'--device 3' \
'--function 0x0' \
'--function 4' \
'--socket-id 0' \
'--port 0' \
'--ip 0.0.0.0' \
'--version'\
)

  local cmd=''
  local -i num_parms
  local parm=''

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "Fuzz Iteration: %d\n" $i

    cmd='mmlink '
    let "num_parms = 1 + ${RANDOM} % ${#short_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#short_parms[@]}"
      parm="${short_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    printf "%s\n" "${cmd}"
    ${cmd}

    cmd='mmlink '
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
#fuzz_mmlink ${iters}
