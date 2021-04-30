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

#include "fpgaperf_counter.h"


/* Not static so other tools can access the PMU data */
int g_first_fd = 0;
struct pmu_type *pmus=NULL;

int  perf_event_attr_initialize(int generic_num, int val)
{
	struct perf_event_attr pea;
	int fd, grpfd, cpumask;

	memset(&pea, 0, sizeof(struct perf_event_attr));
	if(val == 1) {
		grpfd = -1;
	} else {
		grpfd = g_first_fd;
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
		perror("perf_event_open failed");
		return -1;
	} 
	return fd;
}

/* read the performance counter values */
void read_fab_counters(struct counter *perf_counter)
{
	int loop=0,inner_loop=0;
	char buf[4096];
	struct read_format* rdft = (struct read_format*) buf;

	if(read(g_first_fd, rdft, sizeof(buf)) == -1) {
		perror("read fails");
    	}		
		
	for(loop=0; loop<(int)rdft->nr; loop++) {
     		for(inner_loop=0; inner_loop<pmus->num_generic_events; inner_loop++) {
      			if(rdft->values[loop].id == rd_events[inner_loop].id) {
				perf_counter[inner_loop].name = rd_events[inner_loop].name;
				perf_counter[inner_loop].value = rdft->values[loop].value;
      			}
    		}
   	}
}

void perf_stop()
{
	if(ioctl(g_first_fd, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP)==-1) {
		perror("ioctl fails");
	}

	perf_counter = calloc(pmus->num_generic_events, sizeof(struct counter));
	/* read the performance counter for the workload*/
	read_fab_counters(perf_counter);
}

void perf_print()
{
	int loop=0;
	
	for(loop=0; loop<pmus->num_generic_events; loop++) {
		printf("%s\t", perf_counter[loop].name);
	}	
	printf("\n");
	for(loop=0; loop<pmus->num_generic_events; loop++) {
		printf("%ld\t", perf_counter[loop].value);
		printf("\t");
	}	
	printf("\n");
	free(perf_counter);
	free(rd_events);
	free(pmus->formats);
	free(pmus->generic_events);
	free(pmus);
}

/* provides number of files in the directory */
int get_num_files(DIR *dir, char **filter, int filter_size) {
	int loop=0;
	int num_files=0;
	struct dirent *entry;

	while ((entry = readdir(dir)) != NULL) {
                if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                        continue;
		for(loop=0; loop<filter_size;loop++) {
			if (!strcmp(entry->d_name, filter[loop])) break;
		}
		/* if matched with filter then check next file */
		if(loop != filter_size) continue;
		num_files++;
	}
	return num_files;
}

/* parse the each format and get the shift val */
int prepare_format(DIR *dir, char *dir_name) {
	int format_num=0, result=-1;
	struct dirent *format_entry;
	FILE *file=NULL;
	char temp_name[BUFSIZ] = "";
	
	pmus->formats=calloc(pmus->num_formats, sizeof(struct format_type));
	if (pmus->formats==NULL) {
		pmus->num_formats=0;
		return -1;
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
			if(result==-1) {
				perror("format fscanf failed\n");
			}
			fclose(file);
		}
		if(format_num==pmus->num_formats) break;
		format_num++;
	}
	return 0;
}

/* parse the event value and get the type specific config value*/
uint64_t parse_event(char *value) {
	int loop=0, num=0;
	uint64_t config=0;
	char *name_evt_str;
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
int prepare_event_mask(DIR *dir, char **filter, int filter_size, char *dir_name) {
	int loop=0, generic_num=0, result=-1;
	struct dirent *event_entry;
	FILE *file=NULL;
	char temp_name[BUFSIZ] = "";
	char event_value[BUFSIZ] = "";
	uint64_t config;

	pmus->generic_events=calloc(pmus->num_generic_events, sizeof(struct generic_event_type));
	if (pmus->generic_events==NULL) {
		pmus->num_generic_events=0;
		return -1;
	}
	
	rewinddir(dir);
	while ((event_entry = readdir(dir)) != NULL) {
                if (!strcmp(event_entry->d_name, ".") || !strcmp(event_entry->d_name, ".."))
                        continue;
		for(loop=0; loop<filter_size;loop++) {
			if (!strcmp(event_entry->d_name, filter[loop])) break;
		}
		if(loop!=filter_size) continue;
		pmus->generic_events[generic_num].name= strdup(event_entry->d_name);
		sprintf(temp_name,"%s/events/%s",dir_name,event_entry->d_name);
		file=fopen(temp_name,"r");
		if(file!=NULL) {
			result=fscanf(file, "%s", event_value);
			if(result==1) {
				pmus->generic_events[generic_num].value=strdup(event_value);
				config=parse_event(event_value);
				pmus->generic_events[generic_num].config=config;
			}
			fclose(file);
		}			
		if(generic_num==pmus->num_generic_events) break;
		generic_num++;
	}
	return 0;
}

int perf_start() {
	DIR *dir,*event_dir,*format_dir;
	struct dirent *entry;
	char dir_name[BUFSIZ] = "";
	char event_name[BUFSIZ] = "";
	char temp_name[BUFSIZ] = "";
	char format_name[BUFSIZ] = "";
	char *event_filter[] = EVENT_FILTER;
	int type=0, num_pmus=0;
	FILE *file=NULL;
	int result = -1;
	int  loop=0,inner_loop=0,fd=-1;
	int format_size=0,event_size=0;

	/* Count number of PMUs */
	dir=opendir(SYSFS);
	if (dir==NULL) {
		perror("sysfs open failed");
		goto out;
	}
        while ((entry = readdir(dir)) != NULL) {
                if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
                        continue;

                if (!strcmp("dfl_fme0",entry->d_name)) break;
                num_pmus++;
        }
	if (num_pmus<1)
		goto out;

	pmus=calloc(num_pmus,sizeof(struct pmu_type));
	if (pmus==NULL)
		goto out;

	/* Add PMU */
	rewinddir(dir);
        while ((entry = readdir(dir)) != NULL) {
		if (strcmp("dfl_fme0",entry->d_name) != 0) {
			continue;
		} else {
			/* read name */
			pmus->name=strdup(entry->d_name);
			sprintf(dir_name,SYSFS"/%s",entry->d_name);
			/* read type */
			sprintf(temp_name,"%s/type",dir_name);
			file=fopen(temp_name,"r");
			if (file!=NULL) {
				result=fscanf(file,"%d",&type);
				if (result==1) pmus->type=type;
				fclose(file);
			}
			/* read cpumask */
	                sprintf(temp_name,"%s/cpumask",dir_name);
        	        file=fopen(temp_name,"r");
                	if (file!=NULL) {
                        	result=fscanf(file,"%d",&type);
                        	if (result==1) pmus->cpumask=type;
                        	fclose(file);
                	}
			/* Scan format strings */
			sprintf(format_name,"%s/format",dir_name);
			format_dir=opendir(format_name);
			if (format_dir!=NULL) {
			/* Count format strings and parse the format*/
				pmus->num_formats= get_num_files(format_dir, NULL, format_size);
				result = prepare_format(format_dir, dir_name);
				closedir(format_dir);
			}
			sprintf(event_name,"%s/events",dir_name);
			event_dir=opendir(event_name);
			if (event_dir!=NULL) {
				/* count generic events and parse the events*/
				event_size= sizeof(event_filter) / sizeof(event_filter[0]);
				pmus->num_generic_events=get_num_files(event_dir, event_filter, event_size);
				result = prepare_event_mask(event_dir, event_filter, event_size, dir_name);
				closedir(event_dir);
			}
			/*allocate memory for read event structure*/
			rd_events = calloc(pmus->num_generic_events, sizeof(struct read_events));
			if (rd_events == NULL) {
				pmus->num_generic_events = 0;
				goto out;
			}
                        for(loop=0; loop<pmus->num_generic_events; loop++) {
				/* pass generic num to get perticular config and to store first file descriptor*/
                        	inner_loop++;
                                fd = perf_event_attr_initialize(loop, inner_loop);
                                if(inner_loop == 1) {
                                      g_first_fd = fd;
                                 }
                                 rd_events[loop].fd = fd;
                                 rd_events[loop].name = pmus->generic_events[loop].name;
                                 if(ioctl(rd_events[loop].fd, PERF_EVENT_IOC_ID, &rd_events[loop].id)==-1) {
					perror("ioctl fails");
				 } 
                        }
				/* reset the performance counter to 0 */	
                        if(ioctl(g_first_fd, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP)==-1) {
				perror("ioctl fails");
			}
                        if(ioctl(g_first_fd, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP)==-1) {
				perror("ioctl fails");
			}
		}
	}
	result = 0;

out:
	closedir(dir);

	return result;
}
