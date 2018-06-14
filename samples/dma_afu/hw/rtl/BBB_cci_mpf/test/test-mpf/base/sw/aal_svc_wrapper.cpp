// Copyright(c) 2007-2016, Intel Corporation
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

#include "aal_svc_wrapper.h"


///////////////////////////////////////////////////////////////////////////////
///
///  Implementation
///
///////////////////////////////////////////////////////////////////////////////

/// @brief   Constructor registers this objects client interfaces and starts
///          the AAL Runtime. The member m_bisOK is used to indicate an error.
///
AAL_SVC_WRAPPER::AAL_SVC_WRAPPER(bool use_hw) :
    use_hw(use_hw),
    m_Runtime(this),
    m_pALIAFU_AALService(NULL),
    m_pALIBufferService(NULL),
    m_pALIMMIOService(NULL),
    m_pALIResetService(NULL),
    m_pMPF_AALService(NULL),
    pVTPService(NULL),
    pVCMAPService(NULL),
    pLATQOSService(NULL),
    pWROService(NULL),
    pPWRITEService(NULL),
    m_Result(0),
    m_ALIAFUTranID(),
    m_MPFTranID()
{
    // Register our Client side interfaces so that the Service can acquire them.
    //   SetInterface() is inherited from CAASBase
    SetInterface(iidServiceClient, dynamic_cast<IServiceClient *>(this));
    SetInterface(iidRuntimeClient, dynamic_cast<IRuntimeClient *>(this));

    // Initialize our internal semaphore
    m_Sem.Create(0, 1);

    // Start the AAL Runtime, setting any startup options via a NamedValueSet

    // Using Hardware Services requires the Remote Resource Manager Broker Service
    //  Note that this could also be accomplished by setting the environment variable
    //   AALRUNTIME_CONFIG_BROKER_SERVICE to librrmbroker
    NamedValueSet configArgs;
    NamedValueSet configRecord;

    if (use_hw) {
        // Specify that the remote resource manager is to be used.
        configRecord.Add(AALRUNTIME_CONFIG_BROKER_SERVICE, "librrmbroker");
        configArgs.Add(AALRUNTIME_CONFIG_RECORD, &configRecord);
    }

    // Start the Runtime and wait for the callback by sitting on the semaphore.
    //   the runtimeStarted() or runtimeStartFailed() callbacks should set m_bIsOK appropriately.
    if(!m_Runtime.start(configArgs)){
        m_bIsOK = false;
        return;
    }
    m_Sem.Wait();
    m_bIsOK = true;
}

/// @brief   Destructor
///
AAL_SVC_WRAPPER::~AAL_SVC_WRAPPER()
{
    m_Sem.Destroy();
}

/// @brief  initialize() is called from main performs the following:
///             - Allocate the appropriate ALI Service depending
///               on whether a hardware, ASE or software implementation is desired.
///             - Allocates the necessary buffers to be used by the NLB AFU algorithm
///
int AAL_SVC_WRAPPER::initialize(const char *afuID)
{
    // Request the Servcie we are interested in.

    // NOTE: This example is bypassing the Resource Manager's configuration record lookup
    //  mechanism.  Since the Resource Manager Implementation is a sample, it is subject to change.
    //  This example does illustrate the utility of having different implementations of a service all
    //  readily available and bound at run-time.
    NamedValueSet Manifest;
    NamedValueSet ConfigRecord;
    NamedValueSet featureFilter;
    btcString sGUID = MPF_VTP_BBB_GUID;

    unsigned int bufferSize;

    if (use_hw) {
        // Service Library to use
        ConfigRecord.Add(AAL_FACTORY_CREATE_CONFIGRECORD_FULL_SERVICE_NAME, "libALI");

        // the AFUID to be passed to the Resource Manager. It will be used to locate the appropriate device.
        ConfigRecord.Add(keyRegAFU_ID, afuID);

        // indicate that this service needs to allocate an AIAService, too to talk to the HW
        ConfigRecord.Add(AAL_FACTORY_CREATE_CONFIGRECORD_FULL_AIA_NAME, "libaia");
    }
    else {
        Manifest.Add(keyRegHandle, 20);
        Manifest.Add(ALIAFU_NVS_KEY_TARGET, ali_afu_ase);

        // Using AAL_sysALI isn't pretty, but it works.  AAL version 6 and
        // later use libALI for connecting to ASE.  AAL_sysALI is defined
        // in version 6 and later.
#ifdef AAL_sysALI
        ConfigRecord.Add(AAL_FACTORY_CREATE_CONFIGRECORD_FULL_SERVICE_NAME, "libALI");
#else
        // AAL version 5 and earlier
        ConfigRecord.Add(AAL_FACTORY_CREATE_CONFIGRECORD_FULL_SERVICE_NAME, "libASEALIAFU");
#endif
        ConfigRecord.Add(AAL_FACTORY_CREATE_SOFTWARE_SERVICE,true);
    }

    // Add the Config Record to the Manifest describing what we want to allocate
    Manifest.Add(AAL_FACTORY_CREATE_CONFIGRECORD_INCLUDED, &ConfigRecord);

    // in future, everything could be figured out by just giving the service name
    Manifest.Add(AAL_FACTORY_CREATE_SERVICENAME, "CCI-P Test");

    // Allocate the Service and wait for it to complete by sitting on the
    //   semaphore. The serviceAllocated() callback will be called if successful.
    //   If allocation fails the serviceAllocateFailed() should set m_bIsOK appropriately.
    //   (Refer to the serviceAllocated() callback to see how the Service's interfaces
    //    are collected.)
    //  Note that we are passing a custom transaction ID (created during app
    //   construction) to be able in serviceAllocated() to identify which
    //   service was allocated. This is only necessary if you are allocating more
    //   than one service from a single AAL service client.
    m_Runtime.allocService(dynamic_cast<IBase *>(this), Manifest, m_ALIAFUTranID);
    m_Sem.Wait();
    if(!m_bIsOK){
        ERR("ALIAFU allocation failed\n");

        m_Runtime.stop();
        m_Sem.Wait();
        return m_Result;
    }

    // Reuse Manifest and Configrecord for MPF service
    Manifest.Empty();
    ConfigRecord.Empty();

    // Allocate MPF service
    // Service Library to use
    ConfigRecord.Add(AAL_FACTORY_CREATE_CONFIGRECORD_FULL_SERVICE_NAME, "libMPF_AAL");
    ConfigRecord.Add(AAL_FACTORY_CREATE_SOFTWARE_SERVICE,true);

    // Add the Config Record to the Manifest describing what we want to allocate
    Manifest.Add(AAL_FACTORY_CREATE_CONFIGRECORD_INCLUDED, &ConfigRecord);

    // the MPFService will reuse the already established interfaces presented by
    // the ALIAFU service
    Manifest.Add(ALIAFU_IBASE_KEY, static_cast<ALIAFU_IBASE_DATATYPE>(m_pALIAFU_AALService));

    // MPFs feature ID, used to find correct features in DFH list
    Manifest.Add(MPF_FEATURE_ID_KEY, static_cast<MPF_FEATURE_ID_DATATYPE>(1));

    // in future, everything could be figured out by just giving the service name
    Manifest.Add(AAL_FACTORY_CREATE_SERVICENAME, "MPF");

    m_Runtime.allocService(dynamic_cast<IBase *>(this), Manifest, m_MPFTranID);
    m_Sem.Wait();
    if(!m_bIsOK){
        ERR("MPF Service allocation failed\n");

        m_Runtime.stop();
        m_Sem.Wait();
        return m_Result;
    }

    // Initiate AFU Reset
    m_pALIResetService->afuReset();

    // AFU Reset clear VTP, too, so reinitialize hardware
    if (pVTPService != NULL)
    {
        pVTPService->vtpReset();
    }

    return m_Result;
}


int AAL_SVC_WRAPPER::terminate()
{

    // Release the MPF Service through the Services IAALService::Release() method
    (dynamic_ptr<IAALService>(iidService, m_pMPF_AALService))->Release(TransactionID());
    m_Sem.Wait();

    // Release the HWALIAFU Service through the Services IAALService::Release() method
    (dynamic_ptr<IAALService>(iidService, m_pALIAFU_AALService))->Release(TransactionID());
    m_Sem.Wait();

    m_Runtime.stop();
    m_Sem.Wait();

    return 0;
}


//=================
//  IServiceClient
//=================

// <begin IServiceClient interface>
void AAL_SVC_WRAPPER::serviceAllocated(IBase *pServiceBase,
                                       TransactionID const &rTranID)
{
    // This application will allocate two different services (HWALIAFU and
    //  MPFService). We can tell them apart here by looking at the TransactionID.
    if (rTranID ==  m_ALIAFUTranID) {

        // Save the IBase for the Service. Through it we can get any other
        //  interface implemented by the Service
        m_pALIAFU_AALService = pServiceBase;
        ASSERT(NULL != m_pALIAFU_AALService);
        if ( NULL == m_pALIAFU_AALService ) {
            m_bIsOK = false;
            return;
        }

        // Documentation says HWALIAFU Service publishes
        //    IALIBuffer as subclass interface. Used in Buffer Allocation and Free
        m_pALIBufferService = dynamic_ptr<IALIBuffer>(iidALI_BUFF_Service, pServiceBase);
        ASSERT(NULL != m_pALIBufferService);
        if ( NULL == m_pALIBufferService ) {
            m_bIsOK = false;
            return;
        }

        // Documentation says HWALIAFU Service publishes
        //    IALIMMIO as subclass interface. Used to set/get MMIO Region
        m_pALIMMIOService = dynamic_ptr<IALIMMIO>(iidALI_MMIO_Service, pServiceBase);
        ASSERT(NULL != m_pALIMMIOService);
        if ( NULL == m_pALIMMIOService ) {
            m_bIsOK = false;
            return;
        }

        // Documentation says HWALIAFU Service publishes
        //    IALIReset as subclass interface. Used for resetting the AFU
        m_pALIResetService = dynamic_ptr<IALIReset>(iidALI_RSET_Service, pServiceBase);
        ASSERT(NULL != m_pALIResetService);
        if ( NULL == m_pALIResetService ) {
            m_bIsOK = false;
            return;
        }
    }
    else if (rTranID == m_MPFTranID) {

        // Save the IBase for the MPF Service.
        m_pMPF_AALService = pServiceBase;
        ASSERT(NULL != m_pMPF_AALService);
        if ( NULL == m_pMPF_AALService ) {
            m_bIsOK = false;
            return;
        }

        // Services will be NULL if not found
        pVTPService = dynamic_ptr<IMPFVTP>(iidMPFVTPService, pServiceBase);
        pVCMAPService = dynamic_ptr<IMPFVCMAP>(iidMPFVCMAPService, pServiceBase);
        pLATQOSService = dynamic_ptr<IMPFLATQOS>(iidMPFLATQOSService, pServiceBase);
        pWROService = dynamic_ptr<IMPFWRO>(iidMPFWROService, pServiceBase);
        pPWRITEService = dynamic_ptr<IMPFPWRITE>(iidMPFPWRITEService, pServiceBase);
    }
    else
    {
        ERR("Unknown transaction ID encountered on serviceAllocated().");
        m_bIsOK = false;
        return;
    }

    m_Sem.Post(1);
}

void AAL_SVC_WRAPPER::serviceAllocateFailed(const IEvent &rEvent)
{
    ERR("Failed to allocate Service");
    PrintExceptionDescription(rEvent);
    ++m_Result;                     // Remember the error
    m_bIsOK = false;

    m_Sem.Post(1);
}

void AAL_SVC_WRAPPER::serviceReleased(TransactionID const &rTranID)
{
    // Unblock Main()
    m_Sem.Post(1);
}

void AAL_SVC_WRAPPER::serviceReleaseRequest(IBase *pServiceBase, const IEvent &rEvent)
{
    if(NULL != m_pALIAFU_AALService){
        IAALService *pIAALService = dynamic_ptr<IAALService>(iidService, m_pALIAFU_AALService);
        ASSERT(pIAALService);
        pIAALService->Release(TransactionID());
    }
}

void AAL_SVC_WRAPPER::serviceReleaseFailed(const IEvent        &rEvent)
{
    ERR("Failed to release a Service");
    PrintExceptionDescription(rEvent);
    m_bIsOK = false;
    m_Sem.Post(1);
}


void AAL_SVC_WRAPPER::serviceEvent(const IEvent &rEvent)
{
    ERR("unexpected event 0x" << hex << rEvent.SubClassID());
    // The state machine may or may not stop here. It depends upon what happened.
    // A fatal error implies no more messages and so none of the other Post()
    //    will wake up.
    // OTOH, a notification message will simply print and continue.
}
// <end IServiceClient interface>


//=================
//  IRuntimeClient
//=================

// <begin IRuntimeClient interface>
// Because this simple example has one object implementing both IRuntieCLient and IServiceClient
//   some of these interfaces are redundant. We use the IServiceClient in such cases and ignore
//   the RuntimeClient equivalent e.g.,. runtimeAllocateServiceSucceeded()

void AAL_SVC_WRAPPER::runtimeStarted( IRuntime            *pRuntime,
                                      const NamedValueSet &rConfigParms)
{
    m_bIsOK = true;
    m_Sem.Post(1);
}

void AAL_SVC_WRAPPER::runtimeStopped(IRuntime *pRuntime)
{
    m_bIsOK = false;
    m_Sem.Post(1);
}

void AAL_SVC_WRAPPER::runtimeStartFailed(const IEvent &rEvent)
{
    ERR("Runtime start failed");
    PrintExceptionDescription(rEvent);
}

void AAL_SVC_WRAPPER::runtimeStopFailed(const IEvent &rEvent)
{
    ERR("Runtime stop failed");
    m_bIsOK = false;
    m_Sem.Post(1);
}

void AAL_SVC_WRAPPER::runtimeAllocateServiceFailed( IEvent const &rEvent)
{
    ERR("Runtime AllocateService failed");
    PrintExceptionDescription(rEvent);
}

void AAL_SVC_WRAPPER::runtimeAllocateServiceSucceeded(IBase *pClient,
                                                      TransactionID const &rTranID)
{
}

void AAL_SVC_WRAPPER::runtimeEvent(const IEvent &rEvent)
{
}
