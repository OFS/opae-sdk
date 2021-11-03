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

#include "fpgaperf_counter.h"

#include <errno.h>
#include <glob.h>
#include <regex.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <libudev.h>
#include <linux/perf_event.h>

#include <opae/fpga.h>
#include <opae/log.h>
#include <opae/properties.h>
#include <opae/utils.h>
#include "opae_int.h"

#define PCI_DEV_ADDRS		"/sys/bus/pci/devices/*%x*:*%x*:*%x*.*%x*/"

#define DFL_PERF_FME		PCI_DEV_ADDRS "fpga_region/region*/dfl-fme.*"

#define DFL_PERF_SYSFS		"/sys/bus/event_source/devices/dfl_fme"

#define PERF_EVENT		"event=(0x[0-9a-fA-F]{2}),"

#define PERF_EVTYPE		"evtype=(0x[0-9a-fA-F]{2}),"

#define PERF_PORTID		"portid=(0x[0-9a-fA-F]{2})"

#define PERF_EVENT_PATTERN	PERF_EVENT PERF_EVTYPE PERF_PORTID

#define PERF_CONFIG_PATTERN	"config:([0-9]{1,})-([0-9]{2,})"

#define PARSE_MATCH_INT(_p, _m, _v, _b)				\
	do {							\
		errno = 0;					\
		_v = strtoul(_p + _m, NULL, _b);		\
		if (errno) {					\
			OPAE_MSG("error parsing int");		\
		}						\
	} while (0)


/* Read format structure*/
struct read_format {
	uint64_t nr;
	struct {
		uint64_t value;
		uint64_t id;
	} values[];
};

/*
 * Check perf handle object for validity and lock its mutex
 * If fpga_perf_check_and_lock() returns FPGA_OK, assume the mutex to be
 * locked.
 */
STATIC fpga_result fpga_perf_check_and_lock(fpga_perf_counter *fpga_perf)
{
	int res = 0;

	if (!fpga_perf)
		return FPGA_INVALID_PARAM;

	if (opae_mutex_lock(res, &fpga_perf->lock)) {
		OPAE_ERR("Failed to lock perf mutex");
		return FPGA_EXCEPTION;
	}

	if (fpga_perf->magic != FPGA_PERF_MAGIC) {
		opae_mutex_unlock(res, &fpga_perf->lock);
		return FPGA_INVALID_PARAM;
	}

	return FPGA_OK;
}

/* parse the each format and get the shift val
 * parse the events for the particular device directory */
STATIC fpga_result parse_perf_attributes(struct udev_device *dev,
			fpga_perf_counter *fpga_perf, const char *attr)
{
	regex_t re;
	char err[128] 				= { 0 };
	int reg_res 				= 0;
	uint64_t loop 				= 0;
	uint64_t inner_loop			= 0;
	uint64_t value 				= 0;
	char attr_path[DFL_PERF_STR_MAX]	= { 0,};
	char attr_value[128]			= { 0,};
	int  gres				= 0;
	size_t i				= 0;
	FILE *file				= NULL;
	glob_t pglob;
	regmatch_t f_matches[3]			= { {0} };
	regmatch_t e_matches[4]			= { {0} };

	if (!dev || !fpga_perf) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	if (snprintf(attr_path, sizeof(attr_path), "%s/%s/*",
			udev_device_get_syspath(dev), attr) < 0) {
		OPAE_ERR("snprintf buffer overflow");
		return FPGA_EXCEPTION;
	}
	gres = glob(attr_path, GLOB_NOSORT, NULL, &pglob);
	if (gres || !pglob.gl_pathc) {
		OPAE_ERR("Failed pattern match %s", attr_path);
		globfree(&pglob);
		return FPGA_EXCEPTION;
	}
	if (strcmp(attr, "format") == 0 ) {
		fpga_perf->num_format = pglob.gl_pathc;
		if (!fpga_perf->format_type) {
			fpga_perf->format_type = calloc(fpga_perf->num_format,
						sizeof(perf_format_type));
			if (!fpga_perf->format_type) {
				fpga_perf->num_format = 0;
				OPAE_ERR("Failed to allocate Memory");
				globfree(&pglob);
				return FPGA_NO_MEMORY;
			}
		}
	} else {
		fpga_perf->num_perf_events = pglob.gl_pathc;
		if (!fpga_perf->perf_events) {
			fpga_perf->perf_events = calloc(fpga_perf->num_perf_events,
							sizeof(perf_events_type));
			if (!fpga_perf->perf_events) {
				fpga_perf->num_perf_events = 0;
				OPAE_ERR("Failed to allocate Memory");
				globfree(&pglob);
				return FPGA_NO_MEMORY;
			}
		}
	}

	for (i = 0; i < pglob.gl_pathc; i++) {
		file = fopen(pglob.gl_pathv[i], "r");
		if (!file) {
			OPAE_ERR("fopen(%s) failed\n", pglob.gl_pathv[i]);
			globfree(&pglob);
			return FPGA_EXCEPTION;
		}
		if (fscanf(file, "%127s", attr_value) != 1) {
			OPAE_ERR("Failed to read %s", pglob.gl_pathv[i]);
			goto out;
		}
		if (strcmp(attr, "format") == 0 ) {
			reg_res = regcomp(&re, PERF_CONFIG_PATTERN,
				REG_EXTENDED | REG_ICASE);
			if (reg_res) {
				OPAE_ERR("Error compiling regex");
				goto out;
			}
			reg_res = regexec(&re, attr_value, 4, f_matches, 0);
			if (reg_res) {
				regerror(reg_res, &re, err, sizeof(err));
				OPAE_MSG("Error executing regex: %s", err);
			} else {
				PARSE_MATCH_INT(attr_value, f_matches[1].rm_so, value, 10);
				fpga_perf->format_type[loop].shift = value;
				if (snprintf(fpga_perf->format_type[loop].format_name,
					sizeof(fpga_perf->format_type[loop].format_name),
						"%s", (strstr(pglob.gl_pathv[i], attr)
							+ strlen(attr)+1)) < 0) {
					OPAE_ERR("snprintf buffer overflow");
					goto out;
				}
				loop++;
			}
		} else {
			reg_res = regcomp(&re, PERF_EVENT_PATTERN,
				REG_EXTENDED | REG_ICASE);
			if (reg_res) {
				OPAE_ERR("Error compiling regex");
				goto out;
			}
			reg_res = regexec(&re, attr_value, 4, e_matches, 0);
			if (reg_res) {
				regerror(reg_res, &re, err, sizeof(err));
				OPAE_MSG("Error executing regex: %s", err);
			} else {
				uint64_t config = 0;
				uint64_t event = 0;
				if (snprintf(fpga_perf->perf_events[inner_loop].event_name,
					sizeof(fpga_perf->perf_events[inner_loop].event_name),
					"%s", (strstr(pglob.gl_pathv[i], attr)
						+ strlen(attr) + 1)) < 0) {
					OPAE_ERR("snprintf buffer overflow");
					goto out;
				}
				for (loop = 0; loop < fpga_perf->num_format; loop++) {
					PARSE_MATCH_INT(attr_value,
						e_matches[loop + 1].rm_so, event, 16);
					config |= event << fpga_perf->format_type[loop].shift;
				}
				fpga_perf->perf_events[inner_loop].config = config;
				inner_loop++;
			}
		}
		fclose(file);
	}

	globfree(&pglob);
	return FPGA_OK;

out:
	fclose(file);
	globfree(&pglob);
	return FPGA_EXCEPTION;
}

STATIC fpga_result fpga_perf_events(char* perf_sysfs_path, fpga_perf_counter *fpga_perf)
{
	fpga_result ret 	= FPGA_OK;
	struct udev *udev 	= NULL;
	struct udev_device *dev = NULL;
	int fd 			= 0;
	int grpfd		= 0;
	uint64_t loop 		= 0;
	struct perf_event_attr pea;
	
	if (!perf_sysfs_path || !fpga_perf) {
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
	const char * ptr = udev_device_get_sysattr_value(dev, "cpumask");
	if (ptr)
		PARSE_MATCH_INT(ptr, 0, fpga_perf->cpumask, 10);
	ptr = udev_device_get_sysattr_value(dev, "type");
	if (ptr)
		PARSE_MATCH_INT(ptr, 0, fpga_perf->type, 10);

	/* parse the format value */
	ret = parse_perf_attributes(dev, fpga_perf, "format");
	if (ret != FPGA_OK)
		goto out;
	/* parse the event value */
	ret = parse_perf_attributes(dev, fpga_perf, "events");
	if (ret != FPGA_OK)
		goto out;

	/* initialize the pea structure to 0 */
	memset(&pea, 0, sizeof(struct perf_event_attr));

	for (loop = 0; loop < fpga_perf->num_perf_events; loop++) {

		if (fpga_perf->perf_events[0].fd <= 0)
			grpfd = -1;
		else
			grpfd = fpga_perf->perf_events[0].fd;
		
		if (!fpga_perf->perf_events[loop].config)
			continue;

		pea.type = fpga_perf->type;
		pea.size = sizeof(struct perf_event_attr);
		pea.config = fpga_perf->perf_events[loop].config;
		pea.disabled = 1;
		pea.inherit = 1;
		pea.sample_type = PERF_SAMPLE_IDENTIFIER;
		pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
		fd = syscall(__NR_perf_event_open, &pea, -1, fpga_perf->cpumask, grpfd, 0);
		if (fd == -1) {
			OPAE_ERR("Error opening leader %llx\n", pea.config);
			ret = FPGA_EXCEPTION;
			goto out;
		} else {
			fpga_perf->perf_events[loop].fd = fd;
			if (ioctl(fpga_perf->perf_events[loop].fd, PERF_EVENT_IOC_ID,
						&fpga_perf->perf_events[loop].id) == -1) {
				OPAE_ERR("PERF_EVENT_IOC_ID ioctl failed: %s",
						strerror(errno));
				ret = FPGA_EXCEPTION;
				goto out;
			}
		}
	}
	if (ioctl(fpga_perf->perf_events[0].fd, PERF_EVENT_IOC_RESET,
				PERF_IOC_FLAG_GROUP) == -1) {
		OPAE_ERR("PERF_EVENT_IOC_RESET ioctl failed: %s", strerror(errno));
		ret = FPGA_EXCEPTION;
		goto out;
	}
out:
	udev_device_unref(dev);
	udev_unref(udev);
	return ret;
}

/* get fpga sbdf from token */
STATIC fpga_result get_fpga_sbdf(fpga_token token,
		uint16_t *segment,
		uint8_t *bus,
		uint8_t *device,
		uint8_t *function)
{
	fpga_result res		= FPGA_OK;
	fpga_properties props	= NULL;

	if (!segment || !bus ||
		!device || !function) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}
	res = fpgaGetProperties(token, &props);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get properties");
		return res;
	}
	res = fpgaPropertiesGetBus(props, bus);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get bus");
		return res;
	}
	res = fpgaPropertiesGetSegment(props, segment);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Segment");
		return res;
	}
	res = fpgaPropertiesGetDevice(props, device);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Device");
		return res;
	}
	res = fpgaPropertiesGetFunction(props, function);
	if (res != FPGA_OK) {
		OPAE_ERR("Failed to get Function");
		return res;
	}

	return res;
}

/* Initialises magic number, mutex attributes and set the mutex attribute
 * type to PTHREAD_MUTEX_RECURSIVE. Also initialises the mutex referenced by
 * fpga_perf->lock with attributes specified by mutex attributes */
STATIC fpga_result fpga_perf_mutex_init(fpga_perf_counter *fpga_perf)
{
	pthread_mutexattr_t mattr;

	if (!fpga_perf) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	fpga_perf->magic = FPGA_PERF_MAGIC;

	if (pthread_mutexattr_init(&mattr)) {
		OPAE_ERR("pthread_mutexattr_init() failed");
		return FPGA_EXCEPTION;
	}

	if (pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_RECURSIVE)) {
		OPAE_ERR("pthread_mutexattr_settype() failed");
		pthread_mutexattr_destroy(&mattr);
		return FPGA_EXCEPTION;
	}

	if (pthread_mutex_init(&fpga_perf->lock, &mattr)) {
		OPAE_ERR("pthread_mutex_init() failed");
		pthread_mutexattr_destroy(&mattr);
		return FPGA_EXCEPTION;
	}
	pthread_mutexattr_destroy(&mattr);

	return FPGA_OK;
}

/* Reset the magic number and destroy the mutex created */
STATIC fpga_result fpga_perf_mutex_destroy(fpga_perf_counter *fpga_perf)
{
	fpga_result ret = FPGA_OK;
	int res		= 0;

	if (!fpga_perf) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	ret = fpga_perf_check_and_lock(fpga_perf);
	if (ret) {
		OPAE_ERR("Failed to lock perf mutex");
		return ret;
	}

	fpga_perf->magic = 0;

	ret = opae_mutex_unlock(res, &fpga_perf->lock);
	if (ret) {
		OPAE_ERR("Failed to unlock perf mutex");
		return ret;
	}

	ret = pthread_mutex_destroy(&fpga_perf->lock);
	if (ret) {
		OPAE_ERR("Failed to destroy pthread mutex destroy");
		return ret;
	}

	return FPGA_OK;
}


fpga_result fpgaPerfCounterGet(fpga_token token, fpga_perf_counter *fpga_perf)
{
	fpga_result ret				= FPGA_OK;
	int res					= 0;
	char sysfs_path[DFL_PERF_STR_MAX]	= { 0 };
	char sysfs_perf[DFL_PERF_STR_MAX]	= { 0 };
	int gres 				= 0;
	uint32_t fpga_id 			= -1;
	char *endptr 				= NULL;
	glob_t pglob;
	uint8_t bus				= (uint8_t)-1;
	uint16_t segment			= (uint16_t)-1;
	uint8_t device				= (uint8_t)-1;
	uint8_t function			= (uint8_t)-1;


	if (!token || !fpga_perf) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	memset(fpga_perf, 0, sizeof(fpga_perf_counter));

	ret = get_fpga_sbdf(token, &segment, &bus, &device, &function);
	if (ret != FPGA_OK) {
		OPAE_ERR("Failed to get sbdf");
		return ret;
	}
	ret = fpga_perf_mutex_init(fpga_perf);
	if (ret != FPGA_OK) {
		OPAE_ERR("Failed to initialize the mutex");
		return ret;
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
		fpga_id = strtoul(ptr + 4, &endptr, 10);

		if (snprintf(sysfs_perf, sizeof(sysfs_perf),
			DFL_PERF_SYSFS"%d", fpga_id) < 0) {
			OPAE_ERR("snprintf buffer overflow");
			ret = FPGA_EXCEPTION;
			goto out;
		}
		if (fpga_perf_check_and_lock(fpga_perf)) {
			OPAE_ERR("Failed to lock perf mutex");
			ret = FPGA_EXCEPTION;
			goto out;
		}
		if (snprintf(fpga_perf->dfl_fme_name, sizeof(fpga_perf->dfl_fme_name),
			"dfl_fme%d", fpga_id) < 0) {
			OPAE_ERR("snprintf buffer overflow");
			opae_mutex_unlock(res, &fpga_perf->lock);
			ret = FPGA_EXCEPTION;
			goto out;
		}
		ret = fpga_perf_events(sysfs_perf, fpga_perf);
		if (ret != FPGA_OK) {
			OPAE_ERR("Failed to parse fpga perf event");
			opae_mutex_unlock(res, &fpga_perf->lock);
			goto out;
		}
		if (opae_mutex_unlock(res, &fpga_perf->lock)) {
			OPAE_ERR("Failed to unlock perf mutex");
			ret = FPGA_EXCEPTION;
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

fpga_result fpgaPerfCounterStartRecord(fpga_perf_counter *fpga_perf)
{
	uint64_t loop			= 0;
	uint64_t inner_loop		= 0;
	int res				= 0;
	char buf[DFL_PERF_STR_MAX]	= { 0 };
	struct read_format *rdft	= (struct read_format *) buf;

	if (!fpga_perf) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	if (fpga_perf_check_and_lock(fpga_perf)) {
		OPAE_ERR("Failed to lock perf mutex");
		return FPGA_EXCEPTION;
	}
	if (ioctl(fpga_perf->perf_events[0].fd, PERF_EVENT_IOC_ENABLE,
				PERF_IOC_FLAG_GROUP) == -1) {
		OPAE_ERR("PERF_EVENT_IOC_ENABLE ioctl failed: %s",
				strerror(errno));
		goto out;
	}
	if (read(fpga_perf->perf_events[0].fd, rdft, sizeof(buf)) == -1) {
		OPAE_ERR("read fpga perf counter failed");
		goto out;
	}
	for (loop = 0; loop < (uint64_t)rdft->nr; loop++) {
		for (inner_loop = 0; inner_loop < fpga_perf->num_perf_events;
								inner_loop++) {
			if (rdft->values[loop].id == fpga_perf->perf_events[inner_loop].id)
				fpga_perf->perf_events[inner_loop].start_value = rdft->values[loop].value;
		}
	}	
	if (opae_mutex_unlock(res, &fpga_perf->lock)) {
		OPAE_ERR("Failed to unlock perf mutex");
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
out:
	opae_mutex_unlock(res, &fpga_perf->lock);
	return FPGA_EXCEPTION;
}

fpga_result fpgaPerfCounterStopRecord(fpga_perf_counter *fpga_perf)
{
	char buf[DFL_PERF_STR_MAX]	= { 0 };
	uint64_t loop			= 0;
	uint64_t inner_loop		= 0;
	int res				= 0;
	struct read_format *rdft 	= (struct read_format *) buf;

	if (!fpga_perf) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	if (fpga_perf_check_and_lock(fpga_perf)) {
		OPAE_ERR("Failed to lock perf mutex");
		return FPGA_EXCEPTION;
	}
	if (ioctl(fpga_perf->perf_events[0].fd, PERF_EVENT_IOC_DISABLE,
				PERF_IOC_FLAG_GROUP) == -1) {
		OPAE_ERR("PERF_EVENT_IOC_ENABLE ioctl failed: %s",
				strerror(errno));
		goto out;
	}
	if (read(fpga_perf->perf_events[0].fd, rdft, sizeof(buf)) == -1) {
		OPAE_ERR("read fpga perf counter failed");
		goto out;
	}	
	for (loop = 0; loop < (uint64_t)rdft->nr; loop++) {
		for (inner_loop = 0; inner_loop < fpga_perf->num_perf_events;
								inner_loop++) {
			if (rdft->values[loop].id == fpga_perf->perf_events[inner_loop].id)
				fpga_perf->perf_events[inner_loop].stop_value = rdft->values[loop].value;
		}
	}
	if (opae_mutex_unlock(res, &fpga_perf->lock)) {
		OPAE_ERR("Failed to unlock perf mutex");
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
out:
	opae_mutex_unlock(res, &fpga_perf->lock);
	return FPGA_EXCEPTION;
}

fpga_result fpgaPerfCounterPrint(FILE *f, fpga_perf_counter *fpga_perf)
{
	uint64_t loop 	= 0;
	int res		= 0;

	if (!fpga_perf || !f) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}

	if (fpga_perf_check_and_lock(fpga_perf)) {
		OPAE_ERR("Failed to lock perf mutex");
		return FPGA_EXCEPTION;
	}

	fprintf(f, "\n");
	for (loop = 0; loop < fpga_perf->num_perf_events; loop++)
		fprintf(f, "%s\t", fpga_perf->perf_events[loop].event_name);

	fprintf(f, "\n");
	for (loop = 0; loop < fpga_perf->num_perf_events; loop++) {
		if (!fpga_perf->perf_events[loop].config)
			continue;
		fprintf(f, "%ld\t\t", (fpga_perf->perf_events[loop].stop_value
				- fpga_perf->perf_events[loop].start_value));
	}

	fprintf(f, "\n");

	if (opae_mutex_unlock(res, &fpga_perf->lock)) {
		OPAE_ERR("Failed to unlock perf mutex");
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}

fpga_result fpgaPerfCounterDestroy(fpga_perf_counter *fpga_perf)
{
	int res	= 0;

	if (!fpga_perf) {
		OPAE_ERR("Invalid input parameters");
		return FPGA_INVALID_PARAM;
	}
	if (fpga_perf_check_and_lock(fpga_perf)) {
		OPAE_ERR("Failed to lock perf mutex");
		return FPGA_EXCEPTION;
	}
	if (fpga_perf->format_type) {
		free(fpga_perf->format_type);
		fpga_perf->format_type = NULL;
	}
	if (fpga_perf->perf_events) {
		free(fpga_perf->perf_events);
		fpga_perf->perf_events = NULL;
	}

	if (opae_mutex_unlock(res, &fpga_perf->lock)) {
		OPAE_ERR("Failed to unlock perf mutex");
		return FPGA_EXCEPTION;
	}
	if (fpga_perf_mutex_destroy(fpga_perf) != FPGA_OK) {
		OPAE_ERR("Failed to destroy the mutex");
		return FPGA_EXCEPTION;
	}

	return FPGA_OK;
}
