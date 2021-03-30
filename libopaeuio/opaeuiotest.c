// Copyright(c) 2020, Intel Corporation
//
// Redistribution  and  use  in source  and  binary  forms,  with  or  without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of  source code  must retain the  above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name  of Intel Corporation  nor the names of its contributors
//   may be used to  endorse or promote  products derived  from this  software
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,  BUT NOT LIMITED TO,  THE
// IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT  SHALL THE COPYRIGHT OWNER  OR CONTRIBUTORS BE
// LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
// CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT LIMITED  TO,  PROCUREMENT  OF
// SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  DATA, OR PROFITS;  OR BUSINESS
// INTERRUPTION)  HOWEVER CAUSED  AND ON ANY THEORY  OF LIABILITY,  WHETHER IN
// CONTRACT,  STRICT LIABILITY,  OR TORT  (INCLUDING NEGLIGENCE  OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <opae/uio.h>

struct dfh {
	uint64_t version;
	uint64_t afu_id_low;
	uint64_t afu_id_high;
	uint64_t next_afu;
};

void print_dfh(struct opae_uio *u)
{
	struct dfh *mmio = NULL;

	opae_uio_region_get(u, 0, (uint8_t **)&mmio, NULL);

	printf("dfh:         0x%016lx\n"
	       "afu_id low:  0x%016lx\n"
	       "afu_id high: 0x%016lx\n"
	       "next afu:    0x%016lx\n",
	       mmio->version, mmio->afu_id_low,
	       mmio->afu_id_high, mmio->next_afu);
}

int main(int argc, char *argv[])
{
	struct opae_uio u;
	int res;

	if (argc < 2) {
		printf("usage: opaeuiotest <dfl device>\n");
		printf("\n\twhere <dfl device> is of the form dfl_dev.X\n");
		return 1;
	}

	res = opae_uio_open(&u, argv[1]);
	if (res) {
		return res;
	}

	print_dfh(&u);

	opae_uio_close(&u);

	return 0;
}
