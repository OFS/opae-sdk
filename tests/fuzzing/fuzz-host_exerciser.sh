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

# $ host_exerciser --help
#
# host_exerciser
# HE-LB and HE-MEM are used to exercise AFU and DDR
#
#Options:
#  -h,--help                   Print this help message and exit
#  -p,--pci-address TEXT       [<domain>:]<bus>:<device>.<function>
#  -l,--log-level TEXT:{trace,debug,info,warning,error,critical,off} [warning]
#                              stdout logging level
#  -s,--shared                 open in shared mode, default is off
#  -t,--timeout UINT [60000]   test timeout (msec)
#  -m,--mode UINT:value in {lpbk->0,read->1,trput->3,write->2} OR {0,1,3,2} [lpbk]
#                              host exerciser mode {lpbk,read, write, trput}
#  --cls UINT:value in {cl_1->0,cl_2->1,cl_4->2,cl_8->3} OR {0,1,2,3} [cl_1]
#                              number of CLs per request{cl_1, cl_2, cl_4, cl_8}
#  --continuousmode BOOLEAN [false]
#                              test rollover or test termination
#  --atomic UINT:value in {cas_4->9,cas_8->11,fadd_4->1,fadd_8->3,off->0,swap_4->5,swap_8->7} OR {9,11,1,3,0,5,7} [off]
#  --encoding UINT:value in {default->0,dm->1,pu->2,random->3} OR {0,1,2,3} [default]
#                              data mover or power user encoding -- random interleaves both in the same stream
#  -d,--delay BOOLEAN [false]  Enables random delay insertion between requests
#  --interleave UINT:INT in [0 - 2]
#                              Interleave requests pattern to use in throughput mode {0, 1, 2}
#                              indicating one of the following series of read/write requests:
#                              0: rd-wr-rd-wr
#                              1: rd-rd-wr-wr
#  --interrupt UINT:INT in [0 - 4095]
#                              The Interrupt Vector Number for the device
#  --contmodetime UINT [1]     Continuous mode time in seconds
#  --testall BOOLEAN [false]   Run all tests
#  --clock-mhz UINT [0]        Clock frequency (MHz) -- when zero, read the frequency from the AFU
#Subcommands:
#  lpbk                        run simple loopback test
#  mem                         run simple mem test


fuzz_host_exerciser() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_host_exerciser <ITERS>\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

  local -a short_parms=(\
'-h' \
'lpbk' \
'mem' \
'-p 0000:00:00.0 lpbk' \
'-p 0000:00:00.0 mem' \
'-p 0000:b1:00.1 lpbk' \
'-p 0000:b1:00.4 mem' \
'-l trace lpbk' \
'-l trace mem' \
'-l debug lpbk' \
'-l debug mem' \
'-l info lpbk' \
'-l info mem' \
'-l warning lpbk' \
'-l warning mem' \
'-l critical lpbk' \
'-l critical mem' \
'-l off lpbk' \
'-l off mem' \
'-s lpbk' \
'-s mem' \
'-t 10 lpbk' \
'-t 10 mem' \
'-t 100 lpbk' \
'-t 100 mem' \
'-d lpbk' \
'-d mem' \
'-m 0 lpbk' \
'-m 0 mem' \
'-m 1 lpbk' \
'-m 1 mem' \
'-m 2 lpbk' \
'-m 2 mem' \
'-m 3 lpbk' \
'-m 3 mem' \
'-m lpbk lpbk' \
'-m lpbk mem' \
'-m read lpbk' \
'-m read mem' \
'-m trput lpbk' \
'-m trput mem' \
'-m write lpbk' \
'-m write mem'\
)

  local -a long_parms=(\
'--help' \
'lpbk' \
'mem' \
'--pci-address 0000:00:00.0 lpbk' \
'--pci-address 0000:00:00.0 mem' \
'--pci-address 0000:b1:00.1 lpbk' \
'--pci-address 0000:b1:00.4 mem' \
'--log-level trace lpbk' \
'--log-level trace mem' \
'--log-level debug lpbk' \
'--log-level debug mem' \
'--log-level info lpbk' \
'--log-level info mem' \
'--log-level warning lpbk' \
'--log-level warning mem' \
'--log-level critical lpbk' \
'--log-level critical mem' \
'--log-level off lpbk' \
'--log-level off mem' \
'--shared lpbk' \
'--shared mem' \
'--timeout 10 lpbk' \
'--timeout 10 mem' \
'--timeout 100 lpbk' \
'--timeout 100 mem' \
'--delay lpbk' \
'--delay mem' \
'--mode lpbk lpbk' \
'--mode lpbk mem' \
'--mode read lpbk' \
'--mode read mem' \
'--mode trput lpbk' \
'--mode trput mem' \
'--mode write lpbk' \
'--mode write mem' \
'--mode 0 lpbk' \
'--mode 0 mem' \
'--mode 1 lpbk' \
'--mode 1 mem' \
'--mode 2 lpbk' \
'--mode 2 mem' \
'--mode 3 lpbk' \
'--mode 3 mem' \
'--cls cl_1 lpbk' \
'--cls cl_1 mem' \
'--cls cl_2 lpbk' \
'--cls cl_2 mem' \
'--cls cl_4 lpbk' \
'--cls cl_4 mem' \
'--cls cl_8 lpbk' \
'--cls cl_8 mem' \
'--continuousmode lpbk' \
'--continuousmode mem' \
'--atomic cas_4 lpbk' \
'--atomic cas_4 mem' \
'--atomic cas_8 lpbk' \
'--atomic cas_8 mem' \
'--atomic fadd_4 lpbk' \
'--atomic fadd_4 mem' \
'--atomic fadd_8 lpbk' \
'--atomic fadd_8 mem' \
'--atomic off lpbk' \
'--atomic off mem' \
'--atomic swap_4 lpbk' \
'--atomic swap_4 mem' \
'--atomic swap_8 lpbk' \
'--atomic swap_8 mem' \
'--encoding default lpbk' \
'--encoding default mem' \
'--encoding dm lpbk' \
'--encoding dm mem' \
'--encoding pu lpbk' \
'--encoding pu mem' \
'--encoding random lpbk' \
'--encoding random mem' \
'--interleave 0 lpbk' \
'--interleave 0 mem' \
'--interleave 1 lpbk' \
'--interleave 1 mem' \
'--interleave 2 lpbk' \
'--interleave 2 mem' \
'--interrupt 0 lpbk' \
'--interrupt 0 mem' \
'--interrupt 1 lpbk' \
'--interrupt 1 mem' \
'--interrupt 200 lpbk' \
'--interrupt 200 mem' \
'--continuousmode --contmodetime 10 lpbk' \
'--continuousmode --contmodetime 10 mem' \
'--continuousmode --contmodetime 1 lpbk' \
'--continuousmode --contmodetime 1 mem' \
'--continuousmode --contmodetime 200 lpbk' \
'--continuousmode --contmodetime 200 mem' \
'--testall lpbk' \
'--testall mem' \
'--clock-mhz 400 lpbk' \
'--clock-mhz 400 mem' \
'--clock-mhz 10000 lpbk' \
'--clock-mhz 10000 mem'\
)

  local cmd=''
  local -i num_parms
  local parm=''

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "Fuzz Iteration: %d\n" $i

    cmd='host_exerciser '
    let "num_parms = 1 + ${RANDOM} % ${#short_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#short_parms[@]}"
      parm="${short_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    printf "%s\n" "${cmd}"
    ${cmd}

    cmd='host_exerciser '
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
#fuzz_host_exerciser ${iters}
