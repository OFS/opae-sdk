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

# $ hssi --help
#
# hssi
# Usage: hssi [OPTIONS] SUBCOMMAND
#
# Options:
#  -h,--help                   Print this help message and exit
#  -p,--pci-address TEXT       [<domain>:]<bus>:<device>.<function>
#  -l,--log-level TEXT:{trace,debug,info,warning,error,critical,off}=info
#                              stdout logging level
#  -s,--shared                 open in shared mode, default is off
#  -t,--timeout UINT=60000     test timeout (msec)
#
# Subcommands:
#  hssi_10g                    hssi 10G test
#                              
#  hssi_100g                   hssi 100G test
#                              
#  pkt_filt_10g                10G Packet Filter test
#                              
#  pkt_filt_100g               100G Packet Filter test
                              
# $ hssi hssi_10g --help
#
# hssi 10G test
#
# Usage: hssi hssi_10g [OPTIONS]
#
# Options:
#   -h,--help                   Print this help message and exit
#   --port UINT:INT in [0 - 7]=99
#                               QSFP Tx/Rx ports
#   --dst-port UINT:INT in [0 - 7]=0
#                               QSFP Rx port
#   --src-port UINT:INT in [0 - 7]=0
#                               QSFP Tx port
#   --eth-loopback TEXT:{on,off}=on
#                               whether to enable loopback on the eth interface
#   --he-loopback TEXT:{on,off}=none
#                               whether to enable loopback in the Hardware Exerciser (HE)
#   --num-packets UINT=1        number of packets
#   --random-length TEXT:{fixed,random}=fixed
#                               packet length randomization
#   --random-payload TEXT:{incremental,random}=incremental
#                               payload randomization
#   --packet-length UINT=64     packet length
#   --src-addr TEXT=11:22:33:44:55:66
#                               source MAC address
#   --dest-addr TEXT=77:88:99:aa:bb:cc
#                               destination MAC address
#   --eth-ifc TEXT=none         ethernet interface name
#   --rnd-seed0 UINT=1592590336 prbs generator [31:0]
#   --rnd-seed1 UINT=1592590337 prbs generator [47:32]
#   --rnd-seed2 UINT=155373     prbs generator [91:64]

# $ hssi hssi_100g --help
#
# hssi 100G test
#
# Usage: hssi hssi_100g [OPTIONS]
#
# Options:
#   -h,--help                   Print this help message and exit
#   --port INT:INT in [0 - 7] ...
#                               QSFP port
#   --eth-loopback TEXT:{on,off}=on
#                               whether to enable loopback on the eth interface
#   --num-packets UINT=1        number of packets
#   --gap TEXT:{random,none}=none
#                              inter-packet gap
#   --pattern TEXT:{random,fixed,increment}=random
#                              pattern mode
#   --src-addr TEXT=11:22:33:44:55:66
#                              source MAC address
#   --dest-addr TEXT=77:88:99:aa:bb:cc
#                              destination MAC address
#  --eth-ifc TEXT=none         ethernet interface name
#  --start-size UINT=64        packet size in bytes (lower limit for incr mode)
#  --end-size UINT=9600        upper limit of packet size in bytes
#  --end-select TEXT:{pkt_num,gen_idle}=pkt_num
#                              end of packet generation control
#  --continuous TEXT:{on,off}=off
#                              continuous mode
#  --contmonitor UINT=0        time period(in seconds) for performance monitor

# $ hssi pkt_filt_10g --help
#
# 10G Packet Filter test
#
# Usage: hssi pkt_filt_10g [OPTIONS]
#
# Options:
#  -h,--help                   Print this help message and exit
#  --dfl-dev TEXT=none         dfl device
#  --port UINT:INT in [0 - 7]=99
#                              QSFP Tx/Rx ports
#  --dst-port UINT:INT in [0 - 7]=0
#                              QSFP Rx port
#  --src-port UINT:INT in [0 - 7]=0
#                              QSFP Tx port
#  --eth-loopback TEXT:{on,off}=on
#                              whether to enable loopback on the eth interface
#  --he-loopback TEXT:{on,off}=none
#                              whether to enable loopback in the Hardware Exerciser (HE)
#  --num-packets UINT=1        number of packets
#  --random-length TEXT:{fixed,random}=fixed
#                              packet length randomization
#  --random-payload TEXT:{incremental,random}=incremental
#                              payload randomization
#  --packet-length UINT=64     packet length
#  --src-addr TEXT=11:22:33:44:55:66
#                              source MAC address
#  --dest-addr TEXT=77:88:99:aa:bb:cc
#                              destination MAC address
#  --eth-ifc TEXT=none         ethernet interface name
#  --rnd-seed0 UINT=1592590336 prbs generator [31:0]
#  --rnd-seed1 UINT=1592590337 prbs generator [47:32]
#  --rnd-seed2 UINT=155373     prbs generator [91:64]

# $ hssi pkt_filt_100g --help
#
# 100G Packet Filter test
#
# Usage: hssi pkt_filt_100g [OPTIONS]
#
# Options:
#  -h,--help                   Print this help message and exit
#  --dfl-dev TEXT=none         dfl device
#  --port INT:INT in [0 - 7] ...
#                              QSFP port
#  --eth-loopback TEXT:{on,off}=on
#                              whether to enable loopback on the eth interface
#  --num-packets UINT=1        number of packets
#  --gap TEXT:{random,none}=none
#                              inter-packet gap
#  --pattern TEXT:{random,fixed,increment}=random
#                              pattern mode
#  --src-addr TEXT=11:22:33:44:55:66
#                              source MAC address
#  --dest-addr TEXT=77:88:99:aa:bb:cc
#                              destination MAC address
#  --eth-ifc TEXT=none         ethernet interface name
#  --start-size UINT=64        packet size in bytes (lower limit for incr mode)
#  --end-size UINT=9600        upper limit of packet size in bytes
#  --end-select TEXT:{pkt_num,gen_idle}=pkt_num
#                              end of packet generation control
#  --continuous TEXT:{on,off}=off
#                              continuous mode
#  --contmonitor UINT=0        time period(in seconds) for performance monitor
fuzz_hssi() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_hssi <ITERS>\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

  local -a short_parms=(\
'-h' \
'-p 0000:00:00.0' \
'-l trace' \
'-l debug' \
'-l info' \
'-l warning' \
'-l error' \
'-l critical' \
'-l off' \
'-s' \
'-t 5000'\
)

  local -a long_parms=(\
'--help' \
'--pci-address 0000:00:00.0' \
'--log-level trace' \
'--log-level debug' \
'--log-level info' \
'--log-level warning' \
'--log-level error' \
'--log-level critical' \
'--log-level off' \
'--shared' \
'--timeout 5000'\
)

  local -a subcommands=(\
'hssi_10g' \
'hssi_100g' \
'pkt_filt_10g' \
'pkt_filt_100g'\
)

  local -a hssi_10g_parms=(\
'--port 0' \
'--dst-port 0' \
'--src-port 0' \
'--eth-loopback on' \
'--eth-loopback off' \
'--he-loopback on' \
'--he-loopback off' \
'--num-packets 1' \
'--random-length fixed' \
'--random-length random' \
'--random-payload incremental' \
'--random-payload random' \
'--packet-length 64' \
'--src-addr 11:22:33:44:55:66' \
'--dest-addr 77:88:99:aa:bb:cc' \
'--eth-ifc eth0' \
'--rnd-seed0 1592590336' \
'--rnd-seed1 1592590337' \
'--rnd-seed2 155373'\
)

  local -a hssi_100g_parms=(\
'--port 0' \
'--eth-loopback on' \
'--eth-loopback off' \
'--num-packets 1' \
'--gap random' \
'--gap none' \
'--pattern random' \
'--pattern fixed' \
'--pattern increment' \
'--src-addr 11:22:33:44:55:66' \
'--dest-addr 77:88:99:aa:bb:cc' \
'--eth-ifc eth0' \
'--start-size 64' \
'--end-size 9600' \
'--end-select pkt_num' \
'--end-select gen_idle' \
'--continuous on' \
'--continuous off' \
'--contmonitor 1'\
)

  local -a pkt_filt_10g_parms=(\
'--port 0' \
'--dst-port 0' \
'--src-port 0' \
'--eth-loopback on' \
'--eth-loopback off' \
'--he-loopback on' \
'--he-loopback off' \
'--num-packets 1' \
'--random-length fixed' \
'--random-length random' \
'--random-payload incremental' \
'--random-payload random' \
'--packet-length 64' \
'--src-addr 11:22:33:44:55:66' \
'--dest-addr 77:88:99:aa:bb:cc' \
'--eth-ifc eth0' \
'--rnd-seed0 1592590336' \
'--rnd-seed1 1592590337' \
'--rnd-seed2 155373' \
'--dfl-dev dfl_dev.0'\
)

  local -a pkt_filt_100g_parms=(\
'--port 0' \
'--eth-loopback on' \
'--eth-loopback off' \
'--num-packets 1' \
'--gap random' \
'--gap none' \
'--pattern random' \
'--pattern fixed' \
'--pattern increment' \
'--src-addr 11:22:33:44:55:66' \
'--dest-addr 77:88:99:aa:bb:cc' \
'--eth-ifc eth0' \
'--start-size 64' \
'--end-size 9600' \
'--end-select pkt_num' \
'--end-select gen_idle' \
'--continuous on' \
'--continuous off' \
'--contmonitor 1' \
'--dfl-dev dfl_dev.0'\
)

  local cmd=''
  local -i num_parms
  local parm=''

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "Fuzz Iteration: %d\n" $i

    cmd='hssi '
    let "num_parms = 1 + ${RANDOM} % ${#short_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#short_parms[@]}"
      parm="${short_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    let "p = ${RANDOM} % ${#subcommands[@]}"
    cmd="${cmd} ${subcommands[$p]}"
    case "${subcommands[$p]}" in
      'hssi_10g')
        let "num_parms = 1 + ${RANDOM} % ${#hssi_10g_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#hssi_10g_parms[@]}"
          parm="${hssi_10g_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'hssi_100g')
        let "num_parms = 1 + ${RANDOM} % ${#hssi_100g_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#hssi_100g_parms[@]}"
          parm="${hssi_100g_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'pkt_filt_10g')
        let "num_parms = 1 + ${RANDOM} % ${#pkt_filt_10g_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#pkt_filt_10g_parms[@]}"
          parm="${pkt_filt_10g_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'pkt_filt_100g')
        let "num_parms = 1 + ${RANDOM} % ${#pkt_filt_100g_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#pkt_filt_100g_parms[@]}"
          parm="${pkt_filt_100g_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;
    esac

    printf "%s\n" "${cmd}"
    ${cmd}

    cmd='hssi '
    let "num_parms = 1 + ${RANDOM} % ${#long_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#long_parms[@]}"
      parm="${long_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    let "p = ${RANDOM} % ${#subcommands[@]}"
    cmd="${cmd} ${subcommands[$p]}"
    case "${subcommands[$p]}" in
      'hssi_10g')
        let "num_parms = 1 + ${RANDOM} % ${#hssi_10g_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#hssi_10g_parms[@]}"
          parm="${hssi_10g_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'hssi_100g')
        let "num_parms = 1 + ${RANDOM} % ${#hssi_100g_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#hssi_100g_parms[@]}"
          parm="${hssi_100g_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'pkt_filt_10g')
        let "num_parms = 1 + ${RANDOM} % ${#pkt_filt_10g_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#pkt_filt_10g_parms[@]}"
          parm="${pkt_filt_10g_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'pkt_filt_100g')
        let "num_parms = 1 + ${RANDOM} % ${#pkt_filt_100g_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#pkt_filt_100g_parms[@]}"
          parm="${pkt_filt_100g_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;
    esac

    printf "%s\n" "${cmd}"
    ${cmd}

  done
}

#declare -i iters=1
#if [ $# -gt 0 ]; then
#  iters=$1
#fi
#fuzz_hssi ${iters}
