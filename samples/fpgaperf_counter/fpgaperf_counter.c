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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <linux/hw_breakpoint.h>
#include <asm/unistd.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <dirent.h>
#include <glob.h>
#include <opae/log.h>

#include "fpgaperf_counter.h"

#define DFL_PERF_FME "/sys/bus/pci/devices/*%x*:*%x*:*%x*.*%x*/fpga_region/region*/dfl-fme.*"

#define DFL_PERF_SYSFS "/sys/bus/event_source/devices/"

/* This filter removes the unwanted events during enumeration */
#define EVENT_FILTER  {"clock", "fab_port_mmio_read", "fab_port_mmio_write", "fab_port_pcie0_read", "fab_port_pcie0_write"}

#define	DFL_PERF_STR_MAX	256

#define DFL_BUFSIZ_MAX	512

/* Read format structure*/
struct read_format {
	uint64_t nr;
	struct {
		uint64_t value;
		uint64_t id;
	} values[];
};

struct generic_event_type {
	const char *event_name;
	const char *value;
	long long config;
	int fd;
	uint64_t id;
	uint64_t start_value;
	uint64_t stop_value;
};

struct format_type {
	const char *format_name;
	long long value;
	int shift;
};

struct fpga_perf_counter {
	const char *pmu_name;
	char *fme;
	int type;
	int cpumask;
	int is_start;
	int num_formats;
	int num_generic_events;
	int num_read_events;
	struct format_type *formats;
	struct generic_event_type *generic_events;
};

/* Not static so other tools can access the PMU data */
struct fpga_perf_counter *fpga_perf_pmus = NULL;

/*Initialize the perf event attribute structure and return the file descriptor*/
int  perf_event_attr_initialize(int generic_num, int val)
{
	struct perf_event_attr pea;
	int fd, grpfd, cpumask;

	/* initialize the pea structure to 0 */
	memset(&pea, 0, sizeof(struct perf_event_attr));

	if ((fpga_perf_pmus->formats == NULL) || (fpga_perf_pmus->generic_events == NULL) || (fpga_perf_pmus == NULL))
		return -1;  
	if (val == 1)
		grpfd = -1;
	else
		grpfd = fpga_perf_pmus->generic_events[0].fd;
	cpumask = fpga_perf_pmus->cpumask;
	pea.type = fpga_perf_pmus->type;
	pea.size = sizeof(struct perf_event_attr);
	pea.config = fpga_perf_pmus->generic_events[generic_num].config;
	pea.disabled = 1;
	pea.inherit = 1;
	pea.sample_type = PERF_SAMPLE_IDENTIFIER;
	pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
	fd = syscall(__NR_perf_event_open, &pea, -1, cpumask, grpfd, 0);
	if (fd == -1) {
		OPAE_ERR("perf_event_open failed");
		return -1;
	}
	return fd;
}

/* read the performance counter values */
fpga_result read_fab_counters(int is_start)
{
	int loop = 0, inner_loop = 0;
	char buf[DFL_PERF_STR_MAX] = {0};
	struct read_format *rdft = (struct read_format *) buf;

	if ((fpga_perf_pmus->formats == NULL) || (fpga_perf_pmus->generic_events == NULL) || (fpga_perf_pmus == NULL))
		return FPGA_EXCEPTION;
  
	if (read(fpga_perf_pmus->generic_events[0].fd, rdft, sizeof(buf)) == -1) {
		OPAE_ERR("read fpga perf counter failed");
		return FPGA_EXCEPTION;
	}

	for (loop = 0; loop < (int)rdft->nr; loop++) {
		for (inner_loop = 0; inner_loop < fpga_perf_pmus->num_generic_events; inner_loop++) {
			if (rdft->values[loop].id == fpga_perf_pmus->generic_events[inner_loop].id) {
				if (is_start == 1)
					fpga_perf_pmus->generic_events[inner_loop].start_value = rdft->values[loop].value;
				else
					fpga_perf_pmus->generic_events[inner_loop].stop_value = rdft->values[loop].value;
			}
		}
	}
	return FPGA_OK;
}

fpga_result fpgaperfcounterstop(void)
{
	fpga_result res = FPGA_OK;

	if ((fpga_perf_pmus->formats == NULL) || (fpga_perf_pmus->generic_events == NULL) || (fpga_perf_pmus == NULL))
		return FPGA_EXCEPTION;
 
	if (ioctl(fpga_perf_pmus->generic_events[0].fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP) == -1) {
		OPAE_ERR("PERF_EVENT_IOC_DISABLE ioctl failed: %s", strerror(errno));
		return FPGA_EXCEPTION;
	}
	fpga_perf_pmus->is_start = 0;
	/* read the performance counter for the workload*/
	res = read_fab_counters(fpga_perf_pmus->is_start);
	if (res != FPGA_OK)
		return FPGA_EXCEPTION;
	return FPGA_OK;
}

void fpgaperfcounterprint(void)
{
	int loop = 0;

	for (loop = 0; loop < fpga_perf_pmus->num_generic_events; loop++)
		printf("%s\t", fpga_perf_pmus->generic_events[loop].event_name);

	printf("\n");
	for (loop = 0; loop < fpga_perf_pmus->num_generic_events; loop++) {
		printf("%ld\t", (fpga_perf_pmus->generic_events[loop].stop_value - fpga_perf_pmus->generic_events[loop].start_value));
		printf("\t");
	}
	printf("\n");
}

/* provides number of files in the directory 
 * after removing unwanted files provided in filter */
int get_num_files(DIR *dir, char **filter, int filter_size)
{
	int loop = 0;
	int num_files = 0;
	struct dirent *entry = NULL;

	while ((entry = readdir(dir)) != NULL) {
		if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
			continue;
		for (loop = 0; loop < filter_size; loop++) {
			if (!strcmp(entry->d_name, filter[loop]))
				break;
		}
		/* if matched with filter then check next file */
		if (loop != filter_size)
			continue;
		num_files++;
	}
	return num_files;
}

/* parse the each format and get the shift val */
fpga_result prepare_format(DIR *dir, char *dir_name)
{
	int format_num = 0, result = -1;
	struct dirent *format_entry = NULL;
	FILE *file = NULL;
	char sysfspath[DFL_BUFSIZ_MAX] = { 0,};

	if ((dir == NULL) || (dir_name == NULL)){
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	fpga_perf_pmus->formats = calloc(fpga_perf_pmus->num_formats, sizeof(struct format_type));
	if (fpga_perf_pmus->formats == NULL) {
		fpga_perf_pmus->num_formats = 0;
		return FPGA_EXCEPTION;
	}

	rewinddir(dir);
	while ((format_entry = readdir(dir)) != NULL) {
		if (!strcmp(format_entry->d_name, ".") || !strcmp(format_entry->d_name, ".."))
			continue;
		fpga_perf_pmus->formats[format_num].format_name = strdup(format_entry->d_name);
		snprintf(sysfspath, sizeof(sysfspath), "%s/format/%s", dir_name, format_entry->d_name);
		file = fopen(sysfspath, "r");
		if (file != NULL) {
			result = fscanf(file, "config:%d", &fpga_perf_pmus->formats[format_num].shift);
			/*read config first byte success*/
			if (result == -1) {
				OPAE_ERR("format fscanf failed");
				fclose(file);
				return FPGA_EXCEPTION;
			}
			fclose(file);
		} else
			return FPGA_EXCEPTION;
		if (format_num == fpga_perf_pmus->num_formats)
			break;
		format_num++;
	}
	return FPGA_OK;
}

/* parse the event value and get the type specific config value*/
uint64_t parse_event(char *value)
{
	int loop = 0, num = 0;
	uint64_t config = 0;
	char *name_evt_str = NULL;
	char *sub_str = strtok(value, ",");
	long val;

	while (sub_str != NULL) {
		for (loop = 0; loop < fpga_perf_pmus->num_formats; loop++) {
			name_evt_str = strstr(sub_str, fpga_perf_pmus->formats[loop].format_name);
			if (name_evt_str) {
				num = strlen(fpga_perf_pmus->formats[loop].format_name);
				/* Ignore '=0x' and convert to hex */
				val = strtol(sub_str+num+3, NULL, 16);
				config |= (val << fpga_perf_pmus->formats[loop].shift);
			}

		}
		sub_str = strtok(NULL, ",");
	}

	return config;
}

/* parse the evnts for the perticular device directory */
fpga_result prepare_event_mask(DIR *dir, char **filter, int filter_size, char *dir_name)
{
	int loop = 0, generic_num = 0, result = -1;
	struct dirent *event_entry = NULL;
	FILE *file = NULL;
	char sysfspath[DFL_BUFSIZ_MAX] = { 0,};
	char event_value[DFL_BUFSIZ_MAX] = { 0,};
	
	if ((dir == NULL) || (dir_name == NULL)) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}

	fpga_perf_pmus->generic_events = calloc(fpga_perf_pmus->num_generic_events, sizeof(struct generic_event_type));
	if (fpga_perf_pmus->generic_events == NULL) {
		fpga_perf_pmus->num_generic_events = 0;
		return FPGA_EXCEPTION;
	}

	rewinddir(dir);
	while ((event_entry = readdir(dir)) != NULL) {
		if (!strcmp(event_entry->d_name, ".") || !strcmp(event_entry->d_name, ".."))
			continue;
		for (loop = 0; loop < filter_size; loop++) {
			if (!strcmp(event_entry->d_name, filter[loop]))
				break;
		}
		if (loop != filter_size)
			continue;
		fpga_perf_pmus->generic_events[generic_num].event_name = strdup(event_entry->d_name);
		snprintf(sysfspath, sizeof(sysfspath), "%s/events/%s", dir_name, event_entry->d_name);
		file = fopen(sysfspath, "r");
		if (file != NULL) {
			result = fscanf(file, "%s", event_value);
			/* read event_value success*/
			if (result == 1) {
				fpga_perf_pmus->generic_events[generic_num].value = strdup(event_value);
				fpga_perf_pmus->generic_events[generic_num].config = parse_event(event_value);
			} else
				return FPGA_EXCEPTION;
			fclose(file);
		} else
			return FPGA_EXCEPTION;
		if (generic_num == fpga_perf_pmus->num_generic_events)
			break;
		generic_num++;
	}
	return FPGA_OK;
}

/* read the type and cpumask from the sysfs path */
fpga_result read_perf_sysfs(char *sysfs_path, int *val)
{
        FILE *file = NULL;

        file = fopen(sysfs_path, "r");
        if (!file) {
                OPAE_ERR("fopen sysfs_path failed\n");
                return FPGA_NOT_FOUND;
        }
        if(1 != fscanf(file,  "%d", val)) {
                OPAE_ERR("fscanf failed\n");
		fclose(file);
                return FPGA_EXCEPTION;
        }
        fclose(file);
        return FPGA_OK;
}

/*Enumerate the dfl-fme based on the sbdf */
fpga_result fpgaperfcounterinit(uint16_t segment, uint8_t bus, uint8_t device, uint8_t function)
{
	DIR *dir = NULL, *event_dir = NULL, *format_dir = NULL;
	struct dirent *entry = NULL;
	char dir_name[DFL_PERF_STR_MAX] = { 0,};
	char event_path[DFL_BUFSIZ_MAX] = { 0,};
	char sysfspath[DFL_BUFSIZ_MAX] = { 0,};
	char format_path[DFL_BUFSIZ_MAX] = { 0,};
	char *event_filter[] = EVENT_FILTER;
	char path[DFL_BUFSIZ_MAX] = { 0,};
	char *fme = NULL;
	fpga_result result = FPGA_OK;
	glob_t globlist;
	int val = -1, num = 0, cnt = 0;
	char *str = NULL;
	int format_size = 0, event_size = 0;
	int  loop = 0, inner_loop = 0, fd = -1;

	/*allocate memory for PMUs */
	fpga_perf_pmus = malloc(sizeof(struct fpga_perf_counter));
	if (fpga_perf_pmus == NULL)
		return FPGA_EXCEPTION;
	function = 0;
	if (bus == 0) {
		OPAE_ERR("Invalid Input parameters");
		return FPGA_INVALID_PARAM;
	}
		
	snprintf(path, sizeof(path), DFL_PERF_FME, segment, bus, device, function);

	int gres = glob(path, GLOB_NOSORT, NULL, &globlist);
	if (gres) {
		OPAE_ERR("Failed pattern match %s: %s", path, strerror(errno));
		globfree(&globlist);
		return FPGA_NOT_FOUND;
	}
	str = globlist.gl_pathv[0];
	if (str == NULL) {
		globfree(&globlist);
		return FPGA_EXCEPTION;
	}
	fme = strstr(str, "dfl-fme.");
	if (fme == NULL)
		return FPGA_EXCEPTION;
	cnt = strlen(str);
	num = strlen(fme);
	val = strtol(str+cnt-1, NULL, 10);
	snprintf(fme, num, "dfl_fme%d", val);
	fpga_perf_pmus->fme = fme;

	dir = opendir(DFL_PERF_SYSFS);
	if (dir == NULL) {
		OPAE_ERR("sysfs open failed");
		goto out;
	}
	/* Add PMU */
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(fpga_perf_pmus->fme, entry->d_name) != 0)
			continue;
		else {
			/* read name */
			fpga_perf_pmus->pmu_name = strdup(entry->d_name);
			snprintf(dir_name, sizeof(dir_name), DFL_PERF_SYSFS"/%s", fpga_perf_pmus->pmu_name);
			/* read type */
			snprintf(sysfspath, sizeof(sysfspath), "%s/type", dir_name);
			result = read_perf_sysfs(sysfspath, &fpga_perf_pmus->type);
			/* read the pmus type success */
			if (result != FPGA_OK)
				goto out;
 			/* read cpumask */
			snprintf(sysfspath, sizeof(sysfspath), "%s/cpumask", dir_name);
			result = read_perf_sysfs(sysfspath, &fpga_perf_pmus->cpumask);
			/* read the pmus type success */
			if (result != FPGA_OK)
				goto out;
			/* Scan format strings */
			snprintf(format_path, sizeof(format_path), "%s/format", dir_name);
			format_dir = opendir(format_path);
			if (format_dir != NULL) {
				/* Count format strings and parse the format*/
				fpga_perf_pmus->num_formats = get_num_files(format_dir, NULL, format_size);
				result = prepare_format(format_dir, dir_name);
				if (result != FPGA_OK) {
					closedir(format_dir);
					return FPGA_EXCEPTION;
				}
				closedir(format_dir);
			} else
				goto out;
			snprintf(event_path, sizeof(event_path), "%s/events", dir_name);
			event_dir = opendir(event_path);
			if (event_dir != NULL) {
			/* count generic events and parse the events*/
				event_size = sizeof(event_filter) / sizeof(event_filter[0]);
				fpga_perf_pmus->num_generic_events = get_num_files(event_dir, event_filter, event_size);
				result = prepare_event_mask(event_dir, event_filter, event_size, dir_name);
				if (result != FPGA_OK) {
					closedir(event_dir);
					return FPGA_EXCEPTION;
				}
				closedir(event_dir);
			} else
				goto out;
			for (loop = 0; loop < fpga_perf_pmus->num_generic_events; loop++) {
				/* pass generic num to get perticular config and to store first file descriptor*/
				inner_loop++;
				fd = perf_event_attr_initialize(loop, inner_loop);
				if (fd == -1)
					return FPGA_EXCEPTION;
				fpga_perf_pmus->generic_events[loop].fd = fd;
				if (ioctl(fpga_perf_pmus->generic_events[loop].fd, PERF_EVENT_IOC_ID, &fpga_perf_pmus->generic_events[loop].id) == -1) {
					OPAE_ERR("PERF_EVENT_IOC_ID ioctl failed: %s", strerror(errno));
					return FPGA_EXCEPTION;
				}
			}
		}
	}
	/* reset the performance counter to 0 */
	if (ioctl(fpga_perf_pmus->generic_events[0].fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP) == -1) {
		OPAE_ERR("PERF_EVENT_IOC_RESET ioctl failed: %s", strerror(errno));
		return FPGA_EXCEPTION;
	}

	closedir(dir);
	globfree(&globlist);
	return FPGA_OK;

out:
	closedir(dir);
	globfree(&globlist);
	return FPGA_EXCEPTION;

}


fpga_result fpgaperfcounterstart(void)
{
	fpga_perf_pmus->is_start = 1;
	read_fab_counters(fpga_perf_pmus->is_start);
	if (ioctl(fpga_perf_pmus->generic_events[0].fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP) == -1) {
		OPAE_ERR("PERF_EVENT_IOC_ENABLE ioctl failed: %s", strerror(errno));
		return FPGA_EXCEPTION;
	}
	return FPGA_OK;
}

void fpgaperfcounterfree(void)
{
	if (fpga_perf_pmus->formats != NULL)
		free(fpga_perf_pmus->formats);
	if (fpga_perf_pmus->generic_events != NULL)
		free(fpga_perf_pmus->generic_events);
	if (fpga_perf_pmus != NULL)
		free(fpga_perf_pmus);
}
