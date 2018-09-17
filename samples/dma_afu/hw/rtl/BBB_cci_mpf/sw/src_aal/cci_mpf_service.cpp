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
/// @file cci_mpf_service.cpp
/// @brief Implementation of MPF Service.
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

#include "cci_mpf_service.h"


BEGIN_NAMESPACE(AAL)


//=============================================================================
// Typedefs and Constants
//=============================================================================

/////////////////////////////////////////////////////////////////////////////
//////                                                                ///////
//////                                                                ///////
/////                            MPF Service                           //////
//////                                                                ///////
//////                                                                ///////
/////////////////////////////////////////////////////////////////////////////


/// @addtogroup VTPService
/// @{

//-----------------------------------------------------------------------------
// Public functions
//-----------------------------------------------------------------------------

// TODO: turn into doxygen comments
//=============================================================================
// Name: init()
// Description: Initialize the Service
// Interface: public
// Inputs: pclientBase - Pointer to the IBase for the Service Client
//         optArgs - Arguments passed to the Service
//         rtid - Transaction ID
// Outputs: none.
// Comments: Should only return False in case of severe failure that prevents
//           sending a response or calling initFailed.
//=============================================================================
btBool MPF::init( IBase               *pclientBase,
                  NamedValueSet const &optArgs,
                  TransactionID const &rtid)
{
   ALIAFU_IBASE_DATATYPE   aliIBase;

   // check for ALIAFU's IBase in optargs
   if ( ENamedValuesOK != optArgs.Get(ALIAFU_IBASE_KEY, &aliIBase) ) {
      initFailed(new CExceptionTransactionEvent( NULL,
                                                 rtid,
                                                 errBadParameter,
                                                 reasMissingParameter,
                                                 "No HWALIAFU IBase in optArgs."));
      return true;
   }
   m_pALIAFU = reinterpret_cast<IBase *>(aliIBase);

   // Get IALIBuffer interface to AFU
   m_pALIBuffer = dynamic_ptr<IALIBuffer>(iidALI_BUFF_Service, m_pALIAFU);
   ASSERT(NULL != m_pALIBuffer);
   if ( NULL == m_pALIBuffer ) {
      initFailed(new CExceptionTransactionEvent( NULL,
                                                 rtid,
                                                 errBadParameter,
                                                 reasMissingInterface,
                                                 "No IALIBuffer interface in HWALIAFU."));
      m_bIsOK = false;
      return true;
   }

   // Get IALIMMIO interface to AFU
   m_pALIMMIO = dynamic_ptr<IALIMMIO>(iidALI_MMIO_Service, m_pALIAFU);
   ASSERT(NULL != m_pALIMMIO);
   if ( NULL == m_pALIMMIO ) {
      initFailed(new CExceptionTransactionEvent( NULL,
                                                 rtid,
                                                 errBadParameter,
                                                 reasMissingInterface,
                                                 "No IALIMMIO interface in HWALIAFU."));
      m_bIsOK = false;
      return true;
   }

   // ------------- Component detection and initialization -------------
   // Generally, all MPF features carry the same feature ID and are
   // differentiated by their respective GUID.
   // The best practice is to supply the MPF_FEATURE_ID_KEY when allocating an
   // MPF service - this will cause enumeration of all exposed MPF features
   // belonging to a particular MPF module instance.
   // For more fine-grained control, it is also possible to specify MMIO
   // offsets of the respective features directly. Use this method with care.
   // It will take precedence over the feature ID.

   btBool hasVTP = false;
   btBool hasVCMAP = false;
   btBool hasLATQOS = false;
   btBool hasWRO = false;
   btBool hasPWRITE = false;

   // If MPF_FEATURE_ID is specified, try to detect available features.
   MPF_FEATURE_ID_DATATYPE mpfFID;
   if ( ENamedValuesOK == optArgs.Get(MPF_FEATURE_ID_KEY, &mpfFID) ) {

      //
      // VTP
      //

      // Ask ALI for a BBB with MPF's feature ID and the expected VTP GUID
      NamedValueSet filter_vtp;
      filter_vtp.Add( ALI_GETFEATURE_TYPE_KEY, static_cast<ALI_GETFEATURE_TYPE_DATATYPE>(ALI_DFH_TYPE_BBB) );
      filter_vtp.Add( ALI_GETFEATURE_ID_KEY, static_cast<ALI_GETFEATURE_ID_DATATYPE>(mpfFID) );
      filter_vtp.Add( ALI_GETFEATURE_GUID_KEY, (ALI_GETFEATURE_GUID_DATATYPE)MPF_VTP_BBB_GUID );

      // FIXME: This is here only because of a caching bug in
      // ASEALIAFU::mmioGetFeatureAddress() in SR-5.0.2-Beta. Once fixed
      // this call can be removed.
      m_pALIMMIO->mmioGetAddress();

      if ( false == m_pALIMMIO->mmioGetFeatureOffset( &m_vtpDFHOffset, filter_vtp ) ) {
         // No VTP found - this could mean that VTP is not enabled in MPF
         hasVTP = false;
      } else {
         hasVTP = true;
      }


      //
      // VC MAP
      //

      // Ask ALI for a BBB with MPF's feature ID and the expected VC_MAP GUID
      NamedValueSet filter_vcmap;
      filter_vcmap.Add( ALI_GETFEATURE_TYPE_KEY, static_cast<ALI_GETFEATURE_TYPE_DATATYPE>(ALI_DFH_TYPE_BBB) );
      filter_vcmap.Add( ALI_GETFEATURE_ID_KEY, static_cast<ALI_GETFEATURE_ID_DATATYPE>(mpfFID) );
      filter_vcmap.Add( ALI_GETFEATURE_GUID_KEY, (ALI_GETFEATURE_GUID_DATATYPE)MPF_VC_MAP_BBB_GUID );

      if ( false == m_pALIMMIO->mmioGetFeatureOffset( &m_vcmapDFHOffset, filter_vcmap ) ) {
         // No VC MAP found - this could mean that VC MAP is not enabled in MPF
         hasVCMAP = false;
         AAL_INFO(LM_AFU, "No VC MAP feature." << std::endl);
      } else {
         hasVCMAP = true;
         AAL_INFO(LM_All, "Using MMIO address 0x" << std::hex << m_vcmapDFHOffset <<
                  " for VC MAP." << std::endl);

         // Instantiate component class
         m_pVCMAP = new MPFVCMAP( m_pALIMMIO, m_vcmapDFHOffset );

         if ( ! m_pVCMAP->isOK() ) {
            initFailed(new CExceptionTransactionEvent( NULL,
                     rtid,
                     errBadParameter,
                     reasFeatureNotSupported,
                     "VC MAP initialization failed."));
            return true;
         }

         // found VC MAP, expose interface to outside
         SetInterface(iidMPFVCMAPService, dynamic_cast<IMPFVCMAP *>(m_pVCMAP));
      }


      //
      // Latency QoS
      //

      // Ask ALI for a BBB with MPF's feature ID and the expected Latency QoS GUID
      NamedValueSet filter_latqos;
      filter_latqos.Add( ALI_GETFEATURE_TYPE_KEY, static_cast<ALI_GETFEATURE_TYPE_DATATYPE>(ALI_DFH_TYPE_BBB) );
      filter_latqos.Add( ALI_GETFEATURE_ID_KEY, static_cast<ALI_GETFEATURE_ID_DATATYPE>(mpfFID) );
      filter_latqos.Add( ALI_GETFEATURE_GUID_KEY, (ALI_GETFEATURE_GUID_DATATYPE)MPF_LATENCY_QOS_BBB_GUID );

      if ( false == m_pALIMMIO->mmioGetFeatureOffset( &m_latqosDFHOffset, filter_latqos ) ) {
         // No Latency QoS found - this could mean that Latency QoS is not enabled in MPF
         hasLATQOS = false;
         AAL_INFO(LM_AFU, "No Latency QoS feature." << std::endl);
      } else {
         hasLATQOS = true;
         AAL_INFO(LM_All, "Using MMIO address 0x" << std::hex << m_latqosDFHOffset <<
                  " for Latency QoS." << std::endl);

         // Instantiate component class
         m_pLATQOS = new MPFLATQOS( m_pALIMMIO, m_latqosDFHOffset );

         if ( ! m_pLATQOS->isOK() ) {
            initFailed(new CExceptionTransactionEvent( NULL,
                     rtid,
                     errBadParameter,
                     reasFeatureNotSupported,
                     "Latency QoS initialization failed."));
            return true;
         }

         // found Latency QoS, expose interface to outside
         SetInterface(iidMPFLATQOSService, dynamic_cast<IMPFLATQOS *>(m_pLATQOS));
      }


      //
      // WRO
      //

      // Ask ALI for a BBB with MPF's feature ID and the expected WRO GUID
      NamedValueSet filter_wro;
      filter_wro.Add( ALI_GETFEATURE_TYPE_KEY, static_cast<ALI_GETFEATURE_TYPE_DATATYPE>(ALI_DFH_TYPE_BBB) );
      filter_wro.Add( ALI_GETFEATURE_ID_KEY, static_cast<ALI_GETFEATURE_ID_DATATYPE>(mpfFID) );
      filter_wro.Add( ALI_GETFEATURE_GUID_KEY, (ALI_GETFEATURE_GUID_DATATYPE)MPF_WRO_BBB_GUID );

      if ( false == m_pALIMMIO->mmioGetFeatureOffset( &m_wroDFHOffset, filter_wro ) ) {
         // No WRO found - this could mean that WRO is not enabled in MPF
         hasWRO = false;
         AAL_INFO(LM_AFU, "No WRO feature." << std::endl);
      } else {
         hasWRO = true;
         AAL_INFO(LM_All, "Using MMIO address 0x" << std::hex << m_wroDFHOffset <<
                  " for WRO." << std::endl);

         // Instantiate component class
         m_pWRO = new MPFWRO( m_pALIMMIO, m_wroDFHOffset );

         if ( ! m_pWRO->isOK() ) {
            initFailed(new CExceptionTransactionEvent( NULL,
                     rtid,
                     errBadParameter,
                     reasFeatureNotSupported,
                     "WRO initialization failed."));
            return true;
         }

         // found WRO, expose interface to outside
         SetInterface(iidMPFWROService, dynamic_cast<IMPFWRO *>(m_pWRO));
      }


      //
      // PWRITE
      //

      // Ask ALI for a BBB with MPF's feature ID and the expected PWRITE GUID
      NamedValueSet filter_pwrite;
      filter_pwrite.Add( ALI_GETFEATURE_TYPE_KEY, static_cast<ALI_GETFEATURE_TYPE_DATATYPE>(ALI_DFH_TYPE_BBB) );
      filter_pwrite.Add( ALI_GETFEATURE_ID_KEY, static_cast<ALI_GETFEATURE_ID_DATATYPE>(mpfFID) );
      filter_pwrite.Add( ALI_GETFEATURE_GUID_KEY, (ALI_GETFEATURE_GUID_DATATYPE)MPF_PWRITE_BBB_GUID );

      if ( false == m_pALIMMIO->mmioGetFeatureOffset( &m_pwriteDFHOffset, filter_pwrite ) ) {
         // No PWRITE found - this could mean that PWRITE is not enabled in MPF
         hasPWRITE = false;
         AAL_INFO(LM_AFU, "No PWRITE feature." << std::endl);
      } else {
         hasPWRITE = true;
         AAL_INFO(LM_All, "Using MMIO address 0x" << std::hex << m_pwriteDFHOffset <<
                  " for PWRITE." << std::endl);

         // Instantiate component class
         m_pPWRITE = new MPFPWRITE( m_pALIMMIO, m_pwriteDFHOffset );

         if ( ! m_pPWRITE->isOK() ) {
            initFailed(new CExceptionTransactionEvent( NULL,
                     rtid,
                     errBadParameter,
                     reasFeatureNotSupported,
                     "PWRITE initialization failed."));
            return true;
         }

         // found PWRITE, expose interface to outside
         SetInterface(iidMPFPWRITEService, dynamic_cast<IMPFPWRITE *>(m_pPWRITE));
      }
   }

   // Allow directly specified DFH offsets to override autodetection
   // FIXME: may fail silently, e.g. on wrong datatype.
   // Note: this assumes that m_vtpDFHOffset will not be updated if key is not
   // found.
   if ( ! hasVTP &&
        (ENamedValuesOK == optArgs.Get(MPF_VTP_DFH_OFFSET_KEY, &m_vtpDFHOffset)) ) {
      AAL_DEBUG(LM_AFU, "Using directly specified MPF_VTP_DFH_OFFSET)." <<
            std::endl);
      hasVTP = true;
   } else {
      AAL_DEBUG(LM_AFU, "No direct MPF_VTP_DFH_OFFSET supplied." << std::endl);
   }

   if (hasVTP) {
      AAL_INFO(LM_All, "Using MMIO address 0x" << std::hex << m_vtpDFHOffset <<
            " for VTP." << std::endl);

      // Instantiate component class
      // FIXME: all the m_* don't really need to be members of MPF
      m_pVTP = new MPFVTP( m_pALIBuffer, m_pALIMMIO, m_vtpDFHOffset );

      if ( ! m_pVTP->isOK() ) {
         initFailed(new CExceptionTransactionEvent( NULL,
                  rtid,
                  errBadParameter,
                  reasFeatureNotSupported,
                  "VTP initialization failed."));
         return true;
      }

      // found VTP, expose interface to outside
      SetInterface(iidMPFVTPService, dynamic_cast<IMPFVTP *>(m_pVTP));

   } else {
      AAL_INFO(LM_AFU, "No VTP feature." << std::endl);
   }

   initComplete(rtid);
   return true;
}

btBool MPF::Release(TransactionID const &rTranID, btTime timeout)
{
   // clean up VTP data stuctures
   if ( m_pVTP ) {
      delete m_pVTP;
   }
   return ServiceBase::Release(rTranID, timeout);
}

/// @} group VTPService

END_NAMESPACE(AAL)


#if defined( __AAL_WINDOWS__ )

BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
   switch ( ul_reason_for_call ) {
      case DLL_PROCESS_ATTACH :
         break;
      case DLL_THREAD_ATTACH  :
         break;
      case DLL_THREAD_DETACH  :
         break;
      case DLL_PROCESS_DETACH :
         break;
   }
   return TRUE;
}

#endif // __AAL_WINDOWS__


#define SERVICE_FACTORY AAL::InProcSvcsFact< AAL::MPF >

#if defined ( __AAL_WINDOWS__ )
# pragma warning(push)
# pragma warning(disable : 4996) // destination of copy is unsafe
#endif // __AAL_WINDOWS__

MPF_BEGIN_SVC_MOD(SERVICE_FACTORY)
   /* No commands other than default, at the moment. */
MPF_END_SVC_MOD()

#if defined ( __AAL_WINDOWS__ )
# pragma warning(pop)
#endif // __AAL_WINDOWS__

