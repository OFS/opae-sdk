// Copyright(c) 2018-2019, Intel Corporation
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

#ifndef __FPGAD_MONITORED_DEVICE_H__
#define __FPGAD_MONITORED_DEVICE_H__

#include "fpgad.h"

typedef struct _fpgad_supported_device {
	uint16_t vendor_id;
	uint16_t device_id;
	const char *library_path;
	uint32_t flags;
#define FPGAD_DEV_DETECTED 0x00000001
#define FPGAD_DEV_LOADED   0x00000002
	void *dl_handle;
} fpgad_supported_device;

typedef enum _fpgad_plugin_type {
	FPGAD_PLUGIN_TYPE_CALLBACK = 1,
	FPGAD_PLUGIN_TYPE_THREAD
} fpgad_plugin_type;

typedef struct _fpgad_monitored_device
		fpgad_monitored_device;

typedef enum _fpgad_detection_status {
	FPGAD_STATUS_NOT_DETECTED = 0, // no detection
	FPGAD_STATUS_DETECTED,         // detected (normal priority)
	FPGAD_STATUS_DETECTED_HIGH     // detected (high priority)
} fpgad_detection_status;

typedef fpgad_detection_status
		(*fpgad_detect_event_t)(fpgad_monitored_device *dev,
					void *context);
typedef void (*fpgad_respond_event_t)(fpgad_monitored_device *dev,
				      void *context);

typedef void * (*fpgad_plugin_thread_t)(void *context);
typedef void (*fpgad_plugin_thread_stop_t)(void);

typedef struct _fpgad_monitored_device {
	struct config *config;
	fpgad_supported_device *supported;
	fpga_token token;
	uint64_t object_id;
	fpga_objtype object_type;
	struct bitstream_info *bitstr;

	fpgad_plugin_type type;

	// for type FPGAD_PLUGIN_TYPE_CALLBACK {

	// must be NULL-terminated
	fpgad_detect_event_t *detections;
	void **detection_contexts;

	fpgad_respond_event_t *responses;
	void **response_contexts;

	// }

	// for type FPGAD_PLUGIN_TYPE_THREAD {

	fpgad_plugin_thread_t thread_fn;

	// The parameter to thread_fn will be the address
	// of this fpgad_monitored_device. Use the
	// following member to pass a thread-specific
	// context:

	void *thread_context;

	// This routine is called to make the plugin
	// thread stop execution in preparation for
	// being joined.
	fpgad_plugin_thread_stop_t thread_stop_fn;

	pthread_t thread;

	// }

#define MAX_DEV_ERROR_OCCURRENCES 64
	void *error_occurrences[MAX_DEV_ERROR_OCCURRENCES];
	unsigned num_error_occurrences;

#define MAX_DEV_SCRATCHPAD 2
	uint64_t scratchpad[MAX_DEV_SCRATCHPAD];

	struct _fpgad_monitored_device *next;
} fpgad_monitored_device;

#define FPGAD_PLUGIN_CONFIGURE "fpgad_plugin_configure"
typedef int (*fpgad_plugin_configure_t)(fpgad_monitored_device *d,
					const char *cfg);

#define FPGAD_PLUGIN_DESTROY "fpgad_plugin_destroy"
typedef void (*fpgad_plugin_destroy_t)(fpgad_monitored_device *d);

#endif /* __FPGAD_MONITORED_DEVICE_H__ */
