// Copyright(c) 2017, Intel Corporation
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

#ifndef __PACD_CONFIG_H__
#define __PACD_CONFIG_H__
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define MAX_NULL_GBS 16
#define MAX_PAC_DEVICES 32
#define MAX_SENSORS_TO_MONITOR 256
#define DISABLE_THRESHOLD 20

// Wait interval when no driver installed - 30 seconds
#define PACD_WAIT_FOR_CARD (30)

// Constant adjustment for thermal trigger (added)
#define PACD_THERMAL_INCREMENT (5.0)

// Constant adjustment for power trigger (multiplied)
#define PACD_POWER_MULTIPLIER (1.05)

/*
 * Global configuration, set during parse_args()
 */
struct config {
	unsigned int verbosity;
	struct timespec poll_interval;
	struct timespec cooldown_delay;

	int daemon;	    // whether to daemonize
	const char *directory; // working directory when daemonizing
	const char *logfile;   // location of log file
	const char *pidfile;   // where to write pacd.pid
	mode_t filemode;       // argument for umask

	bool running;

	uint16_t segment;
	uint8_t bus;
	uint8_t device;
	uint8_t function;

	const char *null_gbs[MAX_NULL_GBS];
	unsigned int num_null_gbs;

	unsigned int num_PACs;
	uint32_t no_defaults;

	unsigned int num_thresholds;
	int32_t sensor_number[MAX_SENSORS_TO_MONITOR];
	double upper_trigger_value[MAX_SENSORS_TO_MONITOR];
	double upper_reset_value[MAX_SENSORS_TO_MONITOR];
	double lower_trigger_value[MAX_SENSORS_TO_MONITOR];
	double lower_reset_value[MAX_SENSORS_TO_MONITOR];
	uint32_t invalid_count[MAX_SENSORS_TO_MONITOR];

	int remove_driver;
};

#endif
