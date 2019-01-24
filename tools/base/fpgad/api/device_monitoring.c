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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif // HAVE_CONFIG_H

#include "device_monitoring.h"

#ifdef LOG
#undef LOG
#endif
#define LOG(format, ...) \
log_printf("device_monitoring: " format, ##__VA_ARGS__)

bool mon_has_error_occurred(fpgad_monitored_device *d, void *err)
{
	unsigned i;
	for (i = 0 ; i < d->num_error_occurrences ; ++i) {
		if (err == d->error_occurrences[i])
			return true;
	}
	return false;
}

bool mon_add_device_error(fpgad_monitored_device *d, void *err)
{
	if (d->num_error_occurrences <
		(sizeof(d->error_occurrences) /
		 sizeof(d->error_occurrences[0]))) {
		d->error_occurrences[d->num_error_occurrences++] = err;
		return true;
	}
	LOG("exceeded max number of device errors!\n");
	return false;
}

void mon_remove_device_error(fpgad_monitored_device *d, void *err)
{
	unsigned i;
	unsigned j;
	unsigned removed = 0;
	for (i = j = 0 ; i < d->num_error_occurrences ; ++i) {
		if (d->error_occurrences[i] != err)
			d->error_occurrences[j++] = d->error_occurrences[i];
		else
			++removed;
	}
	d->num_error_occurrences -= removed;
}
