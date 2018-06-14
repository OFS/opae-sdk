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
/// @file cci_mpf_shim_vtp_pt.h
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

#ifndef __CCI_MPF_SHIM_VTP_PT_H__
#define __CCI_MPF_SHIM_VTP_PT_H__

#include <aalsdk/AALTypes.h>
#include <aalsdk/utils/Utilities.h>

BEGIN_NAMESPACE(AAL)

/// @addtogroup VTPService
/// @{

//
// The page table supports two physical page sizes.
//
typedef enum
{
    MPFVTP_PAGE_4KB,
    MPFVTP_PAGE_2MB
}
MPFVTP_PAGE_SIZE;

//
// Flags that may be set in page table entries.  These are ORed into the low
// address bits which are guaranteed to be 0 since the smallest page is 4KB
// aligned.
//
typedef enum
{
    // Terminal entry in table hierarchy -- indicates an actual address
    // translation as opposed to an intra-table pointer
    MPFVTP_PT_FLAG_TERMINAL = 1,
    // Is entry the first or last block in an allocated region?  These
    // flags are used only in virtual to physical translation tables.
    MPFVTP_PT_FLAG_ALLOC_START = 2,
    MPFVTP_PT_FLAG_ALLOC_END = 4,

    // All flags (mask)
    MPFVTP_PT_FLAG_MASK = 7
}
MPFVTP_PT_FLAG;

typedef class MPFVTP_PT_TREE_CLASS* MPFVTP_PT_TREE;


//
// MPFVTP_PAGE_TABLE -- Page table management.
//
//  This page table management code supports multiple versions of AAL
//  and multiple FPGA interfaces.  In order to do this it uses only a
//  handful of AAL types and include files.  Otherwise, types are
//  standard C types.
//
class MPFVTP_PAGE_TABLE
{
  public:
    // VTP page table constructor
    MPFVTP_PAGE_TABLE();
    ~MPFVTP_PAGE_TABLE();

    // Initialize page table
    bool ptInitialize();

    // Return the physical address of the root of the page table.  This
    // address must be passed to the FPGA-side page table walker.
    btPhysAddr ptGetPageTableRootPA() const;

    // Add a new page to the table
    bool ptInsertPageMapping(btVirtAddr va,
                             btPhysAddr pa,
                             MPFVTP_PAGE_SIZE size,
                             // ORed MPFVTP_PT_FLAG values
                             uint32_t flags = 0);

    // Remove a page from the table, returning some state from the page
    // as it is dropped.  State pointers are not written if they are NULL.
    bool ptRemovePageMapping(btVirtAddr va,
                             // PA corresponding to VA
                             btPhysAddr *pa = NULL,
                             // PA of the page table entry holding page translation
                             btPhysAddr *pt_pa = NULL,
                             // Physical page size
                             MPFVTP_PAGE_SIZE *size = NULL,
                             uint32_t *flags = NULL);

    // Translate an address from virtual to physical.
    bool ptTranslateVAtoPA(btVirtAddr va,
                           btPhysAddr *pa,
                           uint32_t *flags = NULL);

    // Dump the page table (debugging)
    void ptDumpPageTable();

  private:
    // The parent class must provide a method for allocating memory
    // shared with the FPGA, used here to construct the page table that
    // will be walked in hardware.
    virtual btVirtAddr ptAllocSharedPage(btWSSize length, btPhysAddr* pa) = 0;
    virtual bool ptInvalVAMapping(btVirtAddr va) = 0;

  private:
    // Virtual to physical page hierarchical page table.  This is the table
    // that is passed to the FPGA.
    MPFVTP_PT_TREE ptVtoP;

    // Because the page table is implemented in user space with no access
    // to kernel page mapping.  In order to walk ptVtoP in software we need
    // to record a reverse physical to virtual mapping.
    MPFVTP_PT_TREE ptPtoV;

    btPhysAddr m_pPageTablePA;

    // Allocate an internal page table page.
    btVirtAddr ptAllocTablePage(btPhysAddr* pa);
    void ptFreeTablePage(btVirtAddr va, btPhysAddr pa);
    btVirtAddr m_pageTableFreeList;

    // Add a virtual to physical mapping at depth in the tree.  Returns
    // false if a mapping already exists.
    bool AddVAtoPA(btVirtAddr va, btPhysAddr pa, uint32_t depth, uint32_t flags);

    // Add a physical to virtual mapping at depth in the tree.  Returns
    // false if a mapping already exists.
    bool AddPAtoVA(btPhysAddr pa, btVirtAddr va, uint32_t depth);

    bool ptTranslatePAtoVA(btPhysAddr pa, btVirtAddr *va);

    void DumpPageTableVAtoPA(MPFVTP_PT_TREE table,
                             uint64_t partial_va,
                             uint32_t depth);

    uint32_t ptIdxFromAddr(uint64_t addr, uint32_t depth)
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
};


//
// Hierarchical page table class.  This class is used to build a table
// similar to x86_64 page tables, where 9-bit chunks of an address are
// direct mapped into a tree of 512 entry (4KB) pointers.
//
class MPFVTP_PT_TREE_CLASS
{
  public:
    MPFVTP_PT_TREE_CLASS()
    {
        Reset();
    }

    ~MPFVTP_PT_TREE_CLASS() {};

    void Reset()
    {
        memset(table, -1, sizeof(table));
    }

    bool TableIsEmpty()
    {
        for (int idx = 0; idx < 512; idx++)
        {
            if (table[idx] != -1) return false;
        }

        return true;
    }

    // Does an entry exist at the index?
    bool EntryExists(uint32_t idx)
    {
        if (idx >= 512)
        {
            return false;
        }

        return (table[idx] != -1);
    }

    // Is the entry at idx terminal? If so, use GetTranslatedAddr(). If not,
    // use GetChildAddr().
    bool EntryIsTerminal(uint32_t idx)
    {
        if (idx >= 512)
        {
            return false;
        }

        return (table[idx] & MPFVTP_PT_FLAG_TERMINAL) != 0;
    }

    // Walk the tree.  Ideally this would be a pointer to another
    // MPFVTP_PT_TREE, but that doesn't work for the virtually indexed
    // table since it holds physical addresses.  The caller will have
    // to interpret the child pointer depending on the type of tree.
    int64_t GetChildAddr(uint32_t idx)
    {
        if ((idx >= 512) || EntryIsTerminal(idx))
        {
            return -1;
        }

        return table[idx];
    }

    int64_t GetTranslatedAddr(uint32_t idx)
    {
        if ((idx >= 512) || ! EntryIsTerminal(idx))
        {
            return -1;
        }

        // Clear the flags stored in low bits
        return table[idx] & ~ int64_t(MPFVTP_PT_FLAG_MASK);
    }

    uint32_t GetTranslatedAddrFlags(uint32_t idx)
    {
        if ((idx >= 512) || ! EntryIsTerminal(idx))
        {
            return 0;
        }

        return uint32_t(table[idx]) & uint32_t(MPFVTP_PT_FLAG_MASK);
    }

    void InsertChildAddr(uint32_t idx, int64_t addr)
    {
        if (idx < 512)
        {
            table[idx] = addr;
        }
    }

    void InsertTranslatedAddr(uint32_t idx, int64_t addr,
                              // Flags, ORed from MPFVTP_PT_FLAG
                              int64_t flags = 0)
    {
        if (idx < 512)
        {
            table[idx] = addr | MPFVTP_PT_FLAG_TERMINAL | flags;
        }
    }

    void RemoveTranslatedAddr(uint32_t idx)
    {
        if (idx < 512)
        {
            table[idx] = -1;
        }
    }

  private:
    int64_t table[512];
};


/// @}

END_NAMESPACE(AAL)

#endif // __CCI_MPF_SHIM_VTP_PT_H__
