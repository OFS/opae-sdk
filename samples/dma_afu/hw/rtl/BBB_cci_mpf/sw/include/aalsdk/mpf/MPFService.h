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
/// @file VTPService.h
/// @brief AAL Service Module definitions for VTP Service
/// @ingroup VTPService
/// @verbatim
/// Intel(R) QuickAssist Technology Accelerator Abstraction Layer
///
/// AUTHORS: Enno Luebbers, Intel Corporation
///
/// HISTORY:
/// WHEN:          WHO:     WHAT:
/// 01/27/2016     EL       Initial version.@endverbatim
//****************************************************************************
#ifndef __SERVICE_MPFSERVICE_H__
#define __SERVICE_MPFSERVICE_H__
#include <aalsdk/osal/OSServiceModule.h>
#include <aalsdk/INTCDefs.h>

/// @addtogroup VTPService
/// @{

#if defined ( __AAL_WINDOWS__ )
# ifdef MPF_EXPORTS
#    define MPF_API __declspec(dllexport)
# else
#    define MPF_API __declspec(dllimport)
# endif // MPF_EXPORTS
#else
# ifndef __declspec
#    define __declspec(x)
# endif // __declspec
# define MPF_API    __declspec(0)
#endif // __AAL_WINDOWS__

#define MPF_SVC_MOD         "libMPF_AAL" AAL_SVC_MOD_EXT
#define MPF_SVC_ENTRY_POINT "libMPF_AAL" AAL_SVC_MOD_ENTRY_SUFFIX

#define MPF_BEGIN_SVC_MOD(__svcfactory) AAL_BEGIN_SVC_MOD(__svcfactory, libMPF_AAL, MPF_API, MPF_VERSION, MPF_VERSION_CURRENT, MPF_VERSION_REVISION, MPF_VERSION_AGE)
#define MPF_END_SVC_MOD()               AAL_END_SVC_MOD()

AAL_DECLARE_SVC_MOD(libMPF_AAL, MPF_API)

// FIXME!
// 64-bit  AFU-ID: 00000000-0000-0000-9AEF-FE5F84570612
// 128-bit AFU-ID: C000C966-0D82-4272-9AEF-FE5F84570612

// FIXME
#define MPF_MANIFEST \
"9 20 ConfigRecordIncluded\n \
\t10\n \
\t\t9 17 ServiceExecutable\n \
\t\t\t9 @ALIAFU_SVC_MLEN@ @ALIAFU_SVC_MOD@\n \
\t\t9 18 _CreateSoftService\n \
\t\t0 1\n \
9 29 ---- End of embedded NVS ----\n \
\t9999\n \
9 11 ServiceName\n \
\t9 10 MPF\n"

/// @}

#endif // __SERVICE_MPFSERVICE_H__

