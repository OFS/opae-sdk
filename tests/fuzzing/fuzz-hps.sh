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


# hps utility copys hps image from host to HPS
#
#hps --help
#Usage: hps [OPTIONS] SUBCOMMAND
#
#Options:
#  -h,--help                   Print this help message and exit
#  -p,--pci-address TEXT       [<domain>:]<bus>:<device>.<function>
#  -l,--log-level TEXT:{trace,debug,info,warning,error,critical,off} [info]
#                              stdout logging level
#  -s,--shared                 open in shared mode, default is off
#  -t,--timeout UINT [60000]   test timeout (msec)
#
#Subcommands:
#  cpeng                       Run copy engine commands
#  heartbeat                   Check for HPS heartbeat
#hps  cpeng --help
#Run copy engine commands
#Usage: hps cpeng [OPTIONS]
#
#Options:
#  -h,--help                   Print this help message and exit
#  -f,--filename TEXT:FILE [u-boot.itb]
#                              Image file to copy
#  -d,--destination UINT [33554432]
#                              HPS DDR Offset
#  -t,--timeout UINT:UINT [UNIT] [1000000]
#                              Timeout
#  -r,--data-request-limit UINT:{64,128,512,1024} [512]
#                              data request limit is pcie transfer width
#  -c,--chunk UINT [4096]      Chunk size. 0 indicates no chunks
#  --soft-reset                Issue soft reset only
#  --skip-ssbl-verify          Do not wait for ssbl verify
#  --skip-kernel-verify        Do not wait for kernel verify

fuzz_hps() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_hps <ITERS>\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

# hps short command line parameters
  local -a short_parms=(\
'-h' \
'-p 0000:b1:00.4' \
'-p 0000:00:00.0' \
'-l trace' \
'-l debug' \
'-l info' \
'-l warning' \
'-l error' \
'-l critical' \
'-l off' \
'-s' \
'-t 50' \
'-t 60000' \
'cpeng -h' 
'cpeng -f u-boot.itb' \
'cpeng -f u-boot.itb -d 0x2000' \
'cpeng -f u-boot.itb -d 0x2000 -t 100000' \
'cpeng -f u-boot.itb -d 0x2000 -t 100000' \
'cpeng -f u-boot.itb -d 0x2000 -r 64' \
'cpeng -f u-boot.itb -d 0x2000 -r 128' \
'cpeng -f u-boot.itb -d 0x2000 -r 512' \
'cpeng -f u-boot.itb -d 0x2000 -r 1024' \
'cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 0' \
'cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 100' \
'cpeng -f u-boot.itb  -c 100' \
'-p 0000:b1:00.4 cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 100' \
'-p 0000:00:00.0 cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 100' \
'-l trace cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 100' \
'-l debug cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 100' \
'-l info cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 100' \
'-l warning cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 100' \
'-l error cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 100' \
'-l critical cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 100' \
'-l off cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 100' \
'-s cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 100' \
'-t 50 cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 100' \
'-t 60000 cpeng -f u-boot.itb -d 0x2000 -r 1024 -c 100' \
'heartbeat' \
'-p 0000:b1:00.4 heartbeat'\
)

# hps long command line parameters
  local -a long_parms=(\
'--help' \
'--pci-address 0000:b1:00.4' \
'--pci-address 0000:00:00.0' \
'--log-level trace' \
'--log-level debug' \
'--log-level info' \
'--log-level warning' \
'--log-level error' \
'--log-level critical' \
'--log-level off' \
'--shared' \
'--timeout 50' \
'--timeout 60000' \
'cpeng --help' 
'cpeng --filename u-boot.itb' \
'cpeng --filename u-boot.itb --destination 0x2000' \
'cpeng --filename u-boot.itb --destination 0x2000 --timeout 100000' \
'cpeng --filename u-boot.itb --destination 0x2000 --timeout 100000' \
'cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 64' \
'cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 128' \
'cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 512' \
'cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024' \
'cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024 --chunk 0' \
'cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024 --chunk 100' \
'cpeng --filename u-boot.itb --chunk 100' \
'--pci-address 0000:b1:00.4 cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024 --chunk 100' \
'--pci-address 0000:00:00.0 cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024 --chunk 100' \
'--log-level trace cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024 --chunk 100' \
'--log-level debug cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024 --chunk 100' \
'--log-level info cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024 --chunk 100' \
'--log-level warning cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024 --chunk 100' \
'--log-level error cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024 --chunk 100' \
'--log-level critical cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024 --chunk 100' \
'--log-level off cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024 --chunk 100' \
'--shared cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024 --chunk 100' \
'--timeout 50 cpeng --filename u-boot.itb --destination 0x2000 -data-request-limit 1024 --chunk 100' \
'--timeout 60000 cpeng --filename u-boot.itb --destination 0x2000 --data-request-limit 1024 --chunk 100' \
'cpeng --filename u-boot.itb --soft-reset' \
'cpeng --filename u-boot.itb --skip-ssbl-verify' \
'cpeng --filename u-boot.itb --skip-kernel-verify' \
'heartbeat' \
'--pci-address 0000:b1:00.4 heartbeat'\
)


  local cmd=''
  local -i num_parms
  local parm=''

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "Fuzz Iteration: %d\n" $i

    cmd='hps '
    let "num_parms = 1 + ${RANDOM} % ${#short_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#short_parms[@]}"
      parm="${short_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    printf "%s\n" "${cmd}"
    ${cmd}

    cmd='hps '
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
#fuzz_hps ${iters}
