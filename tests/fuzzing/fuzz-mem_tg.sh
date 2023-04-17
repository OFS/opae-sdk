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


#memory traffic generator (TG) used to exercise memory channels 
#
#mem_tg --help
#Usage: mem_tg [OPTIONS] SUBCOMMAND
#
#Options:
#  -h,--help                   Print this help message and exit
#  -g,--guid TEXT [4DADEA34-2C78-48CB-A3DC-5B831F5CECBB]
#                              GUID
#  -p,--pci-address TEXT       [<domain>:]<bus>:<device>.<function>
#  -l,--log-level TEXT:{trace,debug,info,warning,error,critical,off} [info]
#                              stdout logging level
#  -s,--shared                 open in shared mode, default is off
#  -t,--timeout UINT [60000]   test timeout (msec)
#  -m,--mem-channel UINT [0]   Target memory bank for test to run on (0 indexed)
#  --loops UINT [1]            Number of read/write loops to be run
#  -w,--writes UINT [1]        Number of unique write transactions per loop
#  -r,--reads UINT [1]         Number of unique read transactions per loop
#  -b,--bls UINT [1]           Burst length of each request
#  --stride UINT [1]           Address stride for each sequential transaction
#  --data UINT:value in {fixed->0,prbs15->2,prbs31->3,prbs7->1,rot1->3} OR {0,2,3,1,3} [fixed]
#                              Memory traffic data pattern: fixed, prbs7, prbs15, prbs31, rot1
#  -f,--mem-frequency UINT [0]
#                              Memory traffic clock frequency in MHz
#
#Subcommands:
#  tg_test                     configure & run mem traffic generator test


fuzz_mem_tg() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_mem_tg <ITERS>\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

# mem_tg short command line parameters
  local -a short_parms=(\
'-h' \
'-g 4DADEA34-2C78-48CB-A3DC-5B831F5CECBB tg_test' \
'-p 0000:3b:00.1 tg_test' \
'-p 0000:00:00.0 tg_test' \
'-l trace tg_test' \
'-l debug tg_test' \
'-l info tg_test' \
'-l warning tg_test' \
'-l error tg_test' \
'-l critical tg_test' \
'-l off tg_test' \
'-s tg_test' \
'-t 1000 tg_test' \
'-t 60 tg_test' \
'-m 1 tg_test' \
'-m 20 tg_test' \
'-w 10 tg_test' \
'-w 200 tg_test' \
'-r 10 tg_test' \
'-r 200 tg_test' \
'-b 100 tg_test' \
'-f 400 tg_test'\
)

# mem_tg long command line parameters
  local -a long_parms=(\
'--h' \
'--guid 4DADEA34-2C78-48CB-A3DC-5B831F5CECBB tg_test' \
'--pci-address 0000:3b:00.1 tg_test' \
'--pci-address 0000:00:00.0 tg_test' \
'--log-level trace tg_test' \
'--log-level debug tg_test' \
'--log-level info tg_test' \
'--log-level warning tg_test' \
'--log-level error tg_test' \
'--log-level critical tg_test' \
'--log-level off tg_test' \
'--shared tg_test' \
'--timeout 1000 tg_test' \
'--timeout 60 tg_test' \
'--mem-channel 1 tg_test' \
'--mem-channel 20 tg_test' 
'--loops 200 tg_test' \
'--loops 1000 tg_test' \
'--writes 10 tg_test' \
'--writes 200 tg_test' \
'--reads 10 tg_test' \
'--reads 200 tg_test' \
'--bls 5 tg_test' \
'--stride 9 tg_test' \
'--data fixed tg_test' \
'--data prbs15 tg_test' \
'--data prbs31 tg_test' \
'--data prbs7 tg_test' \
'--data rot1 tg_test' \
'--mem-frequenc 400 tg_test'\
)


  local cmd=''
  local -i num_parms
  local parm=''

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "Fuzz Iteration: %d\n" $i

    cmd='mem_tg '
    let "num_parms = 1 + ${RANDOM} % ${#short_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#short_parms[@]}"
      parm="${short_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    printf "%s\n" "${cmd}"
    ${cmd}

    cmd='mem_tg '
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
#
#if [ $# -gt 0 ]; then
#  iters=$1
#fi
#
#fuzz_mem_tg ${iters}
