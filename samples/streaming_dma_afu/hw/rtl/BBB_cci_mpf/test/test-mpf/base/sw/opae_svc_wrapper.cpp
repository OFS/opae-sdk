//
// Copyright (c) 2017, Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// Neither the name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <stdlib.h>
#include <unistd.h>

#include <uuid/uuid.h>
#include <iostream>
#include <algorithm>

#include "opae_svc_wrapper.h"

using namespace std;


OPAE_SVC_WRAPPER::OPAE_SVC_WRAPPER(bool use_hw) :
    accel_handle(NULL),
    mpf_handle(NULL),
    is_ok(true),
    is_simulated(false)
{
}


OPAE_SVC_WRAPPER::~OPAE_SVC_WRAPPER()
{
}


int OPAE_SVC_WRAPPER::initialize(const char *accel_uuid)
{
    fpga_result r;

    // Is the hardware simulated with ASE?
    is_simulated = probeForASE();

    // Connect to an available accelerator with the requested UUID
    r = findAndOpenAccel(accel_uuid);

    is_ok = (FPGA_OK == r);

    return int(r);
}


int OPAE_SVC_WRAPPER::terminate()
{
    mpfDisconnect(mpf_handle);
    fpgaUnmapMMIO(accel_handle, 0);
    fpgaClose(accel_handle);

    return 0;
}


void*
OPAE_SVC_WRAPPER::allocBuffer(size_t nBytes, uint64_t* ioAddress)
{
    fpga_result r;
    void* va;

    //
    // Allocate an I/O buffer shared with the FPGA.  When VTP is present
    // the FPGA-side address translation allows us to allocate multi-page,
    // virtually contiguous buffers.  When VTP is not present the
    // accelerator must manage physical addresses on its own.  In that case,
    // the I/O buffer allocation (fpgaPrepareBuffer) is limited to
    // allocating one page per invocation.
    //

    if (mpfVtpIsAvailable(mpf_handle))
    {
        // VTP is available.  Use it to get a virtually contiguous region.
        // The region may be composed of multiple non-contiguous physical
        // pages.
        r = mpfVtpBufferAllocate(mpf_handle, nBytes, &va);
        if (FPGA_OK != r) return NULL;

        if (ioAddress)
        {
            *ioAddress = mpfVtpGetIOAddress(mpf_handle, va);
        }
    }
    else
    {
        // VTP is not available.  Map a page without a TLB entry.  nBytes
        // must not be larger than a page.
        uint64_t wsid;
        r = fpgaPrepareBuffer(accel_handle, nBytes, &va, &wsid, 0);
        if (FPGA_OK != r) return NULL;

        if (ioAddress)
        {
            r = fpgaGetIOAddress(accel_handle, wsid, ioAddress);
            if (FPGA_OK != r) return NULL;
        }
    }

    return va;
}

void
OPAE_SVC_WRAPPER::freeBuffer(void* va)
{
    // For now this class only handles VTP cleanly.  Unmanaged pages
    // aren't released.  The kernel will automatically release them
    // at the end of a run.
    if (mpfVtpIsAvailable(mpf_handle))
    {
        mpfVtpBufferFree(mpf_handle, va);
    }
}


fpga_result
OPAE_SVC_WRAPPER::findAndOpenAccel(const char* accel_uuid)
{
    fpga_result r;

    // Set up a filter that will search for an accelerator
    fpga_properties filter = NULL;
    fpgaGetProperties(NULL, &filter);
    fpgaPropertiesSetObjectType(filter, FPGA_ACCELERATOR);

    // Add the desired UUID to the filter
    fpga_guid guid;
    uuid_parse(accel_uuid, guid);
    fpgaPropertiesSetGUID(filter, guid);

    // How many accelerators match the requested properties?
    uint32_t max_tokens;
    fpgaEnumerate(&filter, 1, NULL, 0, &max_tokens);
    if (0 == max_tokens)
    {
        cerr << "FPGA with accelerator uuid " << accel_uuid << " not found!" << endl << endl;
        fpgaDestroyProperties(&filter);
        return FPGA_NOT_FOUND;
    }

    // Now that the number of matches is known, allocate a token vector
    // large enough to hold them.
    fpga_token* tokens = new fpga_token[max_tokens];
    if (NULL == tokens)
    {
        fpgaDestroyProperties(&filter);
        return FPGA_NO_MEMORY;
    }

    // Enumerate and get the tokens
    uint32_t num_matches;
    fpgaEnumerate(&filter, 1, tokens, max_tokens, &num_matches);

    // Not needed anymore
    fpgaDestroyProperties(&filter);

    // Try to open a matching accelerator.  fpgaOpen() will fail if the
    // accelerator is already in use.
    fpga_token accel_token;
    r  = FPGA_NOT_FOUND;
    for (uint32_t i = 0; i < num_matches; i++)
    {
        accel_token = tokens[i];
        r = fpgaOpen(accel_token, &accel_handle, 0);
        // Success?
        if (FPGA_OK == r) break;
    }
    if (FPGA_OK != r)
    {
        cerr << "No accelerator available with uuid " << accel_uuid << endl << endl;
        goto done;
    }

    // Map MMIO
    fpgaMapMMIO(accel_handle, 0, NULL);

    // Connect to MPF
    r = mpfConnect(accel_handle, 0, 0, &mpf_handle, 0/*MPF_FLAG_DEBUG*/);
    if (FPGA_OK != r) goto done;

  done:
    // Done with tokens
    for (uint32_t i = 0; i < num_matches; i++)
    {
        fpgaDestroyToken(&tokens[i]);
    }

    delete[] tokens;

    return r;
}


//
// Is the FPGA real or simulated with ASE?
//
bool
OPAE_SVC_WRAPPER::probeForASE()
{
    fpga_result r = FPGA_OK;
    uint32_t device_id = 0;

    // Connect to the FPGA management engine
    fpga_properties filter = NULL;
    fpgaGetProperties(NULL, &filter);
    fpgaPropertiesSetObjectType(filter, FPGA_DEVICE);

    // Connecting to one is sufficient to find ASE.
    uint32_t num_matches = 1;
    fpga_token fme_token;
    fpgaEnumerate(&filter, 1, &fme_token, 1, &num_matches);
    if (0 != num_matches)
    {
        // Retrieve the device ID of the FME
        fpgaGetProperties(fme_token, &filter);
        r = fpgaPropertiesGetDeviceID(filter, &device_id);
        fpgaDestroyToken(&fme_token);
    }
    fpgaDestroyProperties(&filter);

    // ASE's device ID is 0xa5ea5e
    return ((FPGA_OK == r) && (0xa5ea5e == device_id));
}
