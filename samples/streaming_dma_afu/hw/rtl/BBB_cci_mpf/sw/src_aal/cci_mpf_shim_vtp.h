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
/// @file VTPService-internal.h
/// @brief Definitions for VTP Service.
/// @ingroup VTPService
/// @verbatim
/// Virtual-to-Physical address translation component class
///
/// Provides methods for access to the VTP feature for address translation.
/// Assumes a VTP BBB DFH to be detected and present.
///
/// On initialization, allocates shared buffer for VTP page hash and
/// communicates its location to VTP feature.
///
/// Provides synchronous methods to update page hash on shared buffer
/// allocation.
///
/// Note: this is not an AAL service, but a component of the MPF service (which
/// is).
///
/// AUTHORS: Enno Luebbers, Intel Corporation
///          Michael Adler, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 01/15/2016     EL       Initial version@endverbatim
//****************************************************************************
#ifndef __CCI_MPF_SHIM_VTP_H__
#define __CCI_MPF_SHIM_VTP_H__
#include <aalsdk/aas/AALService.h>
#include <aalsdk/IServiceClient.h>
#include <aalsdk/osal/IDispatchable.h>
#include <aalsdk/utils/Utilities.h>

#include <aalsdk/service/IALIAFU.h>
#include <aalsdk/uaia/IAFUProxy.h>

#include <aalsdk/mpf/IMPF.h>              // Public MPF service interface
#include <aalsdk/mpf/MPFService.h>

#include "cci_mpf_shim_vtp_pt.h"


BEGIN_NAMESPACE(AAL)

/// @addtogroup VTPService
/// @{

class MPFVTP : public CAASBase, public IMPFVTP, private MPFVTP_PAGE_TABLE
{
public:

   /// VTP constructor
   MPFVTP( IALIBuffer *pBufferService,
           IALIMMIO   *pMMIOService,
           btCSROffset vtpDFHOffset );

   // <IVTP>
   ali_errnum_e bufferAllocate( btWSSize             Length,
                                btVirtAddr          *pBufferptr )
   {
      return bufferAllocate(Length, pBufferptr, AAL::NamedValueSet());
   }
   ali_errnum_e bufferAllocate( btWSSize             Length,
                                btVirtAddr          *pBufferptr,
                                NamedValueSet const &rInputArgs )
   {
      NamedValueSet temp = NamedValueSet();
      return bufferAllocate(Length, pBufferptr, rInputArgs, temp);
   }
   ali_errnum_e bufferAllocate( btWSSize             Length,
                                btVirtAddr          *pBufferptr,
                                NamedValueSet const &rInputArgs,
                                NamedValueSet       &rOutputArgs );
   ali_errnum_e bufferFree(     btVirtAddr           Address );

   btPhysAddr   bufferGetIOVA(  btVirtAddr           Address );

   // invalidate FPGA-side translation cache
   btBool vtpReset( void );

   btBool isOK( void ) { return m_isOK; }     // < status after initialization

   // Return all statistics counters
   btBool vtpGetStats( t_cci_mpf_vtp_stats *stats );

   // Return a statistics counter
   btUnsigned64bitInt vtpGetStatCounter( t_cci_mpf_vtp_csr_offsets stat );

protected:
   IALIBuffer            *m_pALIBuffer;
   IALIMMIO              *m_pALIMMIO;
   btCSROffset            m_dfhOffset;

   btBool                 m_isOK;

private:
   // Page allocator used by MPFVTP_PAGE_TABLE to add pages to the
   // shared page table data structure.
   btVirtAddr ptAllocSharedPage(btWSSize length, btPhysAddr* pa);
   // Invalidate a VA mapping.
   bool ptInvalVAMapping(btVirtAddr va);

   static const size_t SMALL_PAGE_SIZE = KB(4);
   static const size_t SMALL_PAGE_MASK = ~(SMALL_PAGE_SIZE - 1);
   static const size_t LARGE_PAGE_SIZE = MB(2);
   static const size_t LARGE_PAGE_MASK = ~(LARGE_PAGE_SIZE - 1);

   static const size_t CCI_MPF_VTP_LARGE_PAGE_THRESHOLD = KB(128);

   ali_errnum_e _allocate(btVirtAddr va, size_t pageSize, uint32_t flags = 0);
   // reinitialize VTP registers after vtpReset
   btBool _vtpEnable( void );

};



/// @}

END_NAMESPACE(AAL)

#endif //__VTP_H__

