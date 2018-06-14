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
/// @file cci_mpf_shim_vc_map.cpp
/// @brief Implementation of MPFVCMAP.
/// @ingroup VCMAPService
/// @verbatim
/// Virtual channel mapping service.
///
/// Note: this is not an AAL service, but a component of the MPF service (which
/// is).
///
/// AUTHOR: Michael Adler, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 06/03/2016     MA       Initial version@endverbatim
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

#include "cci_mpf_shim_vc_map.h"

BEGIN_NAMESPACE(AAL)


//=============================================================================
// Typedefs and Constants
//=============================================================================



/////////////////////////////////////////////////////////////////////////////
//////                                                                ///////
//////                                                                ///////
/////                         VC MAP Service                           //////
//////                                                                ///////
//////                                                                ///////
/////////////////////////////////////////////////////////////////////////////


/// @addtogroup VCMAPService
/// @{

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

//
// Construct MPFVTP object. Check for VC MAP feature presence.
//
MPFVCMAP::MPFVCMAP( IALIMMIO   *pMMIOService,
                    btCSROffset vtpDFHOffset ) : m_pALIMMIO( pMMIOService ),
                                                 m_dfhOffset( vtpDFHOffset ),
                                                 m_isOK( false )
{
   ali_errnum_e err;
   btBool ret;                                // for error checking

   ASSERT( m_pALIMMIO != NULL );
   ASSERT( m_dfhOffset != -1 );

   // Check BBB GUID (are we really a VC MAP?)
   btcString sGUID = MPF_VC_MAP_BBB_GUID;
   AAL_GUID_t structGUID;
   btUnsigned64bitInt readBuf[2];

   ret = m_pALIMMIO->mmioRead64(m_dfhOffset + 8, &readBuf[0]);
   ASSERT(ret);
   ret = m_pALIMMIO->mmioRead64(m_dfhOffset + 16, &readBuf[1]);
   ASSERT(ret);
   if ( 0 != strncmp( sGUID, GUIDStringFromStruct(GUIDStructFrom2xU64(readBuf[1], readBuf[0])).c_str(), 36 ) ) {
      AAL_ERR(LM_AFU, "Feature GUID does not match VC MAP GUID.");
      m_isOK = false;
      return;
   }

   AAL_INFO(LM_AFU, "Found and successfully identified VC MAP feature." << std::endl);

   m_isOK = true;
}

//
// Set mapping mode
//
btBool MPFVCMAP::vcmapSetMode( btBool enable_mapping,
                               btBool enable_dynamic_mapping,
                               btUnsigned32bitInt sampling_window_radix )
{
   ASSERT(sampling_window_radix < 16);
   if (sampling_window_radix >= 16) return false;

   btBool ret;

   ret = m_pALIMMIO->mmioWrite64(m_dfhOffset + CCI_MPF_VC_MAP_CSR_CTRL_REG,
                                 (enable_mapping ? 3 : 0) |
                                 (enable_dynamic_mapping ? 4 : 0) |
                                 (sampling_window_radix << 3) |
                                 // Group A
                                 (btUnsigned64bitInt(1) << 63));
   ASSERT(ret);

   return ret;
}


btBool MPFVCMAP::vcmapSetMapOnlyReadsOrWrites( btBool map_writes )
{
   btBool ret;

   ret = m_pALIMMIO->mmioWrite64(m_dfhOffset + CCI_MPF_VC_MAP_CSR_CTRL_REG,
                                 (map_writes ? 2 : 1) |
                                 // Group A
                                 (btUnsigned64bitInt(1) << 63));
   ASSERT(ret);

   return ret;
}


btBool MPFVCMAP::vcmapDisable( void )
{
   btBool ret;

   ret = m_pALIMMIO->mmioWrite64(m_dfhOffset + CCI_MPF_VC_MAP_CSR_CTRL_REG,
                                 0 |
                                 // Group A
                                 (btUnsigned64bitInt(1) << 63));
   ASSERT(ret);

   return ret;
}


btBool MPFVCMAP::vcmapSetMapAll( btBool map_all_requests )
{
   btBool ret;

   btUnsigned32bitInt map_all = (map_all_requests ? (1 << 7) : 0);

   ret = m_pALIMMIO->mmioWrite64(m_dfhOffset + CCI_MPF_VC_MAP_CSR_CTRL_REG,
                                 map_all |
                                 // Group B
                                 (btUnsigned64bitInt(1) << 62));
   ASSERT(ret);

   return ret;
}


btBool MPFVCMAP::vcmapSetFixedMapping( btBool user_specified,
                                       btUnsigned32bitInt r )
{
   ASSERT(r <= 64);
   if (r > 64) return false;

   btBool ret;

   btUnsigned32bitInt specified = (user_specified ? (1 << 8) : 0);
   if (! specified)
   {
       r = 0;
   }

   ret = m_pALIMMIO->mmioWrite64(m_dfhOffset + CCI_MPF_VC_MAP_CSR_CTRL_REG,
                                 specified | (r << 9) |
                                  // Group C
                                 (btUnsigned64bitInt(1) << 61));
   ASSERT(ret);

   return ret;
}


btBool MPFVCMAP::vcmapSetLowTrafficThreshold( btUnsigned32bitInt t )
{
   // The threshold must fit in the size and must be some 2^n-1 so only
   // a contiguous set of low bits are set.  This way it can be used as
   // a mask in the hardware.
   ASSERT((t <= 0xffff) && (((t + 1) & t) == 0));
   if (! ((t <= 0xffff) && (((t + 1) & t) == 0))) return false;

   btBool ret;

   ret = m_pALIMMIO->mmioWrite64(m_dfhOffset + CCI_MPF_VC_MAP_CSR_CTRL_REG,
                                 (t << 16) |
                                  // Group D
                                 (btUnsigned64bitInt(1) << 60));
   ASSERT(ret);

   return ret;
}


//
// Return all statistics counters
//
btBool MPFVCMAP::vcmapGetStats( t_cci_mpf_vc_map_stats *stats )
{
   stats->numMappingChanges = vcmapGetStatCounter(CCI_MPF_VC_MAP_CSR_STAT_NUM_MAPPING_CHANGES);
   return true;
}

//
// Return a statistics counter
//
btUnsigned64bitInt MPFVCMAP::vcmapGetStatCounter( t_cci_mpf_vc_map_csr_offsets stat )
{
   btUnsigned64bitInt cnt;
   btBool ret;

   ret = m_pALIMMIO->mmioRead64(m_dfhOffset + stat, &cnt);
   ASSERT(ret);

   return cnt;
}


//
// Mapping history -- a vector of 8 bit entries.
//
btUnsigned64bitInt MPFVCMAP::vcmapGetMappingHistory( void )
{
   btUnsigned64bitInt hist;
   btBool ret;

   ret = m_pALIMMIO->mmioRead64(m_dfhOffset + CCI_MPF_VC_MAP_CSR_STAT_HISTORY, &hist);
   ASSERT(ret);

   return hist;
}


/// @} group VCMAPService

END_NAMESPACE(AAL)

