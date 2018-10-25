// Copyright(c) 2018, Intel Corporation
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

/**
 * \fpga_dma.c
 * \brief FPGA DMA User-mode driver
 */

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <opae/fpga.h>
#include <stddef.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <signal.h>
#include "fpga_dma_internal.h"
#include "fpga_dma.h"
#include "globals.h"

// Internal Functions

// End of feature list
inline bool _fpga_dma_feature_eol(uint64_t dfh)
{
	return ((dfh >> AFU_DFH_EOL_OFFSET) & 1) == 1;
}

// Feature type is BBB
inline bool _fpga_dma_feature_is_bbb(uint64_t dfh)
{
	// BBB is type 2
	return ((dfh >> AFU_DFH_TYPE_OFFSET) & 0xf) == FPGA_DMA_BBB;
}

// Offset to the next feature header
inline uint64_t _fpga_dma_feature_next(uint64_t dfh)
{
	return (dfh >> AFU_DFH_NEXT_OFFSET) & 0xffffff;
}

/**
 * _send_descriptor
 *
 * @brief                Queues a DMA descriptor to the FPGA
 * @param[in] dma_h      Handle to the FPGA DMA object
 * @param[in] desc       Pointer to a descriptor structure to send
 * @return fpga_result FPGA_OK on success, return code otherwise
 *
 */
fpga_result _send_descriptor(handle_common *dma_h, msgdma_ext_desc_t *desc)
{
	fpga_result res = FPGA_OK;
	msgdma_status_t status = {0};

	debug_print("desc.rd_address = %x\n", desc->rd_address);
	debug_print("desc.wr_address = %x\n", desc->wr_address);
	debug_print("desc.len = %x\n", desc->len);
	debug_print("desc.wr_burst_count = %x\n", desc->wr_burst_count);
	debug_print("desc.rd_burst_count = %x\n", desc->rd_burst_count);
	debug_print("desc.wr_stride %x\n", desc->wr_stride);
	debug_print("desc.rd_stride %x\n", desc->rd_stride);
	debug_print("desc.rd_address_ext %x\n", desc->rd_address_ext);
	debug_print("desc.wr_address_ext %x\n", desc->wr_address_ext);

	debug_print("SGDMA_CSR_BASE = %lx SGDMA_DESC_BASE=%lx\n",
		    dma_h->dma_csr_base, dma_h->dma_desc_base);

#ifdef CHECK_DELAYS_MM
	bool first = true;
#endif
	do {
		res = MMIORead32Blk(dma_h, CSR_STATUS(dma_h),
				    (uint64_t)&status.reg, sizeof(status.reg));
		ON_ERR_GOTO(res, out, "MMIORead32Blk");
#ifdef CHECK_DELAYS_MM
		if (first && status.st.desc_buf_full) {
			buf_full_count++;
			first = false;
		}
#endif
	} while (status.st.desc_buf_full);

	res = MMIOWrite64Blk(dma_h, dma_h->chan_desc->dma_desc_base, (uint64_t)desc,
			     sizeof(*desc));
	ON_ERR_GOTO(res, out, "MMIOWrite64Blk");

out:
	return res;
}

fpga_result clear_interrupt(handle_common *dma_h)
{
	// clear interrupt by writing 1 to IRQ bit in status register
	msgdma_status_t status = {0};
	status.st.irq = 1;

	return MMIOWrite32Blk(dma_h, CSR_STATUS(dma_h), (uint64_t)&status.reg,
			      sizeof(status.reg));
}

fpga_result poll_interrupt(handle_common *dma_h WHENCE)
{
	struct pollfd pfd = {0};
	fpga_result res = FPGA_OK;
	int poll_res;

	res = fpgaGetOSObjectFromEventHandle(dma_h->eh, &pfd.fd);
	ON_ERR_GOTO(res, out, "fpgaGetOSObjectFromEventHandle failed\n");

	pfd.events = POLLIN;

#ifdef CHECK_DELAYS_MM
	delayz[whence].call_count++;
	poll_res = poll(&pfd, 1, 0);
	if (0 == poll_res) {
		delayz[whence].poll_busy_count++;
		do {
			delayz[whence].poll_wait_count++;
			poll_res = poll(&pfd, 1, 0);
		} while (0 == poll_res);
	}
#else
	poll_res = poll(&pfd, 1, FPGA_DMA_TIMEOUT_MSEC);
#endif
	if (poll_res < 0) {
		fprintf(stderr, "Poll error errno = %s\n", strerror(errno));
		res = FPGA_EXCEPTION;
		goto out;
	} else if (poll_res == 0) {
		fprintf(stderr, "Poll(interrupt) timeout \n");
		res = FPGA_EXCEPTION;
	} else {
		uint64_t count = 0;
		ssize_t bytes_read = read(pfd.fd, &count, sizeof(count));
		if (bytes_read > 0) {
			debug_print("Poll success. Return = %d, count = %d\n",
				    poll_res, (int)count);
			res = FPGA_OK;
		} else {
			fprintf(stderr, "Error: poll failed read: %s\n",
				bytes_read > 0 ? strerror(errno)
					       : "zero bytes read");
			res = FPGA_EXCEPTION;
		}
	}

out:
	clear_interrupt(dma_h);
	return res;
}
