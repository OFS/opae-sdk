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
/// @file cci_mpf_shim_latency_qos.h
/// @brief Definitions for Latency QoS Service.
/// @ingroup LATQOSService
/// @verbatim
/// Latency QoS management service.
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
#ifndef __CCI_MPF_SHIM_LATENCY_QOS_H__
#define __CCI_MPF_SHIM_LATENCY_QOS_H__

#include <aalsdk/mpf/IMPF.h>              // Public MPF service interface
#include <aalsdk/mpf/MPFService.h>


BEGIN_NAMESPACE(AAL)

/// @addtogroup LATQOSService
/// @{

class MPFLATQOS : public CAASBase, public IMPFLATQOS
{
public:

   /// LATQOS constructor
   MPFLATQOS( IALIMMIO   *pMMIOService,
              btCSROffset latqosDFHOffset );

   btBool isOK( void ) { return m_isOK; }     // < status after initialization

   // Set the configuration using one 64 bit register.
   // See cci_mpf_shim_latency_qos.sv for definition.
   btBool latqosSetConfig( btUnsigned64bitInt config );

protected:
   IALIMMIO              *m_pALIMMIO;
   btCSROffset            m_dfhOffset;

   btBool                 m_isOK;

};

/// @}

END_NAMESPACE(AAL)

#endif // __CCI_MPF_SHIM_LATENCY_QOS_H__
