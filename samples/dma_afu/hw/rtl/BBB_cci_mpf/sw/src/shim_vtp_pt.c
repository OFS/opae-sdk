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

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <inttypes.h>

#include <opae/mpf/mpf.h>
#include "mpf_internal.h"


// ========================================================================
//
// Operations on levels within the hierarchical page table.  A VTP page
// table is similar to x86_64 page tables, where 9-bit chunks of an
// address are direct mapped into a tree of 512 entry (4KB) pointers.
//
// ========================================================================

static void nodeReset(
    mpf_vtp_pt_node* node
)
{
    memset(node, -1, sizeof(mpf_vtp_pt_node));
}

static bool nodeIsEmpty(
    mpf_vtp_pt_node* node
)
{
    for (int idx = 0; idx < 512; idx++)
    {
        if ((*node)[idx] != -1) return false;
    }

    return true;
}

// Does an entry exist at the index?
static bool nodeEntryExists(
    mpf_vtp_pt_node* node,
    uint64_t idx
)
{
    if (idx >= 512)
    {
        return false;
    }

    return ((*node)[idx] != -1);
}

// Is the entry at idx terminal? If so, use GetTranslatedAddr(). If not,
// use GetChildAddr().
static bool nodeEntryIsTerminal(
    mpf_vtp_pt_node* node,
    uint64_t idx
)
{
    if (idx >= 512)
    {
        return false;
    }

    return ((*node)[idx] & MPF_VTP_PT_FLAG_TERMINAL) != 0;
}

// Walk the tree.  Ideally this would be a pointer to another
// mpf_vtp_pt_node, but that doesn't work for the virtually indexed
// table since it holds physical addresses.  The caller will have
// to interpret the child pointer depending on the type of tree.
static int64_t nodeGetChildAddr(
    mpf_vtp_pt_node* node,
    uint64_t idx
)
{
    if ((idx >= 512) || nodeEntryIsTerminal(node, idx))
    {
        return -1;
    }

    return (*node)[idx];
}

static int64_t nodeGetValue(
    mpf_vtp_pt_node* node,
    uint64_t idx
)
{
    if (idx >= 512)
    {
        return -1;
    }

    return (*node)[idx];
}

static int64_t nodeGetTranslatedAddr(
    mpf_vtp_pt_node* node,
    uint64_t idx
)
{
    if ((idx >= 512) || ! nodeEntryIsTerminal(node, idx))
    {
        return -1;
    }

    // Clear the flags stored in low bits
    return (*node)[idx] & ~ (int64_t)MPF_VTP_PT_FLAG_MASK;
}

static uint32_t nodeGetTranslatedAddrFlags(
    mpf_vtp_pt_node* node,
    uint64_t idx
)
{
    if ((idx >= 512) || ! nodeEntryIsTerminal(node, idx))
    {
        return 0;
    }

    return (uint32_t)((*node)[idx]) & (uint32_t)MPF_VTP_PT_FLAG_MASK;
}

static void nodeInsertChildAddr(
    mpf_vtp_pt_node* node,
    uint64_t idx,
    int64_t addr
)
{
    if (idx < 512)
    {
        (*node)[idx] = addr;
    }
}

static void nodeInsertTranslatedAddr(
    mpf_vtp_pt_node* node,
    uint64_t idx,
    int64_t addr,
    // Flags, ORed from mpf_vtp_pt_flag
    int64_t flags
)
{
    if (idx < 512)
    {
        (*node)[idx] = addr | MPF_VTP_PT_FLAG_TERMINAL | flags;
    }
}

static void nodeRemoveTranslatedAddr(
    mpf_vtp_pt_node* node,
    uint64_t idx
)
{
    if (idx < 512)
    {
        (*node)[idx] = -1;
    }
}



// ========================================================================
//
// Page table node wsid tracker.
//
// ========================================================================

//
// Add a new I/O mapped page table node to the tracker.
//
static fpga_result trackNodeWsid(
    mpf_vtp_pt* pt,
    uint64_t wsid
)
{
    struct v_to_p_wsid* trk = pt->v_to_p_wsid;

    if ((NULL == trk) || (N_V_TO_P_WSID_ENTRIES == trk->n_entries))
    {
        // Need a new tracker entry in the linked list
        trk = malloc(sizeof(struct v_to_p_wsid));
        if (NULL == trk) return FPGA_NO_MEMORY;

        // Push the new tracker entry on the head of the list
        trk->next = pt->v_to_p_wsid;
        trk->n_entries = 0;
        pt->v_to_p_wsid = trk;
    }

    // Add wsid to the tracker
    trk->wsid[trk->n_entries++] = wsid;

    return FPGA_OK;
}


//
// Release all I/O mapped page table nodes
//
static fpga_result releaseTrackedNodes(
    mpf_vtp_pt* pt
)
{
    struct v_to_p_wsid* trk = pt->v_to_p_wsid;
    pt->v_to_p_wsid = NULL;

    while (NULL != trk)
    {
        for (uint32_t i = 0; i < trk->n_entries; i++)
        {
            if (pt->_mpf_handle->dbg_mode)
            {
                MPF_FPGA_MSG("release I/O mapped TLB node wsid 0x%" PRIx64,
                             trk->wsid[i]);
            }

            assert(FPGA_OK == fpgaReleaseBuffer(pt->_mpf_handle->handle,
                                                trk->wsid[i]));
        }

        // Done with this tracker node.  Release it and move on to the next.
        struct v_to_p_wsid* trk_next = trk->next;
        free(trk);
        trk = trk_next;
    }

    return FPGA_OK;
}


// ========================================================================
//
// Internal page table manipulation functions.
//
// ========================================================================

static fpga_result addPAtoVA(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_paddr pa,
    mpf_vtp_pt_vaddr va,
    uint32_t depth
);


//
// Compute the node index at specified depth in the tree of a page table
// for the given address.
//
static uint64_t ptIdxFromAddr(
    uint64_t addr,
    uint32_t depth
)
{
    // Drop 4KB page offset
    uint64_t idx = addr >> 12;

    // Get index for requested depth
    if (depth)
    {
        idx >>= (depth * 9);
    }

    return idx & 0x1ff;
}


static fpga_result ptTranslatePAtoVA(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_paddr pa,
    mpf_vtp_pt_vaddr *va
)
{
    mpf_vtp_pt_node* table = pt->p_to_v;

    uint32_t depth = 4;
    while (depth--)
    {
        // Index in the current level
        uint64_t idx = ptIdxFromAddr((uint64_t)pa, depth);

        if (! nodeEntryExists(table, idx)) return FPGA_NOT_FOUND;

        if (nodeEntryIsTerminal(table, idx))
        {
            *va = (mpf_vtp_pt_vaddr)nodeGetTranslatedAddr(table, idx);
            return FPGA_OK;
        }

        // Walk down to child
        table = (mpf_vtp_pt_node*)nodeGetChildAddr(table, idx);
    }

    return FPGA_NOT_FOUND;
}


//
// Allocate a shared page with the FPGA.
//
static fpga_result ptAllocSharedPage(
    mpf_vtp_pt* pt,
    uint64_t length,
    mpf_vtp_pt_vaddr* va_p,
    mpf_vtp_pt_paddr* pa_p,
    uint64_t* wsid_p
)
{
    fpga_result r;

    *va_p = NULL;
    *wsid_p = 0;
    r = fpgaPrepareBuffer(pt->_mpf_handle->handle, length,
                          (void*)va_p, wsid_p, 0);
    if (r != FPGA_OK) return r;

    // Get the FPGA-side physical address
    r = fpgaGetIOAddress(pt->_mpf_handle->handle, *wsid_p, pa_p);

    if (pt->_mpf_handle->dbg_mode)
    {
        MPF_FPGA_MSG("allocate I/O mapped TLB node VA %p, PA 0x%" PRIx64 ", wsid 0x%" PRIx64,
                     *va_p, *pa_p, *wsid_p);
    }

    return r;
}


static fpga_result ptAllocTableNode(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_node** node_p,
    mpf_vtp_pt_paddr* pa_p
)
{
    mpf_vtp_pt_node* n;

    // Is a page available from the free list?
    if (pt->page_table_free_list != NULL)
    {
        // Pop page from free list
        n = pt->page_table_free_list;
        pt->page_table_free_list = (mpf_vtp_pt_node*)nodeGetChildAddr(n, 0);

        // Corresponding PA is stored in slot 1
        *pa_p = (mpf_vtp_pt_paddr)nodeGetChildAddr(n, 1);
    }
    else
    {
        // Need a new page
        fpga_result r;
        uint64_t wsid;

        // Virtual to physical map is shared with the FPGA
        r = ptAllocSharedPage(pt, sizeof(mpf_vtp_pt_node),
                              (mpf_vtp_pt_vaddr*)&n, pa_p, &wsid);
        if (r != FPGA_OK) return r;

        r = trackNodeWsid(pt, wsid);
        if (FPGA_OK != r) return r;

        // Add new page to physical to virtual translation so the table
        // can be walked in software
        r = addPAtoVA(pt, *pa_p, (mpf_vtp_pt_vaddr)n, 4);
        if (r != FPGA_OK) return r;
    }

    nodeReset(n);

    *node_p = n;
    return FPGA_OK;
}


static fpga_result ptFreeTableNode(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_node* node,
    mpf_vtp_pt_paddr pa
)
{
    // Push page on the free list
    nodeInsertChildAddr(node, 0, (int64_t)(pt->page_table_free_list));
    nodeInsertChildAddr(node, 1, (int64_t)pa);
    pt->page_table_free_list = node;

    // Invalidate the address in any hardware tables (page table walker cache)
    return mpfVtpInvalVAMapping(pt->_mpf_handle, (mpf_vtp_pt_vaddr)node);
}


static fpga_result addVAtoPA(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_vaddr va,
    mpf_vtp_pt_paddr pa,
    uint64_t wsid,
    uint32_t depth,
    uint32_t flags
)
{
    mpf_vtp_pt_node* table = pt->v_to_p;
    mpf_vtp_pt_node* wsid_table = pt->v_to_wsid;

    // Index in the leaf page
    uint64_t leaf_idx = ptIdxFromAddr((uint64_t)va, 4 - depth);

    uint32_t cur_depth = 4;
    while (--depth)
    {
        // Drop 4KB page offset
        uint64_t idx = ptIdxFromAddr((uint64_t)va, --cur_depth);

        // Need a new page in the table?
        if (! nodeEntryExists(table, idx))
        {
            mpf_vtp_pt_paddr node_p;
            mpf_vtp_pt_node* child_node;
            if (FPGA_OK != ptAllocTableNode(pt, &child_node, &node_p)) return FPGA_NO_MEMORY;

            // Add new page to the FPGA-visible virtual to physical table
            nodeInsertChildAddr(table, idx, node_p);

            // The parallel VA to wsid map should also need a new node
            assert(! nodeEntryExists(wsid_table, idx));
            mpf_vtp_pt_node* wsid_child_node = malloc(sizeof(mpf_vtp_pt_node));
            if (NULL == wsid_child_node) return FPGA_NO_MEMORY;
            nodeReset(wsid_child_node);
            nodeInsertChildAddr(wsid_table, idx, (int64_t)wsid_child_node);
        }

        // Are we being asked to add an entry below a larger region that
        // is already mapped?
        if (nodeEntryIsTerminal(table, idx)) return FPGA_EXCEPTION;

        // Continue down the tree
        mpf_vtp_pt_paddr child_pa = nodeGetChildAddr(table, idx);
        mpf_vtp_pt_vaddr child_va;
        if (FPGA_OK != ptTranslatePAtoVA(pt, child_pa, &child_va)) return FPGA_NOT_FOUND;
        table = (mpf_vtp_pt_node*)child_va;

        // Also down the parallel VA to wsid tree
        assert(nodeEntryExists(wsid_table, idx));
        wsid_table = (mpf_vtp_pt_node*)nodeGetChildAddr(wsid_table, idx);
    }

    // Now at the leaf.  Add the translation.
    if (nodeEntryExists(table, leaf_idx))
    {
        if ((cur_depth == 2) && ! nodeEntryIsTerminal(table, leaf_idx))
        {
            // Entry exists while trying to add a 2MB entry.  Perhaps there is
            // an old leaf that used to hold 4KB pages.  If the existing
            // entry has no active pages then get rid of it.
            mpf_vtp_pt_paddr child_pa = nodeGetChildAddr(table, leaf_idx);
            mpf_vtp_pt_vaddr child_va;
            if (FPGA_OK != ptTranslatePAtoVA(pt, child_pa, &child_va)) return FPGA_EXCEPTION;

            mpf_vtp_pt_node* child_node = (mpf_vtp_pt_node*)child_va;
            if (! nodeIsEmpty(child_node)) return FPGA_EXCEPTION;

            // The old page that held 4KB translations is now empty and the
            // pointer will be overwritten with a 2MB page pointer.
            ptFreeTableNode(pt, child_node, child_pa);

            // Free the parallel VA to wsid node
            free((void*)nodeGetChildAddr(wsid_table, leaf_idx));
        }
        else
        {
            return FPGA_EXCEPTION;
        }
    }

    nodeInsertTranslatedAddr(table, leaf_idx, pa, flags);
    nodeInsertChildAddr(wsid_table, leaf_idx, wsid);

    // Memory fence for updates before claiming the table is ready
    mpfOsMemoryBarrier();

    return FPGA_OK;
}


static fpga_result addPAtoVA(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_paddr pa,
    mpf_vtp_pt_vaddr va,
    uint32_t depth
)
{
    mpf_vtp_pt_node* table = pt->p_to_v;

    // Index in the leaf page
    uint64_t leaf_idx = ptIdxFromAddr((uint64_t)pa, 4 - depth);

    uint32_t cur_depth = 4;
    while (--depth)
    {
        // Drop 4KB page offset
        uint64_t idx = ptIdxFromAddr((uint64_t)pa, --cur_depth);

        // Need a new page in the table?
        if (! nodeEntryExists(table, idx))
        {
            // Add new page to the host-side-only physical to virtual table
            mpf_vtp_pt_node* child_node = malloc(sizeof(mpf_vtp_pt_node));
            if (NULL == child_node) return FPGA_NO_MEMORY;
            nodeReset(child_node);

            nodeInsertChildAddr(table, idx, (int64_t)child_node);
        }

        // Are we being asked to add an entry below a larger region that
        // is already mapped?
        if (nodeEntryIsTerminal(table, idx)) return FPGA_EXCEPTION;

        // Continue down the tree
        table = (mpf_vtp_pt_node*)nodeGetChildAddr(table, idx);
    }

    // Now at the leaf.  Add the translation.
    nodeInsertTranslatedAddr(table, leaf_idx, (int64_t)va, 0);
    return FPGA_OK;
}


//
// Release internal PA to VA mapping nodes.
//
static fpga_result freePAtoVA(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_node* table,
    uint32_t depth
)
{
    for (uint64_t idx = 0; idx < 512; idx++)
    {
        if (nodeEntryExists(table, idx) && ! nodeEntryIsTerminal(table, idx))
        {
            // Recursive walk down the tree
            assert(depth != 1);

            mpf_vtp_pt_node* child_table;
            child_table = (mpf_vtp_pt_node*)nodeGetChildAddr(table, idx);

            freePAtoVA(pt, child_table, depth - 1);
        }
    }

    // Done with this node in the table
    if (pt->_mpf_handle->dbg_mode)
    {
        MPF_FPGA_MSG("release PA->VA node %p", table);
    }
    free(table);

    return FPGA_OK;
}


//
// Called on termination to delete all mapped pages.  This only releases
// the memory returned by mpfVtpBufferAllocate().  It does not release the
// internal page table nodes.
//
// The VA to wsid table is also freed during the walk since it will no longer
// be needed.
//
static void freeAllMappedPages(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_node* table,
    mpf_vtp_pt_node* wsid_table,
    uint32_t depth)
{
    for (uint64_t idx = 0; idx < 512; idx++)
    {
        if (nodeEntryExists(table, idx))
        {
            if (nodeEntryIsTerminal(table, idx))
            {
                // Found an allocated page.  Free it.
                uint64_t wsid = nodeGetValue(wsid_table, idx);

                if (pt->_mpf_handle->dbg_mode)
                {
                    MPF_FPGA_MSG("release page wsid 0x%" PRIx64, wsid);
                }

                fpgaReleaseBuffer(pt->_mpf_handle->handle, wsid);
            }
            else
            {
                // The entry is a pointer internal to the page table.
                // Follow it to the next level.
                assert(depth != 1);

                mpf_vtp_pt_paddr child_pa = nodeGetChildAddr(table, idx);
                mpf_vtp_pt_vaddr child_va;
                assert(FPGA_OK == ptTranslatePAtoVA(pt, child_pa, &child_va));

                // Follow the parallel VA to wsid table
                mpf_vtp_pt_node* child_wsid_table;
                child_wsid_table = (mpf_vtp_pt_node*)nodeGetChildAddr(wsid_table, idx);

                freeAllMappedPages(pt, (mpf_vtp_pt_node*)child_va,
                                   child_wsid_table, depth - 1);
            }
        }
    }

    // Done with this node in the wsid_table
    free(wsid_table);
}


static void dumpPageTableVAtoPA(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_node* table,
    mpf_vtp_pt_node* wsid_table,
    uint64_t partial_va,
    uint32_t depth)
{
    for (uint64_t idx = 0; idx < 512; idx++)
    {
        if (nodeEntryExists(table, idx))
        {
            uint64_t va = partial_va | (idx << (12 + 9 * (depth - 1)));
            if (nodeEntryIsTerminal(table, idx))
            {
                // Found a translation
                const char *kind;
                switch (depth)
                {
                  case 1:
                    kind = "4KB";
                    break;
                  case 2:
                    kind = "2MB";
                    break;
                  default:
                    kind = "?";
                    break;
                }

                printf("    VA 0x%016" PRIx64 " -> PA 0x%016" PRIx64 " (%s)  wsid 0x%" PRIx64,
                       va,
                       nodeGetTranslatedAddr(table, idx),
                       kind,
                       nodeGetValue(wsid_table, idx));

                uint32_t flags = nodeGetTranslatedAddrFlags(table, idx);
                if (flags & (MPF_VTP_PT_FLAG_ALLOC_START |
                             MPF_VTP_PT_FLAG_ALLOC_END))
                {
                    printf(" [");
                    if (flags & MPF_VTP_PT_FLAG_ALLOC_START) printf(" START");
                    if (flags & MPF_VTP_PT_FLAG_ALLOC_END) printf(" END");
                    printf(" ]");
                }
                printf("\n");

                // Validate translation function
                mpf_vtp_pt_paddr check_pa;
                assert(FPGA_OK == mpfVtpPtTranslateVAtoPA(pt, (mpf_vtp_pt_vaddr)va, &check_pa, NULL));
                assert(nodeGetTranslatedAddr(table, idx) == (int64_t) check_pa);
            }
            else
            {
                // Follow pointer to another level
                assert(depth != 1);

                mpf_vtp_pt_paddr child_pa = nodeGetChildAddr(table, idx);
                mpf_vtp_pt_vaddr child_va;
                assert(FPGA_OK == ptTranslatePAtoVA(pt, child_pa, &child_va));

                // Follow the parallel VA to wsid table
                mpf_vtp_pt_node* child_wsid_table;
                child_wsid_table = (mpf_vtp_pt_node*)nodeGetChildAddr(wsid_table, idx);

                dumpPageTableVAtoPA(pt, (mpf_vtp_pt_node*)child_va,
                                    child_wsid_table, va, depth - 1);
            }
        }
    }
}


// ========================================================================
//
// Public (though still MPF internal) functions.
//
// ========================================================================

fpga_result mpfVtpPtInit(
    _mpf_handle_p _mpf_handle,
    mpf_vtp_pt** pt
)
{
    fpga_result r;
    mpf_vtp_pt* new_pt;
    uint64_t wsid;

    new_pt = malloc(sizeof(mpf_vtp_pt));
    *pt = new_pt;
    if (NULL == new_pt) return FPGA_NO_MEMORY;
    memset(new_pt, 0, sizeof(mpf_vtp_pt));

    new_pt->_mpf_handle = _mpf_handle;

    // Virtual to physical map is shared with the FPGA
    r = ptAllocSharedPage(new_pt, sizeof(mpf_vtp_pt_node),
                          (mpf_vtp_pt_vaddr*)&(new_pt->v_to_p),
                          &(new_pt->pt_root_paddr),
                          &wsid);
    if (FPGA_OK != r) return r;
    nodeReset(new_pt->v_to_p);

    r = trackNodeWsid(new_pt, wsid);
    if (FPGA_OK != r) return r;

    // Virtual to wsid is used only in software
    new_pt->v_to_wsid = malloc(sizeof(mpf_vtp_pt_node));
    if (NULL == new_pt->v_to_wsid) return FPGA_NO_MEMORY;
    nodeReset(new_pt->v_to_wsid);

    // Physical to virtual map is used only in software
    new_pt->p_to_v = malloc(sizeof(mpf_vtp_pt_node));
    if (NULL == new_pt->p_to_v) return FPGA_NO_MEMORY;
    nodeReset(new_pt->p_to_v);

    return FPGA_OK;
}


fpga_result mpfVtpPtTerm(
    mpf_vtp_pt* pt
)
{
    // Release all shared pages (VTP allocated pages, not the page table)
    freeAllMappedPages(pt, pt->v_to_p, pt->v_to_wsid, 4);
    // The v_to_wsid table was released by freeAllMappedPages.
    pt->v_to_wsid = NULL;

    // Drop the PA to VA table
    freePAtoVA(pt, pt->p_to_v, 4);
    pt->p_to_v = NULL;

    // Drop the I/O mapped virtual to physical TLB nodes
    releaseTrackedNodes(pt);

    // Release the top-level page table descriptor
    free(pt);

    return FPGA_OK;
}


mpf_vtp_pt_paddr mpfVtpPtGetPageTableRootPA(
    mpf_vtp_pt* pt
)
{
    return pt->pt_root_paddr;
}


fpga_result mpfVtpPtInsertPageMapping(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_vaddr va,
    mpf_vtp_pt_paddr pa,
    uint64_t wsid,
    mpf_vtp_page_size size,
    uint32_t flags
)
{
    // Are the addresses reasonable?
    uint64_t mask = (size == MPF_VTP_PAGE_4KB) ? (1 << 12) - 1 :
                                                 (1 << 21) - 1;
    if ((0 != ((uint64_t)va & mask)) || (0 != (pa & mask)))
    {
        return FPGA_INVALID_PARAM;
    }

    uint32_t depth = (size == MPF_VTP_PAGE_4KB) ? 4 : 3;

    return addVAtoPA(pt, va, pa, wsid, depth, flags);
}


fpga_result mpfVtpPtRemovePageMapping(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_vaddr va,
    mpf_vtp_pt_paddr *pa,
    mpf_vtp_pt_paddr *pt_pa,
    uint64_t *wsid,
    mpf_vtp_page_size *size,
    uint32_t *flags
)
{
    mpf_vtp_pt_node* table = pt->v_to_p;
    mpf_vtp_pt_node* wsid_table = pt->v_to_wsid;
    // Physical address of the page table at current depth
    mpf_vtp_pt_paddr pt_depth_pa = 0;

    uint32_t depth = 4;
    while (depth--)
    {
        // Index in the current level
        uint64_t idx = ptIdxFromAddr((uint64_t)va, depth);

        if (! nodeEntryExists(table, idx)) return FPGA_NOT_FOUND;

        if (nodeEntryIsTerminal(table, idx))
        {
            if (pa)
            {
                *pa = (mpf_vtp_pt_paddr)nodeGetTranslatedAddr(table, idx);
            }

            if (pt_pa)
            {
                // Address of the lowest PTE pointing to the entry just removed
                *pt_pa = pt_depth_pa + 8 * idx;
            }

            if (wsid)
            {
                *wsid = nodeGetValue(wsid_table, idx);
            }

            if (size)
            {
                *size = (depth == 1 ? MPF_VTP_PAGE_2MB : MPF_VTP_PAGE_4KB);
            }

            if (flags)
            {
                *flags = nodeGetTranslatedAddrFlags(table, idx);
            }

            nodeRemoveTranslatedAddr(table, idx);
            nodeRemoveTranslatedAddr(wsid_table, idx);

            mpfOsMemoryBarrier();

            return FPGA_OK;
        }

        // Walk down to child
        mpf_vtp_pt_paddr child_pa = nodeGetChildAddr(table, idx);
        mpf_vtp_pt_vaddr child_va;
        if (FPGA_OK != ptTranslatePAtoVA(pt, child_pa, &child_va)) return FPGA_NOT_FOUND;
        table = (mpf_vtp_pt_node*)child_va;
        pt_depth_pa = child_pa;

        // Follow the parallel VA to wsid table
        wsid_table = (mpf_vtp_pt_node*)nodeGetChildAddr(wsid_table, idx);
    }

    return FPGA_NOT_FOUND;
}


fpga_result mpfVtpPtTranslateVAtoPA(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_vaddr va,
    mpf_vtp_pt_paddr *pa,
    uint32_t *flags
)
{
    mpf_vtp_pt_node* table = pt->v_to_p;

    uint32_t depth = 4;
    while (depth--)
    {
        // Index in the current level
        uint64_t idx = ptIdxFromAddr((uint64_t)va, depth);

        if (! nodeEntryExists(table, idx)) return FPGA_NOT_FOUND;

        if (nodeEntryIsTerminal(table, idx))
        {
            *pa = (mpf_vtp_pt_paddr)nodeGetTranslatedAddr(table, idx);

            if (flags)
            {
                *flags = nodeGetTranslatedAddrFlags(table, idx);
            }

            return FPGA_OK;
        }

        // Walk down to child
        mpf_vtp_pt_paddr child_pa = nodeGetChildAddr(table, idx);
        mpf_vtp_pt_vaddr child_va;
        if (FPGA_OK != ptTranslatePAtoVA(pt, child_pa, &child_va)) return FPGA_NOT_FOUND;
        table = (mpf_vtp_pt_node*)child_va;
    }

    return FPGA_NOT_FOUND;
}


void mpfVtpPtDumpPageTable(
    mpf_vtp_pt* pt
)
{
    printf("  Page table root VA %p -> PA 0x%016" PRIx64 ":\n",
           pt->v_to_p,
           mpfVtpPtGetPageTableRootPA(pt));

    dumpPageTableVAtoPA(pt, pt->v_to_p, pt->v_to_wsid, 0, 4);
}
