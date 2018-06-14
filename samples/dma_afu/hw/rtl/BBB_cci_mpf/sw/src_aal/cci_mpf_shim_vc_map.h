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
/// @file cci_mpf_shim_vc_map.h
/// @brief Definitions for VC MAP Service.
/// @ingroup VCMAPService
/// @verbatim
/// Virtual channel mapping service.
///
/// Note: this is not an AAL service, but a component of the MPF service (which
/// is).
///
/// AUTHOR:  Michael Adler, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 06/03/2016     MA       Initial version@endverbatim
//****************************************************************************
#ifndef __CCI_MPF_SHIM_VCMAP_H__
#define __CCI_MPF_SHIM_VCMAP_H__

#include <aalsdk/mpf/IMPF.h>              // Public MPF service interface
#include <aalsdk/mpf/MPFService.h>


BEGIN_NAMESPACE(AAL)

/// @addtogroup VCMAPService
/// @{

class MPFVCMAP : public CAASBase, public IMPFVCMAP
{
public:

   /// VCMAP constructor
   MPFVCMAP( IALIMMIO   *pMMIOService,
             btCSROffset vcmapDFHOffset );

   // Set the mapping mode:
   //  - enable_mapping turns on eVC_VA to physical channel mapping
   //  - enable_dynamic_mapping turns on automatic tuning of the channel
   //    ratios based on traffic. (Ignored when enable_mapping is false.)
   //  - sampling_window_radix determines the sizes of sampling windows for
   //    dynamic mapping and, consequently, controls the frequency at
   //    which dynamic changes may occur.  Dynamic changes are expensive
   //    since a write fence must be emitted to synchronize traffic.
   //    Passing 0 picks the default value.
   btBool vcmapSetMode( btBool enable_mapping,
                        btBool enable_dynamic_mapping,
                        btUnsigned32bitInt sampling_window_radix = 0 );

   // Map requests either for reads or writes, but not both.
   //
   // This mode does not accomplish the usual VC Map goals, since mapping will
   // not be consistent by address for both reads and writes.  In combination
   // with vcmapSetFixedMapping(), this mode is useful for some bandwidth
   // allocation scenarios, most likely for limiting writes to a subset of
   // the I/O channels.
   //
   // When map_writes is set only writes are mapped and not reads.  When
   // map_writes is not set only reads are mapped.
   btBool vcmapSetMapOnlyReadsOrWrites( btBool map_writes );

   // Disable mapping
   btBool vcmapDisable( void );

   // When false (default), only incoming eVC_VA requests are mapped.
   // When true, all incoming requests are remapped.
   btBool vcmapSetMapAll( btBool map_all_requests );

   // Set fixed mapping where VL0 gets r 64ths of the traffic.  When
   // "user_specified" is set, "r" is used as the new ratio.  When
   // "user_specified" is false, a platform-specific fixed mapping is used.
   btBool vcmapSetFixedMapping( btBool user_specified,
                                btUnsigned32bitInt r = 16 );

   // Set a traffic threshold below which all traffic will be mapped to
   // the low-latency VL0.  The treshold is the sum of read and write requests
   // in a sampling window.  The sampling window can be set in vcmapSetMode()
   // above as sampling_window_radix, defaulting to 10 (1K cycles).
   // The threshold must be some 2^n-1 so it can be used as a mask.
   btBool vcmapSetLowTrafficThreshold( btUnsigned32bitInt t );

   btBool isOK( void ) { return m_isOK; }     // < status after initialization

   // Return all statistics counters
   btBool vcmapGetStats( t_cci_mpf_vc_map_stats *stats );

   // Return a statistics counter
   btUnsigned64bitInt vcmapGetStatCounter( t_cci_mpf_vc_map_csr_offsets stat );

   // Return mapping state history vector.  The vector is an array of 8
   // bit entries with the lowest 8 bits corresponding to the most recent
   // state.  The 8 bit values each are the fraction of references, in 64ths,
   // to direct to VL0.  The remaining references are distributed evenly between
   // the PCIe channels.
   btUnsigned64bitInt vcmapGetMappingHistory( void );

protected:
   IALIMMIO              *m_pALIMMIO;
   btCSROffset            m_dfhOffset;

   btBool                 m_isOK;

};

/// @}

END_NAMESPACE(AAL)

#endif // __CCI_MPF_SHIM_VCMAP_H__
