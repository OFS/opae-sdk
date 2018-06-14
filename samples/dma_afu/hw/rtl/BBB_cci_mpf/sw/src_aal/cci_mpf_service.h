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
/// @file cci_mpf_service.h
/// @brief Definitions for MPF Service.
/// @ingroup VTPService
/// @verbatim
/// Memory Property Factory Basic Building Block Service
///
/// The MPF service provides software interfaces to features exposed by the
/// cci_mpf basic building block which require software interaction, such as
/// VTP (virtual-to-physical). For every present feature, it will expose
/// a separate service interface (defined in IMPF.h), the implementation of
/// which is handled by a respective component class.
///
/// AUTHORS: Enno Luebbers, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 02/29/2016     EL       Initial version@endverbatim
//****************************************************************************
#ifndef __MPF_H__
#define __MPF_H__
#include <aalsdk/aas/AALService.h>
#include <aalsdk/IServiceClient.h>
#include <aalsdk/osal/IDispatchable.h>

#include <aalsdk/service/IALIAFU.h>
#include <aalsdk/uaia/IAFUProxy.h>

#include <aalsdk/mpf/IMPF.h>              // Public MPF service interface
#include <aalsdk/mpf/MPFService.h>

#include "cci_mpf_shim_vc_map.h"
#include "cci_mpf_shim_vtp.h"
#include "cci_mpf_shim_latency_qos.h"
#include "cci_mpf_shim_wro.h"
#include "cci_mpf_shim_pwrite.h"

BEGIN_NAMESPACE(AAL)

/// @addtogroup VTPService
/// @{

//=============================================================================
// Name: MPFService
// Description: Memory Property Factory service class
// Interface: Generic Service interface. Exposes interfaces of component class.
// Comments:
//=============================================================================
/// @brief Memory Property Factory service.
class MPF: public ServiceBase
{
public:

   /// VTP service constructor
   DECLARE_AAL_SERVICE_CONSTRUCTOR(MPF, ServiceBase),
      m_pSvcClient(NULL),
      m_pALIAFU(NULL),
      m_pALIBuffer(NULL),
      m_pALIMMIO(NULL),
      m_vtpDFHOffset(-1),
      m_pVTP(NULL),
      m_vcmapDFHOffset(-1),
      m_pVCMAP(NULL),
      m_latqosDFHOffset(-1),
      m_pLATQOS(NULL),
      m_wroDFHOffset(-1),
      m_pWRO(NULL),
      m_pwriteDFHOffset(-1),
      m_pPWRITE(NULL)
   {
      // MPF exposes service interfaces of component classes (delegate
      // pattern). As such, the "SetInterface()" calls are deferred until the
      // respective features have been detected in init().
   }
   /// @brief Service initialization hook.
   ///
   /// Expects the following in the optArgs passed to it:
   ///
   ///    ALIAFU_IBASE         pointer to ALIAFU
   ///
   /// For every feature XYZ you want exposed by software, also pass either of
   /// the following in optArgs:
   ///
   ///    MPF_XYZ_FEATURE_ID   feature ID of feature
   ///    MPF_XYZ_DFH_OFFSET   offset into DFH for VTP feature
   ///
   /// If both are present, the offset will take precedence.
   ///
   /// Valid features (i.e. replacements for XYZ in the above) are:
   ///
   ///    VTP                  virtual-to-physical translation feature
   ///
   btBool init( IBase *pclientBase,
                NamedValueSet const &optArgs,
                TransactionID const &rtid );

   /// Called when the service is released
   btBool Release( TransactionID const &rTranID,
                   btTime               timeout=AAL_INFINITE_WAIT );

protected:

   IServiceClient        *m_pSvcClient;      //< Pointer to service client
   IBase                 *m_pALIAFU;         //< Pointer to ALIAFU class
   IALIBuffer            *m_pALIBuffer;      //< Pointer to ALIAFUs Buffer IF
   IALIMMIO              *m_pALIMMIO;        //< Pointer to ALIAFUs MMIO IF

   btCSROffset            m_vtpDFHOffset;    //< Offset to VTP feature
   MPFVTP                *m_pVTP;            //< Pointer to VTP SW component

   btCSROffset            m_vcmapDFHOffset;  //< Offset to VC MAP feature
   MPFVCMAP              *m_pVCMAP;          //< Pointer to VC MAP SW component

   btCSROffset            m_latqosDFHOffset; //< Offset to Latency QoS feature
   MPFLATQOS             *m_pLATQOS;         //< Pointer to Latency QoS SW component

   btCSROffset            m_wroDFHOffset;    //< Offset to WRO feature
   MPFWRO                *m_pWRO;            //< Pointer to WRO SW component

   btCSROffset            m_pwriteDFHOffset; //< Offset to PWRITE feature
   MPFPWRITE             *m_pPWRITE;         //< Pointer to PWRITE SW component
};

/// @}

END_NAMESPACE(AAL)

#endif //__MPF_H__
