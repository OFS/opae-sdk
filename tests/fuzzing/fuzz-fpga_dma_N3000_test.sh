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

# $ fpga_dma_N3000_test --help
#
# Usage: fpga_dma_test <use_ase = 1 (simulation only), 0 (hardware)> [options]
# Options are:
# 	-m	Use malloc (default)
#	-p	Use mmap (Incompatible with -m)
#	-c	Use builtin memcpy (default)
#	-2	Use SSE2 memcpy (Incompatible with -c)
#	-n	Do not provide OS advice (default)
#	-a	Use madvise (Incompatible with -n)
#	-y	Do not verify buffer contents - faster (default is to verify)
#	-C	Do not restrict process to CPUs attached to DCP NUMA node
#	-M	Do not restrict process memory allocation to DCP NUMA node
#	-B	Set a target bus number
#	-D	Select DMA to test
#	-S	Set memory test size
#	-G	Set AFU GUID
fuzz_fpga_dma_N3000_test() {
  if [ $# -lt 1 ]; then
    printf "usage: fuzz_fpga_dma_N3000_test <ITERS> [QUIET]\n"
    exit 1
  fi

  local -i iters=$1
  local -i i
  local -i p
  local -i n

  local -i quiet=0
  if [ $# -gt 1 ]; then
    quiet=$2
  fi

  local -a short_parms=(\
'use_ase=1' \
'use_ase=0' \
'-m' \
'-p' \
'-c' \
'-2' \
'-n' \
'-a' \
'-y' \
'-C' \
'-M' \
'-B 0x0' \
'-B 2' \
'-D 0' \
'-S 1024' \
'-G 331DB30C-9885-41EA-9081-F88B8F655CAA'\
)

  local cmd=''
  local -i num_parms
  local parm=''

  for (( i = 0 ; i < ${iters} ; ++i )); do

    printf "fpga_dma_N3000_test Fuzz Iteration: %d\n" $i

    cmd='fpga_dma_N3000_test '
    let "num_parms = 1 + ${RANDOM} % ${#short_parms[@]}"
    for (( n = 0 ; n < ${num_parms} ; ++n )); do
      let "p = ${RANDOM} % ${#short_parms[@]}"
      parm="${short_parms[$p]}"
      parm="$(printf %s ${parm} | radamsa)"
      cmd="${cmd} ${parm}"
    done

    [ ${quiet} -eq 0 ] && printf "%s\n" "${cmd}"
    ${cmd}

  done
}

#declare -i iters=1
#if [ $# -gt 0 ]; then
#  iters=$1
#fi
#fuzz_fpga_dma_N3000_test ${iters}
