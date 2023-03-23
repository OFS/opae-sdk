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

# $ fpga_dma_test --help
#Usage:
#     fpga_dma_test [-h] [-B <bus>] [-D <device>] [-F <function>] [-S <segment>]
#                   -l <loopback on/off> -s <data size (bytes)> -p <payload size (bytes)>
#                   -r <transfer direction> -t <transfer type> [-f <decimation factor>]
#                   -a <FPGA local memory address>
#
#         -h,--help           Print this help
#         -v,--version        Print version and exit
#         -B,--bus            Set target bus number
#         -D,--device         Set target device number
#         -F,--function       Set target function number
#         -S,--segment        Set PCIe segment
#         -s,--data_size      Total data size
#         -p,--payload_size   Payload size per DMA transfer
#         -r,--direction      Transfer direction
#            mtos             Memory to stream (valid for streaming DMA)
#            stom             Stream to memory (valid for streaming DMA)
#            mtom             Write to FPGA local memory and read back (valid for memory-mapped DMA)
#
#         Below options are only valid when -r/--direction is set to mtos/stom:
#
#         -l,--loopback       Loopback mode
#            on               Turn on channel loopback
#            off              Turn off channel loopback (must specify channel using -r/--direction)
#         -t,--type           Transfer type
#            fixed            Deterministic length transfer
#            packet           Packet transfer
#         -f,--decim_factor  Optional decimation factor
#
#         Below options are only valid when -r/--direction is set to mtom:
#
#         -a,--fpga_addr      Address in FPGA local memory (hex format)


fuzz_fpga_dma_test() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_fpga_dma_test <ITERS>\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

# fpga_dma_test short command line parameters
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
'-s 1024' \
'-s 102466' \
'-p 2048' \
'-p 20' \
'-r mtos' \
'-r stom' \
'-r mtom' \
'-l on' \
'-l off' \
'-t fixed' \
'-t packet' \
'-f' \
'-a 0x100'\
)


# fpga_dma_test long command line parameters
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
'--data_size 1024' \
'--data_size 102466' \
'--payload_size 2048' \
'--payload_size 20' \
'--direction mtos' \
'--direction stom' \
'--direction mtom' \
'--loopback on' \
'--loopback off' \
'--type fixed' \
'--type packet' \
'--decim_factor' \
'--fpga_addr 0x100'\
)

  local cmd=''
  local -i num_parms
  local parm=''

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "Fuzz Iteration: %d\n" $i

    cmd='fpga_dma_test '
    let "num_parms = ${RANDOM} % ${#short_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = 1 + ${RANDOM} % ${#short_parms[@]}"
      parm="${short_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    printf "%s\n" "${cmd}"
    ${cmd}

    cmd='fpga_dma_test '
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
#if [ $# -gt 0 ]; then
#  iters=$1
#fi
#fuzz_fpga_dma_test ${iters}
