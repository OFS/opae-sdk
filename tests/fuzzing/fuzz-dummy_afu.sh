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


# AFU Exerciser used to exercise MMIO,DDR,LPBK
#
#dummy_afu --help
#Usage: dummy_afu [OPTIONS] SUBCOMMAND
#
#Options:
#  -h,--help                   Print this help message and exit
#  -g,--guid TEXT [91c2a3a1-4a23-4e21-a7cd-2b36dbf2ed73]
#                              GUID
#  -p,--pci-address TEXT       [<domain>:]<bus>:<device>.<function>
#  -l,--log-level TEXT:{trace,debug,info,warning,error,critical,off} [info]
#                              stdout logging level
#  -s,--shared                 open in shared mode, default is off
#  -t,--timeout UINT [60000]   test timeout (msec)
#  -c,--count UINT [1]         Number of times to run test
#
#Subcommands:
#  mmio                        run mmio test
#  ddr                         run ddr test
#  lpbk                        run simple loopback test


fuzz_dummy_afu() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_dummy_afu <ITERS>\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

# dummy_afu short command line parameters
  local -a short_parms=(\
'-h' \
'-g 91c2a3a1-4a23-4e21-a7cd-2b36dbf2ed73 mmio' \
'-p 0000:00:00.0 mmio' \
'-p 0000:3b:00.1 mmio' \
'-l trace mmio' \
'-l debug mmio' \
'-l info mmio' \
'-l warning mmio' \
'-l error mmio' \
'-l critical mmio' \
'-l off mmio' \
'-s mmio' \
'-t 1000000 mmio' \
'-t 1 mmio ' \
'-c 1 mmio' \
'-c 100 mmio' \
'-g 91c2a3a1-4a23-4e21-a7cd-2b36dbf2ed73 ddr' \
'-p 0000:00:00.0 ddr' \
'-p 0000:3b:00.1 ddr' \
'-l trace ddr' \
'-l debug ddr' \
'-l info ddr' \
'-l warning ddr' \
'-l error ddr' \
'-l critical ddr' \
'-l off ddr' \
'-s ddr' \
'-t 1000000 ddr' \
'-t 1 ddr' \
'-c 1 ddr' \
'-c 100 ddr' \
'-g 91c2a3a1-4a23-4e21-a7cd-2b36dbf2ed73 lpbk' \
'-p 0000:00:00.0 lpbk' \
'-p 0000:3b:00.1 lpbk' \
'-l trace lpbk' \
'-l debug lpbk' \
'-l info lpbk' \
'-l warning lpbk' \
'-l error lpbk' \
'-l critical lpbk' \
'-l off lpbk' \
'-s lpbk' \
'-t 1000000 lpbk' \
'-t 1 lpbk' \
'-c 1 lpbk' \
'-c 100 lpbk'\
)

# dummy_afu long command line parameters
  local -a long_parms=(\
'--help' \
'--guid 91c2a3a1-4a23-4e21-a7cd-2b36dbf2ed73 mmio' \
'--pci-address 0000:00:00.0 mmio' \
'--pci-address 0000:3b:00.1 mmio' \
'--log-level trace mmio' \
'--log-level debug mmio' \
'--log-level info mmio' \
'--log-level warning mmio' \
'--log-level error mmio' \
'--log-level critical mmio' \
'--log-level off mmio' \
'--shared mmio' \
'--timeout 1000000 mmio' \
'--timeout 1 mmio ' \
'--count 1 mmio' \
'--count 100 mmio' \
'--guid 91c2a3a1-4a23-4e21-a7cd-2b36dbf2ed73 ddr' \
'--pci-address 0000:00:00.0 ddr' \
'--pci-address 0000:3b:00.1 ddr' \
'--log-level trace ddr' \
'--log-level debug ddr' \
'--log-level info ddr' \
'--log-level warning ddr' \
'--log-level error ddr' \
'--log-level critical ddr' \
'--log-level off ddr' \
'--shared ddr' \
'--timeout 1000000 ddr' \
'--timeout 1 ddr' \
'--count 1 ddr' \
'--count 100 ddr' \
'--guid 91c2a3a1-4a23-4e21-a7cd-2b36dbf2ed73 lpbk' \
'--pci-address 0000:00:00.0 lpbk' \
'--pci-address 0000:3b:00.1 lpbk' \
'--log-level trace lpbk' \
'--log-level debug lpbk' \
'--log-level info lpbk' \
'--log-level warning lpbk' \
'--log-level error lpbk' \
'--log-level critical lpbk' \
'--log-level off lpbk' \
'--shared lpbk' \
'--timeout 1000000 lpbk' \
'--timeout 1 lpbk' \
'--count 1 lpbk' \
'--count 100 lpbk'\
)

  local cmd=''
  local -i num_parms
  local parm=''

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "Fuzz Iteration: %d\n" $i

    cmd='dummy_afu '
    let "num_parms = ${RANDOM} % ${#short_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#short_parms[@]}"
      parm="${short_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    printf "%s\n" "${cmd}"
    ${cmd}

    cmd='dummy_afu '
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
#fuzz_dummy_afu ${iters}