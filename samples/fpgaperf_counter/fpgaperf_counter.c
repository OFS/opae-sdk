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

#include "fpgaperf_counter.h"

#define DFL_FME "/sys/bus/pci/devices/*%x*:*%x*:*%x*.*%x*/fpga_region/region*/dfl-fme.*"

#define SYSFS "/sys/bus/event_source/devices/"

#define EVENT_FILTER  {"clock","fab_port_mmio_read","fab_port_mmio_write","fab_port_pcie0_read","fab_port_pcie0_write"}

/* Read format structure*/
struct read_format {
    int64_t nr;
    struct {
        uint64_t value;
        uint64_t id;
    } values[];
};

struct generic_event_type {
        const char *name;
        const char *value;
        long long config;
        int fd;
        uint64_t id;
        uint64_t start_value;
        uint64_t stop_value;
};

struct format_type {
        const char *name;
        long long value;
        int shift;
};

struct fpga_perf_counter {
        const char *name;
        char *fme;
        int type;
        int cpumask;
	int before;
        int num_formats;
        int num_generic_events;
        int num_read_events;
        struct format_type *formats;
        struct generic_event_type *generic_events;
};

/* Not static so other tools can access the PMU data */
struct fpga_perf_counter *pmus=NULL;

/*Initialize the perf event attribute structure and return the file descriptor*/
int  perf_event_attr_initialize(int generic_num, int val)
{
	struct perf_event_attr pea;
	int fd, grpfd, cpumask;

	memset(&pea, 0, sizeof(struct perf_event_attr));
	if(val == 1) {
		grpfd = -1;
	} else {
		grpfd = pmus->generic_events[0].fd;
	}
	cpumask = pmus->cpumask;
	pea.type = pmus->type;
	pea.size = sizeof(struct perf_event_attr);
	pea.config = pmus->generic_events[generic_num].config;
	pea.disabled = 1;
	pea.inherit = 1;
	pea.sample_type = PERF_SAMPLE_IDENTIFIER;
	pea.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_ID;
	fd = syscall(__NR_perf_event_open, &pea, -1, cpumask, grpfd, 0);
	if (fd == -1) {
		PERF_ERR("perf_event_open failed");
		return FPGA_EXCEPTION;
	} 
	return fd;
}

/* read the performance counter values */
fpga_result read_fab_counters(int val)
{
	int loop=0,inner_loop=0;
	char buf[4096] = "";
	struct read_format* rdft = (struct read_format*) buf;

	if(read(pmus->generic_events[0].fd, rdft, sizeof(buf)) == -1) {
		PERF_ERR("read fails");
		return FPGA_EXCEPTION;
    	}		
		
	for(loop=0; loop<(int)rdft->nr; loop++) {
        	for(inner_loop=0; inner_loop<pmus->num_generic_events; inner_loop++) {
      	    		if(rdft->values[loop].id == pmus->generic_events[inner_loop].id) {
				if(val == 1) {
			    		pmus->generic_events[inner_loop].start_value = rdft->values[loop].value;
				} else {
			
			    		pmus->generic_events[inner_loop].stop_value = rdft->values[loop].value;
      				}
    	    		}			
	   	}
   	}
	return FPGA_OK;
}

fpga_result fpgaperfcounterstop()
{
	fpga_result res = FPGA_OK;	

	if(ioctl(pmus->generic_events[0].fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP)==-1) {
		PERF_ERR("ioctl fails");
		return FPGA_EXCEPTION;
	}
	pmus->before=0;
	/* read the performance counter for the workload*/
	res=read_fab_counters(pmus->before);
	if(res!=FPGA_OK)
		return FPGA_EXCEPTION;
	return FPGA_OK;
}

void fpgaperfcounterprint()
{
	int loop=0;
	
	for(loop=0; loop<pmus->num_generic_events; loop++) {
		printf("%s\t", pmus->generic_events[loop].name);
	}	
	printf("\n");
	for(loop=0; loop<pmus->num_generic_events; loop++) {
		printf("%ld\t",(pmus->generic_events[loop].stop_value - pmus->generic_events[loop].start_value));
		printf("\t");
	}	
	printf("\n");
	if(pmus->formats!=NULL) 
		free(pmus->formats);
	if(pmus->generic_events!=NULL)
		free(pmus->generic_events);
	if(pmus!=NULL)
		free(pmus);
}

/* provides number of files in the directory */
int get_num_files(DIR *dir, char **filter, int filter_size) {
	int loop=0;
	int num_files=0;
	struct dirent *entry=NULL;

	while ((entry = readdir(dir)) != NULL) {
                if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                        continue;
		for(loop=0; loop<filter_size;loop++) {
			if (!strcmp(entry->d_name, filter[loop]))
				break;
		}
		/* if matched with filter then check next file */
		if(loop != filter_size)
			continue;
		num_files++;
	}
	return num_files;
}

/* parse the each format and get the shift val */
fpga_result prepare_format(DIR *dir, char *dir_name) {
	int format_num=0, result=-1;
	struct dirent *format_entry=NULL;;
	FILE *file=NULL;
	char temp_name[BUFSIZ] = "";
	
	pmus->formats=calloc(pmus->num_formats, sizeof(struct format_type));
	if (pmus->formats==NULL) {
		pmus->num_formats=0;
		return FPGA_EXCEPTION;
	}
	
	rewinddir(dir);
	while ((format_entry = readdir(dir)) != NULL) {
        	if (!strcmp(format_entry->d_name, ".") || !strcmp(format_entry->d_name, ".."))
        		continue;
		pmus->formats[format_num].name= strdup(format_entry->d_name);
		sprintf(temp_name,"%s/format/%s",dir_name,format_entry->d_name);
		file=fopen(temp_name,"r");
		if(file!=NULL) {
			result=fscanf(file,"config:%d",&pmus->formats[format_num].shift);
			/*read config first byte success*/
			if(result==-1) {
				PERF_ERR("format fscanf failed");
				fclose(file);
				return FPGA_EXCEPTION;
			}
			fclose(file);
		} else
			return FPGA_EXCEPTION;
		if(format_num==pmus->num_formats)
			break;
		format_num++;
	}
	return FPGA_OK;
}

/* parse the event value and get the type specific config value*/
uint64_t parse_event(char *value) {
	int loop=0, num=0;
	uint64_t config=0;
	char *name_evt_str=NULL;
	char *sub_str=strtok(value, ",");
	long val;

	while (sub_str != NULL){
		for(loop=0; loop< pmus->num_formats; loop++) {
			name_evt_str = strstr(sub_str, pmus->formats[loop].name);
			if(name_evt_str){
				num = strlen(pmus->formats[loop].name);
				/* Ignore '=0x' and convert to hex */
				val = strtol(sub_str+num+3, NULL, 16);
				config |= (val << pmus->formats[loop].shift);   
			}

		}
		sub_str = strtok(NULL, ",");
	}

	return config;
}

/* parse the evnts for the perticular device directory */
fpga_result prepare_event_mask(DIR *dir, char **filter, int filter_size, char *dir_name) {
	int loop=0, generic_num=0, result=-1;
	struct dirent *event_entry=NULL;
	FILE *file=NULL;
	char temp_name[BUFSIZ] = "";
	char event_value[BUFSIZ] = "";

	pmus->generic_events=calloc(pmus->num_generic_events, sizeof(struct generic_event_type));
	if (pmus->generic_events==NULL) {
		pmus->num_generic_events=0;
		return FPGA_EXCEPTION;
	}
	
	rewinddir(dir);
	while ((event_entry = readdir(dir)) != NULL) {
        	if (!strcmp(event_entry->d_name, ".") || !strcmp(event_entry->d_name, ".."))
            		continue;
		for(loop=0; loop<filter_size;loop++) {
			if (!strcmp(event_entry->d_name, filter[loop]))
				break;
		}
		if(loop!=filter_size)
			continue;
		pmus->generic_events[generic_num].name= strdup(event_entry->d_name);
		sprintf(temp_name,"%s/events/%s",dir_name,event_entry->d_name);
		file=fopen(temp_name,"r");
		if(file!=NULL) {
			result=fscanf(file, "%s", event_value);
			/* read event_value success*/
			if(result==1) {
				pmus->generic_events[generic_num].value=strdup(event_value);
				pmus->generic_events[generic_num].config=parse_event(event_value);
			} else
				return FPGA_EXCEPTION;
			fclose(file);
		} else
			return FPGA_EXCEPTION;			
		if(generic_num==pmus->num_generic_events)
			break;
		generic_num++;
	}
	return FPGA_OK;
}

/*Enumerate the dfl-fme based on the sbdf */
fpga_result fpgaperfcounterinit(uint16_t segment,uint8_t bus, uint8_t device, uint8_t function) {
    char temp_name[BUFSIZ] = "";
    char *fme=NULL;
    glob_t globlist;
    int val=-1,num=0, cnt=0,offset=0;
    char *str=NULL;
    
    /*allocate memory for PMUs */
    pmus=malloc(sizeof(struct fpga_perf_counter));
    if(pmus==NULL)
        return FPGA_EXCEPTION;
    function=0;
    sprintf(temp_name, DFL_FME, segment, bus, device, function);

    if (glob(temp_name, GLOB_PERIOD, NULL, &globlist) == GLOB_NOSPACE
          || glob(temp_name, GLOB_PERIOD, NULL, &globlist) == GLOB_NOMATCH)
            return FPGA_EXCEPTION;
    if (glob(temp_name, GLOB_PERIOD, NULL, &globlist) == GLOB_ABORTED)
            return FPGA_EXCEPTION;

    str = globlist.gl_pathv[0];
    fme = strstr(str, "dfl-fme.");
    if(fme == NULL)
        return FPGA_EXCEPTION;
    cnt=strlen(str);
    num=strlen(fme);
    offset=cnt-num;
    val=strtol(str+offset+num-1, NULL, 10);
    sprintf(fme, "dfl_fme%d", val);
    pmus->fme=fme;

    return FPGA_OK;
}


fpga_result fpgaperfcounterstart() {
	DIR *dir=NULL,*event_dir=NULL,*format_dir=NULL;
	struct dirent *entry=NULL;
	char dir_name[BUFSIZ] = "";
	char event_name[BUFSIZ] = "";
	char temp_name[BUFSIZ] = "";
	char format_name[BUFSIZ] = "";
	char *event_filter[] = EVENT_FILTER;
	int type=0;
	FILE *file=NULL;
	int result = -1;
	int  loop=0,inner_loop=0,fd=-1;
	int format_size=0,event_size=0;

    	dir=opendir(SYSFS);
    	if (dir==NULL) {
        	PERF_ERR("sysfs open failed");
        	goto out;
    	}	
    
    	/* Add PMU */
    	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(pmus->fme,entry->d_name) != 0) {
			continue;
		} else {
			/* read name */
			pmus->name=strdup(entry->d_name);
			sprintf(dir_name,SYSFS"/%s",pmus->name);
			/* read type */
			sprintf(temp_name,"%s/type",dir_name);
			file=fopen(temp_name,"r");
			if (file!=NULL) {
				result=fscanf(file,"%d",&type);
				/* read the pmus type success */
				if (result==1) 
					pmus->type=type;
				fclose(file);
			} else
				goto out;
			/* read cpumask */
	        	sprintf(temp_name,"%s/cpumask",dir_name);
        		file=fopen(temp_name,"r");
            		if (file!=NULL) {
                		result=fscanf(file,"%d",&type);
				/* read the pmus cpumask success*/
                		if (result==1) 
					pmus->cpumask=type;
                    		fclose(file);
           		} else
				goto out;
			/* Scan format strings */
			sprintf(format_name,"%s/format",dir_name);
			format_dir=opendir(format_name);
			if (format_dir!=NULL) {
			/* Count format strings and parse the format*/
				pmus->num_formats= get_num_files(format_dir, NULL, format_size);
				result = prepare_format(format_dir, dir_name);
                		if(result != FPGA_OK) {
					closedir(format_dir);
                    			return FPGA_EXCEPTION;
                		}
				closedir(format_dir);
			} else
				goto out;
			sprintf(event_name,"%s/events",dir_name);
			event_dir=opendir(event_name);
			if (event_dir!=NULL) {
				/* count generic events and parse the events*/
				event_size= sizeof(event_filter) / sizeof(event_filter[0]);
				pmus->num_generic_events=get_num_files(event_dir, event_filter, event_size);
				result = prepare_event_mask(event_dir, event_filter, event_size, dir_name);
                		if(result != FPGA_OK) {
					closedir(event_dir);
                    			return FPGA_EXCEPTION;
                		}
				closedir(event_dir);
			} else
				goto out;
            		for(loop=0; loop<pmus->num_generic_events; loop++) {
				/* pass generic num to get perticular config and to store first file descriptor*/
                		inner_loop++;
                		fd = perf_event_attr_initialize(loop, inner_loop);
                		pmus->generic_events[loop].fd = fd;
                		if(ioctl(pmus->generic_events[loop].fd, PERF_EVENT_IOC_ID, &pmus->generic_events[loop].id)==-1) {
			        	PERF_ERR("ioctl fails");
					return FPGA_EXCEPTION;
				} 
            		}	
				/* reset the performance counter to 0 */	
            		if(ioctl(pmus->generic_events[0].fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP)==-1) {
			        PERF_ERR("ioctl fails");
				return FPGA_EXCEPTION;
			}
			pmus->before=1;
			read_fab_counters(pmus->before);
		
            		if(ioctl(pmus->generic_events[0].fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP)==-1) {
			        PERF_ERR("ioctl fails");
				return FPGA_EXCEPTION;
			}
		}
	}
	closedir(dir);
	return FPGA_OK;

out:
	closedir(dir);

	return FPGA_EXCEPTION;
}
