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
//****************************************************************************
/// @file IMPFSERVICE.h
/// @brief IMPFService.
/// @ingroup vtp_service
/// @verbatim
/// Intel(R) QuickAssist Technology Accelerator Abstraction Layer
/// Virtual-to-Physical address translation service
///
/// TODO: add verbose description
///
/// Provides service for access to the MPF BBB for address translation.
/// Assumes a MPF BBB DFH to be detected and present.
///
/// On initialization, allocates shared buffer for MPF page hash and
/// communicates its location to MPF BBB.
///
/// Provides synchronous methods to update page hash on shared buffer
/// allocation.
///
/// Does not have an explicit client callback interface, as all published
/// service methods are synchronous.
///
/// AUTHORS: Enno Luebbers, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 01/15/2016     EL       Initial version@endverbatim
//****************************************************************************
#ifndef __IMPF_H__
#define __IMPF_H__
#include <aalsdk/AAL.h>
#include <aalsdk/osal/OSServiceModule.h>

#include <aalsdk/service/IALIAFU.h>
#include <opae/mpf/cci_mpf_csrs.h>

BEGIN_NAMESPACE(AAL)

/// @addtogroup VTPService
/// @{

// Service creation parameter keys and datatypes
#define ALIAFU_IBASE_KEY "ALIAFUIBase"
#define ALIAFU_IBASE_DATATYPE btObjectType
#define MPF_VTP_DFH_ADDRESS_KEY "MPFVTPDFHAddress"
#define MPF_VTP_DFH_ADDRESS_DATATYPE btObjectType
#define MPF_VTP_DFH_OFFSET_KEY "MPFVTPDFHOffset"
#define MPF_VTP_DFH_OFFSET_DATATYPE btCSROffset
#define MPF_FEATURE_ID_KEY "MPFFeatureID"
#define MPF_FEATURE_ID_DATATYPE btUnsigned64bitInt

/// VTP BBB GUID, used to identify HW VTP component
#define MPF_VTP_BBB_GUID "C8A2982F-FF96-42BF-A705-45727F501901"

// Read responses ordering feature
#define MPF_RSP_ORDER_BBB_GUID "4C9C96F4-65BA-4DD8-B383-C70ACE57BFE4"

/// MPF VTP Service IID
#ifndef iidMPFVTPService
#define iidMPFVTPService __INTC_IID(INTC_sysSampleAFU, 0x0010)
#endif

/// @brief MPF VTP service interface
///
/// Provides all functions of IALIBuffer, with the difference of now allocating
/// buffers that can be accessed from the AFU using virtual addresses (through
/// VTP.
///
/// @note Since AFU reset will reset all user logic including BBBs like VTP,
///       users need to make sure to reinitialize VTP after an AFU reset
///       using vtpReset().
///
/// @see IALIBuffer

// VTP statistics
typedef struct
{
   // Hits and misses in the TLB. The VTP pipeline has local caches
   // within the pipeline itself that filter requests to the TLB.
   // The counts here increment only for requests to the TLB service
   // that are not satisfied in the VTP pipeline caches.
   btUnsigned64bitInt numTLBHits4KB;
   btUnsigned64bitInt numTLBMisses4KB;
   btUnsigned64bitInt numTLBHits2MB;
   btUnsigned64bitInt numTLBMisses2MB;

   // Number of cycles spent with the page table walker active.  Since
   // the walker manages only one request at a time the latency of the
   // page table walker can be computed as:
   //   numPTWalkBusyCycles / (numTLBMisses4KB + numTLBMisses2MB)
   btUnsigned64bitInt numPTWalkBusyCycles;

   // Number of failed virtual to physical translations. The VTP page
   // table walker currently goes into a terminal error state when a
   // translation failure is seen and blocks all future requests.
   // If this number is non-zero the program has passed an illegal
   // address and should be fixed.
   btUnsigned64bitInt numFailedTranslations;
   // Last virtual address translated. If numFailedTranslations is non-zero
   // this is the failing virtual address.
   btVirtAddr ptWalkLastVAddr;
}
t_cci_mpf_vtp_stats;

class IMPFVTP : public IALIBuffer
{
public:
   virtual ~IMPFVTP() {}

   /// Reset VTP (invalidate TLB)
   virtual btBool vtpReset( void ) = 0;

   // Return all statistics counters
   virtual btBool vtpGetStats( t_cci_mpf_vtp_stats *stats ) = 0;
};

/// @}


/// @addtogroup VCMAPService
/// @{

#define MPF_VC_MAP_DFH_ADDRESS_KEY "MPFVCMAPDFHAddress"
#define MPF_VC_MAP_DFH_ADDRESS_DATATYPE btObjectType
#define MPF_VC_MAP_DFH_OFFSET_KEY "MPFVCMAPDFHOffset"
#define MPF_VC_MAP_DFH_OFFSET_DATATYPE btCSROffset

/// Memory virtual channel mapping feature
#define MPF_VC_MAP_BBB_GUID "5046C86F-BA48-4856-B8F9-3B76E3DD4E74"

/// MPF VC_MAP Service IID
#ifndef iidMPFVCMAPService
#define iidMPFVCMAPService __INTC_IID(INTC_sysSampleAFU, 0x0011)
#endif


typedef struct
{
   // Number of times the mapping function has changed
   btUnsigned64bitInt numMappingChanges;
}
t_cci_mpf_vc_map_stats;


class IMPFVCMAP
{
public:
   virtual ~IMPFVCMAP() {}

   // Set the mapping mode:
   //  - enable_mapping turns on eVC_VA to physical channel mapping
   //  - enable_dynamic_mapping turns on automatic tuning of the channel
   //    ratios based on traffic. (Ignored when enable_mapping is false.)
   //  - sampling_window_radix determines the sizes of sampling windows for
   //    dynamic mapping and, consequently, controls the frequency at
   //    which dynamic changes may occur.  Dynamic changes are expensive
   //    since a write fence must be emitted to synchronize traffic.
   //    Passing 0 picks the default value.
   virtual btBool vcmapSetMode( btBool enable_mapping,
                                btBool enable_dynamic_mapping,
                                btUnsigned32bitInt sampling_window_radix = 0 ) = 0;

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
   virtual btBool vcmapSetMapOnlyReadsOrWrites( btBool map_writes ) = 0;

   // Disable mapping
   virtual btBool vcmapDisable( void ) = 0;

   // When false (default), only incoming eVC_VA requests are mapped.
   // When true, all incoming requests are remapped.
   virtual btBool vcmapSetMapAll( btBool map_all_requests ) = 0;

   // Set fixed mapping where VL0 gets r 64ths of the traffic.  When
   // "user_specified" is set, "r" is used as the new ratio.  When
   // "user_specified" is false, a platform-specific fixed mapping is used.
   virtual btBool vcmapSetFixedMapping( btBool user_specified,
                                        btUnsigned32bitInt r = 16 ) = 0;

   // Set a traffic threshold below which all traffic will be mapped to
   // the low-latency VL0.  The treshold is the sum of read and write requests
   // in a sampling window.  The sampling window can be set in vcmapSetMode()
   // above as sampling_window_radix, defaulting to 10 (1K cycles).
   // The threshold must be some 2^n-1 so it can be used as a mask.
   virtual btBool vcmapSetLowTrafficThreshold( btUnsigned32bitInt t ) = 0;

   // Return all statistics counters
   virtual btBool vcmapGetStats( t_cci_mpf_vc_map_stats *stats ) = 0;

   // Return mapping state history vector.  The vector is an array of 8
   // bit entries with the lowest 8 bits corresponding to the most recent
   // state.  The 8 bit values each are the fraction of references, in 64ths,
   // to direct to VL0.  The remaining references are distributed evenly between
   // the PCIe channels.
   virtual btUnsigned64bitInt vcmapGetMappingHistory( void ) = 0;
};

/// @}


/// @addtogroup LATQOSService
/// @{

#define MPF_LATENCY_QOS_DFH_ADDRESS_KEY "MPFLATENCY_QOSDFHAddress"
#define MPF_LATENCY_QOS_DFH_ADDRESS_DATATYPE btObjectType
#define MPF_LATENCY_QOS_DFH_OFFSET_KEY "MPFLATENCY_QOSDFHOffset"
#define MPF_LATENCY_QOS_DFH_OFFSET_DATATYPE btCSROffset

/// Memory write/read ordering feature
#define MPF_LATENCY_QOS_BBB_GUID "B35138F6-EA39-4603-9412-A4CF1A999C49"

/// MPF LATENCY_QOS Service IID
#ifndef iidMPFLATQOSService
#define iidMPFLATQOSService __INTC_IID(INTC_sysSampleAFU, 0x0012)
#endif


class IMPFLATQOS
{
public:
   virtual ~IMPFLATQOS() {}

   // Set the configuration using one 64 bit register.
   // See cci_mpf_shim_latency_qos.sv for definition.
   virtual btBool latqosSetConfig( btUnsigned64bitInt config ) = 0;
};

/// @}


/// @addtogroup WROService
/// @{

#define MPF_WRO_DFH_ADDRESS_KEY "MPFWRODFHAddress"
#define MPF_WRO_DFH_ADDRESS_DATATYPE btObjectType
#define MPF_WRO_DFH_OFFSET_KEY "MPFWRODFHOffset"
#define MPF_WRO_DFH_OFFSET_DATATYPE btCSROffset

/// Memory write/read ordering feature
#define MPF_WRO_BBB_GUID "56B06B48-9DD7-4004-A47E-0681B4207A6D"

/// MPF WRO Service IID
#ifndef iidMPFWROService
#define iidMPFWROService __INTC_IID(INTC_sysSampleAFU, 0x0013)
#endif


typedef struct
{
   // Cycles in which new read blocked due to conflict with old read
   btUnsigned64bitInt numConflictCyclesRR;
   // Cycles in which new read blocked due to conflict with old write
   btUnsigned64bitInt numConflictCyclesRW;
   // Cycles in which new write blocked due to conflict with old read
   btUnsigned64bitInt numConflictCyclesWR;
   // Cycles in which new write blocked due to conflict with old write
   btUnsigned64bitInt numConflictCyclesWW;
}
t_cci_mpf_wro_stats;


class IMPFWRO
{
public:
   virtual ~IMPFWRO() {}

   // Return all statistics counters
   virtual btBool wroGetStats( t_cci_mpf_wro_stats *stats ) = 0;
};

/// @}


/// @addtogroup PWRITEService
/// @{

#define MPF_PWRITE_DFH_ADDRESS_KEY "MPFPWRITEDFHAddress"
#define MPF_PWRITE_DFH_ADDRESS_DATATYPE btObjectType
#define MPF_PWRITE_DFH_OFFSET_KEY "MPFPWRITEDFHOffset"
#define MPF_PWRITE_DFH_OFFSET_DATATYPE btCSROffset

/// Memory write/read ordering feature
#define MPF_PWRITE_BBB_GUID "9BDBBCAF-2C5A-4D17-A636-75B19A0B4F5C"

/// MPF PWRITE Service IID
#ifndef iidMPFPWRITEService
#define iidMPFPWRITEService __INTC_IID(INTC_sysSampleAFU, 0x0014)
#endif


typedef struct
{
   // Number of partial writes
   btUnsigned64bitInt numPartialWrites;
}
t_cci_mpf_pwrite_stats;


class IMPFPWRITE
{
public:
   virtual ~IMPFPWRITE() {}

   // Return all statistics counters
   virtual btBool pwriteGetStats( t_cci_mpf_pwrite_stats *stats ) = 0;
};

/// @}

END_NAMESPACE(AAL)

#endif // __IMPF_H__

