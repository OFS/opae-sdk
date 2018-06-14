//
// Copyright (c) 2017, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/**
 * \file shim_vtp.h
 * \brief MPF VTP (virtual to physical) translation shim
 */

#ifndef __FPGA_MPF_SHIM_VTP_H__
#define __FPGA_MPF_SHIM_VTP_H__

#include <opae/mpf/csrs.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The page table supports two physical page sizes.
 */
typedef enum
{
    // Enumeration values are log2 of the size
    MPF_VTP_PAGE_4KB = 12,
    MPF_VTP_PAGE_2MB = 21
}
mpf_vtp_page_size;

// mpf_vtp_page_size values are the log2 of the size.  Convert to bytes.
#define mpfPageSizeEnumToBytes(page_size) ((size_t)1 << page_size)



/**
 * Test whether the VTP service is available on the FPGA.
 *
 * @param[in]  mpf_handle  MPF handle initialized by mpfConnect().
 * @returns                True if VTP is available.
 */
bool __MPF_API__ mpfVtpIsAvailable(
    mpf_handle_t mpf_handle
);


/**
 * Allocate a shared host/FPGA buffer.
 *
 * The buffer may be arbitrarily large.  VTP allocates memory shared with
 * an FPGA and populates the VTP page tables.  The calling process may
 * pass any virtual address within the returned buffer to the FPGA and
 * the FPGA-side VTP will translate automatically to physical (IOVA)
 * addresses.
 *
 * @param[in]  mpf_handle  MPF handle initialized by mpfConnect().
 * @param[in]  len         Length of the buffer to allocate in bytes.
 * @param[out] buf_addr    Virtual base address of the allocated buffer.
 * @returns                FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfVtpBufferAllocate(
    mpf_handle_t mpf_handle,
    uint64_t len,
    void** buf_addr
);


/**
 * Free a shared host/FPGA buffer.
 *
 * Release a buffer previously allocated with mpfVtpBufferAllocate().
 * Associated translations are removed from the VTP-managed page table.
 *
 * @param[in]  mpf_handle  MPF handle initialized by mpfConnect().
 * @param[in]  buf_addr    Virtual base address of the allocated buffer.
 * @returns                FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfVtpBufferFree(
    mpf_handle_t mpf_handle,
    void* buf_addr
);


/**
 * Return the IOVA associated with a virtual address.
 *
 * The function works only with addresses allocated by VTP.
 *
 * @param[in]  mpf_handle  MPF handle initialized by mpfConnect().
 * @param[in]  buf_addr    Virtual base address of the allocated buffer.
 * @returns                The corresponding physical address (IOVA) or
 *                         0 if the address is not managed by VTP.
 */
uint64_t __MPF_API__ mpfVtpGetIOAddress(
    mpf_handle_t mpf_handle,
    void* buf_addr
);


/**
 * Invalidate the FPGA-side translation cache.
 *
 * This method does not affect allocated storage or the contents of the
 * VTP-managed translation table.  It invalidates the translation caches
 * in the FPGA, forcing the FPGA to refetch virtual to physical translations
 * on demand.
 *
 * @param[in]  mpf_handle  MPF handle initialized by mpfConnect().
 * @returns                FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfVtpInvalHWTLB(
    mpf_handle_t mpf_handle
);


/**
 * Invalidate a single virtual page in the FPGA-side translation cache.
 *
 * This method does not affect allocated storage or the contents of the
 * VTP-managed translation table.  It invalidates one address in the
 * translation caches in the FPGA.
 *
 * @param[in]  mpf_handle  MPF handle initialized by mpfConnect().
 * @param[in]  va          Virtual address to invalidate.
 * @returns                FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfVtpInvalVAMapping(
    mpf_handle_t mpf_handle,
    void* va
);


/**
 * Set the maximum allocated physical page size.
 *
 * You probably do not want to call this method.  It is more useful for
 * debugging the allocator than in production.  In most cases an application
 * is better off with large pages, which is the default.
 *
 * @param[in]  mpf_handle  MPF handle initialized by mpfConnect().
 * @param[in]  max_psize   Maximum physical page size.
 * @returns                FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfVtpSetMaxPhysPageSize(
    mpf_handle_t mpf_handle,
    mpf_vtp_page_size max_psize
);


/**
 * VTP statistics
 */
typedef struct
{
    // Hits and misses in the TLB. The VTP pipeline has local caches
    // within the pipeline itself that filter requests to the TLB.
    // The counts here increment only for requests to the TLB service
    // that are not satisfied in the VTP pipeline caches.
    uint64_t numTLBHits4KB;
    uint64_t numTLBMisses4KB;
    uint64_t numTLBHits2MB;
    uint64_t numTLBMisses2MB;

    // Number of cycles spent with the page table walker active.  Since
    // the walker manages only one request at a time the latency of the
    // page table walker can be computed as:
    //   numPTWalkBusyCycles / (numTLBMisses4KB + numTLBMisses2MB)
    uint64_t numPTWalkBusyCycles;

    // Number of failed virtual to physical translations. The VTP page
    // table walker currently goes into a terminal error state when a
    // translation failure is seen and blocks all future requests.
    // If this number is non-zero the program has passed an illegal
    // address and should be fixed.
    uint64_t numFailedTranslations;
    // Last virtual address translated. If numFailedTranslations is non-zero
    // this is the failing virtual address.
    void* ptWalkLastVAddr;
}
mpf_vtp_stats;


/**
 * Return VTP statistics.
 *
 * @param[in]  mpf_handle  MPF handle initialized by mpfConnect().
 * @param[out] stats       Statistics.
 * @returns                FPGA_OK on success.
 */
fpga_result __MPF_API__ mpfVtpGetStats(
    mpf_handle_t mpf_handle,
    mpf_vtp_stats* stats
);


#ifdef __cplusplus
}
#endif

#endif // __FPGA_MPF_SHIM_VTP_H__
