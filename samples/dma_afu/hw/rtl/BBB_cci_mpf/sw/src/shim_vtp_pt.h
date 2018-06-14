// Copyright(c) 2017, Intel Corporation
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
 * \file mpf_shim_vtp_pt.h
 * \brief Internal functions and data structures for managing VTP page tables.
 */

#ifndef __FPGA_MPF_SHIM_VTP_PT_H__
#define __FPGA_MPF_SHIM_VTP_PT_H__

#include <opae/mpf/shim_vtp.h>

/**
 * Flags that may be set in page table entries.  These are ORed into the low
 * address bits which are guaranteed to be 0 since the smallest page is 4KB
 * aligned.
 */
typedef enum
{
    // Terminal entry in table hierarchy -- indicates an actual address
    // translation as opposed to an intra-table pointer
    MPF_VTP_PT_FLAG_TERMINAL = 1,
    // Is entry the first or last block in an allocated region?  These
    // flags are used only in virtual to physical translation tables.
    MPF_VTP_PT_FLAG_ALLOC_START = 2,
    MPF_VTP_PT_FLAG_ALLOC_END = 4,

    // All flags (mask)
    MPF_VTP_PT_FLAG_MASK = 7
}
mpf_vtp_pt_flag;


/**
 * The VTP page table is structured like the standard x86 hierarchical table.
 * Each level in the table is an array of 512 pointers to the next level
 * in the table (512 * 8B == 4KB).  Eventually, a terminal node is reached
 * containing the translation.
 */
typedef int64_t mpf_vtp_pt_node[512];


/**
 * Physical address.
 */
typedef uint64_t mpf_vtp_pt_paddr;


/**
 * Virtual address.
 */
typedef void* mpf_vtp_pt_vaddr;


#define N_V_TO_P_WSID_ENTRIES 510

/**
 * Nodes in the page table's virtual to physical map are in pinned memory,
 * shared with the FPGA.  This data structure tracks the page table
 * node's workspace IDs so the table can be released.
 */
struct v_to_p_wsid
{
    // Next tracker (linked list)
    struct v_to_p_wsid* next;
    uint64_t n_entries;
    uint64_t wsid[N_V_TO_P_WSID_ENTRIES];
};


/**
 * VTP page table handle to all page table state.
 */
typedef struct
{
    // Virtual to physical page hierarchical page table.  This is the table
    // that is passed to the FPGA.
    mpf_vtp_pt_node* v_to_p;

    // The FPGA driver manages I/O mapped pages using workspace IDs.
    // The page table manager builds a parallel structure to v_to_p
    // for translating virtual addresses to wsids.
    mpf_vtp_pt_node* v_to_wsid;

    // The page table is implemented in user space with no access
    // to kernel page mapping.  In order to walk ptVtoP in software we
    // need to record a reverse physical to virtual mapping.
    mpf_vtp_pt_node* p_to_v;

    // Physical address of the root of the page table
    mpf_vtp_pt_paddr pt_root_paddr;

    // Free list of tree nodes not currently in use
    mpf_vtp_pt_node* page_table_free_list;

    // Track workspace IDs of pinned page table nodes
    struct v_to_p_wsid* v_to_p_wsid;

    // Opaque parent MPF handle.  It is opaque because the internal MPF handle
    // points to the page table, so the dependence would be circular.
    _mpf_handle_p _mpf_handle;
}
mpf_vtp_pt;


/**
 * Initialize a page table manager.
 *
 * Allocates and initializes a page table.
 *
 * @param[in]  _mpf_handle Internal handle to MPF state.
 * @param[out] pt          Allocated page table.
 * @returns                FPGA_OK on success.
 */
fpga_result mpfVtpPtInit(
    _mpf_handle_p _mpf_handle,
    mpf_vtp_pt** pt
);


/**
 * Destroy a page table manager.
 *
 * Terminate and deallocate a page table.
 *
 * @param[in]  pt          Page table.
 * @returns                FPGA_OK on success.
 */
fpga_result mpfVtpPtTerm(
    mpf_vtp_pt* pt
);


/**
 * Return the root physical address of the page table.
 *
 * @param[in]  pt          Page table.
 * @returns                Root PA.
 */
mpf_vtp_pt_paddr mpfVtpPtGetPageTableRootPA(
    mpf_vtp_pt* pt
);


/**
 * Insert a mapping in the page table.
 *
 * @param[in]  pt          Page table.
 * @param[in]  va          Virtual address to insert.
 * @param[in]  pa          Physical address corresponding to the virtual address.
 * @param[in]  wsid        Driver's handle to the page.
 * @param[in]  size        Size of the page.
 * @param[in]  flags       ORed mpf_vtp_pt_flag values.
 * @returns                FPGA_OK on success.
 */
fpga_result mpfVtpPtInsertPageMapping(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_vaddr va,
    mpf_vtp_pt_paddr pa,
    uint64_t wsid,
    mpf_vtp_page_size size,
    uint32_t flags
);


/**
 * Remove a page from the table.
 *
 * Some state from the page is returned as it is dropped.
 * State pointers are not written if they are NULL.
 *
 * @param[in]  pt          Page table.
 * @param[in]  va          Virtual address to remove.
 * @param[out] pa          PA corresponding to VA.  (Ignored if NULL.)
 * @param[out] pt_pa       PA of the page table entry holding page translation.
 *                         (Ignored if NULL.)
 * @param[out] wsid        Workspace ID corresponding to VA.  (Ignored if NULL.)
 * @param[out] size        Physical page size.  (Ignored if NULL.)
 * @param[out] flags       Page flags.  (Ignored if NULL.)
 * @returns                FPGA_OK on success.
 */
fpga_result mpfVtpPtRemovePageMapping(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_vaddr va,
    mpf_vtp_pt_paddr *pa,
    mpf_vtp_pt_paddr *pt_pa,
    uint64_t *wsid,
    mpf_vtp_page_size *size,
    uint32_t *flags
);


/**
 * Translate an address from virtual to physical.
 *
 * @param[in]  pt          Page table.
 * @param[in]  va          Virtual address to remove.
 * @param[out] pa          PA corresponding to VA.
 * @param[out] flags       Page flags.  (Ignored if NULL.)
 * @returns                FPGA_OK on success.
 */
fpga_result mpfVtpPtTranslateVAtoPA(
    mpf_vtp_pt* pt,
    mpf_vtp_pt_vaddr va,
    mpf_vtp_pt_paddr *pa,
    uint32_t *flags
);


/**
 * Dump page table for debugging.
 *
 * @param[in]  pt          Page table.
 */
void mpfVtpPtDumpPageTable(
    mpf_vtp_pt* pt
);

#endif // __FPGA_MPF_SHIM_VTP_PT_H__
