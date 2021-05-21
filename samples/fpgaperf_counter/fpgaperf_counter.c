// Copyright(c) 2021, Intel Corporation
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

#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>
#include <libudev.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>
#include <opae/log.h>
#include "opae_int.h"
#include <opae/properties.h>
#include <opae/utils.h>
#include <opae/fpga.h>
#include <regex.h>
#include "fpgaperf_counter.h"


#define DFL_PERF_FME 		"/sys/bus/pci/devices/*%x*:*%x*:*%x*.*%x*/fpga_region/region*/dfl-fme.*"

#define DFL_PERF_SYSFS		"/sys/bus/event_source/devices/dfl_fme"

#define DFL_PERF_STR_MAX	256

#define PERF_EVNET_PATTERN	"event=(0x[0-9a-fA-F]{2}),evtype=(0x[0-9a-fA-F]{2}),portid=(0x[0-9a-fA-F]{2})"

#define PERF_CONFIG_PATTERN	"config:([0-9]{1,})-([0-9]{2,})"

#define PARSE_MATCH_INT(_p, _m, _v, _b)				\
	do {							\
		errno = 0;					\
		_v = strtoul(_p + _m, NULL, _b);		\
		if (errno) {					\
			OPAE_MSG("error parsing int");		\
		}						\
	} while (0)

/* mutex to protect fpga perf pmu data structures */
pthread_mutex_t fpga_perf_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/* Read format structure*/
struct read_format {
	uint64_t nr;
	struct {
		uint64_t value;
		uint64_t id;
	} values[];
};

typedef struct  {
	char event_name[DFL_PERF_STR_MAX];
	uint64_t config;
	int fd;
	uint64_t id;
	uint64_t start_value;
	uint64_t stop_value;
}perf_event_type;

typedef struct {
	char format_name[DFL_PERF_STR_MAX];
	uint64_t shift;
}perf_format_type;

typedef struct {
	char dfl_fme_name[DFL_PERF_STR_MAX];
	uint64_t type;
	uint64_t cpumask;
	uint64_t num_format;
	perf_format_type *format_type;
	uint64_t num_perf_events;
	perf_event_type *perf_events;
}fpga_perf_counter;

/* Not static so other tools can access the PMU data */
fpga_perf_counter *g_fpga_perf = NULL;

/* parse the each format and get the shift val */
fpga_result parse_Perf_Format(struct udev_device *dev)
{
	regex_t re;
	regmatch_t matches[3] 	= { {0} };
	char err[128] 		= { 0 };
	int reg_res 		= 0;
	int res			= 0;
	uint64_t loop 		= 0;
	uint64_t value 		= 0;

	if (!dev) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}	
	if (opae_mutex_lock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to lock handle mutex");
		return FPGA_EXCEPTION;
	}	
	struct udev_list_entry *attrs = udev_device_get_sysattr_list_entry(dev);
	struct udev_list_entry *le = NULL;
	g_fpga_perf->num_format = 0;
	
	udev_list_entry_foreach(le, attrs) {
		const char *attr = udev_list_entry_get_name(le);
		if (strstr(attr, "format"))
			g_fpga_perf->num_format++;
	}	
	g_fpga_perf->format_type = calloc(g_fpga_perf->num_format,
			sizeof(perf_format_type));			
	if (!g_fpga_perf->format_type) {
		g_fpga_perf->num_format = 0;
		goto out;
	}
	udev_list_entry_foreach(le, attrs) {
		const char *attr = udev_list_entry_get_name(le);
		if (strstr(attr, "format")) {
			reg_res = regcomp(&re, PERF_CONFIG_PATTERN,
					REG_EXTENDED | REG_ICASE);
			if (reg_res) {
				OPAE_ERR("Error compling regex");
				goto out;
			}
			reg_res = regexec(&re, udev_device_get_sysattr_value(dev, attr),
					4, matches, 0);
			if (reg_res) {
				regerror(reg_res, &re, err, 128);
				OPAE_ERR("Error executing regex: %s", err);
				goto out;
			} else {
				const char *attr_value = udev_device_get_sysattr_value(dev, attr);
				PARSE_MATCH_INT(attr_value, matches[1].rm_so, value, 10);
				g_fpga_perf->format_type[loop].shift = value;
				if (snprintf(g_fpga_perf->format_type[loop].format_name,
					sizeof(g_fpga_perf->format_type[loop].format_name),
					"%s", (strstr(attr, "/")+1)) < 0) {
					OPAE_ERR("snprintf buffer overflow");
					goto out;
				}
				loop++;
			}
		}
	}
	if (opae_mutex_unlock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to unlock handle mutex");
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
out:
	opae_mutex_unlock(res, &fpga_perf_lock);
	return FPGA_EXCEPTION;
}

/* parse the events for the perticular device directory */
fpga_result parse_Perf_Event(struct udev_device *dev)
{	
	regex_t re;
	regmatch_t matches[4] 	= { {0} };
	char err[128] 		= { 0 };
	int reg_res		= 0;
	uint64_t loop 		= 0;
	uint64_t inner_loop 	= 0;
	int res			= 0;

	if (!dev) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}
	if (opae_mutex_lock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to lock handle mutex");
		return FPGA_EXCEPTION;
	}
	struct udev_list_entry *attrs = udev_device_get_sysattr_list_entry(dev);
	struct udev_list_entry *le = NULL;
	g_fpga_perf->num_perf_events = 0;
	
	udev_list_entry_foreach(le, attrs) {
		const char *attr = udev_list_entry_get_name(le);
		if (strstr(attr, "events"))
			g_fpga_perf->num_perf_events++;
	}
	g_fpga_perf->perf_events = calloc(g_fpga_perf->num_perf_events,
			sizeof(perf_event_type));
	if (!g_fpga_perf->perf_events) {
		g_fpga_perf->num_perf_events = 0;
		goto out;
	}
	udev_list_entry_foreach(le, attrs) {
		const char *attr = udev_list_entry_get_name(le);
		if (strstr(attr, "events")) {
			reg_res = regcomp(&re, PERF_EVNET_PATTERN,
					REG_EXTENDED | REG_ICASE);
			if (reg_res) {
				OPAE_ERR("Error compling regex");
				goto out;
			}
			reg_res = regexec(&re, udev_device_get_sysattr_value(dev, attr),
					4, matches, 0);
			if (reg_res) {
				regerror(reg_res, &re, err, 128);
				OPAE_MSG("Error executing regex: %s", err);
			} else {
				const char *attr_value = udev_device_get_sysattr_value(dev, attr);
				uint64_t config = 0;
				uint64_t event = 0;
				if (snprintf(g_fpga_perf->perf_events[inner_loop].event_name,
					sizeof(g_fpga_perf->perf_events[inner_loop].event_name),
						"%s", (strstr(attr, "/") + 1)) < 0) {
					OPAE_ERR("snprintf buffer overflow");
					goto out;
				}

				for (loop = 0; loop < g_fpga_perf->num_format; loop++) {
					PARSE_MATCH_INT(attr_value,
							matches[loop + 1].rm_so, event, 16);
					config |= event << g_fpga_perf->format_type[loop].shift;
				}
				g_fpga_perf->perf_events[inner_loop].config = config;
				inner_loop++;
			}
		}
	}
	if (opae_mutex_unlock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to unlock handle mutex");
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
out:
	opae_mutex_unlock(res, &fpga_perf_lock);
	return FPGA_EXCEPTION;
}


fpga_result fpga_Perf_Events(char* perf_sysfs_path)
{
	fpga_result ret 	= FPGA_OK;
	struct udev *udev 	= NULL;
	struct udev_device *dev = NULL;
	int fd 			= 0;
	int grpfd		= 0;
	int res 		= 0;
	uint64_t loop 		= 0;
	struct perf_event_attr pea;
	
	if (!perf_sysfs_path) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}
	/* create udev object */
	udev = udev_new();
	if (!udev) {
		OPAE_ERR("Cannot create udev context");
		return FPGA_EXCEPTION;
	}
	dev = udev_device_new_from_syspath(udev, perf_sysfs_path);
	if (!dev) {
		OPAE_ERR("Failed to get device");
		udev_unref(udev);
		return FPGA_EXCEPTION;
	}	
	if (opae_mutex_lock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to lock handle mutex");
		goto err;
	}
	const char * ptr = udev_device_get_sysattr_value(dev, "cpumask");
	if (ptr)
		PARSE_MATCH_INT(ptr, 0, g_fpga_perf->cpumask, 10);

	ptr = udev_device_get_sysattr_value(dev, "type");
	if (ptr)
		PARSE_MATCH_INT(ptr, 0, g_fpga_perf->type, 10);

	if (opae_mutex_unlock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to unlock handle mutex");
		goto err;
	}
	/* parse the format value */
	ret = parse_Perf_Format(dev);
	if (ret != FPGA_OK)
		goto err;
	/* parse the event value */
	ret = parse_Perf_Event(dev);
	if (ret != FPGA_OK)
		goto err;

	/* initialize the pea structure to 0 */
	memset(&pea, 0, sizeof(struct perf_event_attr));
	
	if (opae_mutex_lock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to lock handle mutex");
		goto err;
	}

	for (loop = 0; loop < g_fpga_perf->num_perf_events; loop++) {

		if (g_fpga_perf->perf_events[0].fd <= 0)
			grpfd = -1;
		else
			grpfd = g_fpga_perf->perf_events[0].fd;
		
		if (!g_fpga_perf->perf_events[loop].config)
			continue;

		pea.type = g_fpga_perf->type;
		pea.size = sizeof(struct perf_event_attr);
		pea.config = g_fpga_perf->perf_events[loop].config;
		pea.disabled = 1;
		pea.inherit = 1;
		pea.sample_type = PERF_SAMPLE_IDENTIFIER;
		pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
		fd = syscall(__NR_perf_event_open, &pea, -1, g_fpga_perf->cpumask, grpfd, 0);
		if (fd == -1) {
			OPAE_ERR("Error opening leader %llx\n", pea.config);
			ret = FPGA_EXCEPTION;
			goto out;
		} else {
			g_fpga_perf->perf_events[loop].fd = fd;
			if (ioctl(g_fpga_perf->perf_events[loop].fd, PERF_EVENT_IOC_ID,
						&g_fpga_perf->perf_events[loop].id) == -1) {
				OPAE_ERR("PERF_EVENT_IOC_ID ioctl failed: %s",
						strerror(errno));
				ret = FPGA_EXCEPTION;
				goto out;
			}
		}
	}
	if (ioctl(g_fpga_perf->perf_events[0].fd, PERF_EVENT_IOC_RESET,
				PERF_IOC_FLAG_GROUP) == -1) {
		OPAE_ERR("PERF_EVENT_IOC_RESET ioctl failed: %s", strerror(errno));
		ret = FPGA_EXCEPTION;
		goto out;
	}
out:
	udev_unref(udev);
	udev_device_unref(dev);
	opae_mutex_unlock(res, &fpga_perf_lock);
	return ret;
err:
	udev_unref(udev);
	udev_device_unref(dev);
	return FPGA_EXCEPTION;
}

/* get fpga sbdf from token */
fpga_result get_Fpga_Sbdf(fpga_token token,
		uint16_t *segment,
		uint8_t *bus,
		uint8_t *device,
		uint8_t *function)
{
	fpga_result res = FPGA_OK;
	fpga_properties props = NULL;

	if (!segment || !bus ||
		!device || !function) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}
	res = fpgaGetProperties(token, &props);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get properties ");
		return res;
	}
	res = fpgaPropertiesGetBus(props, bus);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get bus ");
		return res;
	}
	res = fpgaPropertiesGetSegment(props, segment);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Segment ");
		return res;
	}
	res = fpgaPropertiesGetDevice(props, device);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Device ");
		return res;
	}
	res = fpgaPropertiesGetFunction(props, function);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Function ");
		return res;
	}

	return res;
}

fpga_result fpgaPerfCounterEnum(fpga_token token)
{
	fpga_result ret				= FPGA_OK;
	char sysfs_path[DFL_PERF_STR_MAX]	= { 0 };
	char sysfs_perf[DFL_PERF_STR_MAX]	= { 0 };
	int gres 				= 0;
	int res 				= 0;
	uint32_t fpga_id 			= -1;
	char *endptr 				= NULL;
	glob_t pglob;
	uint8_t bus				= (uint8_t)-1;
	uint16_t segment			= (uint16_t)-1;
	uint8_t device				= (uint8_t)-1;
	uint8_t function			= (uint8_t)-1;

	res = get_Fpga_Sbdf(token, &segment, &bus, &device, &function);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get sbdf ");
		return res;
	}
	/* when we bind with new device id we will get updated function value */
	/* not able to read the sysfs path using that */
	if(function)
		function = 0;
	if (snprintf(sysfs_path, sizeof(sysfs_path),
		DFL_PERF_FME,
		segment, bus, device, function) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}
	gres = glob(sysfs_path, GLOB_NOSORT, NULL, &pglob);
	if (gres) {
		OPAE_ERR("Failed pattern match %s: %s", sysfs_path, strerror(errno));
		globfree(&pglob);
		return FPGA_NOT_FOUND;
	}
	if (pglob.gl_pathc == 1) {
		char *ptr = strstr(pglob.gl_pathv[0], "fme");
		if (!ptr) {
			ret = FPGA_INVALID_PARAM;
			goto out;
		}
		errno = 0;
		fpga_id = strtoul(ptr + 3, &endptr, 16);

		if (snprintf(sysfs_perf, sizeof(sysfs_perf),
			DFL_PERF_SYSFS"%d", fpga_id) < 0) {
			OPAE_ERR("snprintf buffer overflow");
			ret = FPGA_EXCEPTION;
			goto out;
		}		
		if (opae_mutex_lock(res, &fpga_perf_lock)) {
			OPAE_MSG("Failed to lock handle mutex");
			ret = FPGA_EXCEPTION;
			goto out;
		}
		/*allocate memory for PMUs */
		g_fpga_perf = malloc(sizeof(fpga_perf_counter));
		if (!g_fpga_perf) {
			opae_mutex_unlock(res, &fpga_perf_lock);
			ret = FPGA_EXCEPTION;
			goto out;
		}
		if (snprintf(g_fpga_perf->dfl_fme_name, sizeof(g_fpga_perf->dfl_fme_name),
			"dfl_fme%d", fpga_id) < 0) {
			OPAE_ERR("snprintf buffer overflow");
			opae_mutex_unlock(res, &fpga_perf_lock);
			ret = FPGA_EXCEPTION;
			goto out;
		}
		if (opae_mutex_unlock(res, &fpga_perf_lock)) {
			OPAE_MSG("Failed to unlock handle mutex");
			ret = FPGA_EXCEPTION;
			goto out;
		}
		if (fpga_Perf_Events(sysfs_perf) != ret)	{
			OPAE_ERR("Failed to parse fpga perf event");
			goto out;
		}

	} else {
		ret = FPGA_NOT_FOUND;
		goto out;
	}

out:
	globfree(&pglob);
	return ret;
}

fpga_result fpgaPerfCounterStartRecord(void)
{
	uint64_t loop			= 0;
	uint64_t inner_loop		= 0;
	char buf[DFL_PERF_STR_MAX]	= { 0 };
	struct read_format *rdft	= (struct read_format *) buf;
	int res				= 0;
	
	if (opae_mutex_lock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to lock handle mutex");
		return FPGA_EXCEPTION;
	}
	if (ioctl(g_fpga_perf->perf_events[0].fd, PERF_EVENT_IOC_ENABLE,
				PERF_IOC_FLAG_GROUP) == -1) {
		OPAE_ERR("PERF_EVENT_IOC_ENABLE ioctl failed: %s",
				strerror(errno));
		goto out;
	}
	if (read(g_fpga_perf->perf_events[0].fd, rdft, sizeof(buf)) == -1) {
		OPAE_ERR("read fpga perf counter failed");
		goto out;
	}
	for (loop = 0; loop < (uint64_t)rdft->nr; loop++) {
		for (inner_loop = 0; inner_loop < g_fpga_perf->num_perf_events;
								inner_loop++) {
			if (rdft->values[loop].id == g_fpga_perf->perf_events[inner_loop].id)
				g_fpga_perf->perf_events[inner_loop].start_value = rdft->values[loop].value;
		}
	}	
	if (opae_mutex_unlock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to unlock handle mutex");
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
out:
	opae_mutex_unlock(res, &fpga_perf_lock);
	return FPGA_EXCEPTION;
}

fpga_result fpgaPerfCounterStopRecord(void)
{
	char buf[DFL_PERF_STR_MAX]	= { 0 };
	uint64_t loop			= 0;
	uint64_t inner_loop		= 0;
	struct read_format *rdft 	= (struct read_format *) buf;
	int res				= 0;

	if (opae_mutex_lock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to lock handle mutex");
		return FPGA_EXCEPTION;
	}
	if (ioctl(g_fpga_perf->perf_events[0].fd, PERF_EVENT_IOC_DISABLE,
				PERF_IOC_FLAG_GROUP) == -1) {
		OPAE_ERR("PERF_EVENT_IOC_ENABLE ioctl failed: %s",
				strerror(errno));
		goto out;
	}
	if (read(g_fpga_perf->perf_events[0].fd, rdft, sizeof(buf)) == -1) {
		OPAE_ERR("read fpga perf counter failed");
		goto out;
	}	
	for (loop = 0; loop < (uint64_t)rdft->nr; loop++) {
		for (inner_loop = 0; inner_loop < g_fpga_perf->num_perf_events;
								inner_loop++) {
			if (rdft->values[loop].id == g_fpga_perf->perf_events[inner_loop].id)
				g_fpga_perf->perf_events[inner_loop].stop_value = rdft->values[loop].value;
		}
	}
	if (opae_mutex_unlock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to unlock handle mutex");
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
out:
	opae_mutex_unlock(res, &fpga_perf_lock);
	return FPGA_EXCEPTION;
}

fpga_result fpgaPerfCounterPrint(FILE *f)
{
	uint64_t loop 	= 0;
	int res		= 0;

	if (!f)
		return FPGA_EXCEPTION;
	if (opae_mutex_lock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to lock handle mutex");
		return FPGA_EXCEPTION;
	}
	fprintf(f, "\n");
	for (loop = 0; loop < g_fpga_perf->num_perf_events; loop++)
		fprintf(f, "%s\t", g_fpga_perf->perf_events[loop].event_name);
	fprintf(f, "\n");
	for (loop = 0; loop < g_fpga_perf->num_perf_events; loop++) {
		if (!g_fpga_perf->perf_events[loop].config)
			continue;
		fprintf(f, "%ld\t\t", (g_fpga_perf->perf_events[loop].stop_value
				- g_fpga_perf->perf_events[loop].start_value));
	}
	fprintf(f, "\n");
	if (opae_mutex_unlock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to unlock handle mutex");
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

fpga_result fpgaPerfCounterFree(void)
{
	int res = 0;
        
	if (opae_mutex_lock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to lock handle mutex");
		return FPGA_EXCEPTION;
	}
	if (g_fpga_perf->format_type != NULL) {
		free(g_fpga_perf->format_type);
		g_fpga_perf->format_type = NULL;
	}
	if (g_fpga_perf->perf_events != NULL) {
		free(g_fpga_perf->perf_events);
		g_fpga_perf->perf_events = NULL;
	}
	if (g_fpga_perf != NULL) {
		free(g_fpga_perf);
		g_fpga_perf = NULL;
	}
	if (opae_mutex_unlock(res, &fpga_perf_lock)) {
		OPAE_MSG("Failed to unlock handle mutex");
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
}
