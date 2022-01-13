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

# afu_id is "d8424dc4-a4a3-c413-f89e-433683f9040b"

def afu_addr(offset):
    return 0x40000 + offset


src_buffer = allocate_buffer(4096)
dst_buffer = allocate_buffer(4096)
dsm_buffer = allocate_buffer(4096)

value_register = register()

dsm = value_register(afu_addr(0x110))
src = value_register(afu_addr(0x120))
dst = value_register(afu_addr(0x128))
nl = value_register(afu_addr(0x130))
ctl = value_register(afu_addr(0x138))
cfg = value_register(afu_addr(0x140))


print('asserting afu reset...')
ctl.commit(0)
ctl.commit(1)

for i in range(int(src_buffer.size/8)):
    src_buffer[i] = 0xdeadbeef

for i in range(int(dst_buffer.size/8)):
    dst_buffer[i] = 0

print('writing dsm address..')
dsm.commit(dsm_buffer.io_address)
print('writing srcaddress..')
src.commit(src_buffer.io_address >> 6)
print('writing dst address..')
dst.commit(dst_buffer.io_address >> 6)
print('writing config')
cfg.commit(0x42000)
print('writing number of cachelines')
nl.commit(int(4096/64))
print('kicking it off')
ctl.commit(3)
print('waiting for dsm...')
while dsm_buffer[0x40] & 0x1 == 0:
    time.sleep(0.1)
print('stopping...')
ctl.commit(7)
for i in range(int(dst_buffer.size/8)):
    if dst_buffer[i] != src_buffer[i]:
        raise SystemExit("bah!")

print('done')
