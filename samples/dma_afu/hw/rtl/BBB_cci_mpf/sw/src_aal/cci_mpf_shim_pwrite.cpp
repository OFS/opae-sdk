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
/// @file cci_mpf_shim_pwrite.cpp
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
#ifdef HAVE_CONFIG_H
# include <aalsdk/mpf/config.h>
#endif // HAVE_CONFIG_H

#include <aalsdk/AAL.h>
#include <aalsdk/aas/AALServiceModule.h>
#include <aalsdk/osal/Sleep.h>

#include <aalsdk/AALLoggerExtern.h>              // Logger
#include <aalsdk/utils/AALEventUtilities.h>      // Used for UnWrapAndReThrow()

#include <aalsdk/aas/AALInProcServiceFactory.h>  // Defines InProc Service Factory
#include <aalsdk/service/IALIAFU.h>

#include "cci_mpf_shim_pwrite.h"

BEGIN_NAMESPACE(AAL)


//=============================================================================
// Typedefs and Constants
//=============================================================================



/////////////////////////////////////////////////////////////////////////////
//////                                                                ///////
//////                                                                ///////
/////                        PWRITE Service                            //////
//////                                                                ///////
//////                                                                ///////
/////////////////////////////////////////////////////////////////////////////


/// @addtogroup PWRITEService
/// @{

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

//
// Construct MPFVTP object. Check for PWRITE feature presence.
//
MPFPWRITE::MPFPWRITE( IALIMMIO   *pMMIOService,
                      btCSROffset vtpDFHOffset ) : m_pALIMMIO( pMMIOService ),
                                                   m_dfhOffset( vtpDFHOffset ),
                                                   m_isOK( false )
{
   ali_errnum_e err;
   btBool ret;                                // for error checking

   ASSERT( m_pALIMMIO != NULL );
   ASSERT( m_dfhOffset != -1 );

   // Check BBB GUID (are we really a PWRITE?)
   btcString sGUID = MPF_PWRITE_BBB_GUID;
   AAL_GUID_t structGUID;
   btUnsigned64bitInt readBuf[2];

   ret = m_pALIMMIO->mmioRead64(m_dfhOffset + 8, &readBuf[0]);
   ASSERT(ret);
   ret = m_pALIMMIO->mmioRead64(m_dfhOffset + 16, &readBuf[1]);
   ASSERT(ret);
   if ( 0 != strncmp( sGUID, GUIDStringFromStruct(GUIDStructFrom2xU64(readBuf[1], readBuf[0])).c_str(), 36 ) ) {
      AAL_ERR(LM_AFU, "Feature GUID does not match PWRITE GUID.");
      m_isOK = false;
      return;
   }

   AAL_INFO(LM_AFU, "Found and successfully identified PWRITE feature." << std::endl);

   m_isOK = true;
}

//
// Return all statistics counters
//
btBool MPFPWRITE::pwriteGetStats( t_cci_mpf_pwrite_stats *stats )
{
   stats->numPartialWrites = pwriteGetStatCounter(CCI_MPF_PWRITE_CSR_STAT_NUM_PWRITES);
   return true;
}

//
// Return a statistics counter
//
btUnsigned64bitInt MPFPWRITE::pwriteGetStatCounter( t_cci_mpf_pwrite_csr_offsets stat )
{
   btUnsigned64bitInt cnt;
   btBool ret;

   ret = m_pALIMMIO->mmioRead64(m_dfhOffset + stat, &cnt);
   ASSERT(ret);

   return cnt;
}

/// @} group PWRITEService

END_NAMESPACE(AAL)

