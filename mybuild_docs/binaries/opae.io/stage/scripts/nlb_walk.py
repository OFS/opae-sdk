## Copyright(c) 2020, Intel Corporation
##
## Redistribution  and  use  in source  and  binary  forms,  with  or  without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of  source code  must retain the  above copyright notice,
##   this list of conditions and the following disclaimer.
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and/or other materials provided with the distribution.
## * Neither the name  of Intel Corporation  nor the names of its contributors
##   may be used to  endorse or promote  products derived  from this  software
##   without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
## IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
## LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
## CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
## SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
## INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
## CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.

import argparse
import logging
import sys
import uuid

from opae.io.utils import feature

FORMAT = '%(asctime)-15s  %(message)s'
logging.basicConfig(format=FORMAT,
                    stream=sys.stdout,
                    level=logging.NOTSET)
nlb_guid = uuid.UUID("d8424dc4-a4a3-c413-f89e-433683f9040b")
n3000_afu = uuid.UUID("9aeffe5f-8457-0612-c000-c9660d824272")



def run(afu, num_lines):
    buffer_size = num_lines*64
    
    src_buffer = allocate_buffer(buffer_size)
    dst_buffer = allocate_buffer(buffer_size)

    logging.info('allocated {} for src'.format(src_buffer.size))
    logging.info('allocated {} for dst'.format(dst_buffer.size))
    dsm_buffer = allocate_buffer(4096)
    
    
    dsm = 0x110
    src = 0x120
    dst = 0x128
    num = 0x130
    ctl = 0x138
    cfg = 0x140
    
    
    logging.info('asserting afu reset...')
    f[ctl] = 0
    f[ctl] = 1
    
    src_buffer.fill32(0xdeadbeef)
    dst_buffer.fill32(0x0)
    
    logging.info('writing dsm address..')
    f[dsm] = dsm_buffer.io_address
    logging.info('writing src address: 0x{:08x}'.format(src_buffer.io_address>>6))
    f[src] = src_buffer.io_address >> 6
    logging.info('writing dst address: 0x{:08x}'.format(dst_buffer.io_address>>6))
    f[dst] = dst_buffer.io_address >> 6
    logging.info('writing config')
    f[cfg] = 0x42000
    logging.info('writing number of cachelines: %d', num_lines)
    f[num] = num_lines
    logging.info('kicking it off')
    with f.register(ctl) as r:
        logging.debug('offset: 0x{:0x}, value: {}'.format(r.offset, r.value))
        r.value = 3
    logging.debug('offset: 0x{:0x}, value: {}'.format(f.offset+ctl, f[ctl]))
    logging.info('waiting for dsm...')
    while dsm_buffer[0x40] & 0x1 == 0:
        time.sleep(0.1)
    logging.info('stopping...')
    f[ctl] = 7
  
    logging.info('dsm status: "0x{:04x}"'.format(dsm_buffer.read8(0x40)))
    logging.info('dsm error: "{}"'.format(dsm_buffer.read8(0x44)))
    logging.info('dsm num_clocks: "{}"'.format(dsm_buffer.read8(0x48)))
    logging.info('dsm reads: "{}"'.format(dsm_buffer.read16(0x50)))
    logging.info('dsm writes: "{}"'.format(dsm_buffer.read16(0x54)))
    diff_at = src_buffer.compare(dst_buffer)
    if diff_at < num_lines*64:
        logging.error('buffers do not match: {}'.format(diff_at))
        raise SystemExit("bah!")
    
    logging.info('done')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--num-lines', type=int, default=64)
    parser.add_argument('--guid', type=uuid.UUID, default=nlb_guid)
    parser.add_argument('--afu-offset', type=int, default=0x40000)
    parser.add_argument('--find-all', default=False, action='store_true')
    args = parser.parse_args()
    for f in feature.find(args.guid, args.afu_offset):
        run(f, args.num_lines)
        if not args.find_all:
            break
