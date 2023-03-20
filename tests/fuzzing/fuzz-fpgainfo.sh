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


#FPGA information utility (fpgainfo)
#
#fpgainfo --help
#Usage:
#        fpgainfo [-h] [-S <segment>] [-B <bus>] [-D <device>] [-F <function>] [PCI_ADDR]
#                 {errors,power,temp,fme,port,bmc,mac,phy,security,events}
#
#                -h,--help           Print this help
#                -v,--version        Print version and exit
#                -S,--segment        Set target segment
#                -B,--bus            Set target bus
#                -D,--device         Set target device
#                -F,--function       Set target function
#
#Subcommands:
#
#Print and clear errors
#        fpgainfo errors [-h] [-c] {all,fme,port}
#
#                -h,--help           Print this help
#                -c,--clear          Clear all errors
#                --force             Retry clearing errors 64 times
#                                    to clear certain error conditions
#
#Print power metrics
#        fpgainfo power [-h]
#                -h,--help           Print this help
#
#Print thermal metrics
#        fpgainfo temp [-h]
#                -h,--help           Print this help
#
#Print FME information
#        fpgainfo fme [-h]
#                -h,--help           Print this help
#
#Print accelerator port information
#        fpgainfo port [-h]
#                -h,--help           Print this help
#
#Print all Board Management Controller sensor values
#        fpgainfo bmc [-h]
#                -h,--help           Print this help
#
#Print MAC information
#        fpgainfo mac [-h]
#               -h,--help           Print this help
#
#Print PHY information
#        fpgainfo phy [-h] [-G <group-number>]
#                -h,--help           Print this help
#                -G,--group          Select PHY group {0,1,all}
#
#Print security information
#        fpgainfo security [-h]
#                -h,--help           Print this help
#
#Print event log
#        fpgainfo events [-h]}
#                -l,--list              List boots (implies --all)
#                -b,--boot              Boot index to use, i.e:
#                                         0 for current boot (default),
#                                         1 for previous boot, etc
#                -c,--count             Number of events to print
#                -a,--all               Print all events
#                -s,--sensors           Print sensor data too
#                -s,--sensors           Print sensor data too
#                -s,--sensors           Print sensor data too
#                -s,--sensors           Print sensor data too
#                -i,--bits              Print bit values too
#                -h,--help           Print this help


fuzz_fpgainfo() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_fpgainfo <ITERS>\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

# fpgainfo short command line parameters
  local -a short_parms=(\
'-h' \
'-v' \
'-S 0x0' \
'-S 1' \
'-B 0x0' \
'-B 2' \
'-D 0x0' \
'-D 3' \
'-F 0x0' \
'-F 4' \
'errors -h' \
'errors -c' \
'errors  --force' \
'errors  -c all' \
'errors  -c fme' \
'errors  -c port' \
'power -h' \
'temp -h' \
'fme -h' \
'port -h' \
'bmc -h' \
'mac -h' \
'phy -h' \
'security -h' \
'events -h' \
'phy -G 0' \
'phy -G 1' \
'phy -G 0' \
'phy -G 0x100' \
'phy -G all' \
'events -l' \
'events -b' \
'events -b 0' \
'events -b 0x63' \
'events -b 0x200' \
'events -c' \
'events -c 0' \
'events -c 1' \
'events -c 10' \
'events -c 100' \
'events -a' \
'events -s' \
'events -i' \
)

# fpgainfo long command line parameters
  local -a long_parms=(\
'--help' \
'--version' \
'--segment 0x0' \
'--segment 1' \
'--bus 0x0' \
'--bus 2' \
'--device 0x0' \
'--device 3' \
'--function 0x0' \
'--function 4' \
'errors' \
'errors --help' \
'errors port' \
'errors all' \
'errors fme' \
'errors  --force' \
'errors  --clear' \
'errors  --clear all' \
'errors  --clear fme' \
'errors  --clear port' \
'errors  --force all' \
'errors  --force fme' \
'errors  --force port' \
'power --help' \
'temp --help' \
'fme --help' \
'port --help' \
'bmc --help' \
'mac --help' \
'phy --help' \
'security --help' \
'events --help' \
'port' \
'power' \
'temp' \
'fme' \
'port' \
'bmc' \
'mac' \
'phy' \
'security' \
'events' \
'phy --group 0' \
'phy --group 1' \
'phy --group all' \
'phy --group 0x100' \
'events --list' \
'events --boot' \
'events --boot 0' \
'events --boot 0x63' \
'events --boot 0x200' \
'events --count' \
'events --count 0' \
'events --count 1' \
'events --count 10' \
'events --count 100' \
'events --all ' \
'events --sensors' \
'events --bits' \
)


  local cmd=''
  local -i num_parms
  local parm=''

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "Fuzz Iteration: %d\n" $i

    cmd='fpgainfo '
    let "num_parms = ${RANDOM} % ${#short_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#short_parms[@]}"
      parm="${short_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    printf "%s\n" "${cmd}"
    ${cmd}

    cmd='fpgainfo '
    let "num_parms = ${RANDOM} % ${#long_parms[@]}"
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
#fuzz_fpgainfo ${iters}