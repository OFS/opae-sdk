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
 * \file shim_vtp.c
 * \brief MPF VTP (virtual to physical) translation shim
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#include <opae/mpf/mpf.h>
#include "mpf_internal.h"

static const size_t CCI_MPF_VTP_LARGE_PAGE_THRESHOLD = (128*1024);


// ========================================================================
//
//   Module internal methods.
//
// ========================================================================

//
// Turn the FPGA side on.
//
static fpga_result vtpEnable(
    _mpf_handle_p _mpf_handle
)
{
    fpga_result r;

    r = mpfWriteCsr(_mpf_handle,
                    CCI_MPF_SHIM_VTP, CCI_MPF_VTP_CSR_PAGE_TABLE_PADDR,
                    mpfVtpPtGetPageTableRootPA(_mpf_handle->vtp.pt) / CL(1));

    // Enable VTP
    r = mpfWriteCsr(_mpf_handle, CCI_MPF_SHIM_VTP, CCI_MPF_VTP_CSR_MODE, 1);
    return r;
}


//
// Allocate page of size page_size to virtual address va and add entry to VTP
// page table
//
static fpga_result allocateAndInsertPage(
    _mpf_handle_p _mpf_handle,
    mpf_vtp_pt_vaddr va,
    mpf_vtp_page_size page_size,
    uint32_t flags,
    bool speculative
)
{
    fpga_result r;

    // mpf_vtp_page_size values are the log2 of the size.  Convert to bytes.
    size_t page_bytes = mpfPageSizeEnumToBytes(page_size);

    // Allocate buffer at va
    mpf_vtp_pt_vaddr alloc_va = va;
    uint64_t wsid;
    int fpga_flags = FPGA_BUF_PREALLOCATED;

    if (! _mpf_handle->vtp.use_fpga_buf_preallocated)
    {
        // fpgaPrepareBuffer() is an old version that doesn't support
        // FPGA_BUF_PREALLOCATED.
        fpga_flags = 0;

        // Unmap the page.  fpgaPrepareBuffer() will map it again, hopefully
        // at the same address.
        r = mpfOsUnmapMemory(va, page_bytes);

        // Unmap should never fail unless there is a bug inside VTP
        assert (FPGA_OK == r);
    }

    if (speculative)
    {
        fpga_flags |= FPGA_BUF_QUIET;
    }

    r = fpgaPrepareBuffer(_mpf_handle->handle, page_bytes, &va, &wsid, fpga_flags);
    if (FPGA_OK != r)
    {
        if (_mpf_handle->dbg_mode)
        {
            MPF_FPGA_MSG("FAILED allocating %s page VA %p, status %d",
                         (page_size == MPF_VTP_PAGE_2MB ? "2MB" : "4KB"),
                         alloc_va, r);
        }

        return r;
    }

    // Confirm that the allocated VA is in the expected location
    if (alloc_va != va)
    {
        if (_mpf_handle->dbg_mode)
        {
            MPF_FPGA_MSG("FAILED allocating %s page VA %p -- at %p instead of requested address",
                         (page_size == MPF_VTP_PAGE_2MB ? "2MB" : "4KB"),
                         alloc_va, va);
        }

        return FPGA_NO_MEMORY;
    }

    // Get the physical address of the buffer
    mpf_vtp_pt_paddr alloc_pa;
    r = fpgaGetIOAddress(_mpf_handle->handle, wsid, &alloc_pa);
    if (FPGA_OK != r) return r;

    if (_mpf_handle->dbg_mode)
    {
        MPF_FPGA_MSG("allocate %s page VA %p, PA 0x%" PRIx64 ", wsid 0x%" PRIx64,
                     (page_size == MPF_VTP_PAGE_2MB ? "2MB" : "4KB"),
                     alloc_va, alloc_pa, wsid);
    }

    // Insert VTP page table entry
    r = mpfVtpPtInsertPageMapping(_mpf_handle->vtp.pt,
                                  alloc_va, alloc_pa, wsid, page_size, flags);
    if (FPGA_OK != r)
    {
        if (_mpf_handle->dbg_mode)
        {
            MPF_FPGA_MSG("FAILED inserting page mapping, status %d", r);
        }

        fpgaReleaseBuffer(_mpf_handle->handle, wsid);

        return r;
    }

    return FPGA_OK;
}


// ========================================================================
//
//   MPF internal methods.
//
// ========================================================================

fpga_result mpfVtpInit(
    _mpf_handle_p _mpf_handle
)
{
    fpga_result r;

    // Is the VTP feature present?
    _mpf_handle->vtp.is_available = false;
    if (! mpfShimPresent(_mpf_handle, CCI_MPF_SHIM_VTP)) return FPGA_NOT_SUPPORTED;
    _mpf_handle->vtp.is_available = true;

    // Already initialized?
    if (NULL != _mpf_handle->vtp.pt) return FPGA_EXCEPTION;

    // Allocate a mutex that protects the page table manager
    r = mpfOsPrepareMutex(&(_mpf_handle->vtp.alloc_mutex));
    if (FPGA_OK != r) return r;

    // Initialize the page table
    r = mpfVtpPtInit(_mpf_handle, &(_mpf_handle->vtp.pt));
    if (FPGA_OK != r) goto fail;

    // Reset the HW TLB
    r = mpfVtpInvalHWTLB(_mpf_handle);
    if (FPGA_OK != r) goto fail;

    // Test whether FPGA_BUF_PREALLOCATED is supported.  libfpga on old systems
    // might not.  fpgaPrepareBuffer() has a special mode for probing by
    // setting the buffer size to 0.
    uint64_t dummy_wsid;
    _mpf_handle->vtp.use_fpga_buf_preallocated =
        (FPGA_OK == fpgaPrepareBuffer(_mpf_handle->handle, 0, NULL, &dummy_wsid,
                                      FPGA_BUF_PREALLOCATED));
    if (_mpf_handle->dbg_mode)
    {
        MPF_FPGA_MSG("FPGA_BUF_PREALLOCATED %s",
                     (_mpf_handle->vtp.use_fpga_buf_preallocated ?
                          "supported." :
                          "not supported.  Using compatibility mode."));
    }

    _mpf_handle->vtp.max_physical_page_size = MPF_VTP_PAGE_2MB;

    return FPGA_OK;

  fail:
    mpfOsReleaseMutex(_mpf_handle->vtp.alloc_mutex);
    return r;
}


fpga_result mpfVtpTerm(
    _mpf_handle_p _mpf_handle
)
{
    fpga_result r;

    if (! _mpf_handle->vtp.is_available) return FPGA_NOT_SUPPORTED;

    // Turn off VTP in the FPGA, blocking all traffic.
    mpfWriteCsr(_mpf_handle, CCI_MPF_SHIM_VTP, CCI_MPF_VTP_CSR_MODE, 0);

    if (_mpf_handle->dbg_mode) MPF_FPGA_MSG("VTP terminating...");

    r = mpfVtpPtTerm(_mpf_handle->vtp.pt);
    _mpf_handle->vtp.pt = NULL;

    // Drop the allocated mutex
    mpfOsReleaseMutex(_mpf_handle->vtp.alloc_mutex);
    _mpf_handle->vtp.alloc_mutex = NULL;

    return r;
}



// ========================================================================
//
//   MPF exposed external methods.
//
// ========================================================================


bool __MPF_API__ mpfVtpIsAvailable(
    mpf_handle_t mpf_handle
)
{
    _mpf_handle_p _mpf_handle = (_mpf_handle_p)mpf_handle;
    return _mpf_handle->vtp.is_available;
}


fpga_result __MPF_API__ mpfVtpBufferAllocate(
    mpf_handle_t mpf_handle,
    uint64_t len,
    void** buf_addr
)
{
    fpga_result r;
    _mpf_handle_p _mpf_handle = (_mpf_handle_p)mpf_handle;

    if (! _mpf_handle->vtp.is_available) return FPGA_NOT_SUPPORTED;
    if ((NULL == buf_addr) || (0 == len)) return FPGA_INVALID_PARAM;

    // Page table updates aren't thread safe
    mpfOsLockMutex(_mpf_handle->vtp.alloc_mutex);

    if (_mpf_handle->dbg_mode) MPF_FPGA_MSG("requested 0x%" PRIx64 " byte buffer", len);

    // Pick a requested page size
    mpf_vtp_page_size page_size = MPF_VTP_PAGE_4KB;
    if ((len > CCI_MPF_VTP_LARGE_PAGE_THRESHOLD) &&
        (MPF_VTP_PAGE_2MB <= _mpf_handle->vtp.max_physical_page_size))
    {
        page_size = MPF_VTP_PAGE_2MB;
    }

    // Map the memory
    r = mpfOsMapMemory(len, &page_size, buf_addr);

    if (FPGA_OK != r) goto fail;

    // Share each page with the FPGA and insert them into the VTP page table.
    size_t page_bytes = mpfPageSizeEnumToBytes(page_size);
    uint8_t* page = *buf_addr;
    uint32_t pt_flags = MPF_VTP_PT_FLAG_ALLOC_START;

    const size_t page_bytes_2mb = mpfPageSizeEnumToBytes(MPF_VTP_PAGE_2MB);
    const size_t page_mask_2mb = page_bytes_2mb - 1;

    // Round len up to a multiple of the page size
    len = (len + page_bytes - 1) & ~(page_bytes - 1);

    while (len)
    {
        size_t this_page_bytes = page_bytes;

        // Tag the last page in the group
        if (len == page_bytes)
        {
            pt_flags |= MPF_VTP_PT_FLAG_ALLOC_END;
        }

        // Speculatively request a 2MB page even if in a 4KB allocation when
        // the virtual page is aligned to 2MB and is large enough.
        if ((0 == ((size_t)page & page_mask_2mb)) && (len >= page_bytes_2mb) &&
            (MPF_VTP_PAGE_2MB <= _mpf_handle->vtp.max_physical_page_size))
        {
            // Try for a big page
            uint32_t pt_flags_2mb = pt_flags;
            if (len == page_bytes_2mb)
            {
                pt_flags_2mb |= MPF_VTP_PT_FLAG_ALLOC_END;
            }

            bool speculative = (page_size != MPF_VTP_PAGE_2MB);
            r = allocateAndInsertPage(_mpf_handle, page, MPF_VTP_PAGE_2MB, pt_flags_2mb,
                                      speculative);

            // Did it work?
            if (FPGA_OK == r)
            {
                // Yes!
                this_page_bytes = page_bytes_2mb;
            }
            else if (speculative)
            {
                // No.  Use small pages.
                r = allocateAndInsertPage(_mpf_handle, page, page_size, pt_flags, false);
            }
        }
        else
        {
            // The page isn't aligned to 2MB.
            r = allocateAndInsertPage(_mpf_handle, page, page_size, pt_flags, false);
        }

        if (FPGA_OK != r) goto fail;

        pt_flags = 0;
        page += this_page_bytes;
        len -= this_page_bytes;
    }

    if (_mpf_handle->dbg_mode) mpfVtpPtDumpPageTable(_mpf_handle->vtp.pt);

    mpfOsUnlockMutex(_mpf_handle->vtp.alloc_mutex);
    return FPGA_OK;

  fail:
    mpfOsUnlockMutex(_mpf_handle->vtp.alloc_mutex);
    return FPGA_NO_MEMORY;
}


fpga_result __MPF_API__ mpfVtpBufferFree(
    mpf_handle_t mpf_handle,
    void* buf_addr
)
{
    _mpf_handle_p _mpf_handle = (_mpf_handle_p)mpf_handle;

    mpf_vtp_pt_vaddr va = buf_addr;
    mpf_vtp_pt_paddr pa;
    mpf_vtp_page_size size;
    uint32_t flags;
    uint64_t wsid;
    fpga_result r;

    if (! _mpf_handle->vtp.is_available) return FPGA_NOT_SUPPORTED;

    // Is the address the beginning of an allocation region?
    r = mpfVtpPtTranslateVAtoPA(_mpf_handle->vtp.pt, va, &pa, &flags);
    if (FPGA_OK != r) return r;
    if (0 == (flags & MPF_VTP_PT_FLAG_ALLOC_START))
    {
        return FPGA_NO_MEMORY;
    }

    mpfOsLockMutex(_mpf_handle->vtp.alloc_mutex);

    // Loop through the mapped virtual pages until the end of the region
    // is reached or there is an error.
    while (FPGA_OK == mpfVtpPtRemovePageMapping(_mpf_handle->vtp.pt, va,
                                                &pa, NULL, &wsid, &size, &flags))
    {
        if (_mpf_handle->dbg_mode)
        {
            MPF_FPGA_MSG("release %s page VA %p, PA 0x%" PRIx64 ", wsid 0x%" PRIx64,
                         (size == MPF_VTP_PAGE_2MB ? "2MB" : "4KB"),
                         va, pa, wsid);
        }

        size_t page_bytes = mpfPageSizeEnumToBytes(size);

        r = mpfVtpInvalVAMapping(_mpf_handle, va);
        if (FPGA_OK != r) goto fail;

        // If the kernel deallocation fails just give up.  Something bad
        // is bound to happen.
        assert(FPGA_OK == fpgaReleaseBuffer(_mpf_handle->handle, wsid));
        mpfOsUnmapMemory(va, page_bytes);

        // Done?
        if (flags & MPF_VTP_PT_FLAG_ALLOC_END)
        {
            if (_mpf_handle->dbg_mode) mpfVtpPtDumpPageTable(_mpf_handle->vtp.pt);

            mpfOsUnlockMutex(_mpf_handle->vtp.alloc_mutex);
            return FPGA_OK;
        }

        // Next page address
        va = (char *) va + page_bytes;
    }

    // Failure in the deallocation loop.  Continue to fail path.
    r = FPGA_NO_MEMORY;

  fail:
    mpfOsUnlockMutex(_mpf_handle->vtp.alloc_mutex);
    return r;
}


uint64_t __MPF_API__ mpfVtpGetIOAddress(
    mpf_handle_t mpf_handle,
    void* buf_addr
)
{
    fpga_result r;
    _mpf_handle_p _mpf_handle = (_mpf_handle_p)mpf_handle;

    uint64_t pa;
    r = mpfVtpPtTranslateVAtoPA(_mpf_handle->vtp.pt, buf_addr, &pa, NULL);
    if (FPGA_OK != r) return 0;

    return pa;
}


fpga_result __MPF_API__ mpfVtpInvalHWTLB(
    mpf_handle_t mpf_handle
)
{
    fpga_result r;
    _mpf_handle_p _mpf_handle = (_mpf_handle_p)mpf_handle;

    if (! _mpf_handle->vtp.is_available) return FPGA_NOT_SUPPORTED;

    // Mode 2 blocks traffic and invalidates the FPGA-side TLB cache
    r = mpfWriteCsr(mpf_handle, CCI_MPF_SHIM_VTP, CCI_MPF_VTP_CSR_MODE, 2);
    if (FPGA_OK != r) return r;

    return vtpEnable(_mpf_handle);
}


fpga_result __MPF_API__ mpfVtpInvalVAMapping(
    mpf_handle_t mpf_handle,
    mpf_vtp_pt_vaddr va
)
{
    _mpf_handle_p _mpf_handle = (_mpf_handle_p)mpf_handle;

    if (! _mpf_handle->vtp.is_available) return FPGA_NOT_SUPPORTED;

    return mpfWriteCsr(mpf_handle,
                       CCI_MPF_SHIM_VTP, CCI_MPF_VTP_CSR_INVAL_PAGE_VADDR,
                       // Convert VA to a line index
                       (uint64_t)va / CL(1));
}


fpga_result __MPF_API__ mpfVtpSetMaxPhysPageSize(
    mpf_handle_t mpf_handle,
    mpf_vtp_page_size max_psize
)
{
    _mpf_handle_p _mpf_handle = (_mpf_handle_p)mpf_handle;

    if (max_psize > MPF_VTP_PAGE_2MB) return FPGA_INVALID_PARAM;

    _mpf_handle->vtp.max_physical_page_size = max_psize;
    return FPGA_OK;
}


fpga_result __MPF_API__ mpfVtpGetStats(
    mpf_handle_t mpf_handle,
    mpf_vtp_stats* stats
)
{
    // Is the VTP feature present?
    if (! mpfShimPresent(mpf_handle, CCI_MPF_SHIM_VTP))
    {
        memset(stats, -1, sizeof(mpf_vtp_stats));
        return FPGA_NOT_SUPPORTED;
    }

    stats->numTLBHits4KB = mpfReadCsr(mpf_handle, CCI_MPF_SHIM_VTP, CCI_MPF_VTP_CSR_STAT_4KB_TLB_NUM_HITS, NULL);
    stats->numTLBMisses4KB = mpfReadCsr(mpf_handle, CCI_MPF_SHIM_VTP, CCI_MPF_VTP_CSR_STAT_4KB_TLB_NUM_MISSES, NULL);
    stats->numTLBHits2MB = mpfReadCsr(mpf_handle, CCI_MPF_SHIM_VTP, CCI_MPF_VTP_CSR_STAT_2MB_TLB_NUM_HITS, NULL);
    stats->numTLBMisses2MB = mpfReadCsr(mpf_handle, CCI_MPF_SHIM_VTP, CCI_MPF_VTP_CSR_STAT_2MB_TLB_NUM_MISSES, NULL);
    stats->numPTWalkBusyCycles = mpfReadCsr(mpf_handle, CCI_MPF_SHIM_VTP, CCI_MPF_VTP_CSR_STAT_PT_WALK_BUSY_CYCLES, NULL);
    stats->numFailedTranslations = mpfReadCsr(mpf_handle, CCI_MPF_SHIM_VTP, CCI_MPF_VTP_CSR_STAT_FAILED_TRANSLATIONS, NULL);

    stats->ptWalkLastVAddr = (void*)(CL(1) * mpfReadCsr(mpf_handle, CCI_MPF_SHIM_VTP, CCI_MPF_VTP_CSR_STAT_PT_WALK_LAST_VADDR, NULL));

    return FPGA_OK;
}
