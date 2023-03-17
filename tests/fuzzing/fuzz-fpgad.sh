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

# $ fpgad --help
#
# Usage: fpgad <options>
#
#	-d,--daemon                 run as daemon process.
#	-l,--logfile <file>         the log file for daemon mode [fpgad.log].
#	-p,--pidfile <file>         the pid file for daemon mode [fpgad.pid].
#	-s,--socket <sock>          the unix domain socket [/tmp/fpga_event_socket].
#	-n,--null-bitstream <file>  NULL bitstream (for AP6 handling, may be
#	                            given multiple times).
#	-v,--version                display the version and exit.
fuzz_fpgad() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_fpgad <ITERS>\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

  local -a short_parms=(\
'-d' \
'-l fpgad.log' \
'-p fpgad.pid' \
'-s fpga_event_socket' \
'-n null.gbs' \
'-v'\
)

  local -a long_parms=(\
'--daemon' \
'--logfile fpgad.log' \
'--pidfile fpgad.pid' \
'--socket fpga_event_socket' \
'--null-bitstream null.gbs' \
'--version'\
)

  local cmd=''
  local -i num_parms
  local parm=''

  local -i fpgad_stdin_fd=0
  local fpgad_stdout_file
  local -i fpgad_stdout_fd=0
  local fpgad_stderr_file
  local -i fpgad_stderr_fd=0
  local -i fpgad_pid=0

  local tdir="`mktemp --directory fpgad.fuzz.XXXXXXXXXX`"

  cd "${tdir}"
  fpgad_stdout_file="`mktemp fpgad.stdout.XXXXXXXXXX`"
  fpgad_stderr_file="`mktemp fpgad.stderr.XXXXXXXXXX`"

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "Fuzz Iteration: %d\n" $i

    cmd='fpgad '
    let "num_parms = ${RANDOM} % ${#short_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#short_parms[@]}"
      parm="${short_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    printf "%s\n" "${cmd}"
    coproc ${cmd} >"${fpgad_stdout_file}" 2>"${fpgad_stderr_file}"
    exec {fpgad_stderr_fd}<"${fpgad_stderr_file}"
    fpgad_stdin_fd=${COPROC[1]}
    fpgad_stdout_fd=${COPROC[0]}
    fpgad_pid=${COPROC_PID}

    sleep 0.001

    if ps -p ${fpgad_pid} >/dev/null 2>&1 ; then
      kill -9 ${fpgad_pid}
    fi

    wait ${fpgad_pid} >/dev/null 2>&1
    exec {fpgad_stdin_fd}>&-
    exec {fpgad_stdout_fd}<&-
    exec {fpgad_stderr_fd}<&-

    cmd='fpgad '
    let "num_parms = ${RANDOM} % ${#long_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#long_parms[@]}"
      parm="${long_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    printf "%s\n" "${cmd}"
    coproc ${cmd} >"${fpgad_stdout_file}" 2>"${fpgad_stderr_file}"
    exec {fpgad_stderr_fd}<"${fpgad_stderr_file}"
    fpgad_stdin_fd=${COPROC[1]}
    fpgad_stdout_fd=${COPROC[0]}
    fpgad_pid=${COPROC_PID}

    sleep 0.001

    if ps -p ${fpgad_pid} >/dev/null 2>&1 ; then
      kill -9 ${fpgad_pid}
    fi

    wait ${fpgad_pid} >/dev/null 2>&1
    exec {fpgad_stdin_fd}>&-
    exec {fpgad_stdout_fd}<&-
    exec {fpgad_stderr_fd}<&-

  done

  cd ..
  rm -rf "${tdir}"
}

#declare -i iters=1
#if [ $# -gt 0 ]; then
#  iters=$1
#fi
#fuzz_fpgad ${iters}
