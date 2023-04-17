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


# fpgadiag Exercise afu and network
#
#fpgadiag --help
#usage: fpgadiag [-t {fpga,ase}]
#                [-m {lpbk1,read,write,trput,sw,fvlbypass,fpgalpbk,mactest,fpgastats,fpgamac}]
#
#options:
#  -t {fpga,ase}, --target {fpga,ase}
#                        choose target. Default: fpga
#  -m {lpbk1,read,write,trput,sw,fvlbypass,fpgalpbk,mactest,fpgastats,fpgamac},
#  --mode {lpbk1,read,write,trput,sw,fvlbypass,fpgalpbk,mactest,fpgastats,fpgamac}
#                        choose test mode. Combine this with-h or
#                         --help to see detail help messagefor each mode.


fuzz_fpgadiag() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_fpgadiag <ITERS>\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

# fpgadiag short command line parameters
  local -a short_parms=(\
'-h' \
'-t fpga' \
'-t ase' \
'-m lpbk1' \
'-m read' \
'-m write' \
'-m trput' \
'-m sw' \
'-m fvlbypass' \
'-m fpgalpbk' \
'-m mactest' \
'-m fpgastats' \
'-m fpgamac' \
'-t fpga -m lpbk1' \
'-t fpga -m read' \
'-t fpga -m write' \
'-t fpga -m trput' \
'-t fpga -m sw' \
'-t fpga -m fvlbypass' \
'-t fpga -m fpgalpbk' \
'-t fpga -m mactest' \
'-t fpga -m fpgastats' \
'-t fpga -m fpgamac' \
'-t ase -m lpbk1' \
'-t ase -m read' \
'-t ase -m write' \
'-t ase -m trput' \
'-t ase -m sw' \
'-t ase -m fvlbypass' \
'-t ase -m fpgalpbk' \
'-t ase -m mactest' \
'-t ase -m fpgastats' \
'-t ase -m fpgamac'\
)

# fpgadiag long command line parameters
  local -a long_parms=(\
'--help' \
'--target fpga' \
'--target ase' \
'--mode lpbk1' \
'--mode read' \
'--mode write' \
'--mode trput' \
'--mode sw' \
'--mode fvlbypass' \
'--mode fpgalpbk' \
'--mode mactest' \
'--mode fpgastats' \
'--mode fpgamac' \
'--target fpga --mode lpbk1' \
'--target fpga --mode read' \
'--target fpga --mode write' \
'--target fpga --mode trput' \
'--target fpga --mode sw' \
'--target fpga --mode fvlbypass' \
'--target fpga --mode fpgalpbk' \
'--target fpga --mode mactest' \
'--target fpga --mode fpgastats' \
'--target fpga --mode fpgamac' \
'--target ase --mode lpbk1' \
'--target ase --mode read' \
'--target ase --mode write' \
'--target ase --mode trput' \
'--target ase --mode sw' \
'--target ase --mode fvlbypass' \
'--target ase --mode fpgalpbk' \
'--target ase --mode mactest' \
'--target ase --mode fpgastats' \
'--target ase --mode fpgamac'\
)


  local cmd=''
  local -i num_parms
  local parm=''

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "Fuzz Iteration: %d\n" $i

    cmd='fpgadiag '
    let "num_parms = 1 + ${RANDOM} % ${#short_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#short_parms[@]}"
      parm="${short_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    printf "%s\n" "${cmd}"
    ${cmd}

    cmd='fpgadiag '
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


#fuzz_fpgadiag ${iters}
#declare -i iters=1
#
#if [ $# -gt 0 ]; then
#  iters=$1
#fi
#
#fuzz_fpgadiag ${iters}
