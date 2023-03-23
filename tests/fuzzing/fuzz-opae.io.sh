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

# $ opae.io --help
#
# opae.io 0.2.5
# opae.io - peek and poke FPGA CSRs
#       "opae.io"
#       "opae.io -v | --version"
#       "opae.io -h | --help"
#       "opae.io ls [-v | --viddid <VID:DID>] [-s | --sub-viddid <SVID:SDID>] [--all] [--system-class]"
#       "opae.io [-d | --device <PCI_ADDRESS>] [-r | --region <REGION_NUMBER>] [-a <ACCESS_MODE>] [init | release | peek | poke | <script> [arg1...argN]]
#       "opae.io init [-d <PCI_ADDRESS>] <USER>[:<GROUP>]"
#       "opae.io release [-d <PCI_ADDRESS>]"
#       "opae.io [-d <PCI_ADDRESS>]"
#       "opae.io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] [-a <ACCESS_MODE>]"
#       "opae.io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] [-a <ACCESS_MODE>] walk [<OFFSET>] [-u | --show-uuid]"
#       "opae.io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] [-a <ACCESS_MODE>] dump [<OFFSET>] [-o | --output <FILE>] [-f | --format (hex, bin)] [ -c | --count <WORD COUNT>]
#       "opae.io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] [-a <ACCESS_MODE>] peek <OFFSET>"
#       "opae.io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] [-a <ACCESS_MODE>] poke <OFFSET> <VALUE>"
#       "opae.io [-d <PCI_ADDRESS>] [-r <REGION_NUMBER>] [-a <ACCESS_MODE>] <SCRIPT> <ARG1> <ARG2> ... <ARGN>"
#
#   NOTE
#   If -d or --device is omitted, opae.io will attempt to open the first device found.
#   If -r or --region is omitted, opae.io will default to region 0.
#
#   EXAMPLES
#   Enumerating FPGA's:
#             "$ opae.io ls"
#
#   Initiating a session:
#
#             "$ sudo opae.io init -d 0000:00:00.0 lab:lab" 
#
#   Terminating a session:
#
#             "$ sudo opae.io release -d 0000:00:00.0" 
#
#   Entering an interactive Python environment:
#
#             "$ opae.io -d 0000:00:00.0 -r 0" 
#
#   Peek & Poke from the command line:
#
#             "$ opae.io -d 0000:00:00.0 -r 0 peek 0x28" 
#             "$ opae.io -d 0000:00:00.0 -r 0 poke 0x28 0xbaddecaf" 
#
#   Executing a script:
#
#             "$ opae.io -d 0000:00:00.0 -r 0 script.py a b c"
fuzz_opae_io() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_opae_io <ITERS>\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

  local -a short_parms=(\
'-h' \
'-v' \
'-d 0000:00:00.0' \
'-r 0' \
'-a 32' \
'-a 64'\
)

  local -a long_parms=(\
'--help' \
'--version' \
'--device 0000:00:00.0' \
'--region 0' \
'--access-mode 32' \
'--access-mode 64' \
'--pdb'\
)

  local -a subcommands=(\
'ls' \
'init' \
'release' \
'walk' \
'dump' \
'peek' \
'poke'\
)

  local -a ls_parms=(\
'-v 8086:bcce' \
'--viddid 8086:bcce' \
'-s 8086:1771' \
'--sub-viddid 8086:1771' \
'--all' \
'--system-class'\
)

  local -a init_parms=(\
"${USER}" \
"${USER}:${USER}"\
)

  local -a walk_parms=(\
'--offset 0' \
'-u' \
'--show-uuid' \
'-D' \
'--dump' \
'-c 0' \
'--count 0' \
'-y 0' \
'--delay 0' \
'-s' \
'--safe'\
)

  local -a dump_parms=(\
'--offset 0' \
'-o out.txt' \
'--output out.txt' \
'-f bin' \
'-f hex' \
'--format bin' \
'--format hex' \
'-c 1' \
'--count 1'\
)

  local -a peek_parms=(\
'0' \
'0x0'\
)

  local -a poke_parms=(\
'0 0' \
'0x0 0x0'\
)

  local cmd=''
  local -i num_parms
  local parm=''

  local -i stdin_fd=0
  local stdout_file="`mktemp opae.io.stdout.XXXXXXXXXX`"
  local -i stdout_fd=0
  local stderr_file="`mktemp opae.io.stderr.XXXXXXXXXX`"
  local -i stderr_fd=0
  local -i pid=0

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "Fuzz Iteration: %d\n" $i

    cmd='opae.io '
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
      'ls')
        let "num_parms = 1 + ${RANDOM} % ${#ls_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#ls_parms[@]}"
          parm="${ls_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'init')
        let "num_parms = 1 + ${RANDOM} % ${#init_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#init_parms[@]}"
          parm="${init_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'walk')
        let "num_parms = 1 + ${RANDOM} % ${#walk_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#walk_parms[@]}"
          parm="${walk_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'dump')
        let "num_parms = 1 + ${RANDOM} % ${#dump_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#dump_parms[@]}"
          parm="${dump_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'peek')
        let "num_parms = 1 + ${RANDOM} % ${#peek_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#peek_parms[@]}"
          parm="${peek_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'poke')
        let "num_parms = 1 + ${RANDOM} % ${#poke_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#poke_parms[@]}"
          parm="${poke_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;
    esac

    printf "%s\n" "${cmd}"
    coproc ${cmd} >"${stdout_file}" 2>"${stderr_file}"
    exec {stderr_fd}<"${stderr_file}"
    stdin_fd=${COPROC[1]}
    stdout_fd=${COPROC[0]}
    pid=${COPROC_PID}

    sleep 0.001

    if ps -p ${pid} >/dev/null 2>&1 ; then
      kill -9 ${pid}
    fi

    wait ${pid} >/dev/null 2>&1
    exec {stdin_fd}>&-
    exec {stdout_fd}<&-
    exec {stderr_fd}<&-

    cmd='opae.io '
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
      'ls')
        let "num_parms = 1 + ${RANDOM} % ${#ls_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#ls_parms[@]}"
          parm="${ls_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'init')
        let "num_parms = 1 + ${RANDOM} % ${#init_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#init_parms[@]}"
          parm="${init_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'walk')
        let "num_parms = 1 + ${RANDOM} % ${#walk_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#walk_parms[@]}"
          parm="${walk_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'dump')
        let "num_parms = 1 + ${RANDOM} % ${#dump_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#dump_parms[@]}"
          parm="${dump_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'peek')
        let "num_parms = 1 + ${RANDOM} % ${#peek_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#peek_parms[@]}"
          parm="${peek_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;

      'poke')
        let "num_parms = 1 + ${RANDOM} % ${#poke_parms[@]}"
        for (( n = 0 ; n < ${num_parms} ; ++n )); do
          let "p = ${RANDOM} % ${#poke_parms[@]}"
          parm="${poke_parms[$p]}"
          parm="$(printf %s ${parm} | radamsa)"
          cmd="${cmd} ${parm}"
        done
      ;;
    esac

    printf "%s\n" "${cmd}"
    coproc ${cmd} >"${stdout_file}" 2>"${stderr_file}"
    exec {stderr_fd}<"${stderr_file}"
    stdin_fd=${COPROC[1]}
    stdout_fd=${COPROC[0]}
    pid=${COPROC_PID}

    sleep 0.001

    if ps -p ${pid} >/dev/null 2>&1 ; then
      kill -9 ${pid}
    fi

    wait ${pid} >/dev/null 2>&1
    exec {stdin_fd}>&-
    exec {stdout_fd}<&-
    exec {stderr_fd}<&-

  done

  rm -f "${stdout_file}" "${stderr_file}"
}

#declare -i iters=1
#if [ $# -gt 0 ]; then
#  iters=$1
#fi
#fuzz_opae_io ${iters}
