// Copyright(c) 2016, Intel Corporation
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
/// @file cci_mpf_shim_pwrite.h
/// @brief Definitions for PWRITE Service.
/// @ingroup PWRITEService
/// @verbatim
/// Partial write emulation using read-modify-write.
///
/// Note: this is not an AAL service, but a component of the MPF service (which
/// is).
///
/// AUTHOR:  Michael Adler, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 07/21/2016     MA       Initial version
/// @endverbatim
//****************************************************************************
#ifndef __CCI_MPF_SHIM_PWRITE_H__
#define __CCI_MPF_SHIM_PWRITE_H__

#include <aalsdk/mpf/IMPF.h>              // Public MPF service interface
#include <aalsdk/mpf/MPFService.h>


BEGIN_NAMESPACE(AAL)

/// @addtogroup PWRITEService
/// @{

class MPFPWRITE : public CAASBase, public IMPFPWRITE
{
public:

   /// PWRITE constructor
   MPFPWRITE( IALIMMIO   *pMMIOService,
              btCSROffset pwriteDFHOffset );

   btBool isOK( void ) { return m_isOK; }     // < status after initialization

   // Return all statistics counters
   btBool pwriteGetStats( t_cci_mpf_pwrite_stats *stats );

   // Return a statistics counter
   btUnsigned64bitInt pwriteGetStatCounter( t_cci_mpf_pwrite_csr_offsets stat );

protected:
   IALIMMIO              *m_pALIMMIO;
   btCSROffset            m_dfhOffset;

   btBool                 m_isOK;

};

/// @}

END_NAMESPACE(AAL)

#endif // __CCI_MPF_SHIM_PWRITE_H__
