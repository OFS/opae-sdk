// Copyright(c) 2015-2016, Intel Corporation
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

//****************************************************************************
/// @file cci_mpf_shim_vtp_pt.cpp
/// @brief Page table creation for MPF VTP service.
/// @ingroup VTPService
/// @verbatim
///
/// Construct a page table for translating virtual addresses shared between
/// FPGA and host process to physical addresses.
///
/// Note: this is not an AAL service, but a component of the MPF service (which
/// is).
///
/// AUTHOR: Michael Adler, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 03/05/2016     MA       Initial version
/// @endverbatim
//****************************************************************************

#include <assert.h>
#if __cplusplus > 199711L
#include <atomic>
#endif

#include "cci_mpf_shim_vtp_pt.h"


BEGIN_NAMESPACE(AAL)


/////////////////////////////////////////////////////////////////////////////
//////                                                                ///////
//////                                                                ///////
/////                    VTP Page Table Management                     //////
//////                                                                ///////
//////                                                                ///////
/////////////////////////////////////////////////////////////////////////////

//
// The page table managed here looks very much like a standard x86_64
// hierarchical page table.  It is composed of a tree of 4KB pages, with
// each page holding a vector of 512 64-bit physical addresses.  Each level
// in the tree is selected directly from 9 bit chunks of a virtual address.
// Like x86_64 processors, only the low 48 bits of the virtual address are
// mapped.  Bits 39-47 select the index in the root of the page table,
// which is a physical address pointing to the mapping of the 2nd level
// bits 30-38, etc.  The search proceeds down the tree until a mapping is
// found.
//
// Since pages are aligned on at least 4KB boundaries, at least the low
// 12 bits of any value in the table are zero.  We use some of these bits
// as flags.  Bit 0 set indicates the mapping is complete.  The current
// implementation supports both 4KB and 2MB pages.  Bit 0 will be set
// after searching 3 levels for 2MB pages and 4 levels for 4KB pages.
// Bit 1 indicates no mapping exists and the search has failed.
//
// | 47 ---- 39 | 38 ---- 30 | 29 ---- 21 | 20 ---- 12 | 11 ---------- 0 |
//     9 bits       9 bits       9 bits       9 bits   ^ 4KB page offset ^
//                                        ^        2MB page offset       ^
//
//

/// @addtogroup VTPService
/// @{

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

MPFVTP_PAGE_TABLE::MPFVTP_PAGE_TABLE() :
    m_pageTableFreeList(NULL)
{
}


MPFVTP_PAGE_TABLE::~MPFVTP_PAGE_TABLE()
{
    // We should release the page table here.
}


bool
MPFVTP_PAGE_TABLE::ptInitialize()
{
    // Allocate the roots of both the virtual to physical page table passed
    // to the FPGA and the reverse physical to virtual table used in
    // this module to walk the virtual to physical table.

    // VtoP is shared with the hardware
    ptVtoP = MPFVTP_PT_TREE(ptAllocSharedPage(sizeof(MPFVTP_PT_TREE_CLASS),
                                              &m_pPageTablePA));
    ptVtoP->Reset();

    // PtoV is private to software
    ptPtoV = new MPFVTP_PT_TREE_CLASS();

    return true;
}


btPhysAddr
MPFVTP_PAGE_TABLE::ptGetPageTableRootPA() const
{
    return m_pPageTablePA;
}


bool
MPFVTP_PAGE_TABLE::ptInsertPageMapping(
    btVirtAddr va,
    btPhysAddr pa,
    MPFVTP_PAGE_SIZE size,
    uint32_t flags)
{
    // Are the addresses reasonable?
    uint64_t mask = (size == MPFVTP_PAGE_4KB) ? (1 << 12) - 1 :
                                                (1 << 21) - 1;
    assert((uint64_t(va) & mask) == 0);
    assert((pa & mask) == 0);

    uint32_t depth = (size == MPFVTP_PAGE_4KB) ? 4 : 3;

    return AddVAtoPA(va, pa, depth, flags);
}


bool
MPFVTP_PAGE_TABLE::ptRemovePageMapping(
    btVirtAddr va,
    btPhysAddr *pa,
    btPhysAddr *pt_pa,
    MPFVTP_PAGE_SIZE *size,
    uint32_t *flags)
{
    MPFVTP_PT_TREE table = ptVtoP;
    // Physical address of the page table at current depth
    btPhysAddr pt_depth_pa = 0;

    uint32_t depth = 4;
    while (depth--)
    {
        // Index in the current level
        uint64_t idx = ptIdxFromAddr(uint64_t(va), depth);

        if (! table->EntryExists(idx)) return false;

        if (table->EntryIsTerminal(idx))
        {
            if (pa)
            {
                *pa = btPhysAddr(table->GetTranslatedAddr(idx));
            }

            if (pt_pa)
            {
                // Address of the lowest PTE pointing to the entry just removed
                *pt_pa = pt_depth_pa + 8 * idx;
            }

            if (size)
            {
                *size = (depth == 1 ? MPFVTP_PAGE_2MB : MPFVTP_PAGE_4KB);
            }

            if (flags)
            {
                *flags = table->GetTranslatedAddrFlags(idx);
            }

            table->RemoveTranslatedAddr(idx);

#if __cplusplus > 199711L
            // C++11 knows atomics
            std::atomic_thread_fence(std::memory_order_seq_cst);
#elif __GNUC__
            // GNU C++ before C++11 should be able to do the same using inline asm
            asm volatile ("" : : : "memory");
#else
#   warning "Neither C++ 11 atomics nor GNU asm volatile - don't know how to do memory barriers."
#endif

            return true;
        }

        // Walk down to child
        btPhysAddr child_pa = table->GetChildAddr(idx);
        btVirtAddr child_va;
        if (! ptTranslatePAtoVA(child_pa, &child_va)) return false;
        table = MPFVTP_PT_TREE(child_va);
        pt_depth_pa = child_pa;
    }

    return false;
}


bool
MPFVTP_PAGE_TABLE::ptTranslateVAtoPA(btVirtAddr va,
                                     btPhysAddr *pa,
                                     uint32_t *flags)
{
    MPFVTP_PT_TREE table = ptVtoP;

    uint32_t depth = 4;
    while (depth--)
    {
        // Index in the current level
        uint64_t idx = ptIdxFromAddr(uint64_t(va), depth);

        if (! table->EntryExists(idx)) return false;

        if (table->EntryIsTerminal(idx))
        {
            *pa = btPhysAddr(table->GetTranslatedAddr(idx));

            if (flags)
            {
                *flags = table->GetTranslatedAddrFlags(idx);
            }

            return true;
        }

        // Walk down to child
        btPhysAddr child_pa = table->GetChildAddr(idx);
        btVirtAddr child_va;
        if (! ptTranslatePAtoVA(child_pa, &child_va)) return false;
        table = MPFVTP_PT_TREE(child_va);
    }

    return false;
}


void
MPFVTP_PAGE_TABLE::ptDumpPageTable()
{
    printf("  Page table root VA 0x%016lx -> PA 0x%016lx:\n",
           ptVtoP,
           ptGetPageTableRootPA());
    DumpPageTableVAtoPA(ptVtoP, 0, 4);
}


//-----------------------------------------------------------------------------
// Private functions
//-----------------------------------------------------------------------------

bool
MPFVTP_PAGE_TABLE::ptTranslatePAtoVA(btPhysAddr pa, btVirtAddr *va)
{
    MPFVTP_PT_TREE table = ptPtoV;

    uint32_t depth = 4;
    while (depth--)
    {
        // Index in the current level
        uint64_t idx = ptIdxFromAddr(uint64_t(pa), depth);

        if (! table->EntryExists(idx)) return false;

        if (table->EntryIsTerminal(idx))
        {
            *va = btVirtAddr(table->GetTranslatedAddr(idx));
            return true;
        }

        // Walk down to child
        table = MPFVTP_PT_TREE(table->GetChildAddr(idx));
    }

    return false;
}


btVirtAddr
MPFVTP_PAGE_TABLE::ptAllocTablePage(btPhysAddr* pa)
{
    btVirtAddr va;

    // Is a page available from the free list?
    if (m_pageTableFreeList != NULL)
    {
        // Pop page from free list
        va = m_pageTableFreeList;
        MPFVTP_PT_TREE t = MPFVTP_PT_TREE(va);
        m_pageTableFreeList = btVirtAddr(t->GetChildAddr(0));

        // Corresponding PA is stored in slot 1
        *pa = btPhysAddr(t->GetChildAddr(1));
    }
    else
    {
        // Need a new page
        va = ptAllocSharedPage(sizeof(MPFVTP_PT_TREE_CLASS), pa);
        assert(va != NULL);

        // Add new page to physical to virtual translation so the table
        // can be walked in software
        assert(AddPAtoVA(*pa, va, 4));
    }

    return va;
}


void
MPFVTP_PAGE_TABLE::ptFreeTablePage(btVirtAddr va, btPhysAddr pa)
{
    // Push page on the free list
    MPFVTP_PT_TREE t = MPFVTP_PT_TREE(va);
    t->InsertChildAddr(0, int64_t(m_pageTableFreeList));
    t->InsertChildAddr(1, int64_t(pa));
    m_pageTableFreeList = va;

    // Invalidate the address in any hardware tables (page table walker cache)
    assert(ptInvalVAMapping(va));
}


bool
MPFVTP_PAGE_TABLE::AddVAtoPA(btVirtAddr va, btPhysAddr pa, uint32_t depth, uint32_t flags)
{
    MPFVTP_PT_TREE table = ptVtoP;

    // Index in the leaf page
    uint64_t leaf_idx = ptIdxFromAddr(uint64_t(va), 4 - depth);

    uint32_t cur_depth = 4;
    while (--depth)
    {
        // Drop 4KB page offset
        uint64_t idx = ptIdxFromAddr(uint64_t(va), --cur_depth);

        // Need a new page in the table?
        if (! table->EntryExists(idx))
        {
            btPhysAddr pt_p;
            btVirtAddr pt_v = ptAllocTablePage(&pt_p);
            MPFVTP_PT_TREE child_table = MPFVTP_PT_TREE(pt_v);
            child_table->Reset();

            // Add new page to the FPGA-visible virtual to physical table
            table->InsertChildAddr(idx, pt_p);
        }

        // Are we being asked to add an entry below a larger region that
        // is already mapped?
        if (table->EntryIsTerminal(idx)) return false;

        // Continue down the tree
        btPhysAddr child_pa = table->GetChildAddr(idx);
        btVirtAddr child_va;
        if (! ptTranslatePAtoVA(child_pa, &child_va)) return false;
        table = MPFVTP_PT_TREE(child_va);
    }

    // Now at the leaf.  Add the translation.
    if (table->EntryExists(leaf_idx))
    {
        if ((cur_depth == 2) && ! table->EntryIsTerminal(leaf_idx))
        {
            // Entry exists while trying to add a 2MB entry.  Perhaps there is
            // an old leaf that used to hold 4KB pages.  If the existing
            // entry has no active pages then get rid of it.
            btPhysAddr child_pa = table->GetChildAddr(leaf_idx);
            btVirtAddr child_va;
            if (! ptTranslatePAtoVA(child_pa, &child_va)) return false;
            MPFVTP_PT_TREE child_table = MPFVTP_PT_TREE(child_va);

            if (! child_table->TableIsEmpty()) return false;

            // The old page that held 4KB translations is now empty and the
            // pointer will be overwritten with a 2MB page pointer.
            ptFreeTablePage(child_va, child_pa);
        }
        else
        {
            return false;
        }
    }

    table->InsertTranslatedAddr(leaf_idx, pa, flags);

    // Memory fence for updates before claiming the table is ready
#if __cplusplus > 199711L
    // C++11 knows atomics
    std::atomic_thread_fence(std::memory_order_seq_cst);
#elif __GNUC__
    // GNU C++ before C++11 should be able to do the same using inline asm
    asm volatile ("" : : : "memory");
#else
#   warning "Neither C++ 11 atomics nor GNU asm volatile - don't know how to do memory barriers."
#endif

    return true;
}


bool
MPFVTP_PAGE_TABLE::AddPAtoVA(btPhysAddr pa, btVirtAddr va, uint32_t depth)
{
    MPFVTP_PT_TREE table = ptPtoV;

    // Index in the leaf page
    uint64_t leaf_idx = ptIdxFromAddr(uint64_t(pa), 4 - depth);

    uint32_t cur_depth = 4;
    while (--depth)
    {
        // Drop 4KB page offset
        uint64_t idx = ptIdxFromAddr(uint64_t(pa), --cur_depth);

        // Need a new page in the table?
        if (! table->EntryExists(idx))
        {
            // Add new page to the FPGA-visible virtual to physical table
            MPFVTP_PT_TREE child_table = new MPFVTP_PT_TREE_CLASS();
            if (child_table == NULL) return false;

            table->InsertChildAddr(idx, int64_t(child_table));
        }

        // Are we being asked to add an entry below a larger region that
        // is already mapped?
        if (table->EntryIsTerminal(idx)) return false;

        // Continue down the tree
        table = MPFVTP_PT_TREE(table->GetChildAddr(idx));
    }

    // Now at the leaf.  Add the translation.
    table->InsertTranslatedAddr(leaf_idx, int64_t(va));
    return true;
}


void
MPFVTP_PAGE_TABLE::DumpPageTableVAtoPA(
    MPFVTP_PT_TREE table,
    uint64_t partial_va,
    uint32_t depth)
{
    for (uint64_t idx = 0; idx < 512; idx++)
    {
        if (table->EntryExists(idx))
        {
            uint64_t va = partial_va | (idx << (12 + 9 * (depth - 1)));
            if (table->EntryIsTerminal(idx))
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

                printf("    VA 0x%016lx -> PA 0x%016lx (%s)",
                       va,
                       table->GetTranslatedAddr(idx),
                       kind);

                uint32_t flags = table->GetTranslatedAddrFlags(idx);
                if (flags & (MPFVTP_PT_FLAG_ALLOC_START |
                             MPFVTP_PT_FLAG_ALLOC_END))
                {
                    printf(" [");
                    if (flags & MPFVTP_PT_FLAG_ALLOC_START) printf(" START");
                    if (flags & MPFVTP_PT_FLAG_ALLOC_END) printf(" END");
                    printf(" ]");
                }
                printf("\n");

                // Validate translation function
                btPhysAddr check_pa;
                assert(ptTranslateVAtoPA(btVirtAddr(va), &check_pa));
                assert(check_pa == table->GetTranslatedAddr(idx));
            }
            else
            {
                // Follow pointer to another level
                assert(depth != 1);

                btPhysAddr child_pa = table->GetChildAddr(idx);
                btVirtAddr child_va;
                assert(ptTranslatePAtoVA(child_pa, &child_va));
                DumpPageTableVAtoPA(MPFVTP_PT_TREE(child_va), va, depth - 1);
            }
        }
    }
}


/// @} group VTPService

END_NAMESPACE(AAL)
