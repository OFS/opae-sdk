#ifndef __FPGA_PERF_COUNTER_H__
#define __FPGA_PERF_COUNTER_H__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

/* Counter structure to store the counter values */
struct counter {
        const char *name;
        uint64_t value;
};

struct counter *perf_counter = NULL;


struct read_events {
        int fd;
        const char *name;
        uint64_t id;
};

struct read_events *rd_events = NULL;

struct generic_event_type {
        const char *name;
        const char *value;
        long long config;
};

struct format_type {
        const char *name;
        long long value;
        int shift;
};

struct pmu_type {
        const char *name;
        int type;
        int cpumask;
        int num_formats;
        int num_generic_events;
        int num_read_events;
        struct format_type *formats;
        struct generic_event_type *generic_events;
};

/**
 * * Initialize the perf event attribute structure
 * *
 * * @param[in] int 
 * * @param[in] int
 * * result file descriptor on success
 * *
 * */
int  perf_event_attr_initialize(int, int);


/**
 * * Stops performance counter and get the counters values
 * */

 void perf_stop();
 
/**
 * Dynamically enumerate sysfs path and get the device type, cpumask, format and generic events
 * Reset the counter to 0 and enable the counters to get workload instructions
 * */
 int perf_start();


/**
 * * Print the perf counter values.
 * *
 * */
 void perf_print();
 
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FPGA_PERF_COUNTER_H__ */
